#include "local_embedder.hpp"
#include "logger.hpp"

#include <common/common.h>
#include <common/log.h>
#include <format>
#include <llama.h>

#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace muclaw {

namespace {
std::vector<std::string> split_lines(const std::string& s, const std::string& separator = "\n") {
    std::vector<std::string> lines;
    size_t start = 0;
    size_t end = s.find(separator);

    while (end != std::string::npos) {
        lines.push_back(s.substr(start, end - start));
        start = end + separator.length();
        end = s.find(separator, start);
    }

    lines.push_back(s.substr(start)); // Add the last part

    return lines;
}

static void batch_add_seq(llama_batch& batch, const std::vector<int32_t>& tokens, llama_seq_id seq_id) {
    size_t n_tokens = tokens.size();
    for (size_t i = 0; i < n_tokens; i++)
        common_batch_add(batch, tokens[i], i, {seq_id}, true);
}

static void batch_decode(llama_context* ctx, llama_batch& batch, float* output, int n_seq, int n_embd, int embd_norm) {
    const enum llama_pooling_type pooling_type = llama_pooling_type(ctx);

    // clear previous kv_cache values (irrelevant for embeddings)
    llama_memory_clear(llama_get_memory(ctx), true);

    // run model
    log::info("n_tokens = {}, n_seq = {}", batch.n_tokens, n_seq);
    if (llama_decode(ctx, batch) < 0)
        log::error("failed to process");

    for (int i = 0; i < batch.n_tokens; i++) {
        if (!batch.logits[i])
            continue;

        const float* embd = nullptr;
        int embd_pos = 0;

        if (pooling_type == LLAMA_POOLING_TYPE_NONE) {
            // try to get token embeddings
            embd = llama_get_embeddings_ith(ctx, i);
            embd_pos = i;
            GGML_ASSERT(embd != NULL && "failed to get token embeddings");
        } else {
            // try to get sequence embeddings - supported only when pooling_type is not NONE
            embd = llama_get_embeddings_seq(ctx, batch.seq_id[i][0]);
            embd_pos = batch.seq_id[i][0];
            GGML_ASSERT(embd != NULL && "failed to get sequence embeddings");
        }

        float* out = output + embd_pos * n_embd;
        common_embd_normalize(embd, out, n_embd, embd_norm);
    }
}
} // namespace

std::unique_ptr<LocalEmbedder> LocalEmbedder::instance_;

LocalEmbedder::LocalEmbedder(fs::path const& model, int n_threads) : model_path{model} {
    common_init();
    common_log_set_verbosity_thold(GGML_LOG_LEVEL_CONT);

    params_.cpuparams_batch.n_threads = n_threads;
    params_.model.path = model_path.string();
    params_.embedding = true;
    params_.n_ctx = 0;
    params_.n_batch = 2048;
    params_.n_ubatch = 2048;

    if (params_.n_parallel == 1)
        params_.kv_unified = true;

    llama_backend_init();
    llama_numa_init(params_.numa);

    llama_ = common_init_from_params(params_);
    if (!llama_.model || !llama_.context)
        throw std::runtime_error(std::format("Failed to load llama model/context from {}", model.string()));

    log::info("Successfully loaded local embedding model: {}", model.string());
}

