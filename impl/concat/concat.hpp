#ifndef LIBCPP__RANGE_CONCAT_HPP
#define LIBCPP__RANGE_CONCAT_HPP

#include <array>
#include <cassert>
#include <concepts>
#include <functional>
#include <numeric>
#include <ranges>
#include <tuple>
#include <variant>

#include "utils.hpp"

namespace std::ranges {

namespace xo {  // exposition only things (and persevering face)

template <bool Const, class... Views>
concept all_random_access = (random_access_range<__maybe_const<Const, Views>> &&
                             ...);
template <bool Const, class... Views>
concept all_bidirectional = (bidirectional_range<__maybe_const<Const, Views>> &&
                             ...);
template <bool Const, class... Views>
concept all_forward = (forward_range<__maybe_const<Const, Views>> && ...);

inline namespace not_to_spec {

template <class... Rs>
using concat_reference_t = common_reference_t<range_reference_t<Rs>...>;

template <class... Rs>
using concat_rvalue_reference_t =
    common_reference_t<range_rvalue_reference_t<Rs>...>;

template <class... Rs>
using concat_value_t = common_type_t<range_value_t<Rs>...>;

// modelled only if concat reference_t is a prvalue T and at least one of the 
// argument ranges' reference_t is a T reference
template <class... Rs>
concept concat_require_expert =
    is_reference_v<concat_reference_t<Rs...>> ||
    (is_reference_v<range_reference_t<Rs>>&&...) &&
    (!same_as<std::remove_reference_t<range_reference_t<Rs>>,
              concat_reference_t<Rs...>> &&
     ...);

// clang-format off
template <class Ref, class RRef, class It>
concept concat_indirectly_readable_impl = requires (const It it){
    { *it } -> convertible_to<Ref>;
    { ranges::iter_move(it) } -> convertible_to<RRef>;
};

template <class... Rs>
concept concat_indirectly_readable =
    common_reference_with<concat_reference_t<Rs...> &&, concat_value_t<Rs...>&>&&
    common_reference_with<concat_reference_t<Rs...>&&, concat_rvalue_reference_t<Rs...>&&>&&
    common_reference_with<concat_rvalue_reference_t<Rs...>&&, concat_value_t<Rs...> const&> &&
    (concat_indirectly_readable_impl<concat_reference_t<Rs...>, concat_rvalue_reference_t<Rs...>, iterator_t<Rs>> && ...);
// clang-format on

}  // namespace not_to_spec

// clang-format off
template <bool expert_mode, class... Rs>
concept concatable = requires {
    typename concat_reference_t<Rs...>;
    typename concat_value_t<Rs...>;
    typename concat_rvalue_reference_t<Rs...>;
} && concat_indirectly_readable<Rs...>
  && (expert_mode || concat_require_expert<Rs...>)
;
// clang-format on

static_assert(true);  // clang-format badness

inline namespace not_to_spec {

template <class... T>
using back = tuple_element_t<sizeof...(T) - 1, tuple<T...>>;

template <bool... b, size_t... I>
consteval bool all_but_last_impl(std::index_sequence<I...>) {
  return ((I == sizeof...(I) - 1 || b) && ...);
}

template <bool... b>
constexpr bool all_but_last =
    all_but_last_impl<b...>(make_index_sequence<sizeof...(b)>{});

}  // namespace not_to_spec

template <bool Const, class... Views>
concept concat_is_random_access =
    (all_random_access<Const, Views> && ...) &&
    (all_but_last<sized_range<__maybe_const<Const, Views>>...>);

template <class R>
concept constant_time_reversible = (bidirectional_range<R> &&
                                    common_range<R>) ||
                                   (sized_range<R> && random_access_range<R>);

template <class... Rs>
concept concat_bidirectional = all_but_last<constant_time_reversible<Rs>...> &&
    bidirectional_range<back<Rs...>>;

static_assert(true);  // clang-format badness

inline namespace not_to_spec {

// it is not defined in the standard and we can't refer to it.
template <typename T>
concept has_member_arrow = requires(T it) {
  it.operator->();
};

// iterator_traits<It>::pointer when present gives the result of arrow, or
// presumably the result of arrow is convertible to that.

// when iterator_traits<It>::pointer is not present for not-a-C++17-iterators,
// we should get the pointer type from the arrow expression:
template <__has_arrow It>
decltype(auto) get_arrow_result(It&& it) {
  if constexpr (has_member_arrow<It>) {
    return static_cast<It&&>(it).operator->();
  } else {
    return static_cast<It&&>(it);
  }
}
template <typename It>
void get_arrow_result(It&&);

template <class It>
struct PointerTrait {
  using type = decltype(get_arrow_result(declval<It>()));
};

template <class It>
requires requires { typename iterator_traits<remove_reference_t<It>>::pointer; }
struct PointerTrait<It> {
  using type = typename iterator_traits<remove_reference_t<It>>::pointer;
};

template <class R>
using range_pointer_t = typename PointerTrait<iterator_t<R>>::type;

template <class... Views>
using concat_pointer = common_type<range_pointer_t<Views>...>;
// using concat_pointer = common_type<typename
// iterator_traits<iterator_t<Views>>::pointer...>;
// ^^ hard fails for not-a-c17iterator

}  // namespace not_to_spec

template <class... Views>
using concat_pointer_t = typename concat_pointer<Views...>::type;

// clang-format off
template <class... Views>
concept concat_has_arrow = 
    (__has_arrow<iterator_t<Views>> && ...) &&    
    requires { typename concat_pointer_t<Views...>; } &&
    (convertible_to<const range_pointer_t<Views>&, concat_pointer_t<Views...>> && ...) 
    ;
// clang-format on

inline namespace not_to_spec {

template <bool Const, class... Ts>
consteval auto iterator_concept_test() {
  if constexpr (concat_is_random_access<Const, Ts...>) {
    return random_access_iterator_tag{};
  } else if constexpr (concat_bidirectional<__maybe_const<Const, Ts>...>) {
    return bidirectional_iterator_tag{};
  } else if constexpr ((forward_range<__maybe_const<Const, Ts>> && ...)) {
    return forward_iterator_tag{};
  } else {
    return input_iterator_tag{};
  }
}

// calls f(integral_constant<idx>{}) for a runtime idx in [0,N)
template <size_t N, typename Var, typename F>
constexpr auto visit_i_impl(size_t idx, Var&& v, F&& f) {
  assert(idx < N);
  if constexpr (N > 1) {
    return idx == N - 1
               ? invoke(static_cast<F&&>(f), integral_constant<size_t, N - 1>{},
                        std::get<N - 1>(static_cast<Var&&>(v)))
               : visit_i_impl<N - 1>(idx, static_cast<Var&&>(v),
                                     static_cast<F&&>(f));
  } else {
    return invoke(static_cast<F&&>(f), integral_constant<size_t, 0>{},
                  std::get<0>(static_cast<Var&&>(v)));
  }
}

// calls f(integral_constant<idx>{}, get<idx>(v)) for idx == v.index().
template <typename Var, typename F>
constexpr auto visit_i(Var&& v, F&& f) {
  return visit_i_impl<variant_size_v<remove_reference_t<Var>>>(
      v.index(), static_cast<Var&&>(v), static_cast<F&&>(f));
}

template <typename tag, typename View>
concept has_tag =
    derived_from<tag,
                 typename iterator_traits<iterator_t<View>>::iterator_category>;

template <bool Const, class... Views>
consteval auto iter_cat_test() {
  using reference =
      common_reference_t<range_reference_t<__maybe_const<Const, Views>>...>;
  if constexpr (!is_reference_v<reference>) {
    return input_iterator_tag{};
  } else if constexpr ((has_tag<random_access_iterator_tag,
                                __maybe_const<Const, Views>> &&
                        ...) &&
                       concat_is_random_access<Const, Views...>) {
    return random_access_iterator_tag{};
  } else if constexpr ((has_tag<bidirectional_iterator_tag,
                                __maybe_const<Const, Views>> &&
                        ...) &&
                       concat_bidirectional<__maybe_const<Const, Views>...>) {
    return bidirectional_iterator_tag{};
  } else if constexpr ((has_tag<forward_iterator_tag,
                                __maybe_const<Const, Views>> &&
                        ...)) {
    return forward_iterator_tag{};
  } else {
    return input_iterator_tag{};
  }
}

struct empty_ {};

template <bool Const, class... Views>
struct iter_cat_base {
  using iterator_category = decltype(iter_cat_test<Const, Views...>());
};

template <bool, class...>
consteval auto iter_cat_base_sel() -> empty_;

template <bool Const, class... Views>
consteval auto iter_cat_base_sel() -> iter_cat_base<Const, Views...>
requires((forward_range<__maybe_const<Const, Views>> && ...));

template <bool Const, class... Views>
using iter_cat_base_t = decltype(iter_cat_base_sel<Const, Views...>());

}  // namespace not_to_spec

}  // namespace xo

// clang-format off
// [TODO] constrain less and allow just a `view`? (i.e. including output_range in the mix - need an example)
template <bool expert_mode, input_range... Views>
    requires (view<Views>&&...) && (sizeof...(Views) > 0) && xo::concatable<expert_mode, Views...>  
class concat_view : public view_interface<concat_view<expert_mode, Views...>> {
  // clang-format on
  tuple<Views...> views_;  // exposition only

