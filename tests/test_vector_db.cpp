#include "vector_db.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

TEST_CASE("VectorDatabase basic operations", "[vector_db]") {
    muclaw::VectorDatabase db{128}; // dimension 128

    std::vector<float> vec1(128, 0.1f);
    db.add(vec1, 1);

    std::vector<float> vec2(128, 0.2f);
    db.add(vec2, 2);

    auto results = db.search(vec1, 1);
    REQUIRE(results.size() == 1);
    REQUIRE(results[0].id == 1); // Exact match should be closest
}
