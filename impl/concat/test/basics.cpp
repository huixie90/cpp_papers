#include "concat.hpp"
#include <catch2/catch_test_macros.hpp>
#include <list>
#include <vector>
#include <functional>
#include <memory>
#include <numeric>

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

struct BigCopyable {
    int bigdata[1024];
};


} // namespace



TEST_POINT("motivation") {
    using V = std::vector<int>;
    V v1{1, 2, 3}, v2{4, 5};
    std::ranges::concat_view cv{v1, v2};
    static_assert(std::ranges::range<decltype(cv)>);
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
    STATIC_CHECK(concat_viewable<IntV&>);
    STATIC_CHECK(concat_viewable<IntV> == std::ranges::viewable_range<IntV>);
    // ^^ in implementations that provide owning_view IntV is viewable!

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


    // invalid concat use:
    STATIC_CHECK(!concat_viewable<>);
    STATIC_CHECK(!concat_viewable<IntV&, FooV&>);

    // common_reference_t is valid. but it is a prvalue which the 2nd range (lvalue ref) can not
    // assign to (needs copyable).
    STATIC_CHECK(!concat_viewable<make_view_of<MoveOnly>, make_view_of<MoveOnly&>>);

    // Flag:
    STATIC_CHECK(!concat_viewable<BarV&, QuxV&, FooV&>);
    //    maybe a separate proposal for an explicitly specified value_type range?
    //    ref_t == Foo& would work just fine if it wasn't common_reference_t logic.

    // Flag:
    STATIC_CHECK(concat_viewable<make_view_of<BigCopyable>, make_view_of<BigCopyable&>>);
    //    common_reference_t is BigCopyable (a temporary). 2nd range has BigCopyable& type.
    //    so that means operator* will copy an lvalue to a temporary: a valid but most likely a
    //    useless operation. Should this be ignored as programmer error and silently accepted?
    //    Trouble is it may be too subtle to notice yet common.
    //    [TODO] an example with a transformed range that returns a value from a lambda, but meant
    //           to return a reference). Is there a better  solution, diagnostic, documentation at
    //           least?
    //    [TODO] mention in Design.
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
    static_assert(std::ranges::common_range<decltype(cv)>);
    auto it = cv.begin();
    auto st = cv.end();
    static_assert(std::same_as<decltype(it), decltype(st)>);
    auto it2 = std::as_const(cv).begin();
    auto st2 = std::as_const(cv).end();
    static_assert(std::same_as<decltype(it2), decltype(st2)>);
}

TEST_POINT("end_last_range_non_common_but_random_sized") {
    std::vector<int> v1{1};
    std::ranges::iota_view<int, size_t> io{2, 3};
    std::ranges::concat_view cv{v1, io};
    static_assert(std::ranges::common_range<decltype(cv)>);

    auto it = cv.begin();
    auto st = cv.end();
    static_assert(std::same_as<decltype(it), decltype(st)>);
    auto it2 = std::as_const(cv).begin();
    auto st2 = std::as_const(cv).end();
    static_assert(std::same_as<decltype(it2), decltype(st2)>);

    REQUIRE(*it2 == 1);
    ++it2;
    REQUIRE(*it2 == 2);
    ++it2;
    REQUIRE(it2 == st2);
}

TEST_POINT("operator++") {
    using V = std::vector<int>;
    V v1{}, v2{4, 5}, v3{}, v4{6};
    std::ranges::concat_view cv{v1, v2, v3, v4};
    auto it = cv.begin();
    auto st = cv.end();

    REQUIRE(*it == 4);
    ++it;
    REQUIRE(*it == 5);
    ++it;
    REQUIRE(*it == 6);
    ++it;
    REQUIRE(it == st);
}

TEST_POINT("compare with unreachable sentinel") {
    std::vector v{1};
    std::ranges::concat_view cv{v, std::views::iota(0)};
    auto it = std::ranges::begin(cv);
    auto st = std::ranges::end(cv);
    REQUIRE(it != st);

    ++it;
    REQUIRE(it != st);

    ++it;
    REQUIRE(it != st);

    ++it;
    REQUIRE(it != st);
}



