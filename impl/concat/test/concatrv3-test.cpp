// Range v3 library
//
//  Copyright Eric Niebler 2014-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3

#include <array>
#include <vector>
#include <range/v3/core.hpp>
#include <range/v3/view/concat.hpp>
#include <range/v3/view/generate.hpp>
#include <range/v3/view/reverse.hpp>
#include <range/v3/view/remove_if.hpp>
#include <range/v3/view/take_while.hpp>
#include <range/v3/algorithm/equal.hpp>
#include <range/v3/utility/copy.hpp>


#include <catch2/catch_test_macros.hpp>
//#include "test_utils.hpp"
#include <vector>

TEST_CASE("string vectors", "[rv3 concat]") {
    using namespace ranges;

    using V = std::vector<std::string>;

    V vs1{"a", "b", "c", "d"};
    V vs2{"x", "y", "z"};
    V cat_expected{"a", "b", "c", "d", "x", "y", "z"};
    V revcat_expected{"z", "y", "x", "d", "c", "b", "a"};

    auto cat = views::concat(vs1, vs2);
    auto revcat = cat | views::reverse;
    using C = decltype(cat);

    STATIC_CHECK(view_<C>);
    STATIC_CHECK(random_access_range<C>);
    STATIC_CHECK(std::is_same_v<range_reference_t<C>, std::string&>);

    CHECK(equal(cat, cat_expected));
    CHECK(cat.size() == 7u);
    CHECK((cat.end() - cat.begin()) == 7);

    CHECK((revcat.end() - revcat.begin()) == 7);
    CHECK(equal(revcat, revcat_expected));

    auto first = cat.begin();
    CHECK(*(first + 0) == "a");
    CHECK(*(first + 1) == "b");
    CHECK(*(first + 2) == "c");
    CHECK(*(first + 3) == "d");
    CHECK(*(first + 4) == "x");
    CHECK(*(first + 5) == "y");
    CHECK(*(first + 6) == "z");

    CHECK(*(first) == "a");
    CHECK(*(first += 1) == "b");
    CHECK(*(first += 1) == "c");
    CHECK(*(first += 1) == "d");
    CHECK(*(first += 1) == "x");
    CHECK(*(first += 1) == "y");
    CHECK(*(first += 1) == "z");

    auto last = cat.end();
    CHECK(*(last - 1) == "z");
    CHECK(*(last - 2) == "y");
    CHECK(*(last - 3) == "x");
    CHECK(*(last - 4) == "d");
    CHECK(*(last - 5) == "c");
    CHECK(*(last - 6) == "b");
    CHECK(*(last - 7) == "a");

    CHECK(*(last -= 1) == "z");
    CHECK(*(last -= 1) == "y");
    CHECK(*(last -= 1) == "x");
    CHECK(*(last -= 1) == "d");
    CHECK(*(last -= 1) == "c");
    CHECK(*(last -= 1) == "b");
    CHECK(*(last -= 1) == "a");
}


TEST_CASE("int array", "[rv3 concat]") {
    using namespace ranges;
    const std::array<int, 3> a{{0, 1, 2}};
    const std::array<int, 2> b{{3, 4}};
    CHECK(equal(views::concat(a, b), std::array{0, 1, 2, 3, 4}));

    auto odd = [](int i) { return i % 2 != 0; };
    auto even_filter = ranges::views::remove_if(odd);

    auto f_rng0 = a | even_filter;
    auto f_rng1 = b | even_filter;

    CHECK(equal(views::concat(f_rng0, f_rng1), std::array{0, 2, 4}));
}

TEST_CASE("regression395", "[rv3 concat]") {
    // Regression test for http://github.com/ericniebler/range-v3/issues/395.
    int i = 0;
    auto rng = ranges::views::concat(ranges::views::generate([&] { return i++; })) |
               ranges::views::take_while([](int j) { return j < 30; });
    CHECK(ranges::distance(ranges::begin(rng), ranges::end(rng)) == 30);
}
