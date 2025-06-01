#ifndef LIBCPP__RANGE_ANY_VIEW_TE_HPP
#define LIBCPP__RANGE_ANY_VIEW_TE_HPP

#include <cassert>
#include <iterator>
#include <memory>
#include <ranges>
#include <type_traits>

#include "storage.hpp"

namespace std::ranges {

inline namespace __any_view {

enum class any_view_options {
  none = 0,
  input = 1,
  forward = 3,
  bidirectional = 7,
  random_access = 15,
  contiguous = 31,
  category_mask = contiguous,
  sized = 32,
  borrowed = 64,
  copyable = 128
};

constexpr any_view_options operator&(any_view_options lhs,
                                     any_view_options rhs) noexcept {
  return static_cast<any_view_options>(
      static_cast<std::underlying_type_t<any_view_options>>(lhs) &
      static_cast<std::underlying_type_t<any_view_options>>(rhs));
}

constexpr any_view_options operator|(any_view_options lhs,
                                     any_view_options rhs) noexcept {
  return static_cast<any_view_options>(
      static_cast<std::underlying_type_t<any_view_options>>(lhs) |
      static_cast<std::underlying_type_t<any_view_options>>(rhs));
}

constexpr auto operator<=>(any_view_options lhs,
                           any_view_options rhs) noexcept {
  return static_cast<std::underlying_type_t<any_view_options>>(lhs) <=>
         static_cast<std::underlying_type_t<any_view_options>>(rhs);
}

constexpr bool __flag_is_set(any_view_options opts,
                             any_view_options flag) noexcept {
  return (opts & flag) != any_view_options::none;
}

}  // namespace __any_view

template <class T>
struct __rvalue_ref {
  using type = T;
};

template <class T>
struct __rvalue_ref<T &> {
  using type = T &&;
};

template <class T>
using __rvalue_ref_t = typename __rvalue_ref<T>::type;

template <class Element, any_view_options Opts = any_view_options::input,
          class Ref = Element &, class RValueRef = __rvalue_ref_t<Ref>,
          class Diff = ptrdiff_t>
class any_view {
 public:
  struct any_iterator;
  struct any_sentinel;

  static constexpr any_view_options Traversal =
      Opts & any_view_options::category_mask;
  static constexpr bool is_view_copyable =
      (Opts & any_view_options::copyable) != any_view_options::none;
  static constexpr bool is_iterator_copyable =
      Traversal >= any_view_options::forward;

  template <class T, bool HasT>
  struct maybe_t : T {};

  template <class T>
  struct maybe_t<T, false> {};

  using iterator_storage =
      detail::storage<3 * sizeof(void *), sizeof(void *), is_iterator_copyable>;

  struct input_iterator_vtable {
    Ref (*deref_)(const iterator_storage &);
    void (*increment_)(iterator_storage &);
    RValueRef (*iter_move_)(const iterator_storage &);
  };

  struct equality_vtable {
    bool (*equal_)(const iterator_storage &, const iterator_storage &);
  };

  struct forward_iterator_vtable : input_iterator_vtable, equality_vtable {};

  struct bidirectional_iterator_vtable : forward_iterator_vtable {
    void (*decrement_)(iterator_storage &);
  };

  struct random_access_iterator_vtable : bidirectional_iterator_vtable {
    void (*advance_)(iterator_storage &, Diff);
    Diff (*distance_to_)(const iterator_storage &, const iterator_storage &);
  };

  struct contiguous_iterator_vtable : random_access_iterator_vtable {
    std::add_pointer_t<Ref> (*arrow_)(const iterator_storage &);
  };

  using any_iterator_vtable = conditional_t<
      Traversal == any_view_options::contiguous, contiguous_iterator_vtable,
      conditional_t<
          Traversal == any_view_options::random_access,
          random_access_iterator_vtable,
          conditional_t<
              Traversal == any_view_options::bidirectional,
              bidirectional_iterator_vtable,
              conditional_t<Traversal == any_view_options::forward,
                            forward_iterator_vtable, input_iterator_vtable>>>>;

