
#include <algorithm>
#include <array>
#include <cassert>
#include <catch2/catch_test_macros.hpp>
#include <type_traits>

#include "../helper.hpp"
#include "any_view.hpp"

#define TEST_POINT(x) TEST_CASE(x, "[sized_range]")

namespace {
using AnyView =
    std::ranges::any_view<int, std::ranges::any_view_options::input |
                                   std::ranges::any_view_options::sized>;

static_assert(!std::is_constructible_v<AnyView, UnsizedInputView>);
static_assert(std::is_constructible_v<AnyView, SizedInputView>);


struct WeirdView : std::ranges::view_base {
  constexpr int* begin() {return nullptr;}
  constexpr int* begin() const {return nullptr;}
  constexpr int* end() {return nullptr;}
  constexpr int* end() const {return nullptr;}
  constexpr std::size_t size() {return 5;}
  constexpr std::size_t size() const {return 6;}
};

constexpr bool test() {
  {
    std::array v{1, 2, 3, 4, 5};

    AnyView view(std::views::all(v));
    assert(std::ranges::size(view) == 5);

    auto it = view.begin();
    assert(*it == 1);

    auto st = view.end();
    assert(it != st);

    ++it;
    ++it;
    ++it;

    assert(*it == 4);
  }
  {
    WeirdView view;
    AnyView any_view(view);
    assert(std::ranges::size(any_view) == 5);
  }

  return true;
}

TEST_POINT("sized") {
  test();
  static_assert(test());
}
}  // namespace
