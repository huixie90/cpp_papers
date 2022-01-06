#include "concat.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

TEST_CASE("Hello world", "[basics]") {
    using V = std::vector<int>;
    V v1{1, 2, 3}, v2{4, 5};
    std::ranges::concat_view<V, V> cv;
    static_assert(std::ranges::range<decltype(cv)>);
    REQUIRE(true);
}
