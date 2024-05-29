#ifndef LIBCPP__RANGE_ANY_VIEW_HPP
#define LIBCPP__RANGE_ANY_VIEW_HPP

#include <cassert>
#include <memory>
#include <ranges>
#include <type_traits>
namespace std::ranges {

inline namespace __any_view {

enum class category {
  none = 0,
  input = 1,
  forward = 3,
  bidirectional = 7,
  random_access = 15,
  contiguous = 31,
  category_mask = contiguous,
  sized = 32,
  borrowed = 64,
  common = 128,
  move_only_view = 256
};

constexpr category operator&(category lhs, category rhs) noexcept {
  return static_cast<category>(
      static_cast<std::underlying_type_t<category>>(lhs) &
      static_cast<std::underlying_type_t<category>>(rhs));
}

constexpr category operator|(category lhs, category rhs) noexcept {
  return static_cast<category>(
      static_cast<std::underlying_type_t<category>>(lhs) |
      static_cast<std::underlying_type_t<category>>(rhs));
}

constexpr auto operator<=>(category lhs, category rhs) noexcept {
  return static_cast<std::underlying_type_t<category>>(lhs) <=>
         static_cast<std::underlying_type_t<category>>(rhs);
}

}  // namespace __any_view

template <class Ref, category Cat = category::input, class Value = decay_t<Ref>,
          class RValueRef = add_rvalue_reference_t<Value>,
          class Diff = ptrdiff_t>
class any_view {
 public:
  // "requires" is not allowed to use with virtual functions.
  // so it looks complicated
  template <category>
  struct iterator_interface;

  struct basic_input_iterator_interface {
    constexpr virtual ~basic_input_iterator_interface() = default;

    constexpr virtual std::unique_ptr<basic_input_iterator_interface>
    move() = 0;

    constexpr virtual Ref deref() const = 0;
    constexpr virtual void increment() = 0;

    constexpr virtual RValueRef iter_move() const = 0;
  };

  struct common_input_iterator : basic_input_iterator_interface {
    constexpr virtual std::unique_ptr<common_input_iterator> clone() const = 0;
    constexpr virtual bool equal(const common_input_iterator &) const = 0;
  };

  template <>
  struct iterator_interface<category::input>
      : std::conditional_t<(Cat & category::common) != category::none,
                           common_input_iterator,
                           basic_input_iterator_interface> {};

  template <>
  struct iterator_interface<category::forward>
      : basic_input_iterator_interface {
    iterator_interface() = default;
    constexpr virtual std::unique_ptr<iterator_interface> clone() const = 0;

    constexpr virtual bool equal(const iterator_interface &) const = 0;
  };

  template <>
  struct iterator_interface<category::bidirectional>
      : iterator_interface<category::forward> {
    constexpr virtual void decrement() = 0;
  };

  template <>
  struct iterator_interface<category::random_access>
      : iterator_interface<category::bidirectional> {
    constexpr virtual void advance(Diff) = 0;
    constexpr virtual Diff distance_to(const iterator_interface &) const = 0;
  };

  using any_iter_interface = iterator_interface<Cat & category::category_mask>;
  template <class Iter>
  struct iterator_impl : any_iter_interface {
    constexpr explicit iterator_impl(Iter tt) : it(std::move(tt)) {}

    // input
    constexpr std::unique_ptr<basic_input_iterator_interface> move() {
      return std::make_unique<iterator_impl>(std::move(it));
    }

    constexpr Ref deref() const { return *it; };
    constexpr void increment() { ++it; };

    constexpr RValueRef iter_move() const {
      return std::ranges::iter_move(it);
    };

    // note: the following functions should ideally be constrained
    // but requires is not allowed in virtual functions

    // forward

    iterator_impl() = default;

    using common_interface = std::conditional_t<
        std::is_base_of_v<common_input_iterator, any_iter_interface>,
        common_input_iterator, iterator_interface<category::forward>>;

    constexpr virtual std::unique_ptr<common_interface> clone() const {
      if constexpr ((Cat & category::category_mask) >= category::forward ||
                    (Cat & category::common) != category::none) {
        return std::make_unique<iterator_impl>(it);
      } else {
        assert(false);
        return nullptr;
      }
    }

