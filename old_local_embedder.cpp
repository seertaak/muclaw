#include "local_embedder.hpp"
#include "logger.hpp"

#include <algorithm>
#include <llama.h>

#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace muclaw {

LocalEmbedder::LocalEmbedder(std::string const& model_path) {
    llama_backend_init();

    llama_model_params model_params = llama_model_default_params();
    model_ = llama_model_load_from_file(model_path.c_str(), model_params);
    if (!model_)
        throw std::runtime_error("Failed to load llama model from " + model_path);

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 4096;
    ctx_params.n_batch = 2048;
    ctx_params.n_ubatch = 512; // Crucial for non-causal (encoder) models!
    ctx_params.embeddings = true;
    ctx_params.kv_unified = true; // Required since n_parallel == 1 fallback internally uses unified cache

    ctx_ = llama_init_from_model(model_, ctx_params);
    if (!ctx_) {
        llama_model_free(model_);
        throw std::runtime_error("Failed to create llama context");
    }

    log::info("Successfully loaded local embedding model: {}", model_path);
}

LocalEmbedder::~LocalEmbedder() {
    if (ctx_)
        llama_free(ctx_);
    if (model_)
        llama_model_free(model_);
    llama_backend_free();
}

auto LocalEmbedder::embed(std::string_view text) -> std::vector<float> {
    // 1. Tokenize
    int const max_tokens = 512;
    std::vector<llama_token> tokens(max_tokens);
    const struct llama_vocab* vocab = llama_model_get_vocab(model_);
    int n_tokens = llama_tokenize(vocab, text.data(), text.size(), tokens.data(), max_tokens, true, false);

    if (n_tokens < 0) {
        log::error("Failed to tokenize text for embedding. Try increasing max_tokens. Needed: {}", -n_tokens);
        return {};
    }
    tokens.resize(n_tokens);

    // 2. Prepare batch
    llama_batch batch = llama_batch_init(n_tokens, 0, 1);
    batch.n_tokens = 0;
    for (int i = 0; i < n_tokens; ++i) {
        batch.token[batch.n_tokens] = tokens[i];
        batch.pos[batch.n_tokens] = i;
        batch.n_seq_id[batch.n_tokens] = 1;
        batch.seq_id[batch.n_tokens][0] = 0;
        batch.logits[batch.n_tokens] = true;
        batch.n_tokens++;
    }

    // Always ensure clean KV state for encoder
    llama_memory_clear(llama_get_memory(ctx_), true);

    // Debug tokens
    std::string tdbg = "Tokens: ";
    for (int i = 0; i < n_tokens; ++i)
        tdbg += std::to_string(tokens[i]) + " ";
    log::info("{}", tdbg);

    // 3. Decode
    if (llama_decode(ctx_, batch) != 0) {
        log::error("llama_decode failed during embedding");
        llama_batch_free(batch);
        return {};
    }

    // 4. Extract embedding (seq_id = 0)
    int const n_embd = llama_model_n_embd(model_);
    auto const* embd = llama_get_embeddings_seq(ctx_, 0);

    if (embd) {
        log::info("Found pooled embedding for sequence 0");
    } else {
        log::info("No sequence embedding found. Falling back to general embeddings");
        embd = llama_get_embeddings(ctx_);
    }

    if (!embd) {
        log::error("Failed to get embeddings from llama_context");
        llama_batch_free(batch);
        return {};
    }

    std::vector<float> res(n_embd);

    // Normalize (euclidean, norm=2)
    double sum = 0.0;
    for (int i = 0; i < n_embd; i++)
        sum += embd[i] * embd[i];
    sum = std::sqrt(sum);
    const float norm = sum > 0.0 ? 1.0f / sum : 0.0f;

    for (int i = 0; i < n_embd; i++)
        res[i] = embd[i] * norm;

    // Debug print
    std::string dbg = "Embedding snippet: ";
    for (int i = 0; i < std::min(5, n_embd); ++i)
        dbg += std::to_string(res[i]) + " ";
    log::info("{}", dbg);

    llama_batch_free(batch);
    return res;
}

} // namespace muclaw
