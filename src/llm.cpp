#include "llm.hpp"
#include "logger.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

namespace asio = boost::asio;

namespace muclaw {

LlmClient::LlmClient(asio::io_context& io, std::string host, std::string api_key)
    : curl_{io}, host_{std::move(host)}, api_key_{std::move(api_key)} {}

auto LlmClient::post_request(std::string_view target, std::string const& body) -> asio::awaitable<std::string> {
    std::vector<HttpHeader> headers = {{"Content-Type", "application/json"},
                                       {"Authorization", fmt::format("Bearer {}", api_key_)}};

    co_return co_await curl_.post(host_, target, body, headers);
}

auto LlmClient::chat_completion(std::string_view system_prompt, std::string_view user_prompt)
    -> asio::awaitable<std::string> {
    nlohmann::json req_body = {
        {"messages", {{{"role", "system"}, {"content", system_prompt}}, {{"role", "user"}, {"content", user_prompt}}}},
        {"model", "gpt-3.5-turbo"} // Fallback or configurable later
    };

    try {
        log::info("Sending LLM completion request to {}...", host_);
        auto res_str = co_await post_request("/v1/chat/completions", req_body.dump());
        auto res_json = nlohmann::json::parse(res_str);

        if (res_json.contains("choices") && !res_json["choices"].empty()) {
            co_return res_json["choices"][0]["message"]["content"].get<std::string>();
        } else {
            log::error("LLM completion error response: {}", res_str);
            co_return "Error: Unexpected LLM response format.";
        }
    } catch (std::exception const& e) {
        log::error("LLM connection error: {}", e.what());
        co_return "Error: Could not reach LLM API.";
    }
}

} // namespace muclaw