    constexpr bool equal(const common_interface &other) const {
      if constexpr ((Cat & category::category_mask) >= category::forward ||
                    (Cat & category::common) != category::none) {
        return it == static_cast<const iterator_impl &>(other).it;
      } else {
        assert(false);
        return false;
      }
    }

    // bidi

    constexpr void decrement() {
      if constexpr ((Cat & category::category_mask) >=
                    category::bidirectional) {
        --it;
      } else {
        assert(false);
      }
    }

    // random access

    constexpr void advance(Diff diff) {
      if constexpr ((Cat & category::category_mask) >=
                    category::random_access) {
        it += diff;
      } else {
        assert(false);
      }
    }

    constexpr Diff distance_to(const any_iter_interface &other) const {
      if constexpr ((Cat & category::category_mask) >=
                    category::random_access) {
        return Diff(it - static_cast<const iterator_impl &>(other).it);
      } else {
        assert(false);
        return Diff{};
      }
    }

    Iter it;
  };

  struct singular_impl : any_iter_interface {
    singular_impl() = default;

    constexpr std::unique_ptr<basic_input_iterator_interface> move() {
      return std::make_unique<singular_impl>();
    }

    constexpr Ref deref() const { assert(false); };
    constexpr void increment() { assert(false); };

    constexpr RValueRef iter_move() const { assert(false); };

    using common_interface = std::conditional_t<
        std::is_base_of_v<common_input_iterator, any_iter_interface>,
        common_input_iterator, iterator_interface<category::forward>>;

    constexpr virtual std::unique_ptr<common_interface> clone() const {
      return std::make_unique<singular_impl>();
    }

    constexpr bool equal(const common_interface &other) const {
      return dynamic_cast<singular_impl const *>(&other) != nullptr;
    }

    // bidi
    constexpr void decrement() { assert(false); }

    // random access

    constexpr void advance(Diff) { assert(false); }
    constexpr Diff distance_to(const any_iter_interface &) const {
      assert(false);
      return Diff{};
    }
  };

  struct empty_iterator_category {};
  struct with_iterator_category {
   private:
    constexpr static auto get_category() {
      constexpr auto cat_mask = Cat & category::category_mask;
      if constexpr (!std::is_reference_v<Ref>) {
        return std::input_iterator_tag{};
      } else if constexpr (cat_mask >= category::random_access) {
        return std::random_access_iterator_tag{};
      } else if constexpr (cat_mask == category::bidirectional) {
        return std::bidirectional_iterator_tag{};
      } else if constexpr (cat_mask == category::forward) {
        return std::forward_iterator_tag{};
      } else {
        return std::input_iterator_tag{};
      }
    }

