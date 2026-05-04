
#include <algorithm>
#include <array>
#include <cassert>
#include <catch2/catch_test_macros.hpp>
#include <type_traits>

#include "../helper.hpp"
#include "any_view.hpp"

#define TEST_POINT(x) TEST_CASE(x, "[borrowed_view]")

struct NonViewBorrowed {
  int* begin() { return nullptr; }
  int* end() { return nullptr; }
};

struct NonViewNonBorrowed {
  int* begin() { return nullptr; }
  int* end() { return nullptr; }
};

template <>
constexpr bool std::ranges::enable_borrowed_range<NonViewBorrowed> = true;
namespace {
using AnyView =
    std::ranges::any_view<int, std::ranges::any_view_options::input |
                                   std::ranges::any_view_options::borrowed>;

static_assert(!std::is_constructible_v<AnyView, NonBorrowedInputView>);
static_assert(std::is_constructible_v<AnyView, BorrowedInputView>);

constexpr bool test() {
  {
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
  }

  {
    std::array v{1, 2, 3, 4, 5};
    auto non_borrowed_view =
        v | std::views::transform([](int& i) -> int& { return i; });
    using NonBorrowedView = decltype(non_borrowed_view);
    static_assert(!std::is_constructible_v<AnyView, NonBorrowedView>);
    static_assert(!std::is_constructible_v<AnyView, NonBorrowedView&>);
    static_assert(!std::is_constructible_v<AnyView, NonBorrowedView&&>);
  }

  {
    std::array v{1, 2, 3, 4, 5};
    auto borrowed_view = v | std::views::take(3);
    using BorrowedView = decltype(borrowed_view);
    static_assert(std::is_constructible_v<AnyView, BorrowedView>);
    static_assert(std::is_constructible_v<AnyView, BorrowedView&>);
    static_assert(std::is_constructible_v<AnyView, BorrowedView&&>);
    AnyView av1(v | std::views::take(3));
    AnyView av2(borrowed_view);
    AnyView av3(std::move(borrowed_view));
  }
  {
    static_assert(std::is_constructible_v<AnyView, NonViewBorrowed>);
    static_assert(std::is_constructible_v<AnyView, NonViewBorrowed&>);
    static_assert(std::is_constructible_v<AnyView, NonViewBorrowed&&>);
  }
  {
    static_assert(!std::is_constructible_v<AnyView, NonViewNonBorrowed>);
    static_assert(std::is_constructible_v<AnyView, NonViewNonBorrowed&>);
    static_assert(!std::is_constructible_v<AnyView, NonViewNonBorrowed&&>);
  }

  return true;
}

TEST_POINT("borrowed_view") {
  test();
  static_assert(test());
}
}  // namespace