  template <bool Const>
  class iterator : public xo::iter_cat_base_t<Const, Views...> {
   public:
    using value_type = xo::concat_value_t<__maybe_const<Const, Views>...>;
    using difference_type =
        common_type_t<range_difference_t<__maybe_const<Const, Views>>...>;
    using iterator_concept =
        decltype(xo::iterator_concept_test<Const, Views...>());

   private:
    using ParentView = __maybe_const<Const, concat_view>;
    using BaseIt = variant<iterator_t<__maybe_const<Const, Views>>...>;

    ParentView* parent_ = nullptr;
    BaseIt it_;

    friend class iterator<!Const>;
    friend class concat_view;

    template <std::size_t N>
    constexpr void satisfy() {
      if constexpr (N < (sizeof...(Views) - 1)) {
        if (get<N>(it_) == ranges::end(get<N>(parent_->views_))) {
          it_.template emplace<N + 1>(
              ranges::begin(get<N + 1>(parent_->views_)));
          satisfy<N + 1>();
        }
      }
    }

    template <std::size_t N>
    constexpr void prev() {
      if constexpr (N == 0) {
        --get<0>(it_);
      } else {
        if (get<N>(it_) == ranges::begin(get<N>(parent_->views_))) {
          using prev_view =
              __maybe_const<Const, tuple_element_t<N - 1, tuple<Views...>>>;
          if constexpr (common_range<prev_view>) {
            it_.template emplace<N - 1>(
                ranges::end(get<N - 1>(parent_->views_)));
          } else {
            static_assert(random_access_range<prev_view> &&
                          sized_range<prev_view>);
            it_.template emplace<N - 1>(
                ranges::next(ranges::begin(get<N - 1>(parent_->views_)),
                             ranges::size(get<N - 1>(parent_->views_))));
          }
          prev<N - 1>();
        } else {
          --get<N>(it_);
        }
      }
    }

