#include "concat.hpp"

#include <concepts>
#include <type_traits>
#include <iterator>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#define TEST_POINT(x) TEST_CASE(x, "[itermoveswap]")


#include <range/v3/view/zip.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/drop.hpp>
#include <range/v3/view/stride.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/iota.hpp>
#include <functional>
#include <algorithm>


#ifdef __GNUC__

// hack to allow gcc to compile range-v3 views as viewable ranges

namespace std::ranges {

template <typename... Ts>
inline constexpr bool enable_borrowed_range<::ranges::zip_view<Ts...>> = true;

}


#endif

TEST_POINT("iter_move") {
    std::vector v1{10, 11};
    std::vector v2{20, 21};
    std::array a1{-10, -11};
    std::array a2{-20, -21};

    auto zv12 = ranges::views::zip(v1, v2);
    auto za12 = ranges::views::zip(a1, a2);

    auto c = std::views::concat(zv12, za12);

    auto i1 = c.begin();
    auto i2 = i1 + v1.size();

    std::ranges::range_value_t<decltype(c)> vi1 = std::ranges::iter_move(i1);

    *i1 = std::ranges::iter_move(i2);
    *i2 = std::move(vi1);

    CHECK(v1[0] == -10);
    CHECK(v2[0] == -20);
    CHECK(a1[0] == 10);
    CHECK(a2[0] == 20);
}

TEST_POINT("iter_move_nothrow") {
    std::vector v1{10, 11};
    std::array a1{-10, -11};
    auto c = std::views::concat(v1, a1);
    auto it = c.begin();
    static_assert(noexcept(std::ranges::iter_move(it)));
}

struct throwing_iter {
    int* i = nullptr;
    int& operator*() const { return *i; }

    throwing_iter& operator++() { return *this; }
    throwing_iter operator++(int) { return {}; }

    bool operator==(throwing_iter) const { return true; }

    using value_type = int;                 // to model indirectly_readable_traits
    using difference_type = std::ptrdiff_t; // to model incrementable_traits
    friend int&& iter_move(const throwing_iter& x) /* throws */ { return std::move(*(x.i)); }
};

struct throwing_range {
    throwing_iter begin() const { return {}; }
    throwing_iter end() const { return {}; }
};

TEST_POINT("iter_move_throw") {
    throwing_range a, b;
    auto c = std::views::concat(a, b);
    auto it = c.begin();
    static_assert(!noexcept(std::ranges::iter_move(it)));
}

struct throw_on_move {
    throw_on_move() = default;
    throw_on_move(throw_on_move const&) noexcept {}
    throw_on_move(throw_on_move&&) /*throw*/ {}
};

TEST_POINT("iter_move_throw_on_converting_to_rvalue_reference") {
    std::vector<throw_on_move> v;
    auto tv = std::views::iota(0) | std::views::transform([](auto) { return throw_on_move{}; });

    auto cv = std::views::concat(v, tv);

    using vector_rvalue_ref = std::ranges::range_rvalue_reference_t<decltype(v)>;
    using common_rvalue_ref = std::ranges::range_rvalue_reference_t<decltype(cv)>;

    auto it = cv.begin();
    static_assert(!noexcept(std::ranges::iter_move(it))); // because
    static_assert(std::same_as<vector_rvalue_ref, throw_on_move&&>);
    static_assert(std::same_as<common_rvalue_ref, throw_on_move>);
    static_assert(!std::is_nothrow_convertible_v<throw_on_move&&, throw_on_move>);
}

TEST_POINT("iter_swap_concept") {
    std::vector v{1, 2, 3};
    auto tv = v | std::views::transform([](auto) { return 5; });
    auto cv = std::views::concat(tv, tv);
    using It = std::ranges::iterator_t<decltype(cv)>;
    static_assert(!std::indirectly_swappable<It>);

    auto cv2 = std::views::concat(v, v);
    using It2 = std::ranges::iterator_t<decltype(cv2)>;
    static_assert(std::indirectly_swappable<It2>);
}


#ifdef _MSC_VER

TEST_POINT("smallsort") {
    std::vector v1{10, 15};
    std::vector v2{3, 4};
    std::array a1{9, 15};
    std::array a2{0, 3};

    std::vector<std::pair<int, int>> expected{
        {9, 0},
        {10, 3},
        {15, 3},
        {15, 4},
    };

    auto concat = std::views::concat(ranges::views::zip(v1, v2), ranges::views::zip(a1, a2));

    std::ranges::sort(concat);

    CHECK_THAT(concat | ranges::to_vector, Catch::Matchers::Equals(expected));

    namespace rv = ranges::views;
    auto v1_expected = expected | rv::take(2) | rv::keys | ranges::to_vector;
    auto v2_expected = expected | rv::take(2) | rv::values | ranges::to_vector;
    auto a1_expected = expected | rv::drop(2) | rv::keys | ranges::to_vector;
    auto a2_expected = expected | rv::drop(2) | rv::values | ranges::to_vector;

    CHECK(v1 == v1_expected);
    CHECK(v2 == v2_expected);
    CHECK((a1 | ranges::to_vector) == a1_expected);
    CHECK((a2 | ranges::to_vector) == a2_expected);
}