  struct iterator_vtable_gen {
    template <class Iter>
    static constexpr auto generate() {
      any_iterator_vtable t;

      t.deref_ = &deref<Iter>;
      t.increment_ = &increment<Iter>;
      t.iter_move_ = &iter_move<Iter>;

      if constexpr (Traversal >= any_view_options::forward) {
        t.equal_ = &equal<Iter>;
      }

      if constexpr (Traversal >= any_view_options::bidirectional) {
        t.decrement_ = &decrement<Iter>;
      }

      if constexpr (Traversal >= any_view_options::random_access) {
        t.advance_ = &advance<Iter>;
        t.distance_to_ = &distance_to<Iter>;
      }

      if constexpr (Traversal == any_view_options::contiguous) {
        t.arrow_ = &arrow<Iter>;
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

    // contiguous
    template <class Iter>
    static constexpr add_pointer_t<Ref> arrow(const iterator_storage &self) {
      return std::to_address(*(self.template get_ptr<Iter>()));
    };
  };

  struct empty_iterator_category {};
  struct with_iterator_category {
   private:
    constexpr static auto get_category() {
      if constexpr (!std::is_reference_v<Ref>) {
        return std::input_iterator_tag{};
      } else if constexpr (Traversal >= any_view_options::random_access) {
        return std::random_access_iterator_tag{};
      } else if constexpr (Traversal == any_view_options::bidirectional) {
        return std::bidirectional_iterator_tag{};
      } else if constexpr (Traversal == any_view_options::forward) {
        return std::forward_iterator_tag{};
      } else {
        return std::input_iterator_tag{};
      }
    }

   public:
    using iterator_category = decltype(get_category());
  };
  constexpr static auto get_concept() {
    if constexpr (Traversal == any_view_options::contiguous) {
      return std::contiguous_iterator_tag{};
    } else if constexpr (Traversal == any_view_options::random_access) {
      return std::random_access_iterator_tag{};
    } else if constexpr (Traversal == any_view_options::bidirectional) {
      return std::bidirectional_iterator_tag{};
    } else if constexpr (Traversal == any_view_options::forward) {
      return std::forward_iterator_tag{};
    } else {
      return std::input_iterator_tag{};
    }
  }
  struct any_iterator
      : std::conditional_t<Traversal >= any_view_options::forward,
                           with_iterator_category, empty_iterator_category> {
    using iterator_concept = decltype(get_concept());
    using value_type = remove_cv_t<Element>;
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
      requires(Traversal >= any_view_options::forward)
    {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }

    constexpr any_iterator &operator--()
      requires(Traversal >= any_view_options::bidirectional)
    {
      assert(!is_singular());
      (*(iter_vtable_->decrement_))(iter_);
      return *this;
    }

    constexpr any_iterator operator--(int)
      requires(Traversal >= any_view_options::bidirectional)
    {
      auto tmp = *this;
      --(*this);
      return tmp;
    }

    constexpr any_iterator &operator+=(difference_type n)
      requires(Traversal >= any_view_options::random_access)
    {
      assert(!is_singular());
      (*(iter_vtable_->advance_))(iter_, n);
      return *this;
    }

    constexpr any_iterator &operator-=(difference_type n)
      requires(Traversal >= any_view_options::random_access)
    {
      *this += -n;
      return *this;
    }

    constexpr Ref operator[](difference_type n) const
      requires(Traversal >= any_view_options::random_access)
    {
      return *((*this) + n);
    }

    constexpr std::add_pointer_t<Ref> operator->() const
      requires(Traversal == any_view_options::contiguous)
    {
      assert(!is_singular());
      return (*(iter_vtable_->arrow_))(iter_);
    }

    friend constexpr bool operator<(const any_iterator &x,
                                    const any_iterator &y)
      requires(Traversal >= any_view_options::random_access)
    {
      return (x - y) < 0;
    }

    friend constexpr bool operator>(const any_iterator &x,
                                    const any_iterator &y)
      requires(Traversal >= any_view_options::random_access)
    {
      return (x - y) > 0;
    }

    friend constexpr bool operator<=(const any_iterator &x,
                                     const any_iterator &y)
      requires(Traversal >= any_view_options::random_access)
    {
      return (x - y) <= 0;
    }

    friend constexpr bool operator>=(const any_iterator &x,
                                     const any_iterator &y)
      requires(Traversal >= any_view_options::random_access)
    {
      return (x - y) >= 0;
    }

    friend constexpr any_iterator operator+(const any_iterator &it,
                                            difference_type n)
      requires(Traversal >= any_view_options::random_access)
    {
      auto temp = it;
      temp += n;
      return temp;
    }

    friend constexpr any_iterator operator+(difference_type n,
                                            const any_iterator &it)
      requires(Traversal >= any_view_options::random_access)
    {
      return it + n;
    }

    friend constexpr any_iterator operator-(const any_iterator &it,
                                            difference_type n)
      requires(Traversal >= any_view_options::random_access)
    {
      auto temp = it;
      temp -= n;
      return temp;
    }

    friend constexpr difference_type operator-(const any_iterator &x,
                                               const any_iterator &y)
      requires(Traversal >= any_view_options::random_access)
    {
      assert(!x.is_singular());
      assert(!y.is_singular());
      assert(x.iter_vtable_ == y.iter_vtable_);
      return (*(x.iter_vtable_->distance_to_))(x.iter_, y.iter_);
    }

    friend constexpr bool operator==(const any_iterator &x,
                                     const any_iterator &y)
      requires(Traversal >= any_view_options::forward)
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

  using iterator = any_iterator;

  using sentinel_storage =
      detail::storage<3 * sizeof(void *), sizeof(void *), true>;
  struct any_sentinel_vtable {
    bool (*equal_)(const iterator &, const any_sentinel &);
  };

  struct sentinel_vtable_gen {
    template <class Iter, class Sent>
    static constexpr auto generate() {
      any_sentinel_vtable t;
      t.equal_ = &equal<Iter, Sent>;
      return t;
    }

    template <class Iter, class Sent>
    static constexpr bool equal(const iterator &iter,
                                const any_sentinel &sent) {
      if (sent.is_singular() || iter.is_singular()) return false;
      return *(iter.iter_.template get_ptr<Iter>()) ==
             *(sent.sent_.template get_ptr<Sent>());
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

  using sentinel = any_sentinel;

  using view_storage =
      detail::storage<4 * sizeof(void *), sizeof(void *), is_view_copyable>;
  struct sized_vtable {
    std::size_t (*size_)(const view_storage &);
  };
  struct any_view_vtable
      : maybe_t<sized_vtable,
                (Opts & any_view_options::sized) != any_view_options::none> {
    iterator (*begin_)(view_storage &);
    sentinel (*end_)(view_storage &);
  };

  struct view_vtable_gen {
    template <class View>
    static constexpr auto generate() {
      any_view_vtable t;
      t.begin_ = &begin<View>;
      t.end_ = &end<View>;
      if constexpr ((Opts & any_view_options::sized) !=
                    any_view_options::none) {
        t.size_ = &size<View>;
      }

      return t;
    }

    template <class View>
    static constexpr iterator begin(view_storage &v) {
      auto &view = *(v.template get_ptr<View>());
      return any_iterator(&iter_vtable<std::ranges::iterator_t<View>>,
                          std::ranges::begin(view));
    }

    template <class View>
    static constexpr sentinel end(view_storage &v) {
      auto &view = *(v.template get_ptr<View>());
      return any_sentinel(&sent_vtable<std::ranges::iterator_t<View>,
                                       std::ranges::sentinel_t<View>>,
                          std::ranges::end(view));
    }

    template <class View>
    static constexpr std::size_t size(const view_storage &v) {
      return std::ranges::size(*(v.template get_ptr<View>()));
    }
  };

  template <class View>
  consteval static bool view_options_constraint() {
    if constexpr (__flag_is_set(Opts, any_view_options::sized) &&
                  !std::ranges::sized_range<View>) {
      return false;
    }

    if constexpr (__flag_is_set(Opts, any_view_options::borrowed) &&
                  !std::ranges::borrowed_range<View>) {
      return false;
    }

    if constexpr (is_view_copyable && !std::copyable<View>) {
      return false;
    }
    constexpr auto cat_mask = Opts & any_view_options::category_mask;
    if constexpr (cat_mask == any_view_options::contiguous) {
      return std::ranges::contiguous_range<View>;
    } else if constexpr (cat_mask == any_view_options::random_access) {
      return std::ranges::random_access_range<View>;
    } else if constexpr (cat_mask == any_view_options::bidirectional) {
      return std::ranges::bidirectional_range<View>;
    } else if constexpr (cat_mask == any_view_options::forward) {
      return std::ranges::forward_range<View>;
    } else {
      return std::ranges::input_range<View>;
    }
  }

  template <class Range>
    requires(!std::same_as<remove_cvref_t<Range>, any_view> &&
             std::ranges::viewable_range<Range> &&
             view_options_constraint<views::all_t<Range>>())
  constexpr any_view(Range &&range)
      : view_vtable_(&view_vtable<views::all_t<Range>>),
        view_(detail::type<views::all_t<Range>>{},
              views::all(std::forward<Range>(range))) {}

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
    requires((Opts & any_view_options::sized) != any_view_options::none)
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

template <class Value, any_view_options Opts, class Ref, class RValueRef,
          class Diff>
inline constexpr bool
    enable_borrowed_range<any_view<Value, Opts, Ref, RValueRef, Diff>> =
        (Opts & any_view_options::borrowed) != any_view_options::none;

}  // namespace std::ranges

#endif
