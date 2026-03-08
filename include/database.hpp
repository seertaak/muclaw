#pragma once

#include <sqlite3.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/thread_pool.hpp>

namespace muclaw {

// Simple wrapper around sqlite3* to handle RAII and basic queries
class Database {
public:
    Database() = default;
    ~Database();

    // Prevent copies
    Database(Database const&) = delete;
    auto operator=(Database const&) -> Database& = delete;

    Database(Database&&) = default;
    auto operator=(Database&&) -> Database& = default;

    auto set_pool(boost::asio::thread_pool& pool) -> void {
        pool_ = &pool;
    }

    auto open(std::string_view path) -> void;
    auto close() -> void;

    auto execute(std::string_view sql) -> void;
    auto query_first_string(std::string_view sql, std::vector<std::string> const& args = {})
        -> std::optional<std::string>;
    auto execute_async(std::string sql) -> boost::asio::awaitable<void>;

private:
    boost::asio::thread_pool* pool_{nullptr};
    sqlite3* db_{nullptr};

    auto init_schema() -> void;
};

} // namespace muclaw