   public:
    using iterator_category = decltype(get_category());
  };
  constexpr static auto get_concept() {
    constexpr auto cat_mask = Cat & category::category_mask;
    if constexpr (cat_mask >= category::random_access) {
      return std::random_access_iterator_tag{};
    } else if constexpr (cat_mask == category::bidirectional) {
      return std::bidirectional_iterator_tag{};
    } else if constexpr (cat_mask == category::forward) {
      return std::forward_iterator_tag{};
    } else {
      return std::input_iterator_tag{};
    }
  }
  struct any_iterator
      : std::conditional_t<(Cat & category::category_mask) >= category::forward,
                           with_iterator_category, empty_iterator_category> {
    using iterator_concept = decltype(get_concept());
    using value_type = Value;
    using difference_type = Diff;

    constexpr any_iterator() : impl_(std::make_unique<singular_impl>()) {}

    template <class Iter>
    consteval static bool category_constraint() {
      constexpr auto cat_mask = Cat & category::category_mask;
      if constexpr (cat_mask == category::contiguous) {
        return std::contiguous_iterator<Iter>;
      } else if constexpr (cat_mask == category::random_access) {
        return std::random_access_iterator<Iter>;
      } else if constexpr (cat_mask == category::bidirectional) {
        return std::bidirectional_iterator<Iter>;
      } else if constexpr (cat_mask == category::forward) {
        return std::forward_iterator<Iter>;
      } else {
        return std::input_iterator<Iter>;
      }
    }

    template <class Iter>
      requires(!std::same_as<Iter, any_iterator> && category_constraint<Iter>())
    constexpr any_iterator(Iter iter)
        : impl_(std::make_unique<iterator_impl<Iter>>(std::move(iter))) {}

    // TODO: do we allow copy for input iterator and how?
    constexpr any_iterator(const any_iterator &other)
      requires((Cat & category::category_mask) >= category::forward ||
               (Cat & category::common) != category::none)
    {
      impl_ = cast_unique_ptr(other.impl_->clone());
    }

    constexpr any_iterator(any_iterator &&other) {
      impl_ = cast_unique_ptr(other.impl_->move());
    }

    constexpr any_iterator &operator=(const any_iterator &other)
      requires((Cat & category::category_mask) >= category::forward ||
               (Cat & category::common) != category::none)
    {
      if (this != &other) {
        impl_ = cast_unique_ptr(other.impl_->clone());
      }
      return *this;
    }

    constexpr any_iterator &operator=(any_iterator &&other) {
      if (this != &other) {
        impl_ = cast_unique_ptr(other.impl_->move());
      }
      return *this;
    }

    constexpr Ref operator*() const { return impl_->deref(); }

    constexpr any_iterator &operator++() {
      impl_->increment();
      return *this;
    }

    constexpr void operator++(int) { ++(*this); }

    constexpr any_iterator operator++(int)
      requires((Cat & category::category_mask) >= category::forward)
    {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }

    constexpr any_iterator &operator--()
      requires((Cat & category::category_mask) >= category::bidirectional)
    {
      impl_->decrement();
      return *this;
    }

    constexpr any_iterator operator--(int)
      requires((Cat & category::category_mask) >= category::bidirectional)
    {
      auto tmp = *this;
      --(*this);
      return tmp;
    }

    constexpr any_iterator &operator+=(difference_type n)
      requires((Cat & category::category_mask) >= category::random_access)
    {
      impl_->advance(n);
      return *this;
    }

    constexpr any_iterator &operator-=(difference_type n)
      requires((Cat & category::category_mask) >= category::random_access)
    {
      *this += -n;
      return *this;
    }

    constexpr Ref operator[](difference_type n) const
      requires((Cat & category::category_mask) >= category::random_access)
    {
      return *((*this) + n);
    }

    friend constexpr bool operator<(const any_iterator &x,
                                    const any_iterator &y)
      requires((Cat & category::category_mask) >= category::random_access)
    {
      return (x - y) < 0;
    }

    friend constexpr bool operator>(const any_iterator &x,
                                    const any_iterator &y)
      requires((Cat & category::category_mask) >= category::random_access)
    {
      return (x - y) > 0;
    }

    friend constexpr bool operator<=(const any_iterator &x,
                                     const any_iterator &y)
      requires((Cat & category::category_mask) >= category::random_access)
    {
      return (x - y) <= 0;
    }

    friend constexpr bool operator>=(const any_iterator &x,
                                     const any_iterator &y)
      requires((Cat & category::category_mask) >= category::random_access)
    {
      return (x - y) >= 0;
    }

    friend constexpr any_iterator operator+(const any_iterator &it,
                                            difference_type n)
      requires((Cat & category::category_mask) >= category::random_access)
    {
      auto temp = it;
      temp += n;
      return temp;
    }

    friend constexpr any_iterator operator+(difference_type n,
                                            const any_iterator &it)
      requires((Cat & category::category_mask) >= category::random_access)
    {
      return it + n;
    }

    friend constexpr any_iterator operator-(const any_iterator &it,
                                            difference_type n)
      requires((Cat & category::category_mask) >= category::random_access)
    {
      auto temp = it;
      temp -= n;
      return temp;
    }

    friend constexpr difference_type operator-(const any_iterator &x,
                                               const any_iterator &y)
      requires((Cat & category::category_mask) >= category::random_access)
    {
      return x.impl_->distance_to(*(y.impl_));
    }

    friend constexpr bool operator==(const any_iterator &x,
                                     const any_iterator &y)
      requires((Cat & category::category_mask) >= category::forward ||
               (Cat & category::common) != category::none)
    {
      return x.impl_->equal(*(y.impl_));
    }

    friend constexpr RValueRef iter_move(const any_iterator &iter) {
      return iter.impl_->iter_move();
    }

    constexpr any_iter_interface const &get_impl() const { return *impl_; }

   private:
    std::unique_ptr<any_iter_interface> impl_;

    constexpr any_iterator(std::unique_ptr<any_iter_interface> &&impl)
        : impl_(std::move(impl)) {}

    template <class T>
    constexpr static std::unique_ptr<any_iter_interface> cast_unique_ptr(
        std::unique_ptr<T> &&p) {
      return std::unique_ptr<any_iter_interface>(
          static_cast<any_iter_interface *>(p.release()));
    }
  };