auto LocalEmbedder::embed(std::vector<std::string> const& prompts) -> std::vector<float> {
    llama_model* model = llama_.model.get();
    llama_context* ctx = llama_.context.get();

    // Everything below: lifted shamelessly from llama-cpp/examples/embedding.cpp!

    const llama_vocab* vocab = llama_model_get_vocab(model);

    const int n_ctx_train = llama_model_n_ctx_train(model);
    const int n_ctx = llama_n_ctx(ctx);

    const enum llama_pooling_type pooling_type = llama_pooling_type(ctx);

    if (llama_model_has_encoder(model) && llama_model_has_decoder(model))
        throw std::runtime_error("computing embeddings in encoder-decoder models is not supported");

    if (n_ctx > n_ctx_train)
        log::warn("warning: model was trained on only {} context tokens ({} specified)", n_ctx_train, n_ctx);

    log::info("{}", common_params_get_system_info(params_));

    // max batch size
    const uint64_t n_batch = params_.n_batch;

    // get added sep and eos token, if any
    const std::string added_sep_token =
        llama_vocab_get_add_sep(vocab) ? llama_vocab_get_text(vocab, llama_vocab_sep(vocab)) : "";
    const std::string added_eos_token =
        llama_vocab_get_add_eos(vocab) ? llama_vocab_get_text(vocab, llama_vocab_eos(vocab)) : "";

    // tokenize the prompts and trim
    std::vector<std::vector<int32_t>> inputs;
    for (const auto& prompt : prompts) {
        std::vector<llama_token> inp;

        // split classification pairs and insert expected separator tokens
        if (pooling_type == LLAMA_POOLING_TYPE_RANK && prompt.find(params_.cls_sep) != std::string::npos) {
            std::vector<std::string> pairs = split_lines(prompt, params_.cls_sep);
            std::string final_prompt;

            for (size_t i = 0; i < pairs.size(); i++) {
                final_prompt += pairs[i];
                if (i != pairs.size() - 1) {
                    if (!added_eos_token.empty())
                        final_prompt += added_eos_token;
                    if (!added_sep_token.empty())
                        final_prompt += added_sep_token;
                }
            }

            inp = common_tokenize(ctx, final_prompt, true, true);
        } else {
            inp = common_tokenize(ctx, prompt, true, true);
        }
        if (inp.size() > n_batch)
            throw std::runtime_error(
                std::format("number of tokens in input line ({}) exceeds batch size ({}), increase batch size",
                            (long long int)inp.size(), (long long int)n_batch));
        inputs.push_back(inp);
    }

    // check if the last token is SEP/EOS
    // it should be automatically added by the tokenizer when 'tokenizer.ggml.add_eos_token' is set to 'true'
    for (auto& inp : inputs) {
        if (inp.empty() || (inp.back() != llama_vocab_sep(vocab) && inp.back() != llama_vocab_eos(vocab))) {
            log::warn("last token in the prompt is not SEP or EOS");
            log::warn("'tokenizer.ggml.add_eos_token' should be set to 'true' in the GGUF header");
        }
    }

    // tokenization stats
    if (params_.verbose_prompt) {
        for (int i = 0; i < (int)inputs.size(); i++) {
            log::info("%s: prompt %d: '%s'\n", __func__, i, prompts[i].c_str());
            log::info("%s: number of tokens in prompt = %zu\n", __func__, inputs[i].size());
            for (int j = 0; j < (int)inputs[i].size(); j++)
                log::info("%6d -> '%s'\n", inputs[i][j], common_token_to_piece(ctx, inputs[i][j]).c_str());
        }
    }

    // initialize batch
    const int n_prompts = prompts.size();
    struct llama_batch batch = llama_batch_init(n_batch, 0, 1);

    // count number of embeddings
    int n_embd_count = 0;
    if (pooling_type == LLAMA_POOLING_TYPE_NONE)
        for (int k = 0; k < n_prompts; k++)
            n_embd_count += inputs[k].size();
    else
        n_embd_count = n_prompts;

    // allocate output
    const int n_embd = llama_model_n_embd(model);
    std::vector<float> embeddings(n_embd_count * n_embd, 0);
    float* emb = embeddings.data();

    // break into batches
    int e = 0; // number of embeddings already stored
    int s = 0; // number of prompts in current batch
    for (int k = 0; k < n_prompts; k++) {
        // clamp to n_batch tokens
        auto& inp = inputs[k];

        const uint64_t n_toks = inp.size();

        // encode if at capacity
        if (batch.n_tokens + n_toks > n_batch) {
            float* out = emb + e * n_embd;
            batch_decode(ctx, batch, out, s, n_embd, params_.embd_normalize);
            e += pooling_type == LLAMA_POOLING_TYPE_NONE ? batch.n_tokens : s;
            s = 0;
            common_batch_clear(batch);
        }

        // add to batch
        batch_add_seq(batch, inp, s);
        s += 1;
    }

    // final batch
    float* out = emb + e * n_embd;
    batch_decode(ctx, batch, out, s, n_embd, params_.embd_normalize);

    llama_perf_context_print(ctx);

    // clean up
    llama_batch_free(batch);
    llama_backend_free();

    return embeddings;
}

auto LocalEmbedder::instance() -> LocalEmbedder& {
    return *instance_;
}

void LocalEmbedder::init(fs::path const& model, int n_threads) {
    if (!instance_)
        instance_.reset(new LocalEmbedder{model, n_threads});
}

void LocalEmbedder::init(int n_threads) {
#ifndef MUCLAW_ASSETS_DIR
#error "MUCLAW_ASSETS_DIR must be defined"
#endif
    LocalEmbedder::init(std::filesystem::path(MUCLAW_ASSETS_DIR) / "models/all-MiniLM-L6-v2-f16.gguf", n_threads);
}
} // namespace muclaw