
#include <algorithm>
#include <array>
#include <cassert>
#include <catch2/catch_test_macros.hpp>
#include <type_traits>

#include "../helper.hpp"
#include "any_view.hpp"

#define TEST_POINT(x) TEST_CASE(x, "[approximately_sized_range]")

namespace {
using AnyView = std::ranges::any_view<
    int, std::ranges::any_view_options::input |
             std::ranges::any_view_options::approximately_sized>;

static_assert(!std::is_constructible_v<AnyView, UnsizedInputView>);
static_assert(std::is_constructible_v<AnyView, SizedInputView>);

struct ApproxiSizedView : std::ranges::view_base {
  using Iter = test_iter<int*, std::input_iterator_tag>;
  int* begin_iter;
  int* end_iter;

  constexpr ApproxiSizedView(int* first, int* last) : begin_iter(first), end_iter(last) {}

  constexpr auto reserve_hint() const { return end_iter - begin_iter; }

  constexpr auto begin() const { return Iter(begin_iter); }
  constexpr auto end() const { return Iter(end_iter); }
};

static_assert(std::ranges::approximately_sized_range<ApproxiSizedView>);
static_assert(!std::ranges::sized_range<ApproxiSizedView>);

constexpr bool test() {
  {
    // from sized_range
    std::array v{1, 2, 3, 4, 5};

    AnyView view(std::views::all(v));
    assert(std::ranges::reserve_hint(view) == 5);
  }

  {
    // from only_approximately_sized
    int arr[] = {1, 2, 3, 4, 5};
    ApproxiSizedView as(arr, arr + 5);

    AnyView view(std::views::all(as));
    assert(std::ranges::reserve_hint(view) == 5);
  }

  return true;
}

TEST_POINT("reserve_hint") {
  test();
  static_assert(test());
}
}  // namespace
