#include "asio_curl.hpp"
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <catch2/catch_test_macros.hpp>
#include <exception>
#include <string>

TEST_CASE("AsioCurl GET request", "[asio_curl]") {
    boost::asio::io_context io;
    muclaw::AsioCurl curl{io};

    bool completed = false;
    boost::asio::co_spawn(
        io,
        [&]() -> boost::asio::awaitable<void> {
            try {
                // A simple HTTPS get to httpbin
                auto result = co_await curl.get("httpbin.org", "/get");
                REQUIRE(result.find("\"url\": \"https://httpbin.org/get\"") != std::string::npos);
            } catch (...) {
                FAIL("Request threw an exception");
            }
            completed = true;
        },
        boost::asio::detached);

    io.run();
    REQUIRE(completed == true);
}
