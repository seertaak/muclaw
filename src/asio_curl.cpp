#include "asio_curl.hpp"
#include "logger.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>

#include <chrono>
#include <stdexcept>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace ssl = asio::ssl;
using tcp = asio::ip::tcp;

namespace nclaw {

AsioCurl::AsioCurl(asio::io_context& io)
    : io_{io}, ssl_ctx_{ssl::context::tlsv12_client} {
    ssl_ctx_.set_default_verify_paths();
    ssl_ctx_.set_verify_mode(ssl::verify_peer);
}

auto AsioCurl::get(std::string_view host, std::string_view target, std::vector<HttpHeader> const& headers) -> asio::awaitable<std::string> {
    co_return co_await do_request(Method::Get, host, target, "", headers);
}

auto AsioCurl::post(std::string_view host, std::string_view target, std::string const& body, std::vector<HttpHeader> const& headers) -> asio::awaitable<std::string> {
    co_return co_await do_request(Method::Post, host, target, body, headers);
}

auto AsioCurl::do_request(Method method, std::string_view host, std::string_view target, std::string const& body, std::vector<HttpHeader> const& headers) -> asio::awaitable<std::string> {
    auto port = "443";
    std::string host_str{host};

    tcp::resolver resolver(io_);
    beast::ssl_stream<beast::tcp_stream> stream(io_, ssl_ctx_);

    if (!SSL_set_tlsext_host_name(stream.native_handle(), host_str.c_str())) {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()};
        throw beast::system_error{ec};
    }

    auto const results = co_await resolver.async_resolve(host_str, port, asio::use_awaitable);

    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
    co_await beast::get_lowest_layer(stream).async_connect(results, asio::use_awaitable);

    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
    co_await stream.async_handshake(ssl::stream_base::client, asio::use_awaitable);

    http::verb b_method = (method == Method::Get) ? http::verb::get : http::verb::post;
    http::request<http::string_body> req{b_method, target, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    
    for (auto const& h : headers) {
        req.set(h.name, h.value);
    }

    if (method == Method::Post) {
        req.body() = body;
        req.prepare_payload();
    }

    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
    co_await http::async_write(stream, req, asio::use_awaitable);

    beast::flat_buffer buffer;
    http::response<http::string_body> res;

    co_await http::async_read(stream, buffer, res, asio::use_awaitable);

    beast::error_code ec;
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(1));
    co_await stream.async_shutdown(asio::redirect_error(asio::use_awaitable, ec));

    co_await stream.async_shutdown(asio::redirect_error(asio::use_awaitable, ec));

    if (res.result() != http::status::ok) {
        log::error("AsioCurl HTTP Request failed: {} - Body: {}", res.result_int(), res.body());
    }

    co_return res.body();
}

} // namespace nclaw
