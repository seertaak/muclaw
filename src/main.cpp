#include "assistant.hpp"
#include "database.hpp"
#include "telegram.hpp"
#include "logger.hpp"

#include <CLI/CLI.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include <cstdlib>
#include <exception>
#include <string>

namespace asio = boost::asio;
using namespace nclaw;

auto run_assistant(asio::io_context& io) -> asio::awaitable<void> {
    log::info("muclaw main loop starting...");
    
    auto tg_token = std::getenv("TELEGRAM_BOT_TOKEN");
    auto llm_key = std::getenv("OPENAI_API_KEY");

    if (!tg_token || !llm_key) {
        log::error("Environment variables TELEGRAM_BOT_TOKEN and OPENAI_API_KEY must be set.");
        co_return;
    }

    Assistant assistant{io, std::string{tg_token}, "api.openai.com", std::string{llm_key}};
    co_await assistant.start();
}

auto main(int argc, char** argv) -> int {
    CLI::App app{"muclaw AI Assistant"};
    app.require_subcommand(0, 1);

    auto test_cmd = app.add_subcommand("test", "Test integrations");
    
    auto test_tg = test_cmd->add_subcommand("telegram", "Test telegram integration");
    int64_t chat_id = 0;
    test_tg->add_option("-c,--chat", chat_id, "Telegram Chat ID to send the test message to")->required();
    
    auto test_llm = test_cmd->add_subcommand("llm", "Test LLM integration");

    CLI11_PARSE(app, argc, argv);

    try {
        log::info("muclaw booting...");

        if (test_tg->parsed()) {
            asio::io_context io{};
            auto tg_token = std::getenv("TELEGRAM_BOT_TOKEN");
            if (!tg_token) {
                log::error("Environment variable TELEGRAM_BOT_TOKEN must be set.");
                return 1;
            }
            TelegramClient tg{io, std::string{tg_token}};
            asio::co_spawn(io, [&]() -> asio::awaitable<void> {
                log::info("Sending test message to chat id {}...", chat_id);
                co_await tg.send_message(chat_id, "Hello, World! from muclaw test");
                log::info("Test message sent successfully.");
            }, asio::detached);
            io.run();
            return 0;
        }

        if (test_llm->parsed()) {
            log::info("LLM test not yet built.");
            return 0;
        }

        // Default daemon running mode
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
    } catch (std::exception const& e) {
        log::error("Fatal exception: {}", e.what());
        return 1;
    }

    return 0;
}