    template <std::size_t N>
    constexpr void advance_fwd(difference_type current_offset,
                               difference_type steps) {
      using underlying_diff_type =
          std::iter_difference_t<std::variant_alternative_t<N, BaseIt>>;
      if constexpr (N == sizeof...(Views) - 1) {
        get<N>(it_) += static_cast<underlying_diff_type>(steps);
      } else {
        static_assert(
            std::ranges::sized_range<decltype(get<N>(parent_->views_))>);
        auto n_size = ranges::distance(get<N>(parent_->views_));
        if (current_offset + steps < n_size) {
          get<N>(it_) += static_cast<underlying_diff_type>(steps);
        } else {
          it_.template emplace<N + 1>(
              ranges::begin(get<N + 1>(parent_->views_)));
          advance_fwd<N + 1>(
              0, current_offset + steps - n_size);
        }
      }
    }

    template <std::size_t N>
    constexpr void advance_bwd(difference_type current_offset,
                               difference_type steps) {
      using underlying_diff_type =
          std::iter_difference_t<std::variant_alternative_t<N, BaseIt>>;
      if constexpr (N == 0) {
        get<N>(it_) -= static_cast<underlying_diff_type>(steps);
      } else {
        if (current_offset >= steps) {
          get<N>(it_) -= static_cast<underlying_diff_type>(steps);
        } else {
          static_assert(
              std::ranges::sized_range<decltype(get<N - 1>(parent_->views_))>);
          auto prev_size = ranges::distance(get<N - 1>(parent_->views_));
          it_.template emplace<N - 1>(
              ranges::begin(get<N - 1>(parent_->views_)) + prev_size);
          advance_bwd<N - 1>(prev_size, steps - current_offset);
        }
      }
    }

