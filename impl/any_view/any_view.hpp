#ifndef LIBCPP__RANGE_ANY_VIEW_TE_HPP
#define LIBCPP__RANGE_ANY_VIEW_TE_HPP

#include <cassert>
#include <memory>
#include <ranges>
#include <type_traits>

#include "storage.hpp"

namespace std::ranges {

inline namespace __any_view {

enum class any_view_category {
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

constexpr any_view_category operator&(any_view_category lhs,
                                      any_view_category rhs) noexcept {
  return static_cast<any_view_category>(
      static_cast<std::underlying_type_t<any_view_category>>(lhs) &
      static_cast<std::underlying_type_t<any_view_category>>(rhs));
}

constexpr any_view_category operator|(any_view_category lhs,
                                      any_view_category rhs) noexcept {
  return static_cast<any_view_category>(
      static_cast<std::underlying_type_t<any_view_category>>(lhs) |
      static_cast<std::underlying_type_t<any_view_category>>(rhs));
}

constexpr auto operator<=>(any_view_category lhs,
                           any_view_category rhs) noexcept {
  return static_cast<std::underlying_type_t<any_view_category>>(lhs) <=>
         static_cast<std::underlying_type_t<any_view_category>>(rhs);
}

}  // namespace __any_view

template <class Value, any_view_category Cat = any_view_category::input,
          class Ref = Value &,
          class RValueRef = add_rvalue_reference_t<remove_reference_t<Ref>>,
          class Diff = ptrdiff_t>
class any_view {
 public:
  struct any_iterator;
  struct any_sentinel;

  static constexpr any_view_category Traversal =
      Cat & any_view_category::category_mask;
  static constexpr bool is_common =
      (Cat & any_view_category::common) == any_view_category::common;
  static constexpr bool is_view_copyable =
      (Cat & any_view_category::move_only_view) == any_view_category::none;
  static constexpr bool is_iterator_copyable =
      Traversal >= any_view_category::forward || is_common;

  template <class T, bool HasT>
  struct maybe_t : T {};

  template <class T>
  struct maybe_t<T, false> {};

  using iterator_storage =
      detail::storage<3 * sizeof(void *), sizeof(void *), is_iterator_copyable>;

  struct basic_input_iterator_vtable {
    Ref (*deref_)(const iterator_storage &);
    void (*increment_)(iterator_storage &);
    RValueRef (*iter_move_)(const iterator_storage &);
  };

  struct equality_vtable {
    bool (*equal_)(const iterator_storage &, const iterator_storage &);
  };

  struct common_input_iterator : basic_input_iterator_vtable,
                                 equality_vtable {};

  struct forward_iterator_vtable : basic_input_iterator_vtable,
                                   equality_vtable {};

  struct bidirectional_iterator_vtable : forward_iterator_vtable {
    void (*decrement_)(iterator_storage &);
  };

  struct random_access_iterator_vtable : bidirectional_iterator_vtable {
    void (*advance_)(iterator_storage &, Diff);
    Diff (*distance_to_)(const iterator_storage &, const iterator_storage &);
  };

  // for contiguous , we just return raw pointers
  struct contiguous_iterator_vtable {};

  using any_iterator_vtable = conditional_t<
      Traversal == any_view_category::contiguous, contiguous_iterator_vtable,
      conditional_t<
          Traversal == any_view_category::random_access,
          random_access_iterator_vtable,
          conditional_t<
              Traversal == any_view_category::bidirectional,
              bidirectional_iterator_vtable,
              conditional_t<Traversal == any_view_category::forward,
                            forward_iterator_vtable,
                            conditional_t<is_common, common_input_iterator,
                                          basic_input_iterator_vtable>>>>>;

  struct iterator_vtable_gen {
    template <class Iter>
    static constexpr auto generate() {
      any_iterator_vtable t;

      if constexpr (Traversal != any_view_category::contiguous) {
        t.deref_ = &deref<Iter>;
        t.increment_ = &increment<Iter>;
        t.iter_move_ = &iter_move<Iter>;

        if constexpr (Traversal >= any_view_category::forward || is_common) {
          t.equal_ = &equal<Iter>;
        }

        if constexpr (Traversal >= any_view_category::bidirectional) {
          t.decrement_ = &decrement<Iter>;
        }

        if constexpr (Traversal >= any_view_category::random_access) {
          t.advance_ = &advance<Iter>;
          t.distance_to_ = &distance_to<Iter>;
        }
      }
      return t;
    }

    // input

    template <class Iter>
    static constexpr Ref deref(const iterator_storage &self) {
      return **(self.template get_ptr<Iter>());
    };

    template <class Iter>
    static constexpr void increment(iterator_storage &self) {
      ++(*(self.template get_ptr<Iter>()));
    };

    template <class Iter>
    static constexpr RValueRef iter_move(const iterator_storage &self) {
      return std::ranges::iter_move(*(self.template get_ptr<Iter>()));
    };

    // forward

    template <class Iter>
    static constexpr bool equal(const iterator_storage &lhs,
                                const iterator_storage &rhs) {
      return *lhs.template get_ptr<Iter>() == *rhs.template get_ptr<Iter>();
    }

    // bidi

    template <class Iter>
    static constexpr void decrement(iterator_storage &self) {
      --(*(self.template get_ptr<Iter>()));
    }

    // random access

    template <class Iter>
    static constexpr void advance(iterator_storage &self, Diff diff) {
      (*self.template get_ptr<Iter>()) += diff;
    }

    template <class Iter>
    static constexpr Diff distance_to(const iterator_storage &self,
                                      const iterator_storage &other) {
      return Diff((*self.template get_ptr<Iter>()) -
                  (*other.template get_ptr<Iter>()));
    }
  };

  struct empty_iterator_category {};
  struct with_iterator_category {
   private:
    constexpr static auto get_category() {
      if constexpr (!std::is_reference_v<Ref>) {
        return std::input_iterator_tag{};
      } else if constexpr (Traversal >= any_view_category::random_access) {
        return std::random_access_iterator_tag{};
      } else if constexpr (Traversal == any_view_category::bidirectional) {
        return std::bidirectional_iterator_tag{};
      } else if constexpr (Traversal == any_view_category::forward) {
        return std::forward_iterator_tag{};
      } else {
        return std::input_iterator_tag{};
      }
    }

   public:
    using iterator_category = decltype(get_category());
  };
  constexpr static auto get_concept() {
    if constexpr (Traversal >= any_view_category::random_access) {
      return std::random_access_iterator_tag{};
    } else if constexpr (Traversal == any_view_category::bidirectional) {
      return std::bidirectional_iterator_tag{};
    } else if constexpr (Traversal == any_view_category::forward) {
      return std::forward_iterator_tag{};
    } else {
      return std::input_iterator_tag{};
    }
  }
  struct any_iterator
      : std::conditional_t<Traversal >= any_view_category::forward,
                           with_iterator_category, empty_iterator_category> {
    using iterator_concept = decltype(get_concept());
    using value_type = Value;
    using difference_type = Diff;

    constexpr any_iterator() = default;

    constexpr any_iterator(const any_iterator &)
      requires is_iterator_copyable
    = default;

    constexpr any_iterator(any_iterator &&) = default;

    constexpr any_iterator &operator=(const any_iterator &)
      requires is_iterator_copyable
    = default;

    constexpr any_iterator &operator=(any_iterator &&) = default;

    constexpr ~any_iterator() = default;

    constexpr Ref operator*() const {
      assert(!is_singular());
      return (*(iter_vtable_->deref_))(iter_);
    }

    constexpr any_iterator &operator++() {
      assert(!is_singular());
      (*(iter_vtable_->increment_))(iter_);
      return *this;
    }

    constexpr void operator++(int) { ++(*this); }

    constexpr any_iterator operator++(int)
      requires(Traversal >= any_view_category::forward)
    {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }

    constexpr any_iterator &operator--()
      requires(Traversal >= any_view_category::bidirectional)
    {
      assert(!is_singular());
      (*(iter_vtable_->decrement_))(iter_);
      return *this;
    }

    constexpr any_iterator operator--(int)
      requires(Traversal >= any_view_category::bidirectional)
    {
      auto tmp = *this;
      --(*this);
      return tmp;
    }

    constexpr any_iterator &operator+=(difference_type n)
      requires(Traversal >= any_view_category::random_access)
    {
      assert(!is_singular());
      (*(iter_vtable_->advance_))(iter_, n);
      return *this;
    }

    constexpr any_iterator &operator-=(difference_type n)
      requires(Traversal >= any_view_category::random_access)
    {
      *this += -n;
      return *this;
    }

    constexpr Ref operator[](difference_type n) const
      requires(Traversal >= any_view_category::random_access)
    {
      return *((*this) + n);
    }

    friend constexpr bool operator<(const any_iterator &x,
                                    const any_iterator &y)
      requires(Traversal >= any_view_category::random_access)
    {
      return (x - y) < 0;
    }

    friend constexpr bool operator>(const any_iterator &x,
                                    const any_iterator &y)
      requires(Traversal >= any_view_category::random_access)
    {
      return (x - y) > 0;
    }

    friend constexpr bool operator<=(const any_iterator &x,
                                     const any_iterator &y)
      requires(Traversal >= any_view_category::random_access)
    {
      return (x - y) <= 0;
    }

    friend constexpr bool operator>=(const any_iterator &x,
                                     const any_iterator &y)
      requires(Traversal >= any_view_category::random_access)
    {
      return (x - y) >= 0;
    }

    friend constexpr any_iterator operator+(const any_iterator &it,
                                            difference_type n)
      requires(Traversal >= any_view_category::random_access)
    {
      auto temp = it;
      temp += n;
      return temp;
    }

    friend constexpr any_iterator operator+(difference_type n,
                                            const any_iterator &it)
      requires(Traversal >= any_view_category::random_access)
    {
      return it + n;
    }

    friend constexpr any_iterator operator-(const any_iterator &it,
                                            difference_type n)
      requires(Traversal >= any_view_category::random_access)
    {
      auto temp = it;
      temp -= n;
      return temp;
    }

    friend constexpr difference_type operator-(const any_iterator &x,
                                               const any_iterator &y)
      requires(Traversal >= any_view_category::random_access)
    {
      assert(!x.is_singular());
      assert(!y.is_singular());
      assert(x.iter_vtable_ == y.iter_vtable_);
      return (*(x.iter_vtable_->distance_to_))(x.iter_, y.iter_);
    }

    friend constexpr bool operator==(const any_iterator &x,
                                     const any_iterator &y)
      requires(Traversal >= any_view_category::forward || is_common)
    {
      if (x.iter_vtable_ != y.iter_vtable_) return false;
      if (!x.iter_vtable_) return true;
      return (*(x.iter_vtable_->equal_))(x.iter_, y.iter_);
    }

    friend constexpr RValueRef iter_move(const any_iterator &iter) {
      assert(!iter.is_singular());
      return (*(iter.iter_vtable_->iter_move_))(iter.iter_);
    }

    // private:
    const any_iterator_vtable *iter_vtable_ = nullptr;
    iterator_storage iter_;

    template <class Iter>
    constexpr any_iterator(const any_iterator_vtable *table, Iter iter)
        : iter_vtable_(table), iter_(detail::type<Iter>{}, std::move(iter)) {}

    constexpr bool is_singular() const { return iter_.is_singular(); }
  };

  using iterator =
      std::conditional_t<Traversal == any_view_category::contiguous,
                         std::add_pointer_t<Ref>, any_iterator>;

  using sentinel_storage =
      detail::storage<3 * sizeof(void *), sizeof(void *), true>;
  struct sentinel_vtable {
    bool (*equal_)(const iterator &, const any_sentinel &);
  };

  struct any_sentinel_vtable : maybe_t<sentinel_vtable, !is_common> {};

  struct sentinel_vtable_gen {
    template <class Iter, class Sent>
    static constexpr auto generate() {
      any_sentinel_vtable t;

      if constexpr (!is_common) {
        t.equal_ = &equal<Iter, Sent>;
      }

      return t;
    }

    template <class Iter, class Sent>
    static constexpr bool equal(const iterator &iter,
                                const any_sentinel &sent) {
      if (sent.is_singular()) return false;
      if constexpr (Traversal == any_view_category::contiguous) {
        return iter == *sent.sent_.template get_ptr<Sent>();
      } else {
        if (iter.is_singular()) return false;
        return *(iter.iter_.template get_ptr<Iter>()) ==
               *(sent.sent_.template get_ptr<Sent>());
      }
    }
  };

  struct any_sentinel {
    constexpr any_sentinel() = default;

    constexpr any_sentinel(const any_sentinel &) = default;

    constexpr any_sentinel(any_sentinel &&) = default;

    constexpr any_sentinel &operator=(const any_sentinel &) = default;

    constexpr any_sentinel &operator=(any_sentinel &&) = default;

    constexpr ~any_sentinel() = default;

    friend constexpr bool operator==(const iterator &iter,
                                     const any_sentinel &sent) {
      return (*(sent.sent_vtable_->equal_))(iter, sent);
    }

    // private:
    const any_sentinel_vtable *sent_vtable_ = nullptr;
    sentinel_storage sent_;

    template <class Sent>
    constexpr any_sentinel(const any_sentinel_vtable *table, Sent sent)
        : sent_vtable_(table), sent_(detail::type<Sent>{}, std::move(sent)) {}

    constexpr bool is_singular() const { return sent_.is_singular(); }
  };

  using sentinel = std::conditional_t<!is_common, any_sentinel, iterator>;

  using view_storage =
      detail::storage<3 * sizeof(void *), sizeof(void *), is_view_copyable>;
  struct sized_vtable {
    std::size_t (*size_)(const view_storage &);
  };
  struct any_view_vtable
      : maybe_t<sized_vtable,
                (Cat & any_view_category::sized) != any_view_category::none> {
    iterator (*begin_)(view_storage &);
    sentinel (*end_)(view_storage &);
  };

  struct view_vtable_gen {
    template <class View>
    static constexpr auto generate() {
      any_view_vtable t;
      t.begin_ = &begin<View>;
      t.end_ = &end<View>;
      if constexpr ((Cat & any_view_category::sized) !=
                    any_view_category::none) {
        t.size_ = &size<View>;
      }

      return t;
    }

    template <class View>
    static constexpr iterator begin(view_storage &v) {
      auto &view = *(v.template get_ptr<View>());
      if constexpr (Traversal == any_view_category::contiguous) {
        return std::ranges::begin(view);
      } else {
        return any_iterator(&iter_vtable<std::ranges::iterator_t<View>>,
                            std::ranges::begin(view));
      }
    }

    template <class View>
    static constexpr sentinel end(view_storage &v) {
      auto &view = *(v.template get_ptr<View>());
      if constexpr (Traversal == any_view_category::contiguous && is_common) {
        return std::ranges::end(view);
      } else if constexpr (is_common) {
        return any_iterator(&iter_vtable<std::ranges::iterator_t<View>>,
                            std::ranges::end(view));
      } else {
        return any_sentinel(&sent_vtable<std::ranges::iterator_t<View>,
                                         std::ranges::sentinel_t<View>>,
                            std::ranges::end(view));
      }
    }

    template <class View>
    static constexpr std::size_t size(const view_storage &v) {
      return std::ranges::size(*(v.template get_ptr<View>()));
    }
  };

  template <class View>
  consteval static bool view_category_constraint() {
    if constexpr ((Cat & any_view_category::sized) != any_view_category::none &&
                  !std::ranges::sized_range<View>) {
      return false;
    }

    if constexpr (is_common && !std::ranges::common_range<View>) {
      return false;
    }

    if constexpr ((Cat & any_view_category::borrowed) !=
                      any_view_category::none &&
                  !std::ranges::borrowed_range<View>) {
      return false;
    }

    if constexpr (is_view_copyable && !std::copyable<View>) {
      return false;
    }
    constexpr auto cat_mask = Cat & any_view_category::category_mask;
    if constexpr (cat_mask == any_view_category::contiguous) {
      return std::ranges::contiguous_range<View>;
    } else if constexpr (cat_mask == any_view_category::random_access) {
      return std::ranges::random_access_range<View>;
    } else if constexpr (cat_mask == any_view_category::bidirectional) {
      return std::ranges::bidirectional_range<View>;
    } else if constexpr (cat_mask == any_view_category::forward) {
      return std::ranges::forward_range<View>;
    } else {
      return std::ranges::input_range<View>;
    }
  }

  template <class View>
    requires(!std::same_as<View, any_view> && std::ranges::view<View> &&
             view_category_constraint<View>())
  constexpr any_view(View view)
      : view_vtable_(&view_vtable<View>),
        view_(detail::type<View>{}, std::move(view)) {}

  constexpr any_view(const any_view &)
    requires is_view_copyable
  = default;

  constexpr any_view(any_view &&) = default;

  constexpr any_view &operator=(const any_view &)
    requires is_view_copyable
  = default;

  constexpr any_view &operator=(any_view &&) = default;

  constexpr ~any_view() = default;

  constexpr iterator begin() { return (*(view_vtable_->begin_))(view_); }
  constexpr sentinel end() { return (*(view_vtable_->end_))(view_); }

  constexpr std::size_t size() const
    requires((Cat & any_view_category::sized) != any_view_category::none)
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
  view_storage view_;
};

template <class Ref, any_view_category Cat, class Value, class RValueRef,
          class Diff>
inline constexpr bool
    enable_borrowed_range<any_view<Ref, Cat, Value, RValueRef, Diff>> =
        (Cat & any_view_category::borrowed) != any_view_category::none;

}  // namespace std::ranges

#endif
