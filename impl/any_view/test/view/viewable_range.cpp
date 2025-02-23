#include <algorithm>
#include <array>
#include <cassert>
#include <catch2/catch_test_macros.hpp>

#include "any_view.hpp"

#define TEST_POINT(x) TEST_CASE(x, "[viewable_range]")

namespace {

struct MoveOnly {
  int i;
  MoveOnly(int ii) : i(ii) {}

  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) = default;
  MoveOnly& operator=(MoveOnly&&) = default;

  friend bool operator==(const MoveOnly& x, const MoveOnly& y) = default;
};

void test_impl(std::ranges::any_view<MoveOnly> view) {
  assert(std::ranges::equal(view, std::array<MoveOnly, 5>{1, 2, 3, 4, 5}));
}

constexpr bool test() {
  {
    std::vector<MoveOnly> v;
    for (int i = 1; i <= 5; ++i) {
      v.push_back(MoveOnly(i));
    }
    test_impl(v);
  }

  return true;
}

TEST_POINT("viewable_range") {
  test();
  // static_assert(test());
}

}  // namespace