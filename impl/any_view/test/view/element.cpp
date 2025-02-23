#include <algorithm>
#include <array>
#include <cassert>
#include <catch2/catch_test_macros.hpp>
#include <ranges>

#include "any_view.hpp"

#define TEST_POINT(x) TEST_CASE(x, "[element]")

namespace {

constexpr bool test() {
  {
    std::vector<int> v = {1, 2, 3, 4, 5};

    std::ranges::any_view<int> view(v);
    static_assert(
        std::is_same_v<std::ranges::range_reference_t<decltype(view)>, int&>);
    assert(std::ranges::equal(view, v));

    std::ranges::any_view<const int> view2(v);
    static_assert(
        std::is_same_v<std::ranges::range_reference_t<decltype(view2)>,
                       const int&>);
    assert(std::ranges::equal(view2, v));
  }

  return true;
}

TEST_POINT("element") {
  test();
  static_assert(test());
}

}  // namespace