    decltype(auto) get_parent_views() const { return (parent_->views_); }

    template <class... Args>
    explicit constexpr iterator(ParentView* parent, Args&&... args) requires
        constructible_from<BaseIt, Args&&...>
        : parent_{parent}, it_{static_cast<Args&&>(args)...} {}

   public:
    iterator() = default;

    constexpr iterator(iterator<!Const> i) requires Const &&
        (convertible_to<iterator_t<Views>, iterator_t<const Views>>&&...)
        // [TODO] noexcept specs?
        : parent_{i.parent_}, it_{std::move(i.it_)} {}

    constexpr decltype(auto) operator*() const {
      using reference = xo::concat_reference_t<__maybe_const<Const, Views>...>;
      return visit([](auto&& it) -> reference { return *it; }, it_);
    }

    constexpr auto operator->()
        const requires xo::concat_has_arrow<__maybe_const<Const, Views>...> {
      return visit(
          [](auto const& it)
              -> xo::concat_pointer_t<__maybe_const<Const, Views>...> {
            using It = remove_reference_t<decltype(it)>;
            if constexpr (xo::has_member_arrow<It>) {
              return it.operator->();
            } else {
              static_assert(is_pointer_v<It>);
              return it;
            }
          },
          it_);
    }

    constexpr iterator& operator++() {
      xo::visit_i(it_, [this](auto I, auto&& it) {
        ++it;
        this->satisfy<I>();
      });
      return *this;
    }

    constexpr void operator++(int) { ++*this; }

