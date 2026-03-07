#pragma once

#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace muclaw {

struct HttpHeader {
    std::string name;
    std::string value;
};

class AsioCurl {
public:
    explicit AsioCurl(boost::asio::io_context& io);

    auto get(std::string_view host, std::string_view target, std::vector<HttpHeader> const& headers = {})
        -> boost::asio::awaitable<std::string>;

    auto post(std::string_view host, std::string_view target, std::string const& body,
              std::vector<HttpHeader> const& headers = {}) -> boost::asio::awaitable<std::string>;

private:
    enum class Method { Get, Post };

    auto do_request(Method method, std::string_view host, std::string_view target, std::string const& body,
                    std::vector<HttpHeader> const& headers) -> boost::asio::awaitable<std::string>;

    boost::asio::io_context& io_;
    boost::asio::ssl::context ssl_ctx_;
};

} // namespace muclaw
