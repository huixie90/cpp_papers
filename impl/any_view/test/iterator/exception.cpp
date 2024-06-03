#include <array>
#include <catch2/catch_test_macros.hpp>

#include "any_view.hpp"

#define TEST_POINT(x) TEST_CASE(x, "[exception]")
using AnyView = std::ranges::any_view<int&, std::ranges::category::forward>;
using Iter = std::ranges::iterator_t<AnyView>;

struct ThrowIterator {
  using It = int*;
  It it;

  constexpr ThrowIterator() = default;
  constexpr ThrowIterator(int* i) : it(i) {}
  constexpr ThrowIterator(const ThrowIterator& other) : it(other.it) {
    throw 5;
  }
  constexpr ThrowIterator(ThrowIterator&& other) : it(std::move(other.it)) {
    if (MoveThrow) throw 6;
  }
  constexpr ThrowIterator& operator=(const ThrowIterator&) = default;
  constexpr ThrowIterator& operator=(ThrowIterator&&) = default;

  int& operator*() const { return *it; }

  ThrowIterator& operator++() {
    ++it;
    return *this;
  }
  ThrowIterator operator++(int) { return {it++}; }

  bool operator==(ThrowIterator other) const { return it == other.it; }

  using value_type = int;  // to model indirectly_readable_traits
  using difference_type = std::ptrdiff_t;  // to model incrementable_traits

  static inline bool MoveThrow = false;
};

template <class Iter, class Sent = Iter, class NonConstIter = Iter,
          class NonConstSent = Sent>
struct BufferView : std::ranges::view_base {
  using T = std::iter_value_t<Iter>;
  T* data_;
  std::size_t size_;

  template <std::size_t N>
  constexpr BufferView(T (&b)[N]) : data_(b), size_(N) {}
  constexpr BufferView(T* p, std::size_t s) : data_(p), size_(s) {}

  constexpr NonConstIter begin()
    requires(!std::is_same_v<Iter, NonConstIter>)
  {
    return NonConstIter(this->data_);
  }
  constexpr Iter begin() const { return Iter(this->data_); }

  constexpr NonConstSent end()
    requires(!std::is_same_v<Sent, NonConstSent>)
  {
    if constexpr (std::is_same_v<NonConstIter, NonConstSent>) {
      return NonConstIter(this->data_ + this->size_);
    } else {
      return NonConstSent(NonConstIter(this->data_ + this->size_));
    }
  }

  constexpr Sent end() const {
    if constexpr (std::is_same_v<Iter, Sent>) {
      return Iter(this->data_ + this->size_);
    } else {
      return Sent(Iter(this->data_ + this->size_));
    }
  }
};

constexpr void copy_exception() {
  int a[] = {1, 2, 3};
  using View = BufferView<ThrowIterator>;
  AnyView av{View{a}};
  Iter iter1 = av.begin();
  ++iter1;
  Iter iter2 = av.begin();

  try {
    iter2 = iter1;
    assert(false);
  } catch (int i) {
    assert(i == 5);
  }
  assert(*iter1 == 2);
  assert(*iter2 == 1);
}

constexpr void move_exception() {
  int a[] = {1, 2, 3};
  using View = BufferView<ThrowIterator>;
  AnyView av{View{a}};
  Iter iter1 = av.begin();
  ++iter1;
  Iter iter2 = av.begin();

  ThrowIterator::MoveThrow = true;
  
  // won't throw because there should be no move happening
  iter2 = std::move(iter1);

  assert(*iter2 == 2);
  assert(iter1.is_singular());
}

constexpr bool test() {
  copy_exception();
  move_exception();
  return true;
}

TEST_POINT("exception") {
  test();
  //  static_assert(test());
}