    constexpr iterator operator++(int) requires(
        forward_range<__maybe_const<Const, Views>>&&...) {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    constexpr iterator& operator--() requires
        xo::concat_bidirectional<__maybe_const<Const, Views>...> {
      xo::visit_i(it_, [this](auto I, auto&&) { this->prev<I>(); });
      return *this;
    }

    constexpr iterator operator--(
        int) requires xo::concat_bidirectional<__maybe_const<Const, Views>...> {
      auto tmp = *this;
      --*this;
      return tmp;
    }

    constexpr iterator& operator+=(difference_type n)  //
        requires xo::concat_is_random_access<Const, Views...> {
      if (n > 0) {
        xo::visit_i(it_, [this, n](auto I, auto&& it) {
          this->advance_fwd<I>(it - ranges::begin(get<I>(parent_->views_)), n);
        });
      } else if (n < 0) {
        xo::visit_i(it_, [this, n](auto I, auto&& it) {
          this->advance_bwd<I>(it - ranges::begin(get<I>(parent_->views_)), -n);
        });
      }
      return *this;
    }

    constexpr iterator& operator-=(difference_type n)  //
        requires xo::concat_is_random_access<Const, Views...> {
      *this += -n;
      return *this;
    }

    constexpr decltype(auto) operator[](difference_type n) const  //
        requires xo::concat_is_random_access<Const, Views...> {
      return *((*this) + n);
    }

    friend constexpr bool
    operator==(const iterator& it1, const iterator& it2) requires(
        equality_comparable<iterator_t<__maybe_const<Const, Views>>>&&...) {
      return it1.it_ == it2.it_;
    }

    friend constexpr bool operator==(const iterator& it,
                                     const default_sentinel_t&) {
      constexpr auto LastIdx = sizeof...(Views) - 1;
      return it.it_.index() == LastIdx &&
             get<LastIdx>(it.it_) ==
                 ranges::end(get<LastIdx>(it.get_parent_views()));
    }

    friend constexpr bool
    operator<(const iterator& x, const iterator& y) requires(
        random_access_range<__maybe_const<Const, Views>>&&...) {
      return x.it_ < y.it_;
    }

    friend constexpr bool
    operator>(const iterator& x, const iterator& y) requires(
        random_access_range<__maybe_const<Const, Views>>&&...) {
      return y < x;
    }

    friend constexpr bool
    operator<=(const iterator& x, const iterator& y) requires(
        random_access_range<__maybe_const<Const, Views>>&&...) {
      return !(y < x);
    }

    friend constexpr bool
    operator>=(const iterator& x, const iterator& y) requires(
        random_access_range<__maybe_const<Const, Views>>&&...) {
      return !(x < y);
    }

    friend constexpr auto
    operator<=>(const iterator& x, const iterator& y) requires(
        (random_access_range<__maybe_const<Const, Views>> &&
         three_way_comparable<__maybe_const<Const, Views>>)&&...) {
      return x.it_ <=> y.it_;
    }

    friend constexpr iterator operator+(const iterator& it,
                                        difference_type n) requires
        xo::concat_is_random_access<Const, Views...> {
      return iterator{it} += n;
    }

    friend constexpr iterator operator+(difference_type n,
                                        const iterator& it) requires
        xo::concat_is_random_access<Const, Views...> {
      return it + n;
    }

    friend constexpr iterator operator-(const iterator& it,
                                        difference_type n) requires
        xo::concat_is_random_access<Const, Views...> {
      return iterator{it} -= n;
    }

    friend constexpr difference_type operator-(const iterator& x,
                                               const iterator& y) requires
        xo::concat_is_random_access<Const, Views...> {
      auto ix = x.it_.index();
      auto iy = y.it_.index();
      if (ix > iy) {
        // distance(y, yend) + size(ranges_in_between)... + distance(xbegin, x)
        const auto all_sizes = std::apply(
            [&](const auto&... views) {
              const auto getSize = [](const auto& view) {
                if constexpr (ranges::sized_range<decay_t<decltype(view)>>) {
                  return ranges::size(view);
                } else {
                  return 0u;  // only the last range can be non-sized, and its
                              // value is not used
                }
              };
              return std::array{
                  static_cast<difference_type>(getSize(views))...};
            },
            x.get_parent_views());
        auto in_between =
            std::accumulate(all_sizes.data() + iy + 1, all_sizes.data() + ix,
                            difference_type(0));

        auto y_to_end = xo::visit_i(y.it_, [&](auto I, auto&& it) {
          return all_sizes[I] -
                 (it - ranges::begin(get<I>(y.get_parent_views())));
        });

        auto begin_to_x = xo::visit_i(x.it_, [&](auto I, auto&& it) {
          return it - ranges::begin(get<I>(x.get_parent_views()));
        });

        return y_to_end + in_between + begin_to_x;

      } else if (ix < iy) {
        return -(y - x);
      } else {
        return xo::visit_i(x.it_, [&](auto I, auto&&) {
          return get<I>(x.it_) - get<I>(y.it_);
        });
      }
    }

    friend constexpr difference_type
    operator-(const iterator& it, default_sentinel_t) requires(
        xo::concat_is_random_access<Const, Views...>&&
            sized_range<xo::back<__maybe_const<Const, Views>...>>) {
      const auto idx = it.it_.index();
      const auto all_sizes = std::apply(
          [&](const auto&... views) {
            return std::array{
                static_cast<difference_type>(ranges::size(views))...};
          },
          it.get_parent_views());
      auto to_the_end = std::accumulate(all_sizes.begin() + idx + 1,
                                        all_sizes.end(), difference_type(0));

      auto i_to_idx_end = xo::visit_i(it.it_, [&](auto I, auto&& i) {
        return all_sizes[I] -
               static_cast<difference_type>(
                   i - ranges::begin(get<I>(it.get_parent_views())));
      });
      return -(i_to_idx_end + to_the_end);
    }

    friend constexpr difference_type
    operator-(default_sentinel_t, const iterator& it) requires(
        xo::concat_is_random_access<Const, Views...>&&
            sized_range<xo::back<__maybe_const<Const, Views>...>>) {
      return -(it - default_sentinel);
    }

    friend constexpr decltype(auto) iter_move(iterator const& ii) noexcept(
        ((std::is_nothrow_invocable_v<
              decltype(ranges::iter_move),
              const iterator_t<__maybe_const<Const, Views>>&> &&
          std::is_nothrow_convertible_v<
              range_rvalue_reference_t<__maybe_const<Const, Views>>,
              common_reference_t<range_rvalue_reference_t<
                  __maybe_const<Const, Views>>...>>)&&...)) {
      return std::visit(
          [](auto const& i) -> xo::concat_rvalue_reference_t<
                                __maybe_const<Const, Views>...> {  //
            return ranges::iter_move(i);
          },
          ii.it_);
    }

    friend constexpr void
    iter_swap(const iterator& x, const iterator& y) requires(
        swappable_with<iter_reference_t<iterator>, iter_reference_t<iterator>> &&...&&
        indirectly_swappable<iterator_t<__maybe_const<Const, Views>>>)
    // todo: noexcept?
    {
      std::visit(
          [&](const auto& it1, const auto& it2) {
            if constexpr (std::is_same_v<decltype(it1), decltype(it2)>) {
              ranges::iter_swap(it1, it2);
            } else {
              ranges::swap(*x, *y);
            }
          },
          x.it_, y.it_);
    }
  };

