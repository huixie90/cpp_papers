#include "concat.hpp"
#include <catch2/catch_test_macros.hpp>
#include <list>
#include <vector>
#include <functional>
#include <memory>
#define TEST_POINT(x) TEST_CASE(x, "[basics]")

namespace {

template <typename... R>
concept concat_viewable = requires(R&&... r) {
    std::ranges::views::concat((R &&) r...);
};


template <typename R>
R f(int);

template <typename R>
using make_view_of = decltype(std::views::iota(0) | std::views::transform(f<R>));

struct Foo {};

struct Bar : Foo {};
struct Qux : Foo {};

struct MoveOnly {
    MoveOnly(MoveOnly const&) = delete;
    MoveOnly& operator=(MoveOnly const&) = delete;
    MoveOnly(MoveOnly&&) = default;
    MoveOnly& operator=(MoveOnly&&) = default;
};


static_assert(std::movable<MoveOnly>);
static_assert(!std::copyable<MoveOnly>);

} // namespace



TEST_POINT("motivation") {
    using V = std::vector<int>;
    V v1{1, 2, 3}, v2{4, 5};
    std::ranges::concat_view cv{v1, v2};
    // static_assert(std::ranges::range<decltype(cv)>);
    REQUIRE(std::ranges::size(cv) == 5);
}



TEST_POINT("concept") {

    using IntV = std::vector<int>;
    using IntL = std::list<int>;
    using FooV = std::vector<Foo>;
    using BarV = std::vector<Bar>;
    using QuxV = std::vector<Qux>;

    // single arg
    STATIC_CHECK(concat_viewable<IntV&>);
    STATIC_CHECK(!concat_viewable<IntV>); // because:
    STATIC_REQUIRE(!std::ranges::viewable_range<IntV>);
    STATIC_REQUIRE(std::ranges::viewable_range<IntV&>);

    // nominal use
    STATIC_CHECK(concat_viewable<IntV&, IntV&>);
    STATIC_CHECK(concat_viewable<IntV&, IntV const&>);
    STATIC_CHECK(concat_viewable<IntV&, std::vector<std::reference_wrapper<int>>&>);
    STATIC_CHECK(concat_viewable<IntV&, IntL&, IntV&>);
    STATIC_CHECK(concat_viewable<FooV&, BarV&>);
    STATIC_CHECK(concat_viewable<BarV&, FooV&>);
    STATIC_CHECK(concat_viewable<FooV&, BarV&, QuxV const&>);
    STATIC_CHECK(concat_viewable<IntV&, make_view_of<int&>>);
    STATIC_CHECK(concat_viewable<make_view_of<int&>, make_view_of<int&&>, make_view_of<int>>);
    STATIC_CHECK(concat_viewable<make_view_of<MoveOnly>, make_view_of<MoveOnly>&&>);

    // [TODO] discuss!
    STATIC_CHECK(concat_viewable<make_view_of<MoveOnly>, make_view_of<MoveOnly>&>);
    // static_assert(!concat_view_of<make_view_of<std::unique_ptr<int>>,
    //                               make_view_of<std::unique_ptr<int>&>>,
    //               "common reference is prvalue move-only but with lvalue reference");

    // invalid concat use:
    STATIC_CHECK(!concat_viewable<>);
    STATIC_CHECK(!concat_viewable<IntV&, FooV&>);

    // Flag:
    STATIC_CHECK(!concat_viewable<BarV&, QuxV&,
                                  FooV&>); // maybe a separate proposal for an explicitly specified
                                           // value_type range? (ref_t == Foo& would work just fine
                                           // if it wasn't common_reference_t logic)
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