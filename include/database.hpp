#pragma once

#include <rocksdb/db.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace muclaw {

class Database {
public:
    Database() = default;
    ~Database();

    // Prevent copies
    Database(Database const&) = delete;
    auto operator=(Database const&) -> Database& = delete;

    Database(Database&&) = default;
    auto operator=(Database&&) -> Database& = default;

    auto open(std::string_view path) -> void;
    auto close() -> void;

    auto put(std::string_view k, std::string_view v) -> void;
    auto value(std::string_view k) const -> std::optional<std::string>;

private:
    std::unique_ptr<rocksdb::DB> db{};
};

} // namespace muclaw
