#include "telegram.hpp"
#include "logger.hpp"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <chrono>

namespace asio = boost::asio;

namespace nclaw {

TelegramClient::TelegramClient(asio::io_context& io, std::string token)
    : io_{io}, curl_{io}, token_{std::move(token)} {
}

auto TelegramClient::post_request(std::string_view method, std::string const& body) -> asio::awaitable<std::string> {
    auto target = fmt::format("/bot{}/{}", token_, method);
    log::info("Targeting: {}", target);
    
    std::vector<HttpHeader> headers = {
        {"Content-Type", "application/json"}
    };

    co_return co_await curl_.post("api.telegram.org", target, body, headers);
}

auto TelegramClient::send_message(int64_t chat_id, std::string_view text) -> asio::awaitable<void> {
    nlohmann::json req_body = {
        {"chat_id", chat_id},
        {"text", text}
    };
    co_await post_request("sendMessage", req_body.dump());
}

auto TelegramClient::poll(MessageHandler handler) -> asio::awaitable<void> {
    log::info("Starting Telegram poll loop...");
    bool delay_needed = false;
    
    while (true) {
        if (delay_needed) {
            boost::asio::steady_timer timer{io_, std::chrono::seconds(5)};
            co_await timer.async_wait(asio::use_awaitable);
            delay_needed = false;
        }

        try {
            nlohmann::json req_body = {
                {"offset", offset_},
                {"timeout", 20}
            };
            
            auto res_str = co_await post_request("getUpdates", req_body.dump());
            auto res_json = nlohmann::json::parse(res_str);
            
            if (res_json["ok"].get<bool>()) {
                for (auto const& update : res_json["result"]) {
                    offset_ = update["update_id"].get<int64_t>() + 1;
                    if (update.contains("message") && update["message"].contains("text")) {
                        auto text = update["message"]["text"].get<std::string>();
                        auto chat_id = update["message"]["chat"]["id"].get<int64_t>();
                        
                        log::info("Received message: {}", text);
                        
                        // Dispatch to handler
                        if (handler) {
                            co_await handler(chat_id, text);
                        }
                    }
                }
            } else {
                log::error("Telegram polling error: {}", res_str);
                delay_needed = true;
            }
        } catch (std::exception const& e) {
            log::error("Telegram connection error: {}", e.what());
            // Backoff on error
            delay_needed = true;
        }
    }
}

} // namespace nclaw
