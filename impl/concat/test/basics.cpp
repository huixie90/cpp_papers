#include "concat.hpp"
#include <catch2/catch_test_macros.hpp>
#include <list>
#include <vector>
#include <memory>
#include <functional>

#define TEST_POINT(x) TEST_CASE(x, "[basics]")

namespace {

template <typename... Ts>
concept concat_view_of = requires(Ts&&... ts) {
    {std::ranges::concat_view{static_cast<Ts&&>(ts)...}};
};

template <typename R>
R f(int);

template <typename R>
using make_view_of = decltype(std::views::iota(0) | std::views::transform(f<R>));

struct Foo {};

} // namespace

TEST_POINT("concept") {
    static_assert(!concat_view_of<>, "empty not allowed");
    static_assert(concat_view_of<std::vector<int>&>, "single viewable range");
    static_assert(concat_view_of<std::vector<int>&, std::vector<int>&>, "two same type");
    static_assert(
        concat_view_of<std::vector<int>&, std::vector<int>&, std::vector<int>&, std::vector<int>&>,
        "multiple same type");

    static_assert(concat_view_of<std::vector<int>&, make_view_of<int&>>,
                  "two different type same reference");
    static_assert(concat_view_of<std::vector<int>&, std::vector<int> const&>,
                  "two with convertible reference");
    static_assert(concat_view_of<std::vector<int>&, std::vector<std::reference_wrapper<int>>&>,
                  "two with convertible reference");
    static_assert(!concat_view_of<std::vector<int>&, std::vector<Foo>&>, "inconvertible reference");

    static_assert(concat_view_of<make_view_of<int&>, make_view_of<int&&>,
                                 make_view_of<int>>,
                  "common reference is prvalue");

    static_assert(concat_view_of<make_view_of<std::unique_ptr<int>>,
                                 make_view_of<std::unique_ptr<int>&&>>,
                  "common reference is prvalue move-only and no lvalue reference");

    static_assert(!concat_view_of<make_view_of<std::unique_ptr<int>>,
                                  make_view_of<std::unique_ptr<int>&>>,
                  "common reference is prvalue move-only but with lvalue reference");
}

TEST_POINT("motivation") {
    using V = std::vector<int>;
    V v1{1, 2, 3}, v2{4, 5};
    std::ranges::concat_view cv{v1, v2};
    // static_assert(std::ranges::range<decltype(cv)>);
    REQUIRE(std::ranges::size(cv) == 5);
}

TEST_POINT("begin_basic") {

    using V = std::vector<int>;
    V v1{}, v2{4, 5}, v3{6};
    std::ranges::concat_view cv{v1, v2, v3};
    auto it = cv.begin();
    static_assert(std::same_as<decltype(*it), int&>);
    REQUIRE(*it == 4);
}

TEST_POINT("end_basic_common_range") {
    using V = std::vector<int>;
    V v1{}, v2{4, 5}, v3{6};
    std::ranges::concat_view cv{v1, v2, v3};
    auto it = cv.begin();
    auto st = cv.end();
    static_assert(std::same_as<decltype(it), decltype(st)>);
    auto it2 = std::as_const(cv).begin();
    auto st2 = std::as_const(cv).end();
    static_assert(std::same_as<decltype(it2), decltype(st2)>);
}

TEST_POINT("Sentinel") {
    // using V = std::vector<int>;
    // using W = std::list<int>;

    // add non-trivial combinations of underlying ranges/views and concept checks for
    // - sentinel size independent of number of ranges
    // - sentinel cross-const comparison
    // - sentinel being default constructible or not mirroring on last view's property
    // - ...
}