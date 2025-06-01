
#include <algorithm>
#include <array>
#include <cassert>
#include <catch2/catch_test_macros.hpp>
#include <type_traits>

#include "../helper.hpp"
#include "any_view.hpp"

#define TEST_POINT(x) TEST_CASE(x, "[borrowed_view]")

namespace {
using AnyView =
    std::ranges::any_view<int, std::ranges::any_view_options::input |
                                   std::ranges::any_view_options::borrowed>;

static_assert(!std::is_constructible_v<AnyView, NonBorrowedInputView>);
static_assert(std::is_constructible_v<AnyView, BorrowedInputView>);

constexpr bool test() {
  std::array v{1, 2, 3, 4, 5};

  AnyView view(std::views::all(v));

  auto it = view.begin();
  assert(*it == 1);

  auto st = view.end();
  assert(it != st);

  ++it;
  ++it;
  ++it;

  assert(*it == 4);

  return true;
}

TEST_POINT("borrowed_view") {
  test();
  static_assert(test());
}
}  // namespace