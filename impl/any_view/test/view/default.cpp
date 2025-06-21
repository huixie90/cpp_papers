#include <algorithm>
#include <array>
#include <cassert>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <ranges>

#include "../helper.hpp"
#include "any_view.hpp"

#define TEST_POINT(x) TEST_CASE(x, "[default]")

namespace {

using AnyView =
    std::ranges::any_view<int, std::ranges::any_view_options::forward>;

[[maybe_unused]] constexpr void test_default() {
  std::array arr{1, 2, 3, 4, 5};

  AnyView view1;

  assert(std::ranges::empty(view1));
  for (const auto& elem : view1) {
    assert(false);
    (void)elem;
  }

  view1 = arr;
  assert(*view1.begin() == 1);
}


constexpr bool test() {
  test_default();
  return true;
}

TEST_POINT("default") {
  test();
  static_assert(test());
}

}  // namespace