TEST_POINT("largesort") {
    constexpr int largeN = 50;
    namespace rv = ranges::views;
    auto neg = rv::transform(std::negate{});
    auto v1 = rv::iota(0) | rv::stride(2) | rv::take(largeN) | ranges::to_vector; // 0,2,4,..
    auto v2 = v1 | neg | ranges::to_vector;                                       // 0,-2,-4,...

    auto vp = rv::zip(rv::iota(1) | rv::stride(2) | rv::take(largeN),
                      rv::iota(1) | rv::stride(2) | neg) |
              ranges::to_vector; // (1,-1), (3,-3), ...

    auto concat = std::views::concat(rv::zip(v1, v2), vp);

    std::ranges::sort(concat);

    auto v1_expected = rv::iota(0) | rv::take(largeN) | ranges::to_vector;       // 0,1,2, ...
    auto v2_expected = rv::iota(0) | neg | rv::take(largeN) | ranges::to_vector; // 0,-1,-2, ...
    auto vp_expected =
        rv::zip(rv::iota(largeN), rv::iota(largeN) | neg) | rv::take(largeN) | ranges::to_vector;
    // (50,-50), (51,-51), ...

    CHECK(v1 == v1_expected);
    CHECK(v2 == v2_expected);
    CHECK(vp == vp_expected);
}

#endif

TEST_POINT("iter_swap_non_swappable_check1") {
    int v1[] = {1, 2};
    long v2[] = {3, 4};
    auto cv = std::views::concat(v1, v2);
    auto it1 = cv.begin();
    using iter = decltype(it1);
    static_assert(!std::indirectly_swappable<iter>);
}

TEST_POINT("iter_swap_non_swappable_check2") {
    std::string_view v1[] = {"aa"};
    std::string v2[] = {"bbb"};
    auto cv = std::views::concat(v1, v2);
    auto it1 = cv.begin();
    using iter = decltype(it1);
    static_assert(!std::indirectly_swappable<iter>);
}

struct iter_swap_iter {
    int* i = nullptr;
    int& operator*() const { return *i; }

    iter_swap_iter& operator++() {
        ++i;
        return *this;
    }
    iter_swap_iter operator++(int) { return {i + 1}; }

    bool operator==(iter_swap_iter other) const { return i == other.i; }

    using value_type = int;                 // to model indirectly_readable_traits
    using difference_type = std::ptrdiff_t; // to model incrementable_traits

    friend void iter_swap(iter_swap_iter x, iter_swap_iter y) {
        std::swap(*(x.i), *(y.i));
        *(x.i) += 10;
        *(y.i) += 10;
    }
};

struct iter_swap_iter_range {
    std::vector<int> vec;
    iter_swap_iter begin() { return iter_swap_iter{vec.data()}; }
    iter_swap_iter end() { return iter_swap_iter{vec.data() + vec.size()}; }
};

TEST_POINT("iter_swap_customisation") {
    iter_swap_iter_range r1;
    r1.vec = {1, 2, 3};
    std::vector<int> r2 = {4, 5, 6};
    auto cv = std::views::concat(r1, r2);

    // same range with customisation
    auto it1 = cv.begin();
    auto it2 = cv.begin();
    ++it2;
    std::ranges::iter_swap(it1, it2);
    CHECK(r1.vec[0] == 12);
    CHECK(r1.vec[1] == 11);

    auto it3 = cv.begin();
    ++it3;
    ++it3;

    auto it4 = it3;
    ++it4;

    std::ranges::iter_swap(it3, it4);
    CHECK(r1.vec[2] == 4);
    CHECK(r2[0] == 3);
}


TEST_POINT("tricky_tuple_case") {
    std::vector<std::tuple<int, int, int>> r1{
        {1, 2, 3},
        {4, 5, 6},
    };

    std::vector<int> v1{
        7,
        8,
    };

    std::vector<int> v2{
        9,
        10,
    };

    std::vector<int> v3{
        11,
        12,
    };

    auto r2 = std::views::zip(v1, v2, v3);
    //  {7, 9, 11}
    //  {8, 10, 12}

    auto cv = std::views::concat(r1, r2);

    auto it1 = cv.begin();
    auto it2 = cv.begin() + 2;
    std::ranges::iter_swap(it1, it2);
    // swap (1,2,3) with (7,9,11)

    CHECK(r1[0] == std::tuple(7, 9, 11));
    CHECK(v1[0] == 1);
    CHECK(v2[0] == 2);
    CHECK(v3[0] == 3);
}
