#include "database.hpp"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <string>

TEST_CASE("Database basic operations", "[database]") {
    std::filesystem::remove_all("test_db");

    muclaw::Database db;
    db.open("test_db");

    db.put("key1", "value1");

    auto val = db.value("key1");
    REQUIRE(val.has_value());
    REQUIRE(*val == "value1");

    auto missing = db.value("missing_key");
    REQUIRE(!missing.has_value());

    std::filesystem::remove_all("test_db");
}
