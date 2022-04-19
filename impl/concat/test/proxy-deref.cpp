#include "concat.hpp"

#include <catch2/catch_test_macros.hpp>
#define TEST_POINT(x) TEST_CASE(x, "[proxy-deref]")


TEST_POINT("sg9 disappearing rvalue") {
    std::vector<std::string> v{"abc", "def", "ghi"};
    auto v1 =
        std::views::iota(0, 4) | std::views::transform([](auto x) { return std::string(x, 'c'); });
    auto v2 = v | std::views::transform([](auto& x) -> decltype(auto) { return std::move(x); });
    std::ranges::random_access_range auto cc = std::views::concat(v2, v1);

    *cc.begin();
    CHECK(std::string(*cc.begin()) == "abc");
}