  using iterator = std::conditional_t<(Cat & category::category_mask) ==
                                          category::contiguous,
                                      std::add_pointer_t<Ref>, any_iterator>;
  struct any_sentinel_interface {
    constexpr virtual std::unique_ptr<any_sentinel_interface> move() = 0;

    constexpr virtual std::unique_ptr<any_sentinel_interface> clone() const = 0;

    constexpr virtual ~any_sentinel_interface() = default;

    constexpr virtual bool equal(const iterator &) const = 0;
  };

  template <class Sent, class Iter>
  struct sentinel_impl : any_sentinel_interface {
    constexpr explicit sentinel_impl(Sent st) : st_(std::move(st)) {}
    constexpr std::unique_ptr<any_sentinel_interface> move() override {
      return std::make_unique<sentinel_impl>(std::move(st_));
    }

    constexpr std::unique_ptr<any_sentinel_interface> clone() const override {
      return std::make_unique<sentinel_impl>(st_);
    }

    constexpr bool equal(const iterator &iter) const override {
      if constexpr ((Cat & category::category_mask) == category::contiguous) {
        return st_ == iter;
      } else {
        if (auto iter_impl =
                dynamic_cast<iterator_impl<Iter> const *>(&iter.get_impl())) {
          return iter_impl->it == st_;
        }
      }
      return false;
    }

   private:
    Sent st_;
  };

  struct any_sentinel {
    constexpr any_sentinel() = default;

    template <class T>
    struct tag {};

    template <class Sent, class Iter>
    constexpr any_sentinel(Sent sent, tag<Iter>)
        : impl_(std::make_unique<sentinel_impl<Sent, Iter>>(std::move(sent))) {}

    constexpr any_sentinel(const any_sentinel &other) {
      if (other.impl_) {
        impl_ = other.impl_->clone();
      }
    }

    constexpr any_sentinel(any_sentinel &&other) {
      if (other.impl_) {
        impl_ = other.impl_->move();
      }
    }

    constexpr any_sentinel &operator=(const any_sentinel &other) {
      if (this != &other) {
        impl_ = other.impl_ ? other.impl_->clone() : other.impl_;
      }
      return *this;
    }

    constexpr any_sentinel &operator=(any_sentinel &&other) {
      if (this != &other) {
        impl_ = other.impl_ ? other.impl_->move() : other.impl_;
      }
      return *this;
    }

    friend constexpr bool operator==(const iterator &iter,
                                     const any_sentinel &sent) {
      if (!sent.impl_) return false;
      return sent.impl_->equal(iter);
    }

   private:
    std::unique_ptr<any_sentinel_interface> impl_ = nullptr;

    constexpr any_sentinel(std::unique_ptr<any_sentinel_interface> &&impl)
        : impl_(std::move(impl)) {}
  };

  using sentinel =
      std::conditional_t<(Cat & category::common) == category::none,
                         any_sentinel, iterator>;

  struct sized_interface {
    constexpr virtual std::size_t size() const = 0;
  };

  template <class derived>
  struct clonable_interface {
    constexpr virtual std::unique_ptr<derived> clone() const = 0;
  };

  template <class derived>
  struct basic_any_view_interface {
    constexpr virtual std::unique_ptr<derived> move() = 0;

    constexpr virtual iterator begin() = 0;
    constexpr virtual sentinel end() = 0;

    constexpr virtual ~basic_any_view_interface() = default;
  };

