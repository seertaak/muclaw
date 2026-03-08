#include "assistant.hpp"
#include "database.hpp"
#include "llm.hpp"
#include "local_embedder.hpp"
#include "logger.hpp"
#include "telegram.hpp"

#include <CLI/CLI.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fmt/format.h>
#include <iostream>
#include <string>
#include <string_view>

namespace asio = boost::asio;
namespace fs = std::filesystem;
using namespace muclaw;

#ifndef MUCLAW_ASSETS_DIR
#error "MUCLAW_ASSETS_DIR must be defined"
#endif

auto run_assistant(asio::io_context& io) -> asio::awaitable<void> {
    log::info("muclaw main loop starting...");

    auto tg_token = std::getenv("TELEGRAM_BOT_TOKEN");
    auto llm_key = std::getenv("GROQ_API_KEY");

    if (!tg_token || !llm_key) {
        log::error("Environment variables TELEGRAM_BOT_TOKEN and GROQ_API_KEY "
                   "must be set.");
        co_return;
    }

    Assistant assistant{io, std::string{tg_token}, "api.groq.com", std::string{llm_key}};
    co_await assistant.start();
}

auto test_telegram(int64_t chat_id, std::chrono::seconds timeout) -> int {
    asio::io_context io{};
    auto tg_token = std::getenv("TELEGRAM_BOT_TOKEN");
    if (!tg_token) {
        log::error("Environment variable TELEGRAM_BOT_TOKEN must be set.");
        return 1;
    }
    TelegramClient tg{io, std::string{tg_token}};
    asio::co_spawn(
        io,
        [&]() -> asio::awaitable<void> {
            log::info("Sending test message to chat id {}...", chat_id);
            auto sent_msg_id = co_await tg.send_message(
                chat_id, "Hi -- I'm muclaw! Please confirm you can see me by replying with any message in this chat.");

            log::info("Waiting for reply (timeout: {}s, after message {})...", timeout.count(), sent_msg_id);
            auto reply = co_await tg.wait_for_reply(chat_id, timeout, sent_msg_id);

            if (reply) {
                log::info("Reply received: {}", *reply);
                co_await tg.send_message(
                    chat_id, fmt::format("Great, Telegram functionality is working. Here's what you said: {}", *reply));
                log::info("Success message sent.");
            } else {
                log::error("Timeout waiting for user reply in chat {}.", chat_id);
                co_await tg.send_message(chat_id, "Hey silly, you forgot to reply!");
            }
        },
        asio::detached);
    io.run();
    return 0;
}

auto test_groq(std::string_view prompt) -> int {
    asio::io_context io{};
    auto llm_key = std::getenv("GROQ_API_KEY");
    if (!llm_key) {
        log::error("Environment variable GROQ_API_KEY must be set.");
        return 1;
    }
    LlmClient llm{io, "api.groq.com", std::string{llm_key}};
    asio::co_spawn(
        io,
        [prompt, &llm]() -> asio::awaitable<void> {
            log::info("Sending test prompt to Groq: {}", prompt);
            auto reply = co_await llm.chat_completion("You are a helpful assistant.", prompt);
            log::info("Groq Reply: {}", reply);
            co_return;
        },
        asio::detached);
    io.run();
    return 0;
}

auto test_embeddings(std::string_view text) -> int {
    log::info("Getting embeddings for: '{}'", text);
    auto vector = LocalEmbedder::instance().embed(text);
    std::cout << "std::vector<float> expected_embedding = {\n    ";
    for (size_t i = 0; i < vector.size(); ++i) {
        std::cout << fmt::format("{:.6f}f", vector[i]);
        if (i != vector.size() - 1) {
            std::cout << ", ";
            if ((i + 1) % 10 == 0)
                std::cout << "\n    ";
        }
    }
    std::cout << "\n};\n";
    return 0;
}

auto main(int argc, char** argv) -> int {
    CLI::App app{"muclaw AI Assistant"};
    app.require_subcommand(0, 1);

    int32_t n_worker_threads_opt;
    int32_t n_llama_threads_opt;
    fs::path embedding_model;

    app.add_option("-n,--n-worker-threads", n_worker_threads_opt, "Number of threads in worker pool")->default_val(1);
    app.add_option("-l,--n-llama-threads", n_llama_threads_opt, "Number of threads for llama embeddings")
        ->default_val(1);
    app.add_option("-m,--embedding-model", embedding_model, "Path to embedding model")
        ->default_val(fs::path(MUCLAW_ASSETS_DIR) / "models/all-MiniLM-L6-v2-f16.gguf");

    auto test_cmd = app.add_subcommand("test", "Test integrations");

    auto test_telegram_subcmd = test_cmd->add_subcommand("telegram", "Test telegram integration");
    int64_t chat_id;
    int32_t timeout_sec;
    test_telegram_subcmd->add_option("-c,--chat", chat_id, "Telegram Chat ID to send the test message to")->required();
    test_telegram_subcmd->add_option("-t,--timeout", timeout_sec, "Timeout in seconds to wait for a reply")
        ->default_val(30);

    auto test_groq_subcmd = test_cmd->add_subcommand("groq", "Test Groq integration");
    std::string prompt = "Hello Groq! What can you do?";
    test_groq_subcmd->add_option("-p,--prompt", prompt, "Prompt to send to Groq model");

    auto print_embeddings_subcmd = app.add_subcommand("print-embeddings", "Print raw embeddings for text");
    std::string text;
    print_embeddings_subcmd->add_option("text", text, "Text to embed")->required();

    CLI11_PARSE(app, argc, argv);

    try {
        log::info("muclaw booting...");

        if (test_telegram_subcmd->parsed())
            return test_telegram(chat_id, std::chrono::seconds(timeout_sec));
        else if (test_groq_subcmd->parsed())
            return test_groq(prompt);
        else {
            LocalEmbedder::init(embedding_model, n_llama_threads_opt);

            if (print_embeddings_subcmd->parsed())
                return test_embeddings(text);
            else {

                Database db{};
                db.open("test_data.db");

                asio::io_context io{};

                asio::signal_set signals{io, SIGINT, SIGTERM};
                signals.async_wait([&](auto, auto) {
                    log::info("Termination signal received. Shutting down io_context...");
                    io.stop();
                });

                asio::co_spawn(io, run_assistant(io), asio::detached);

                io.run();

                log::info("muclaw exited cleanly.");
            }
        }
    } catch (std::exception const& e) {
        log::error("Fatal exception: {}", e.what());
        return 1;
    }

    return 0;
}
