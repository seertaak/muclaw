#include "assistant.hpp"
#include "logger.hpp"

#include <fmt/core.h>

namespace asio = boost::asio;

namespace nclaw {

Assistant::Assistant(asio::io_context& io, std::string telegram_token, std::string llm_host, std::string llm_api_key)
    : vdb_{1536},
      llm_{io, std::move(llm_host), std::move(llm_api_key)},
      tg_{io, std::move(telegram_token)} {
    db_.open("nanoclaw_db");
}

auto Assistant::handle_message(int64_t chat_id, std::string text) -> asio::awaitable<void> {
    log::info("Handling message from chat {}: {}", chat_id, text);
    
    // Simple initial logic: Pass message straight to LLM
    auto reply = co_await llm_.chat_completion(
        "You are muclaw, a helpful technical AI assistant built in C++23.",
        text
    );

    co_await tg_.send_message(chat_id, reply);
}

auto Assistant::start() -> asio::awaitable<void> {
    log::info("Starting muclaw Assistant orchestration...");
    
    co_await tg_.poll([this](int64_t chat_id, std::string text) {
        return handle_message(chat_id, std::move(text));
    });
}

} // namespace nclaw
