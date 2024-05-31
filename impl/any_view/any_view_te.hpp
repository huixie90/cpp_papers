#ifndef LIBCPP__RANGE_ANY_VIEW_TE_HPP
#define LIBCPP__RANGE_ANY_VIEW_TE_HPP

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
  struct any_iterator;
  struct any_sentinel;

  template <class T, bool HasT>
  struct maybe_t : T {};

  template <class T>
  struct maybe_t<T, false> {};

  struct destructor_vtable {
    void (*destructor_)(void *);
  };

  struct movable_vtable {
    void *(*move_)(void *);
  };

  struct copyable_vtable {
    void *(*copy_)(const void *);
  };

  template <category>
  struct iterator_vtable;

  struct basic_input_iterator_vtable : destructor_vtable, movable_vtable {
    Ref (*deref_)(const void *);
    void (*increment_)(void *);
    RValueRef (*iter_move_)(const void *);
  };

  struct equality_vtable {
    bool (*equal_)(const void *, const void *);
  };

  struct common_input_iterator : basic_input_iterator_vtable,
                                 copyable_vtable,
                                 equality_vtable {};

  template <>
  struct iterator_vtable<category::input>
      : std::conditional_t<(Cat & category::common) != category::none,
                           common_input_iterator, basic_input_iterator_vtable> {
  };

  template <>
  struct iterator_vtable<category::forward>
      : basic_input_iterator_vtable, copyable_vtable, equality_vtable {};

  template <>
  struct iterator_vtable<category::bidirectional>
      : iterator_vtable<category::forward> {
    void (*decrement_)(void *);
  };

  template <>
  struct iterator_vtable<category::random_access>
      : iterator_vtable<category::bidirectional> {
    void (*advance_)(void *, Diff);
    Diff (*distance_to_)(const void *, const void *);
  };

  // for contiguous , we just return raw pointers
  template <>
  struct iterator_vtable<category::contiguous> {};

  using any_iterator_vtable = iterator_vtable<Cat & category::category_mask>;

  struct basic_vtable_gen {
    template <class T>
    static constexpr void *move(void *self) {
      return new T(std::move(*(static_cast<T *>(self))));
    }

    template <class T>
    static constexpr void *copy(const void *self) {
      return new T(*(static_cast<const T *>(self)));
    }

    template <class T>
    static constexpr void destroy(void *self) {
      return std::destroy_at(static_cast<T *>(self));
    }
  };

  struct iterator_vtable_gen : basic_vtable_gen {
    template <class Iter>
    static constexpr auto generate() {
      any_iterator_vtable t;

      if constexpr ((Cat & category::category_mask) != category::contiguous) {
        t.move_ = &basic_vtable_gen::template move<Iter>;
        t.destructor_ = &basic_vtable_gen::template destroy<Iter>;
        t.deref_ = &deref<Iter>;
        t.increment_ = &increment<Iter>;
        t.iter_move_ = &iter_move<Iter>;

        if constexpr ((Cat & category::category_mask) >= category::forward ||
                      (Cat & category::common) != category::none) {
          t.copy_ = &basic_vtable_gen::template copy<Iter>;
          t.equal_ = &equal<Iter>;
        }

        if constexpr ((Cat & category::category_mask) >=
                      category::bidirectional) {
          t.decrement_ = &decrement<Iter>;
        }

        if constexpr ((Cat & category::category_mask) >=
                      category::random_access) {
          t.advance_ = &advance<Iter>;
          t.distance_to_ = &distance_to<Iter>;
        }
      }
      return t;
    }

    // input

    template <class Iter>
    static constexpr Ref deref(const void *self) {
      return **(static_cast<const Iter *>(self));
    };

    template <class Iter>
    static constexpr void increment(void *self) {
      ++(*(static_cast<Iter *>(self)));
    };

    template <class Iter>
    static constexpr RValueRef iter_move(const void *self) {
      return std::ranges::iter_move(*(static_cast<const Iter *>(self)));
    };

    // forward

    template <class Iter>
    static constexpr bool equal(const void *self, const void *other) {
      return (*static_cast<const Iter *>(self)) ==
             (*static_cast<const Iter *>(other));
    }

    // bidi

    template <class Iter>
    static constexpr void decrement(void *self) {
      --(*(static_cast<Iter *>(self)));
    }

    // random access

    template <class Iter>
    static constexpr void advance(void *self, Diff diff) {
      (*static_cast<Iter *>(self)) += diff;
    }

    template <class Iter>
    static constexpr Diff distance_to(const void *self, const void *other) {
      return Diff((*static_cast<const Iter *>(self)) -
                  (*static_cast<const Iter *>(other)));
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

    constexpr any_iterator() = default;

    constexpr any_iterator(const any_iterator &other)
      requires((Cat & category::category_mask) >= category::forward ||
               (Cat & category::common) != category::none)
        : iter_vtable_(other.iter_vtable_) {
      if (!other.is_singular()) {
        iter_ = (*(other.iter_vtable_->copy_))(other.iter_);
      }
    }

    constexpr any_iterator(any_iterator &&other)
        : iter_vtable_(other.iter_vtable_) {
      if (!other.is_singular()) {
        iter_ = (*(other.iter_vtable_->move_))(other.iter_);
      }
    }

    constexpr any_iterator &operator=(const any_iterator &other)
      requires((Cat & category::category_mask) >= category::forward ||
               (Cat & category::common) != category::none)
    {
      if (this != &other) {
        if (!is_singular()) {
          (*(iter_vtable_->destructor_))(iter_);
        }
        if (!other.is_singular()) {
          iter_ = (*(other.iter_vtable_->copy_))(other.iter_);
        }
        iter_vtable_ = other.iter_vtable_;
      }
      return *this;
    }

    constexpr any_iterator &operator=(any_iterator &&other) {
      if (this != &other) {
        if (!is_singular()) {
          (*(iter_vtable_->destructor_))(iter_);
        }
        if (!other.is_singular()) {
          iter_ = (*(other.iter_vtable_->move_))(other.iter_);
        }
        iter_vtable_ = other.iter_vtable_;
      }
      return *this;
    }

    constexpr ~any_iterator() {
      if (!is_singular()) {
        (*(iter_vtable_->destructor_))(iter_);
      }
    }

    constexpr Ref operator*() const {
      assert(iter_vtable_);
      return (*(iter_vtable_->deref_))(iter_);
    }

    constexpr any_iterator &operator++() {
      assert(iter_vtable_);
      (*(iter_vtable_->increment_))(iter_);
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
      assert(iter_vtable_);
      (*(iter_vtable_->decrement_))(iter_);
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
      assert(iter_vtable_);
      (*(iter_vtable_->advance_))(iter_, n);
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
      assert(x.iter_vtable_);
      assert(y.iter_vtable_);
      assert(x.iter_vtable_ == y.iter_vtable_);
      return (*(x.iter_vtable_->distance_to_))(x.iter_, y.iter_);
    }

    friend constexpr bool operator==(const any_iterator &x,
                                     const any_iterator &y)
      requires((Cat & category::category_mask) >= category::forward ||
               (Cat & category::common) != category::none)
    {
      if (x.iter_vtable_ != y.iter_vtable_) return false;
      if (!x.iter_vtable_) return true;
      return (*(x.iter_vtable_->equal_))(x.iter_, y.iter_);
    }

    friend constexpr RValueRef iter_move(const any_iterator &iter) {
      assert(iter.iter_vtable_);
      return (*(iter.iter_vtable_->iter_move_))(iter.iter_);
    }

    // private:
    const any_iterator_vtable *iter_vtable_ = nullptr;
    void *iter_ = nullptr;

    template <class Iter>
    constexpr any_iterator(const any_iterator_vtable *table, Iter iter)
        : iter_vtable_(table), iter_(new Iter(std::move(iter))) {}

    constexpr bool is_singular() const { return !iter_vtable_; }
  };

  using iterator = std::conditional_t<(Cat & category::category_mask) ==
                                          category::contiguous,
                                      std::add_pointer_t<Ref>, any_iterator>;

  struct sentinel_vtable : movable_vtable, copyable_vtable, destructor_vtable {
    bool (*equal_)(const iterator &, const any_sentinel &);
  };

  struct any_sentinel_vtable
      : maybe_t<sentinel_vtable, (Cat & category::common) == category::none> {};

  struct sentinel_vtable_gen : basic_vtable_gen {
    template <class Iter, class Sent>
    static constexpr auto generate() {
      any_sentinel_vtable t;

      if constexpr ((Cat & category::common) == category::none) {
        t.move_ = &basic_vtable_gen::template move<Sent>;
        t.copy_ = &basic_vtable_gen::template copy<Sent>;
        t.destructor_ = &basic_vtable_gen::template destroy<Sent>;
        t.equal_ = &equal<Iter, Sent>;
      }

      return t;
    }

    template <class Iter, class Sent>
    static constexpr bool equal(const iterator &iter,
                                const any_sentinel &sent) {
      if (sent.is_singular()) return false;
      if constexpr ((Cat & category::category_mask) == category::contiguous) {
        return iter == *static_cast<const Sent *>(sent.sent_);
      } else {
        if (iter.is_singular()) return false;
        return *static_cast<const Iter *>(iter.iter_) ==
               *static_cast<const Sent *>(sent.sent_);
      }
    }
  };

  struct any_sentinel {
    constexpr any_sentinel() = default;

    constexpr any_sentinel(const any_sentinel &other)
        : sent_vtable_(other.sent_vtable_) {
      if (!other.is_singular()) {
        sent_ = (*(other.sent_vtable_->copy_))(other.sent_);
      }
    }

    constexpr any_sentinel(any_sentinel &&other)
        : sent_vtable_(other.sent_vtable_) {
      if (!other.is_singular()) {
        sent_ = (*(other.sent_vtable_->move_))(other.sent_);
      }
    }

    constexpr any_sentinel &operator=(const any_sentinel &other) {
      if (this != &other) {
        if (!is_singular()) {
          (*(sent_vtable_->destructor_))(sent_);
        }
        if (!other.is_singular()) {
          sent_ = (*(other.sent_vtable_->copy_))(other.sent_);
        }
        sent_vtable_ = other.sent_vtable_;
      }
      return *this;
    }

    constexpr any_sentinel &operator=(any_sentinel &&other) {
      if (this != &other) {
        if (!is_singular()) {
          (*(sent_vtable_->destructor_))(sent_);
        }
        if (!other.is_singular()) {
          sent_ = (*(other.sent_vtable_->move_))(other.sent_);
        }
        sent_vtable_ = other.sent_vtable_;
      }
      return *this;
    }

    constexpr ~any_sentinel() {
      if (!is_singular()) {
        (*(sent_vtable_->destructor_))(sent_);
      }
    }

    friend constexpr bool operator==(const iterator &iter,
                                     const any_sentinel &sent) {
      return (*(sent.sent_vtable_->equal_))(iter, sent);
    }

    // private:
    const any_sentinel_vtable *sent_vtable_ = nullptr;
    void *sent_ = nullptr;

    template <class Sent>
    constexpr any_sentinel(const any_sentinel_vtable *table, Sent sent)
        : sent_vtable_(table), sent_(new Sent(std::move(sent))) {}

    constexpr bool is_singular() const { return !sent_vtable_; }
  };

  using sentinel =
      std::conditional_t<(Cat & category::common) == category::none,
                         any_sentinel, iterator>;

  struct sized_vtable {
    std::size_t (*size_)(const void *);
  };

  struct any_view_vtable
      : destructor_vtable,
        movable_vtable,
        maybe_t<copyable_vtable,
                (Cat & category::move_only_view) == category::none>,
        maybe_t<sized_vtable, (Cat & category::sized) != category::none> {
    iterator (*begin_)(void *);
    sentinel (*end_)(void *);
  };

  struct view_vtable_gen : basic_vtable_gen {
    template <class View>
    static constexpr auto generate() {
      any_view_vtable t;
      t.move_ = &basic_vtable_gen::template move<View>;
      t.destructor_ = &basic_vtable_gen::template destroy<View>;

      t.begin_ = &begin<View>;
      t.end_ = &end<View>;

      if constexpr ((Cat & category::move_only_view) == category::none) {
        t.copy_ = &basic_vtable_gen::template copy<View>;
      }

      if constexpr ((Cat & category::sized) != category::none) {
        t.size_ = &size<View>;
      }

      return t;
    }

    template <class View>
    static constexpr iterator begin(void *v) {
      auto &view = *(static_cast<View *>(v));
      if constexpr ((Cat & category::category_mask) == category::contiguous) {
        return std::ranges::begin(view);
      } else {
        return any_iterator(&iter_vtable<std::ranges::iterator_t<View>>,
                            std::ranges::begin(view));
      }
    }

    template <class View>
    static constexpr sentinel end(void *v) {
      auto &view = *(static_cast<View *>(v));
      if constexpr ((Cat & category::category_mask) == category::contiguous &&
                    (Cat & category::common) != category::none) {
        return std::ranges::end(view);
      } else if constexpr ((Cat & category::common) != category::none) {
        return any_iterator(&iter_vtable<std::ranges::iterator_t<View>>,
                            std::ranges::end(view));
      } else {
        return any_sentinel(&sent_vtable<std::ranges::iterator_t<View>,
                                         std::ranges::sentinel_t<View>>,
                            std::ranges::end(view));
      }
    }

    template <class View>
    static constexpr std::size_t size(const void *view) {
      return std::ranges::size(*(static_cast<const View *>(view)));
    }
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
      : view_vtable_(&view_vtable<View>), view_(new View(std::move(view))) {}

  constexpr any_view(const any_view &other)
    requires((Cat & category::move_only_view) == category::none)
      : view_vtable_(other.view_vtable_),
        view_((*(view_vtable_->copy_))(other.view_)) {}

  constexpr any_view(any_view &&other)
      : view_vtable_(other.view_vtable_),
        view_((*(view_vtable_->move_))(other.view_)) {}

  constexpr any_view &operator=(const any_view &other)
    requires((Cat & category::move_only_view) == category::none)
  {
    if (this != &other) {
      (*(view_vtable_->destructor_))(view_);
      view_ = (*(other.view_vtable_->copy_))(other.view_);
      view_vtable_ = other.view_vtable_;
    }
    return *this;
  }

  constexpr any_view &operator=(any_view &&other) {
    if (this != &other) {
      (*(view_vtable_->destructor_))(view_);
      view_ = (*(other.view_vtable_->move_))(other.view_);
      view_vtable_ = other.view_vtable_;
    }
    return *this;
  }

  constexpr iterator begin() { return (*(view_vtable_->begin_))(view_); }
  constexpr sentinel end() { return (*(view_vtable_->end_))(view_); }

  constexpr std::size_t size() const
    requires((Cat & category::sized) != category::none)
  {
    return (*(view_vtable_->size_))(view_);
  }

 private:
  template <class Iter>
  static constexpr any_iterator_vtable iter_vtable =
      iterator_vtable_gen::template generate<Iter>();

  template <class Iter, class Sent>
  static constexpr any_sentinel_vtable sent_vtable =
      sentinel_vtable_gen::template generate<Iter, Sent>();

  template <class View>
  static constexpr any_view_vtable view_vtable =
      view_vtable_gen::template generate<View>();

  const any_view_vtable *view_vtable_;
  void *view_;
};

template <class Ref, category Cat, class Value, class RValueRef, class Diff>
inline constexpr bool
    enable_borrowed_range<any_view<Ref, Cat, Value, RValueRef, Diff>> =
        (Cat & category::borrowed) != category::none;

}  // namespace std::ranges

#endif
