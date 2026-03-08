#include "database.hpp"
#include "logger.hpp"

#include <boost/asio/awaitable.hpp>
#include <cstddef>
#include <fmt/core.h>
#include <fmt/format.h>
#include <optional>
#include <stdexcept>

#include <fstream>
#include <sstream>

#include <boost/asio/post.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace muclaw {

Database::~Database() {
    close();
}

auto Database::open(std::string_view path) -> void {
    if (db_)
        close();

    int rc = sqlite3_open(std::string{path}.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string err = db_ ? sqlite3_errmsg(db_) : "Unknown error";
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
        log::error("SQLite open failed: {}", err);
        throw std::runtime_error(fmt::format("Failed to open db: {}", err));
    }

    log::info("SQLite opened successfully at {}", path);

    init_schema();
}

auto Database::close() -> void {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
        log::info("SQLite closed.");
    }
}

auto Database::execute(std::string_view sql) -> void {
    if (!db_)
        throw std::runtime_error("Database is not open");

    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, std::string{sql}.c_str(), nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK) {
        std::string err = err_msg;
        sqlite3_free(err_msg);
        log::error("SQLite execute failed: {} \nSQL: {}", err, sql);
        throw std::runtime_error(fmt::format("Failed to execute SQL: {}", err));
    }
}

auto Database::query_first_string(std::string_view sql, std::vector<std::string> const& args)
    -> std::optional<std::string> {
    if (!db_)
        return std::nullopt;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.data(), sql.size(), &stmt, nullptr);
    if (rc != SQLITE_OK) {
        log::error("SQLite prepare failed: {}", sqlite3_errmsg(db_));
        return std::nullopt;
    }

    for (size_t i = 0; i < args.size(); ++i)
        sqlite3_bind_text(stmt, i + 1, args[i].c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    std::optional<std::string> result = std::nullopt;

    if (rc == SQLITE_ROW) {
        auto* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (text)
            result = std::string(text);
    }

    sqlite3_finalize(stmt);
    return result;
}

auto Database::execute_async(std::string sql) -> boost::asio::awaitable<void> {
    if (pool_)
        co_await boost::asio::post(*pool_, boost::asio::use_awaitable);
    execute(sql);
}

auto Database::init_schema() -> void {
#ifndef MUCLAW_ASSETS_DIR
#error "MUCLAW_ASSETS_DIR must be defined"
#endif
    std::string schema_path = std::string(MUCLAW_ASSETS_DIR) + "/db_schema.sql";
    std::ifstream file(schema_path);
    if (!file.is_open()) {
        log::error("Could not open schema file at {}", schema_path);
        throw std::runtime_error(fmt::format("Could not open schema file at {}", schema_path));
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string schema_sql = buffer.str();

    execute(schema_sql);

    log::info("SQLite schema and triggers initialized successfully from {}.", schema_path);
}

} // namespace muclaw
