#include "vector_db.hpp"

#include <catch2/catch_test_macros.hpp>
#include <vector>

#include <boost/asio/thread_pool.hpp>

TEST_CASE("VectorDB basic operations", "[vector_db]") {
    boost::asio::thread_pool pool{1};
    muclaw::VectorDB db{pool, 128};

    std::vector<float> x(128, 0.1f);
    db.add(x, 1);

    std::vector<float> y(128, 0.2f);
    db.add(y, 2);

    std::vector<float> x_dash(128, 0.11f);

    auto results = db.search(x_dash, 1);
    REQUIRE(results.size() == 1);
    REQUIRE(results[0].id == 1);
}