TEST_POINT("compare with reachable sentinel") {
    std::vector v{1};
    std::ranges::concat_view cv{v, std::ranges::iota_view<int, size_t>(0, 2)};

    auto it = std::ranges::begin(cv);
    auto st = std::ranges::end(cv);
    REQUIRE(it != st);

    ++it;
    REQUIRE(it != st);

    ++it;
    REQUIRE(it != st);

    ++it;
    REQUIRE(it == st);
}

constexpr int constexp_test() {
    std::ranges::concat_view cv{std::views::iota(0, 5), std::views::iota(3, 7)};
    return std::accumulate(cv.begin(), cv.end(), 0);
}

TEST_POINT("constexpr") {
    // Question: in libcxx, std::variant is not constexpr yet. can you try msvc stl to compile the
    // following line?
    // static_assert(constexp_test()==28);
}

template <typename... Ts>
using concat_view_of = decltype(std::ranges::concat_view{std::declval<Ts&&>()...});



TEST_POINT("bidirectional_concept") {
    using namespace std::ranges;
    using IntV = std::vector<int>;

    STATIC_CHECK(bidirectional_range<concat_view_of<IntV&, IntV&>>);
    STATIC_CHECK(bidirectional_range<concat_view_of<iota_view<int, int>, IntV&>>); // because
    STATIC_REQUIRE(bidirectional_range<iota_view<int, int>>);
    STATIC_REQUIRE(common_range<iota_view<int, int>>);

    STATIC_CHECK(bidirectional_range<concat_view_of<iota_view<int, size_t>, IntV&>>); // because
    STATIC_REQUIRE(bidirectional_range<iota_view<int, size_t>>);
    STATIC_REQUIRE(!common_range<iota_view<int, size_t>>);
    STATIC_REQUIRE(random_access_range<iota_view<int, size_t>>);
    STATIC_REQUIRE(sized_range<iota_view<int, size_t>>);

    STATIC_CHECK(!bidirectional_range<concat_view_of<iota_view<int>, IntV&>>); // because
    STATIC_REQUIRE(bidirectional_range<iota_view<int>>);
    STATIC_REQUIRE(!common_range<iota_view<int>>);
    STATIC_REQUIRE(random_access_range<iota_view<int>>);
    STATIC_REQUIRE(!sized_range<iota_view<int>>);

    // some ranges to play around with
    std::vector<int> v1, v2;
    std::list<int> l1;
    auto nonCommonBidirRange = take_view(l1, 2);

    static_assert(!common_range<decltype(nonCommonBidirRange)>);
    static_assert(bidirectional_range<decltype(nonCommonBidirRange)>);
    STATIC_REQUIRE(xo::constant_time_reversible<decltype(v1)>);                   // random
    STATIC_REQUIRE(xo::constant_time_reversible<decltype(l1)>);                   // common
    STATIC_REQUIRE(!xo::constant_time_reversible<decltype(nonCommonBidirRange)>); // bidir only

    STATIC_CHECK(bidirectional_range<decltype(views::concat(v1, nonCommonBidirRange))>);
}

TEST_POINT("bidirectional_common") {
    std::vector<int> v1{1}, v2{}, v3{};
    std::list<int> l4{2, 3};
    auto cv = std::views::concat(v1, v2, v3, l4);

    auto it = std::ranges::begin(cv);
    REQUIRE(*it == 1);

    ++it;
    REQUIRE(*it == 2);

    ++it;
    REQUIRE(*it == 3);

    ++it;
    REQUIRE(it == std::ranges::end(cv));

    --it;
    REQUIRE(*it == 3);

    --it;
    REQUIRE(*it == 2);

    --it;
    REQUIRE(*it == 1);
}


TEST_POINT("bidirectional_noncommon_random_access_sized") {
    std::vector<int> v1{1};
    std::ranges::empty_view<int> e2{};
    std::ranges::iota_view<int, size_t> i3{0, 0};
    std::ranges::iota_view<int, size_t> i4{2, 4};
    auto cv = std::views::concat(v1, e2, i3, i4);

    auto it = std::ranges::begin(cv);
    REQUIRE(*it == 1);

    ++it;
    REQUIRE(*it == 2);

    ++it;
    REQUIRE(*it == 3);

    ++it;
    REQUIRE(it == std::ranges::end(cv));

    --it;
    REQUIRE(*it == 3);

    --it;
    REQUIRE(*it == 2);

    --it;
    REQUIRE(*it == 1);
}

