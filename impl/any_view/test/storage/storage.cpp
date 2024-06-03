#include <algorithm>
#include <array>
#include <cassert>
#include <catch2/catch_test_macros.hpp>

#include "storage.hpp"

#define TEST_POINT(x) TEST_CASE(x, "[storage]")

namespace {

using Storage =
    std::ranges::detail::storage<3 * sizeof(void*), sizeof(void*), true>;

using std::ranges::detail::type;

using Small = int;

struct Big {
  constexpr Big(int ii) : i(ii) {}

  constexpr operator int() const { return i; }
  int i;
  char c[100] = {};
};

struct Stats {
  int construct = 0;
  int copy_construct = 0;
  int copy_assignment = 0;
  int move_construct = 0;
  int move_assignment = 0;
  int destroy = 0;
};

template <class T>
struct alignas(alignof(void*)) Track {
  Stats* stats_;
  T t_;

  template <class... Args>
    requires std::constructible_from<T, Args...>
  constexpr Track(Stats& stats, Args&&... args)
      : stats_(&stats), t_(std::forward<Args>(args)...) {
    ++stats_->construct;
  }

  constexpr Track(const Track& t) : stats_(t.stats_), t_(t.t_) {
    ++stats_->copy_construct;
  }

  constexpr Track(Track&& t) noexcept(std::is_nothrow_move_constructible_v<T>)
      : stats_(t.stats_), t_(std::move(t.t_)) {
    ++stats_->move_construct;
  }

  constexpr Track& operator=(const Track& other) {
    stats_ = other.stats_;
    t_ = other.t_;
    ++stats_->copy_assignment;
    return *this;
  }

  constexpr Track& operator=(Track&& other) noexcept(
      std::is_nothrow_move_assignable_v<T>) {
    stats_ = other.stats_;
    t_ = std::move(other.t_);
    ++stats_->move_assignment;
    return *this;
  }

