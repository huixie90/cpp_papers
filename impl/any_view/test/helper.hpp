#pragma once

#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>

template <class Iter, class Concept, class Cat = Concept>
struct test_iter {
  using iterator_concept = Concept;
  using iterator_category = Cat;
  using value_type = typename std::iterator_traits<Iter>::value_type;
  using difference_type = typename std::iterator_traits<Iter>::difference_type;

  constexpr test_iter() = default;
  constexpr explicit test_iter(Iter iter) : iter_(std::move(iter)) {}

  constexpr decltype(auto) operator*() const { return *iter_; }

  constexpr test_iter &operator++() {
    ++iter_;
    return *this;
  }

  constexpr test_iter operator++(int) {
    auto tmp = *this;
    ++(*this);
    return tmp;
  }

  constexpr test_iter &operator--()
    requires std::derived_from<Concept, std::bidirectional_iterator_tag>
  {
    --iter_;
    return *this;
  }
  constexpr test_iter operator--(int)
    requires std::derived_from<Concept, std::bidirectional_iterator_tag>
  {
    auto tmp = *this;
    --(*this);
    return tmp;
  }

  constexpr test_iter &operator+=(difference_type n)
    requires std::derived_from<Concept, std::random_access_iterator_tag>
  {
    iter_ += n;
    return *this;
  }

  constexpr test_iter &operator-=(difference_type n)
    requires std::derived_from<Concept, std::random_access_iterator_tag>
  {
    iter_ -= n;
    return *this;
  }

  constexpr friend test_iter operator+(test_iter i, difference_type n)
    requires std::derived_from<Concept, std::random_access_iterator_tag>
  {
    i += n;
    return i;
  }

  constexpr friend test_iter operator+(difference_type n, test_iter i)
    requires std::derived_from<Concept, std::random_access_iterator_tag>
  {
    return i + n;
  }

  constexpr friend test_iter operator-(test_iter i, difference_type n)
    requires std::derived_from<Concept, std::random_access_iterator_tag>
  {
    i -= n;
    return i;
  }

  constexpr difference_type operator-(const test_iter &other) const
    requires std::derived_from<Concept, std::random_access_iterator_tag>
  {
    return iter_ - other.iter_;
  }
  constexpr decltype(auto) operator[](difference_type n) const
    requires std::derived_from<Concept, std::random_access_iterator_tag>
  {
    return iter_[n];
  }

  constexpr std::add_pointer_t<value_type> operator->() const
    requires std::derived_from<Concept, std::contiguous_iterator_tag>
  {
    return std::addressof(*iter_);
  }

  friend constexpr bool operator==(const test_iter &,
                                   const test_iter &) = default;
  friend constexpr auto operator<=>(const test_iter &,
                                    const test_iter &) = default;

  constexpr Iter base() const { return iter_; }

 private:
  Iter iter_;
};

static_assert(std::input_iterator<test_iter<int *, std::input_iterator_tag>>);
static_assert(
    std::forward_iterator<test_iter<int *, std::forward_iterator_tag>>);
static_assert(std::bidirectional_iterator<
              test_iter<int *, std::bidirectional_iterator_tag>>);
static_assert(std::random_access_iterator<
              test_iter<int *, std::random_access_iterator_tag>>);
static_assert(
    std::contiguous_iterator<test_iter<int *, std::contiguous_iterator_tag>>);

template <class Iter, class Sent = Iter, bool sized = false,
          bool copyable = false, bool borrowed = false>
struct ArrView : std::ranges::view_base {
  using Value = typename std::iterator_traits<Iter>::value_type;

  template <std::size_t N>
  constexpr ArrView(Value (&arr)[N]) : arr_(arr), size_(N) {}

  constexpr ArrView(const ArrView &)
    requires copyable
  = default;
  constexpr ArrView(ArrView &&) = default;
  constexpr ArrView &operator=(const ArrView &)
    requires copyable
  = default;
  constexpr ArrView &operator=(ArrView &&) = default;

  constexpr Iter begin() { return Iter(arr_); }
  constexpr Sent end() { return Sent(Iter(arr_ + size_)); }

  constexpr std::size_t size() const
    requires sized
  {
    return size_;
  }

  Value *arr_;
  std::size_t size_;
};

template <class Iter, class Sent, bool sized, bool copyable, bool borrowed>
inline constexpr bool std::ranges::enable_borrowed_range<
    ArrView<Iter, Sent, sized, copyable, borrowed>> = borrowed;

using InputView = ArrView<test_iter<int *, std::input_iterator_tag>>;
using ForwardView = ArrView<test_iter<int *, std::forward_iterator_tag>>;
using BidiView = ArrView<test_iter<int *, std::bidirectional_iterator_tag>>;
using RandomAccessView =
    ArrView<test_iter<int *, std::random_access_iterator_tag>>;
using ContiguousView = ArrView<test_iter<int *, std::contiguous_iterator_tag>>;

using UnsizedInputView = InputView;
using SizedInputView = ArrView<test_iter<int *, std::input_iterator_tag>,
                               test_iter<int *, std::input_iterator_tag>, true>;
static_assert(!std::ranges::sized_range<UnsizedInputView>);
static_assert(std::ranges::sized_range<SizedInputView>);

using MoveOnlyInputView = InputView;
using CopyableInputView =
    ArrView<test_iter<int *, std::input_iterator_tag>,
            test_iter<int *, std::input_iterator_tag>, false, true>;
static_assert(!std::copyable<MoveOnlyInputView>);
static_assert(std::copyable<CopyableInputView>);

using NonBorrowedInputView = InputView;
using BorrowedInputView =
    ArrView<test_iter<int *, std::input_iterator_tag>,
            test_iter<int *, std::input_iterator_tag>, false, false, true>;
static_assert(!std::ranges::borrowed_range<NonBorrowedInputView>);
static_assert(std::ranges::borrowed_range<BorrowedInputView>);

static_assert(std::ranges::input_range<InputView>);
static_assert(std::ranges::forward_range<ForwardView>);
static_assert(std::ranges::bidirectional_range<BidiView>);
static_assert(std::ranges::random_access_range<RandomAccessView>);
static_assert(std::ranges::contiguous_range<ContiguousView>);



struct MoveOnly {
    int i;
    constexpr explicit MoveOnly(int x) : i(x) {}
    constexpr MoveOnly(MoveOnly &&) noexcept = default;
    constexpr MoveOnly(const MoveOnly &) = delete;
    constexpr MoveOnly &operator=(MoveOnly &&) noexcept = default;
    constexpr MoveOnly &operator=(const MoveOnly &) = delete;
};
