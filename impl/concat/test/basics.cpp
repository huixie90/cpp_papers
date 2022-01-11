#include "concat.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

#define TEST_POINT(x) TEST_CASE(x, "[basics]")

TEST_POINT("motivation") {
    using V = std::vector<int>;
    V v1{1, 2, 3}, v2{4, 5};
    std::ranges::concat_view cv{v1,v2};
    static_assert(std::ranges::range<decltype(cv)>);
    REQUIRE(std::ranges::size(cv) == 5);


}


TEST_POINT("Sentinel") {
    using V = std::vector<int>;
    using W = std::list<int>;
    
    // add non-trivial combinations of underlying ranges/views and concept checks for
    // - sentinel size independent of number of ranges
    // - sentinel cross-const comparison
    // - sentinel being default constructible or not mirroring on last view's property
    // - ...
}

