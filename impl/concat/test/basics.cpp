#include "concat.hpp"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <list>
#include <vector>
#include <functional>
#include <memory>
#include <numeric>
#include "std_exposition_only_concepts.hpp"
#include "test/range_fixtures.hpp"

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
#if !(defined(__GNUC__) && !defined(_LIBCPP_VERSION))
    STATIC_CHECK(concat_viewable<IntV>);
#endif
    STATIC_CHECK(!concat_viewable<const std::vector<MoveOnly>>);

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
    static_assert(!std::ranges::common_range<decltype(cv)>,
                  "This is no longer a common range as we dropped the common support for !common "
                  "&& random_access && sized");

    [[maybe_unused]] auto it = cv.begin();
    auto st = cv.end();
    // static_assert(std::same_as<decltype(it), decltype(st)>);
    static_assert(std::same_as<decltype(st), std::default_sentinel_t>);
    [[maybe_unused]] auto it2 = std::as_const(cv).begin();
    auto st2 = std::as_const(cv).end();
    // static_assert(std::same_as<decltype(it2), decltype(st2)>);
    static_assert(std::same_as<decltype(st2), std::default_sentinel_t>);

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


struct NonCommonRandomSized : std::ranges::view_base{
    const int* begin() const;
    std::nullptr_t end() const;
    std::size_t size() const;
};