  constexpr ~Track() { ++stats_->destroy; }
};

static_assert(Storage::unittest_is_small<Small>());
static_assert(!Storage::unittest_is_small<Big>());
static_assert(Storage::unittest_is_small<Track<Small>>());
static_assert(!Storage::unittest_is_small<Track<Big>>());

constexpr void singular() {
  Storage s;
  assert(s.is_singular());
}

template <class T>
constexpr void basic() {
  Storage s{type<T>{}, 5};
  assert(*s.get_ptr<T>() == 5);
}

template <class T>
constexpr void copy() {
  // non singular
  {
    Storage s1{type<T>{}, 5};
    Storage s2{s1};

    assert(*s1.get_ptr<T>() == 5);
    assert(*s2.get_ptr<T>() == 5);
  }

  // singular
  {
    Storage s1{};
    auto s2 = s1;

    assert(s1.is_singular());
    assert(s2.is_singular());
  }

  // track
  {
    Stats stats{};
    {
      Storage s1{type<Track<T>>{}, stats, 5};
      assert(stats.construct == 1);
      assert(stats.copy_construct == 0);
      assert(stats.move_construct == 0);
      assert(stats.destroy == 0);

      assert(s1.get_ptr<Track<T>>()->t_ == 5);

      Storage s2{s1};
      assert(stats.construct == 1);
      assert(stats.copy_construct == 1);
      assert(stats.move_construct == 0);
      assert(stats.destroy == 0);

      assert(s2.get_ptr<Track<T>>()->t_ == 5);
    }
    assert(stats.destroy == 2);
  }
}

template <class T>
constexpr void move() {
  // non singular
  {
    Storage s1{type<T>{}, 5};
    Storage s2{std::move(s1)};

    if constexpr (Storage::unittest_is_small<T>()) {
      assert(!s1.is_singular());
      assert(*s1.get_ptr<T>() == 5);
    } else {
      assert(s1.is_singular());
    }
    assert(!s2.is_singular());
    assert(*s2.get_ptr<T>() == 5);
  }

  // singular
  {
    Storage s1{};
    auto s2 = std::move(s1);

    assert(s1.is_singular());
    assert(s2.is_singular());
  }
  // track
  {
    Stats stats{};
    {
      Storage s1{type<Track<T>>{}, stats, 5};
      assert(stats.construct == 1);
      assert(stats.copy_construct == 0);
      assert(stats.move_construct == 0);
      assert(stats.destroy == 0);

      assert(s1.get_ptr<Track<T>>()->t_ == 5);

      Storage s2{std::move(s1)};
      assert(stats.construct == 1);
      assert(stats.copy_construct == 0);

      auto expected_move = Storage::unittest_is_small<Track<T>>() ? 1 : 0;
      assert(stats.move_construct == expected_move);
      assert(stats.destroy == 0);

      assert(s2.get_ptr<Track<T>>()->t_ == 5);
    }
    auto expected_destroy = Storage::unittest_is_small<Track<T>>() ? 2 : 1;
    assert(stats.destroy == expected_destroy);
  }
}

template <class LHS, class RHS>
constexpr void move_assignment() {
  constexpr bool lhsSmall = Storage::unittest_is_small<LHS>();
  constexpr bool rhsSmall = Storage::unittest_is_small<RHS>();
  // non singular
  {
    Storage s1{type<LHS>{}, 5};
    Storage s2{type<RHS>{}, 6};

    s1 = std::move(s2);
    assert(*s1.get_ptr<RHS>() == 6);
    if constexpr (rhsSmall) {
      assert(*s2.get_ptr<RHS>() == 6);
    } else {
      assert(s2.is_singular());
    }
  }

  // lhs singular
  {
    Storage s1{};
    Storage s2{type<RHS>{}, 6};

    s1 = std::move(s2);
    assert(*s1.get_ptr<RHS>() == 6);
    if constexpr (rhsSmall) {
      assert(*s2.get_ptr<RHS>() == 6);
    } else {
      assert(s2.is_singular());
    }
  }

  // rhs singular
  {
    Storage s1{type<LHS>{}, 5};
    Storage s2{};

    s1 = std::move(s2);
    assert(s1.is_singular());
    assert(s2.is_singular());
  }

  // track
  {
    Stats stats1{};
    Stats stats2{};
    {
      Storage s1{type<Track<LHS>>{}, stats1, 5};
      assert(stats1.construct == 1);
      assert(stats1.copy_construct == 0);
      assert(stats1.move_construct == 0);
      assert(stats1.destroy == 0);

      assert(s1.get_ptr<Track<LHS>>()->t_ == 5);

      Storage s2{type<Track<RHS>>{}, stats2, 6};
      assert(stats2.construct == 1);
      assert(stats2.copy_construct == 0);
      assert(stats2.move_construct == 0);
      assert(stats2.destroy == 0);

      assert(s2.get_ptr<Track<RHS>>()->t_ == 6);

      s1 = std::move(s2);

      assert(stats1.construct == 1);
      assert(stats1.copy_construct == 0);
      assert(stats1.copy_assignment == 0);
      assert(stats1.move_assignment == 0);

      assert(stats2.construct == 1);
      assert(stats2.copy_construct == 0);
      assert(stats2.copy_assignment == 0);
      assert(stats2.move_assignment == 0);

      int expected_move_construct1 = 0;
      int expected_destroy1 = 0;
      int expected_move_construct2 = 0;
      int expected_destroy2 = 0;

      if (lhsSmall && rhsSmall) {
        expected_move_construct1 = 2;
        expected_destroy1 = 3;
        expected_move_construct2 = 2;
        expected_destroy2 = 1;
      } else if (lhsSmall) {
        expected_move_construct1 = 2;
        expected_destroy1 = 3;
        expected_move_construct2 = 0;
        expected_destroy2 = 0;
      } else if (rhsSmall) {
        expected_move_construct1 = 0;
        expected_destroy1 = 1;
        expected_move_construct2 = 2;
        expected_destroy2 = 1;
      } else {
        expected_move_construct1 = 0;
        expected_destroy1 = 1;
        expected_move_construct2 = 0;
        expected_destroy2 = 0;
      }
      assert(stats2.move_construct == expected_move_construct2);
      assert(stats2.destroy == expected_destroy2);

      assert(stats1.move_construct == expected_move_construct1);
      assert(stats1.destroy == expected_destroy1);
    }

    int total_expected_destroy1 = 0;
    int total_expected_destroy2 = 0;
    if (lhsSmall && rhsSmall) {
      total_expected_destroy1 = 3;
      total_expected_destroy2 = 3;
    } else if (lhsSmall) {
      total_expected_destroy1 = 3;
      total_expected_destroy2 = 1;
    } else if (rhsSmall) {
      total_expected_destroy1 = 1;
      total_expected_destroy2 = 3;
    } else {
      total_expected_destroy1 = 1;
      total_expected_destroy2 = 1;
    }
    assert(stats1.destroy == total_expected_destroy1);
    assert(stats2.destroy == total_expected_destroy2);
  }
}

template <class LHS, class RHS>
constexpr void copy_assignment() {
  // non singular
  {
    Storage s1{type<LHS>{}, 5};
    Storage s2{type<RHS>{}, 6};

    s1 = s2;
    assert(*s1.get_ptr<RHS>() == 6);
    assert(*s2.get_ptr<RHS>() == 6);
  }

  // lhs singular
  {
    Storage s1{};
    Storage s2{type<RHS>{}, 6};

    s1 = s2;
    assert(*s1.get_ptr<RHS>() == 6);
    assert(*s2.get_ptr<RHS>() == 6);
  }

  // rhs singular
  {
    Storage s1{type<LHS>{}, 5};
    Storage s2{};

    s1 = s2;
    assert(s1.is_singular());
    assert(s2.is_singular());
  }

  // track
  {
    Stats stats1{};
    Stats stats2{};
    bool lhsSmall = Storage::unittest_is_small<Track<LHS>>();
    bool rhsSmall = Storage::unittest_is_small<Track<RHS>>();
    {
      Storage s1{type<Track<LHS>>{}, stats1, 5};
      assert(stats1.construct == 1);
      assert(stats1.copy_construct == 0);
      assert(stats1.move_construct == 0);
      assert(stats1.destroy == 0);

      assert(s1.get_ptr<Track<LHS>>()->t_ == 5);

      Storage s2{type<Track<RHS>>{}, stats2, 6};
      assert(stats2.construct == 1);
      assert(stats2.copy_construct == 0);
      assert(stats2.move_construct == 0);
      assert(stats2.destroy == 0);

      assert(s2.get_ptr<Track<RHS>>()->t_ == 6);

      s1 = s2;

      assert(stats1.construct == 1);
      assert(stats1.copy_construct == 0);
      assert(stats1.copy_assignment == 0);
      assert(stats1.move_assignment == 0);

      assert(stats2.construct == 1);
      assert(stats2.copy_construct == 1);
      assert(stats2.copy_assignment == 0);
      assert(stats2.move_assignment == 0);

      int expected_move_construct1 = 0;
      int expected_destroy1 = 0;
      int expected_move_construct2 = 0;
      int expected_destroy2 = 0;

      if (lhsSmall && rhsSmall) {
        expected_move_construct1 = 2;
        expected_destroy1 = 3;
        expected_move_construct2 = 1;
        expected_destroy2 = 1;
      } else if (lhsSmall) {
        expected_move_construct1 = 2;
        expected_destroy1 = 3;
        expected_move_construct2 = 0;
        expected_destroy2 = 0;
      } else if (rhsSmall) {
        expected_move_construct1 = 0;
        expected_destroy1 = 1;
        expected_move_construct2 = 1;
        expected_destroy2 = 1;
      } else {
        expected_move_construct1 = 0;
        expected_destroy1 = 1;
        expected_move_construct2 = 0;
        expected_destroy2 = 0;
      }
      assert(stats2.move_construct == expected_move_construct2);
      assert(stats2.destroy == expected_destroy2);

      assert(stats1.move_construct == expected_move_construct1);
      assert(stats1.destroy == expected_destroy1);
    }

    int total_expected_destroy1 = 0;
    int total_expected_destroy2 = 0;
    if (lhsSmall && rhsSmall) {
      total_expected_destroy1 = 3;
      total_expected_destroy2 = 3;
    } else if (lhsSmall) {
      total_expected_destroy1 = 3;
      total_expected_destroy2 = 2;
    } else if (rhsSmall) {
      total_expected_destroy1 = 1;
      total_expected_destroy2 = 3;
    } else {
      total_expected_destroy1 = 1;
      total_expected_destroy2 = 2;
    }
    assert(stats1.destroy == total_expected_destroy1);
    assert(stats2.destroy == total_expected_destroy2);
  }
}

[[maybe_unused]] constexpr bool test() {
  singular();
  basic<Big>();
  copy<Big>();
  move<Big>();
  copy_assignment<Big, Big>();
  move_assignment<Big, Big>();
  return true;
}

void non_constexpr_test() {
  // type punning in small buffer not allowed in constexpr
  basic<Small>();
  copy<Small>();
  move<Small>();
  copy_assignment<Small, Small>();
  copy_assignment<Big, Small>();
  copy_assignment<Small, Big>();
  move_assignment<Small, Small>();
  move_assignment<Big, Small>();
  move_assignment<Small, Big>();
}

TEST_POINT("storage") {
  test();
  non_constexpr_test();

  static_assert(test());
}

}  // namespace