TEST_POINT("bidirectional_last_range_not_cheaply_reverse") {

    std::list<int> l1{1, 2};
    auto nonCommonBidirRange = std::ranges::take_view(l1, 1);

    auto cv = std::views::concat(l1, nonCommonBidirRange);

    auto it = std::ranges::begin(cv);
    REQUIRE(*it == 1);

    ++it;
    REQUIRE(*it == 2);

    ++it;
    REQUIRE(*it == 1);

    ++it;
    REQUIRE(it == std::ranges::end(cv));

    --it;
    REQUIRE(*it == 1);

    --it;
    REQUIRE(*it == 2);

    --it;
    REQUIRE(*it == 1);
}

TEST_POINT("operator+=") {

    std::vector<int> v1{1}, v2{2, 3, 4}, v3{}, v4{}, v5{5, 6};
    std::ranges::concat_view cv{v1, v2, v3, v4, v5};
    auto it = cv.begin();
    REQUIRE(*it == 1);
    it += 2;
    REQUIRE(*it == 3);
    it += 2;
    REQUIRE(*it == 5);
    it += -3;
    REQUIRE(*it == 2);
    it += -1;
    REQUIRE(*it == 1);
}

TEST_POINT("operator-(iter,iter)") {

    std::vector<int> v1{1}, v2{2, 3, 4}, v3{}, v4{}, v5{5, 6};
    std::ranges::concat_view cv{v1, v2, v3, v4, v5};
    auto it1 = cv.begin();
    REQUIRE(it1 - it1 == 0);
    auto it2 = it1 + 1;
    REQUIRE(*it2 == 2);
    REQUIRE(it2 - it1 == 1);
    REQUIRE(it1 - it2 == -1);

    auto it3 = it1 + 3;
    REQUIRE(*it3 == 4);
    REQUIRE(it3 - it2 == 2);
    REQUIRE(it2 - it3 == -2);
    REQUIRE(it3 - it1 == 3);
    REQUIRE(it1 - it3 == -3);

    auto it4 = it3 + 2;
    REQUIRE(*it4 == 6);
    REQUIRE(it4 - it1 == 5);
    REQUIRE(it1 - it4 == -5);

    auto it5 = it4 + 1;
    REQUIRE(it5 == std::ranges::end(cv));
    REQUIRE(it5 - it3 == 3);
    REQUIRE(it3 - it5 == -3);
}

TEST_POINT("operator-(iter,default_sentinel_t)") {

    std::vector<int> v1{1}, v2{2, 3, 4}, v3{}, v4{}, v5{5, 6};
    std::ranges::concat_view cv{v1, v2, v3, v4, v5};
    auto it1 = cv.begin();
    using std::default_sentinel;
    REQUIRE(it1 - default_sentinel == -6);
    REQUIRE(default_sentinel - it1 == 6);

    auto it2 = it1 + 4;
    REQUIRE(it2 - default_sentinel == -2);
    REQUIRE(default_sentinel - it2 == 2);

    auto it3 = it2 + 2;
    REQUIRE(it3 - default_sentinel == 0);
    REQUIRE(default_sentinel - it3 == 0);
}

TEST_POINT("random access") {

    std::vector<int> v1{1}, v2{2, 3, 4}, v3{}, v4{}, v5{5, 6};
    std::ranges::concat_view cv{v1, v2, v3, v4, v5};
    static_assert(std::ranges::random_access_range<decltype(cv)>);
    REQUIRE(cv[0] == 1);
    REQUIRE(cv[1] == 2);
    REQUIRE(cv[2] == 3);
    REQUIRE(cv[3] == 4);
    REQUIRE(cv[4] == 5);
    REQUIRE(cv[5] == 6);
}


TEST_POINT("single range view works") {

    std::vector<int> v1{1, 2, 3, 4};
    std::ranges::concat_view cv{v1};
    static_assert(std::ranges::random_access_range<decltype(cv)>);
    REQUIRE(cv[0] == 1);
    REQUIRE(cv[1] == 2);
    REQUIRE(cv[2] == 3);
    REQUIRE(cv[3] == 4);

    REQUIRE(std::ranges::size(cv) == 4);

    auto it = std::ranges::begin(cv);
    REQUIRE(it + 4 == std::ranges::end(cv));
    it += 3;
    it -= 1;
    REQUIRE(*it == 3);

    ++it;
    REQUIRE(*it == 4);

    --it;
    REQUIRE(*it == 3);
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