 public:
  constexpr concat_view() = default;

  constexpr explicit concat_view(Views... views)
      : views_{static_cast<Views&&>(views)...} {}

  constexpr iterator<false> begin() requires(!(__simple_view<Views> && ...))  //
  {
    iterator<false> it(this, in_place_index<0u>, ranges::begin(get<0>(views_)));
    it.template satisfy<0>();
    return it;
  }

  constexpr iterator<true> begin() const
      requires((range<const Views> && ...) &&
               xo::concatable<expert_mode, const Views...>)  //
  {
    iterator<true> it(this, in_place_index<0u>, ranges::begin(get<0>(views_)));
    it.template satisfy<0>();
    return it;
  }

  constexpr auto end() requires(!(__simple_view<Views> && ...)) {
    using LastView = xo::back<Views...>;
    if constexpr (common_range<LastView>) {
      constexpr auto N = sizeof...(Views);
      return iterator<false>(this, in_place_index<N - 1>,
                             ranges::end(get<N - 1>(views_)));
    } else {
      return default_sentinel;
    }
  }

  constexpr auto end() const requires(range<const Views>&&...) {
    using LastView = xo::back<const Views...>;
    if constexpr (common_range<LastView>) {
      constexpr auto N = sizeof...(Views);
      return iterator<true>(this, in_place_index<N - 1>,
                            ranges::end(get<N - 1>(views_)));
    } else {
      return default_sentinel;
    }
  }

  constexpr auto size() requires(sized_range<Views>&&...) {
    return apply(
        [](auto... sizes) {
          using CT = make_unsigned_t<common_type_t<decltype(sizes)...>>;
          return (CT(sizes) + ...);
        },
        concat_detail::tuple_transform(ranges::size, views_));
  }

  constexpr auto size() const requires(sized_range<const Views>&&...) {
    return apply(
        [](auto... sizes) {
          using CT = make_unsigned_t<common_type_t<decltype(sizes)...>>;
          return (CT{0} + ... + CT{sizes});
        },
        concat_detail::tuple_transform(ranges::size, views_));
  }
};

template <class... R>
concat_view(R&&...) -> concat_view<false, views::all_t<R>...>;

// cpo:

namespace views {
namespace xo {
class concat_fn {
 public:
  constexpr void operator()() const = delete;

  template <viewable_range V>
  constexpr auto operator()(V&& v) const
      noexcept(noexcept(std::views::all(static_cast<V&&>(v)))) {
    return std::views::all(static_cast<V&&>(v));
  }

  template <input_range... V>
  requires(sizeof...(V) > 1) && ranges::xo::concatable<false, all_t<V&&>...> &&
      (viewable_range<V> && ...)  //
      constexpr auto
      operator()(V&&... v)
          const {  // noexcept(noexcept(concat_view{static_cast<V&&>(v)...})) {
    return concat_view{static_cast<V&&>(v)...};
  }
};

class concat_expert_fn : concat_fn {
 public:
  template <input_range... V>
  requires(sizeof...(V) > 1) && ranges::xo::concatable<true, all_t<V&&>...> &&
      (viewable_range<V> && ...)  //
      constexpr auto
      operator()(V&&... v)
          const {  // noexcept(noexcept(concat_view{static_cast<V&&>(v)...})) {
    return concat_view<true, views::all_t<V>...>{static_cast<V&&>(v)...};
  }
};
}  // namespace xo

inline constexpr xo::concat_fn concat;
inline constexpr xo::concat_expert_fn concat_expert;
}  // namespace views

}  // namespace std::ranges

#endif
