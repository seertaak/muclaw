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

namespace muclaw {

TelegramClient::TelegramClient(asio::io_context& io, std::string token)
    : io_{io}, curl_{io}, token_{std::move(token)} {}

auto TelegramClient::post_request(std::string_view method, std::string const& body, std::chrono::milliseconds timeout)
    -> asio::awaitable<std::string> {
    auto target = fmt::format("/bot{}/{}", token_, method);
    log::info("Targeting: {}", target);

    std::vector<HttpHeader> headers = {{"Content-Type", "application/json"}};

    co_return co_await curl_.post("api.telegram.org", target, body, headers, timeout);
}

auto TelegramClient::send_message(int64_t chat_id, std::string_view text) -> asio::awaitable<int64_t> {
    nlohmann::json req_body = {{"chat_id", chat_id}, {"text", text}};
    auto res_str = co_await post_request("sendMessage", req_body.dump());
    
    try {
        auto res_json = nlohmann::json::parse(res_str);
        if (res_json["ok"].get<bool>()) {
            co_return res_json["result"]["message_id"].get<int64_t>();
        } else {
             log::error("sendMessage failed: {}", res_str);
        }
    } catch (std::exception const& e) {
        log::error("Failed to parse sendMessage response: {}", e.what());
    }
    
    co_return -1;
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
            nlohmann::json req_body = {{"offset", offset_}, {"timeout", 20}};

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

auto TelegramClient::wait_for_reply(int64_t expected_chat_id, std::chrono::milliseconds total_timeout, int64_t after_message_id)
    -> asio::awaitable<std::optional<std::string>> {

    auto start_time = std::chrono::steady_clock::now();

    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
        auto remaining = total_timeout - elapsed;

        if (remaining.count() <= 0) {
            log::info("wait_for_reply timed out");
            co_return std::nullopt;
        }

        // We use long polling, tell telegram server to hold connection open for `poll_timeout_sec`
        auto poll_timeout_sec = std::min<int>(20, std::max<int>(1, remaining.count() / 1000));
        // Add a few seconds buffer to the HTTP client timeout so it doesn't kill the connection before telegram replies empty
        auto http_timeout = std::chrono::seconds(poll_timeout_sec + 5);

        nlohmann::json req_body = {{"offset", offset_}, {"timeout", poll_timeout_sec}};
        std::string res_str;
        bool request_success = false;

        try {
            res_str = co_await post_request("getUpdates", req_body.dump(), http_timeout);
            request_success = true;
        } catch (std::exception const& e) {
            log::error("Telegram connection error during wait_for_reply: {}", e.what());
        }

        if (request_success) {
            bool parse_success = false;
            nlohmann::json res_json;

            try {
                res_json = nlohmann::json::parse(res_str);
                parse_success = true;
            } catch (std::exception const& e) {
                 log::error("Error parsing Telegram response: {}", e.what());
            }

            if (parse_success) {
                if (res_json["ok"].get<bool>()) {
                    for (auto const& update : res_json["result"]) {
                        offset_ = update["update_id"].get<int64_t>() + 1;
                        if (update.contains("message") && update["message"].contains("text")) {
                            auto text = update["message"]["text"].get<std::string>();
                            auto msg_chat_id = update["message"]["chat"]["id"].get<int64_t>();
                            auto msg_id = update["message"]["message_id"].get<int64_t>();

                            if (msg_chat_id == expected_chat_id) {
                                if (msg_id > after_message_id) {
                                    co_return text;
                                } else {
                                    log::info("Ignored old message ({}) during wait_for_reply", msg_id);
                                }
                            } else {
                                log::info("Ignored message from another chat during wait_for_reply: {}", msg_chat_id);
                            }
                        }
                    }
                } else {
                    log::error("Telegram polling error during wait_for_reply: {}", res_str);
                    boost::asio::steady_timer timer{io_, std::chrono::seconds(1)};
                    co_await timer.async_wait(asio::use_awaitable);
                }
            } else {
                boost::asio::steady_timer timer{io_, std::chrono::seconds(1)};
                co_await timer.async_wait(asio::use_awaitable);
            }
        } else {
            boost::asio::steady_timer timer{io_, std::chrono::seconds(1)};
            co_await timer.async_wait(asio::use_awaitable);
        }
    }
}

} // namespace muclaw
