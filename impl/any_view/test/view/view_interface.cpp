#include <algorithm>
#include <array>
#include <cassert>
#include <catch2/catch_test_macros.hpp>

#include "any_view.hpp"

#define TEST_POINT(x) TEST_CASE(x, "[view_interface]")

namespace {
constexpr bool test() {
  std::vector<int> v = {1, 2, 3, 4, 5};
  std::ranges::any_view<int> view = v;
  assert(v.front() == 1);
  return true;
}

TEST_POINT("viewable_range") {
  test();
  static_assert(test());
}
}  // namespace