  struct any_view_interface : basic_any_view_interface<any_view_interface>,
                              sized_interface,
                              clonable_interface<any_view_interface> {};

  template <class View>
  struct any_view_impl : any_view_interface {
    constexpr explicit any_view_impl(View view) : view_(std::move(view)) {}

    constexpr std::unique_ptr<any_view_interface> move() override {
      return std::make_unique<any_view_impl>(std::move(view_));
    }

    constexpr std::unique_ptr<any_view_interface> clone() const override {
      if constexpr ((Cat & category::move_only_view) == category::none) {
        return std::make_unique<any_view_impl>(view_);
      } else {
        assert(false);
        return nullptr;
      }
    }

    constexpr iterator begin() override {
      if constexpr ((Cat & category::category_mask) == category::contiguous) {
        return std::ranges::begin(view_);
      } else {
        return any_iterator(std::ranges::begin(view_));
      }
    }

    constexpr sentinel end() override {
      if constexpr ((Cat & category::category_mask) == category::contiguous &&
                    (Cat & category::common) != category::none) {
        return std::ranges::end(view_);
      } else if constexpr ((Cat & category::common) != category::none) {
        return any_iterator(std::ranges::end(view_));
      } else {
        using tag_t = any_sentinel::template tag<std::ranges::iterator_t<View>>;
        return any_sentinel(std::ranges::end(view_), tag_t{});
      }
    }

    constexpr std::size_t size() const override {
      if constexpr ((Cat & category::sized) != category::none) {
        return std::ranges::size(view_);
      } else {
        assert(false);
        return 0;
      }
    }

   private:
    View view_;
  };

  template <class View>
  consteval static bool view_category_constraint() {
    if constexpr ((Cat & category::sized) != category::none &&
                  !std::ranges::sized_range<View>) {
      return false;
    }

    if constexpr ((Cat & category::common) != category::none &&
                  !std::ranges::common_range<View>) {
      return false;
    }

    if constexpr ((Cat & category::borrowed) != category::none &&
                  !std::ranges::borrowed_range<View>) {
      return false;
    }

    if constexpr ((Cat & category::move_only_view) == category::none &&
                  !std::copyable<View>) {
      return false;
    }
    constexpr auto cat_mask = Cat & category::category_mask;
    if constexpr (cat_mask == category::contiguous) {
      return std::ranges::contiguous_range<View>;
    } else if constexpr (cat_mask == category::random_access) {
      return std::ranges::random_access_range<View>;
    } else if constexpr (cat_mask == category::bidirectional) {
      return std::ranges::bidirectional_range<View>;
    } else if constexpr (cat_mask == category::forward) {
      return std::ranges::forward_range<View>;
    } else {
      return std::ranges::input_range<View>;
    }
  }

  template <class View>
    requires(!std::same_as<View, any_view> && std::ranges::view<View> &&
             view_category_constraint<View>())
  constexpr any_view(View view)
      : impl_(std::make_unique<any_view_impl<View>>(std::move(view))) {}

  constexpr any_view(const any_view &other)
    requires((Cat & category::move_only_view) == category::none)
      : impl_(other.impl_->clone()) {}

  constexpr any_view(any_view &&other) : impl_(other.impl_->move()) {}

  constexpr any_view &operator=(const any_view &other)
    requires((Cat & category::move_only_view) == category::none)
  {
    if (this != &other) {
      impl_ = other.impl_->clone();
    }
    return *this;
  }

  constexpr any_view &operator=(any_view &&other) {
    if (this != &other) {
      impl_ = other.impl_->move();
    }
    return *this;
  }

  constexpr iterator begin() { return impl_->begin(); }
  constexpr sentinel end() { return impl_->end(); }

  constexpr std::size_t size() const
    requires((Cat & category::sized) != category::none)
  {
    return impl_->size();
  }

 private:
  std::unique_ptr<any_view_interface> impl_;
};

template <class Ref, category Cat, class Value, class RValueRef, class Diff>
inline constexpr bool
    enable_borrowed_range<any_view<Ref, Cat, Value, RValueRef, Diff>> =
        (Cat & category::borrowed) != category::none;

}  // namespace std::ranges

#endif
