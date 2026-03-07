#pragma once

#include "asio_curl.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>

#include <string>
#include <string_view>

namespace muclaw {

class LlmClient {
public:
    LlmClient(boost::asio::io_context& io, std::string host, std::string api_key);

    auto chat_completion(std::string_view system_prompt, std::string_view user_prompt)
        -> boost::asio::awaitable<std::string>;

private:
    auto post_request(std::string_view target, std::string const& body) -> boost::asio::awaitable<std::string>;

    AsioCurl curl_;
    std::string host_;
    std::string api_key_;
};

} // namespace muclaw
