#include "concat.hpp"
#include <catch2/catch_test_macros.hpp>

#include <vector>
#include <ranges>
#include <functional>

#define TEST_POINT(x) TEST_CASE(x, "[example]")

TEST_POINT("example 1: vector, array, and scalar") {
    std::vector<int> v1{1, 2, 3}, v2{4, 5}, v3{};
    std::array a{6, 7, 8};
    auto s = std::ranges::single_view(9);
    auto cv = std::views::concat(v1, v2, v3, a, s);

    std::vector expected{1, 2, 3, 4, 5, 6, 7, 8, 9};
    for (auto i = 0; i != 9; ++i) {
        REQUIRE(cv[i] == expected[i]);
    }
}

struct Foo {
    int i;
};

struct Bar {
    Foo foo_;
    const Foo& getFoo() const {
        return foo_;
    };
};

struct MyClass {
    Foo foo_;
    std::vector<Bar> bars_;

    auto getFoos() const {
        return std::views::concat(
            std::ranges::single_view(std::cref(foo_)) |
                std::views::transform([](auto&& w) -> const Foo& { return w.get(); }),
            bars_ | std::views::transform(&Bar::getFoo));
    }
};

TEST_POINT("example 2: segmented data") {
    MyClass c{Foo{5}, std::vector<Bar>{
                          Bar{Foo{2}},
                          Bar{Foo{7}},
                          Bar{Foo{9}},
                      }};

    std::vector expected{5, 2, 7, 9};
    for (int i = 0; const Foo& foo : c.getFoos()) {
        REQUIRE(foo.i == expected[i]);
        ++i;
    }
}