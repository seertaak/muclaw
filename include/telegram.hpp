#pragma once

#include "asio_curl.hpp"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace muclaw {

class TelegramClient {
public:
    TelegramClient(boost::asio::io_context& io, std::string token);

    using MessageHandler = std::function<boost::asio::awaitable<void>(int64_t chat_id, std::string text)>;

    auto poll(MessageHandler handler) -> boost::asio::awaitable<void>;
    auto send_message(int64_t chat_id, std::string_view text) -> boost::asio::awaitable<int64_t>;
    auto wait_for_reply(int64_t chat_id, std::chrono::milliseconds timeout, int64_t after_message_id)
        -> boost::asio::awaitable<std::optional<std::string>>;

private:
    auto post_request(std::string_view method, std::string const& body,
                      std::chrono::milliseconds timeout = std::chrono::seconds(30))
        -> boost::asio::awaitable<std::string>;

    boost::asio::io_context& io_;
    AsioCurl curl_;
    std::string token_;
    int64_t offset_{0};
};

} // namespace muclaw