TEST_POINT("random_but_not_bidi_impossible"){
    using namespace std::ranges;
    static_assert(!common_range<NonCommonRandomSized>);
    static_assert(random_access_range<NonCommonRandomSized>);
    static_assert(sized_range<NonCommonRandomSized>);

    using CV = decltype(std::views::concat(NonCommonRandomSized{}, NonCommonRandomSized{}));
    static_assert(!common_range<CV>);
    static_assert(bidirectional_range<CV>);
    static_assert(random_access_range<CV>);
    static_assert(sized_range<CV>);
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


TEST_POINT("iter_move and iter_swap basic") {
    std::vector v1{5, 3, 9}, v2{1, 8}, v3{7, 6};
    auto v = std::views::concat(v1, v2, v3);
    std::sort(v.begin(), v.end());

    REQUIRE(v1[0] == 1);
    REQUIRE(v1[1] == 3);
    REQUIRE(v1[2] == 5);

    REQUIRE(v2[0] == 6);
    REQUIRE(v2[1] == 7);

    REQUIRE(v3[0] == 8);
    REQUIRE(v3[1] == 9);
}


TEST_POINT("->") {
    struct MyInt {
        int i = 0;
    };
    std::vector<MyInt> v1{{1}}, v2{{2}};
    const std::vector<MyInt> v3{{3}};
    std::list<MyInt> l1{{4}};
    MyInt a1[]{{5}, {6}, {7}};

    SECTION("two vectors") {
        auto cv = std::views::concat(v1, v2);
        auto it = cv.begin();
        using It = decltype(it);
        STATIC_CHECK(std::ranges::__has_arrow<It>);
        STATIC_CHECK(std::same_as<pointer_t<It>, MyInt*>);
        it->i = 4;
        CHECK(v1[0].i == 4);
    }

    SECTION("const and non-const vectors") {
        auto cv = std::views::concat(v1, v2, v3);
        auto it = cv.begin();
        using It = decltype(it);
        STATIC_CHECK(std::ranges::__has_arrow<It>);
        STATIC_CHECK(std::same_as<pointer_t<It>, MyInt const*>);
        CHECK(it->i == v1[0].i);
        std::advance(it, v2.size());
        CHECK(it->i == v2[0].i);
        std::advance(it, v3.size());
        CHECK(it->i == v3[0].i);
    }

    SECTION("vector and list") {
        auto cv = std::views::concat(l1, v1);
        auto it = cv.begin();
        using It = decltype(it);
        STATIC_CHECK(std::ranges::__has_arrow<It>);
        STATIC_CHECK(std::same_as<pointer_t<It>, MyInt*>);
        CHECK(it->i == l1.front().i);
        std::advance(it, l1.size());
        CHECK(it->i == v1[0].i);
    }

#ifndef _LIBCPP_VERSION
    SECTION("filter views") {
        v1 = {{1}, {2}, {3}, {4}, {5}};
        auto fvOdd = v1 | std::views::filter([](auto m) { return m.i % 2 == 1; });
        auto fvEven = v1 | std::views::filter([](auto m) { return m.i % 2 == 0; });
        auto cv = std::views::concat(fvOdd, fvEven);
        auto it = cv.begin();
        using It = decltype(it);
        STATIC_CHECK(std::ranges::__has_arrow<It>);
        STATIC_CHECK(std::same_as<pointer_t<It>, pointer_t<decltype(fvOdd.begin())>>);
        STATIC_CHECK(std::same_as<pointer_t<It>, std::vector<MyInt>::iterator>);
        CHECK(it++->i == 1);
        // CHECK(it++->i == 2);
        // CHECK(it++->i == 4);
        // CHECK(it++->i == 3);
        // CHECK(it++->i == 5);
    }

    SECTION("filter views") {
        auto fvOdd = v1 | std::views::filter([](auto m) { return m.i % 2 == 1; });
        auto fvEven = l1 | std::views::filter([](auto m) { return m.i % 2 == 0; });
        auto cv = std::views::concat(fvOdd, fvEven);
        auto it = cv.begin();
        using It = decltype(it);
        STATIC_CHECK(!std::ranges::__has_arrow<It>);
        //         STATIC_CHECK(std::same_as<pointer_t<It>, pointer_t<decltype(fvOdd.begin())>>);
        //         STATIC_CHECK(std::same_as<pointer_t<It>, std::vector<MyInt>::iterator>);
        //         CHECK(it++->i == 1);
        //         CHECK(it++->i == 2);
        //         CHECK(it++->i == 4);
        //         CHECK(it++->i == 3);
        //         CHECK(it++->i == 5);
    }
#endif
    SECTION("raw array and others") {
        auto cv = std::views::concat(v1, a1, l1);
        auto it = cv.begin();
        STATIC_CHECK(std::ranges::__has_arrow<decltype(it)>);
        CHECK(it->i == v1[0].i);
        std::advance(it, v1.size());
        CHECK(it->i == a1[0].i);
        std::advance(it, std::ranges::size(a1));
        CHECK(it->i == l1.front().i);
    }

    SECTION("no arrow") {
        auto t = v1 | std::views::transform([](auto&& m) -> MyInt& { return m; });
        using ItBase = decltype(t.begin());
        STATIC_REQUIRE(!std::ranges::__has_arrow<ItBase>);
        STATIC_REQUIRE(std::same_as<pointer_t<ItBase>, void>);
        auto cv = std::views::concat(v1, t);
        auto it = cv.begin();
        using It = decltype(it);
        STATIC_CHECK(!std::ranges::__has_arrow<It>);
        STATIC_CHECK(std::same_as<pointer_t<It>, void>);
    }

    SECTION("common iter with proxy as pointer type") {
        auto io = std::views::iota(0);
        auto t = io | std::views::transform([&](size_t i) -> MyInt { return v1[i]; });
        auto comm = t | std::views::common;
        STATIC_CHECK(!std::same_as<decltype(comm), decltype(t)>);
        auto it = comm.begin();
        [[maybe_unused]] auto p = it.operator->();
        bool r = it->i == v1[0].i;
        CHECK(r);

        auto c = std::views::concat(comm, comm);
        auto cit = c.begin();
#ifndef _MSC_VER
        [[maybe_unused]] auto cp = cit.operator->();
#endif

        // auto io = std::views::iota(0);
        // STATIC_CHECK(!std::ranges::common_range<decltype(io)>);
        // auto comm = io | std::views::common;
        //
        // auto i = comm.begin();
        // i++;
        // auto proxy = i.operator->();
    }
}



template <typename Derived>
struct ArrowIteratorBase {
    using It = int*;

    int& operator*() const;
    It operator->() const;

    Derived& operator++();
    Derived operator++(int);

    friend bool operator==(Derived const&, Derived const&) { return true; }
    bool operator==(std::default_sentinel_t) const;

    using value_type = int;                 // to model indirectly_readable_traits
    using difference_type = std::ptrdiff_t; // to model incrementable_traits
};

struct ArrowIterator : ArrowIteratorBase<ArrowIterator> {};

struct ArrowRange {
    ArrowIterator begin() const;
    ArrowIterator end() const;
};

struct MoveOnlyArrowIterator : ArrowIteratorBase<MoveOnlyArrowIterator> {
    MoveOnlyArrowIterator() = default;
    MoveOnlyArrowIterator(const MoveOnlyArrowIterator&) = delete;
    MoveOnlyArrowIterator& operator=(const MoveOnlyArrowIterator&) = delete;
    MoveOnlyArrowIterator(MoveOnlyArrowIterator&&) = default;
    MoveOnlyArrowIterator& operator=(MoveOnlyArrowIterator&&) = default;
};

struct MoveOnlyArrowRange {
    MoveOnlyArrowIterator begin() const;
    std::default_sentinel_t end() const;
};

TEST_POINT("move only ->") {
    using Tr1 = std::iterator_traits<ArrowIterator>;
    using Tr2 = std::iterator_traits<MoveOnlyArrowIterator>;

    STATIC_REQUIRE(requires { typename Tr1::iterator_category; });
    STATIC_REQUIRE(requires { typename Tr1::value_type; });
    STATIC_REQUIRE(requires { typename Tr1::reference; });
    STATIC_REQUIRE(requires { typename Tr1::difference_type; });
    STATIC_REQUIRE(requires { typename Tr1::pointer; });

    // empty iterator_traits for MoveOnlyArrowIterator
    STATIC_REQUIRE(!has_iterator_category<Tr2>);
    STATIC_REQUIRE(!has_value_type<Tr2>);

    STATIC_REQUIRE(std::input_iterator<MoveOnlyArrowIterator>);
    STATIC_REQUIRE(std::ranges::__has_arrow<MoveOnlyArrowIterator>);


    {
        ArrowRange r1, r2;
        using CV = decltype(std::views::concat(r1, r2));
        using It = std::ranges::iterator_t<CV>;
        static_assert(std::ranges::__has_arrow<It>);
    }
    {
        MoveOnlyArrowRange r1, r2;
        using CV = decltype(std::views::concat(r1, r2));
        using It = std::ranges::iterator_t<CV>;
        static_assert(std::ranges::__has_arrow<It>);
    }
}

struct cpp20_input_iter_cpp17_not_iter {
    int operator*() const { return 5; }

    cpp20_input_iter_cpp17_not_iter& operator++() { return *this; }
    void operator++(int) {}

    bool operator==(std::default_sentinel_t) const { return true; }

    using value_type = int;                 // to model indirectly_readable_traits
    using difference_type = std::ptrdiff_t; // to model incrementable_traits
};

struct cpp20_input_range {
    cpp20_input_iter_cpp17_not_iter begin() const { return {}; }
    std::default_sentinel_t end() const { return {}; }
};


TEST_POINT("this should work") {
    static_assert(std::ranges::input_range<cpp20_input_range>);
    cpp20_input_range r1, r2;
    auto x = std::views::concat(r1, r2);
    [[maybe_unused]] auto it = x.begin();
}
