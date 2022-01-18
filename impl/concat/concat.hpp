#ifndef LIBCPP__RANGE_CONCAT_HPP
#define LIBCPP__RANGE_CONCAT_HPP

#include <concepts>
#include <functional>
#include <numeric>
#include <ranges>
#include <tuple>
#include <variant>
#include <array>
#include <cassert>

#include "utils.hpp"

namespace std::ranges {

namespace xo { // exposition only things (and persevering face)

template <class... T>
concept concatable =
    (convertible_to<range_reference_t<T>, common_reference_t<range_reference_t<T>...>> && ...);

template <class... T>
using back = tuple_element_t<sizeof...(T) - 1, tuple<T...>>;

template <bool... b, size_t... I>
consteval bool all_but_last(std::index_sequence<I...>) {
    return ((I == sizeof...(I) - 1 || b) && ...);
}

template <bool Const, class... Ts>
concept concat_random_access = ((random_access_range<__maybe_const<Const, Ts>> &&
                                 sized_range<__maybe_const<Const, Ts>>)&&...);

template <class R>
concept constant_time_reversible = (bidirectional_range<R> && common_range<R>) ||
                                   (sized_range<R> && random_access_range<R>);

template <bool Const, class... Ts>
concept concat_bidirectional = all_but_last<constant_time_reversible<__maybe_const<Const, Ts>>...>(
    index_sequence_for<Ts...>{}) && bidirectional_range<back<__maybe_const<Const, Ts>...>>;

inline namespace not_to_spec {

template <bool Const, class... Ts>
constexpr auto iterator_concept_test() {
    if constexpr (concat_random_access<Const, Ts...>) {
        return random_access_iterator_tag{};
    } else if constexpr (concat_bidirectional<Const, Ts...>) {
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
        return idx == N - 1 ? invoke(static_cast<F&&>(f), integral_constant<size_t, N - 1>{},
                                     std::get<N - 1>(static_cast<Var&&>(v)))
                            : visit_i_impl<N - 1>(idx, static_cast<Var&&>(v), static_cast<F&&>(f));
    } else {
        return invoke(static_cast<F&&>(f), integral_constant<size_t, 0>{},
                      std::get<0>(static_cast<Var&&>(v)));
    }
}
} // namespace not_to_spec


// calls f(integral_constant<idx>{}, get<idx>(v)) for idx == v.index().
template <typename Var, typename F>
constexpr auto visit_i(Var&& v, F&& f) {
    return visit_i_impl<variant_size_v<remove_reference_t<Var>>>(v.index(), static_cast<Var&&>(v),
                                                                 static_cast<F&&>(f));
}



} // namespace xo


// clang-format off
// [TODO] constrain less and allow just a `view`? (i.e. including output_range in the mix - need an example)
template <input_range... Views>
    requires (view<Views>&&...) && (sizeof...(Views) > 0) && xo::concatable<Views...>  
class concat_view : public view_interface<concat_view<Views...>> {
    // clang-format on
    tuple<Views...> views_ = tuple<Views...>();    // exposition only

    template <bool Const>
    class iterator {
      public:
        // [TODO] range-v3 has pointed out that rvalue_reference is a problem
        using reference = common_reference_t<range_reference_t<__maybe_const<Const, Views>>...>;
        using difference_type = common_type_t<range_difference_t<__maybe_const<Const, Views>>...>;
        using value_type = common_type_t<range_value_t<__maybe_const<Const, Views>>...>;
        using iterator_concept = decltype(xo::iterator_concept_test<Const, Views...>());

        /* [TODO]
         * this one is tricky.
         * it depends on the iterate_category of every base and also depends on if
         * the common_reference_t<...> is actually a reference
         */
        // using iterator_category = ; // not always present.
      private:
        // use of exposition only trait `maybe-const` defined in
        // http://eel.is/c++draft/ranges#syn
        using ParentView = __maybe_const<Const, concat_view>;
        using BaseIt = variant<iterator_t<__maybe_const<Const, Views>>...>;

        ParentView* parent_ = nullptr;
        BaseIt it_ = BaseIt();

        friend class iterator<!Const>;
        friend class concat_view;

        template <std::size_t N>
        constexpr void satisfy() {
            if constexpr (N != (sizeof...(Views) - 1)) {
                if (get<N>(it_) == ranges::end(get<N>(parent_->views_))) {
                    it_.template emplace<N + 1>(ranges::begin(get<N + 1>(parent_->views_)));
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
                    using prev_view = __maybe_const<Const, tuple_element_t<N - 1, tuple<Views...>>>;
                    if constexpr (common_range<prev_view>) {
                        it_.template emplace<N - 1>(ranges::end(get<N - 1>(parent_->views_)));
                    } else {
                        static_assert(random_access_range<prev_view> && sized_range<prev_view>);
                        it_.template emplace<N - 1>(
                            ranges::next(ranges::begin(get<N - 1>(parent_->views_)),
                                         ranges::size(get<N - 1>(parent_->views_))));
                    }
                    this->prev<N - 1>();
                } else {
                    --get<N>(it_);
                }
            }
        }

        template <std::size_t N>
        constexpr void advance_fwd(difference_type current_offset, difference_type steps) {
            if constexpr (N == sizeof...(Views) - 1) {
                get<N>(it_) += steps;
            } else {
                auto n_size = ranges::size(get<N>(parent_->views_));
                if (current_offset + steps < static_cast<difference_type>(n_size)) {
                    get<N>(it_) += steps;
                } else {
                    it_.template emplace<N + 1>(ranges::begin(get<N + 1>(parent_->views_)));
                    advance_fwd<N + 1>(0, current_offset + steps - n_size);
                }
            }
        }

        template <std::size_t N>
        constexpr void advance_bwd(difference_type current_offset, difference_type steps) {
            if constexpr (N == 0) {
                get<N>(it_) -= steps;
            } else {
                if (current_offset >= steps) {
                    get<N>(it_) -= steps;
                } else {
                    it_.template emplace<N - 1>(ranges::begin(get<N - 1>(parent_->views_)) +
                                                ranges::size(get<N - 1>(parent_->views_)));
                    advance_bwd<N - 1>(
                        static_cast<difference_type>(ranges::size(get<N - 1>(parent_->views_))),
                        steps - current_offset);
                }
            }
        }

        // We can skip this in the spec, as I checked other views assumed the friend access
        decltype(auto) get_parent_views() const { return (parent_->views_); }

      public:
        iterator() requires(default_initializable<iterator_t<__maybe_const<Const, Views>>>&&...) =
            default;

        template <class... Args>
        explicit constexpr iterator(ParentView* parent,
                                    Args&&... args) requires constructible_from<BaseIt, Args&&...>
            : parent_{parent}, it_{static_cast<Args&&>(args)...} {}

        constexpr iterator(iterator<!Const> i) requires Const &&
            (convertible_to<iterator_t<Views>, iterator_t<__maybe_const<Const, Views>>>&&...)
            // [TODO] noexcept specs?
            : parent_{i.parent_}
            , it_{std::move(i.it_)} {}

        constexpr reference operator*() const {
            return visit([](auto&& it) -> reference { return *it; }, it_);
        }

        constexpr iterator& operator++() {
            /*
             * in the spec, we can say
             * ++get<I>(it_);
             * this->satisfy<I>();
             * return *this;
             * where I equals to it_.index()
             */
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

        constexpr iterator& operator--() requires xo::concat_bidirectional<Const, Views...> {
            /*
             * in the spec, we can say
             * this->prev<I>();
             * return *this;
             * where I equals to it_.index()
             */
            xo::visit_i(it_, [this](auto I, auto&&) { this->prev<I>(); });
            return *this;
        }

        constexpr iterator operator--(int) requires xo::concat_bidirectional<Const, Views...> {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        constexpr iterator& operator+=(difference_type n) //
            requires xo::concat_random_access<Const, Views...> {
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

        constexpr iterator& operator-=(difference_type n) //
            requires xo::concat_random_access<Const, Views...> {
            *this += -n;
            return *this;
        }

        constexpr reference operator[](difference_type n) const //
            requires xo::concat_random_access<Const, Views...> {
            return *((*this) + n);
        }

        friend constexpr bool operator==(const iterator& it1, const iterator& it2) requires(
            equality_comparable<iterator_t<__maybe_const<Const, Views>>>&&...) {
            return it1.it_ == it2.it_;
        }

        friend constexpr bool operator==(const iterator& it, const default_sentinel_t&) {
            constexpr auto LastIdx = sizeof...(Views) - 1;
            return it.it_.index() == LastIdx &&
                   get<LastIdx>(it.it_) == ranges::end(get<LastIdx>(it.get_parent_views()));
        }

        friend constexpr auto operator<(const iterator& x, const iterator& y) requires(
            random_access_range<__maybe_const<Const, Views>>&&...) {
            return x.it_ < y.it_;
        }

        friend constexpr auto operator>(const iterator& x, const iterator& y) requires(
            random_access_range<__maybe_const<Const, Views>>&&...) {
            return y < x;
        }

        friend constexpr auto operator<=(const iterator& x, const iterator& y) requires(
            random_access_range<__maybe_const<Const, Views>>&&...) {
            return !(y < x);
        }

        friend constexpr auto operator>=(const iterator& x, const iterator& y) requires(
            random_access_range<__maybe_const<Const, Views>>&&...) {
            return !(x < y);
        }

        friend constexpr auto operator<=>(const iterator& x, const iterator& y) requires(
            (random_access_range<__maybe_const<Const, Views>> &&
             three_way_comparable<__maybe_const<Const, Views>>)&&...) {
            return x.it_ <=> y.it_;
        }

        friend constexpr iterator operator+(const iterator& x, difference_type y) requires(
            xo::concat_random_access<Const, Views>&&...) {
            return iterator{x} += y;
        }

        friend constexpr iterator operator+(difference_type x, const iterator& y) requires(
            xo::concat_random_access<Const, Views>&&...) {
            return y + x;
        }

        friend constexpr iterator operator-(const iterator& x, difference_type y) requires(
            xo::concat_random_access<Const, Views>&&...) {
            return iterator{x} -= y;
        }

        friend constexpr difference_type operator-(const iterator& x, const iterator& y) requires(
            xo::concat_random_access<Const, Views>&&...) {
            auto ix = x.it_.index();
            auto iy = y.it_.index();
            if (ix > iy) {
                // distance(y, yend) + size(ranges_in_between)... + distance(xbegin, x)
                const auto all_sizes = std::apply(
                    [&](const auto&... views) {
                        return std::array{static_cast<difference_type>(ranges::size(views))...};
                    },
                    x.get_parent_views());
                auto in_between = std::accumulate(all_sizes.data() + iy + 1, all_sizes.data() + ix,
                                                  difference_type(0));

                auto y_to_end = xo::visit_i(y.it_, [&](auto I, auto&& it) {
                    return all_sizes[I] - (it - ranges::begin(get<I>(y.get_parent_views())));
                });

                auto begin_to_x = xo::visit_i(x.it_, [&](auto I, auto&& it) {
                    return it - ranges::begin(get<I>(x.get_parent_views()));
                });

                return y_to_end + in_between + begin_to_x;

            } else if (ix < iy) {
                return -(y - x);
            } else {
                return xo::visit_i(
                    x.it_, [&](auto I, auto&&) { return get<I>(x.it_) - get<I>(y.it_); });
            }
        }

        friend constexpr difference_type operator-(const iterator& i, default_sentinel_t) requires(
            xo::concat_random_access<Const, Views>&&...) {

            const auto idx = i.it_.index();
            const auto all_sizes = std::apply(
                [&](const auto&... views) {
                    return std::array{static_cast<difference_type>(ranges::size(views))...};
                },
                i.get_parent_views());
            auto to_the_end =
                std::accumulate(all_sizes.begin() + idx + 1, all_sizes.end(), difference_type(0));

            auto i_to_idx_end = xo::visit_i(i.it_, [&](auto I, auto&& it) {
                return all_sizes[I] - (it - ranges::begin(get<I>(i.get_parent_views())));
            });
            return -(i_to_idx_end + to_the_end);
        }

        friend constexpr difference_type operator-(default_sentinel_t, const iterator& i) requires(
            xo::concat_random_access<Const, Views>&&...) {
            return -(i - default_sentinel);
        }

        /*   TODO:  I think we don't need them. the zip/cartesian product needs them because
                    their reference is a tuple and move(tuple) doesn't creates the proper
                    rvalue references of the elements. But ours reference is proper reference
                    to the elements and move the reference should just work
    friend constexpr auto iter_move(const iterator& i) noexcept(see below);
    friend constexpr void iter_swap(const iterator& l, const iterator& r) noexcept(see below)
        requires (indirectly_swappable<iterator_t<maybe-const<Const, First>>> && ... &&
            indirectly_swappable<iterator_t<maybe-const<Const, Views>>>);
        */
    };


  public:
    constexpr concat_view() requires(default_initializable<Views>&&...) = default;

    constexpr explicit concat_view(Views... views)
        : views_{static_cast<Views&&>(views)...} {}

    // used exposition only concepts simple-view defined here:
    // http://eel.is/c++draft/ranges#range.utility.helpers (we can reuse in the spec)
    constexpr iterator<false> begin() requires(!(__simple_view<Views> && ...)) //
    {
        iterator<false> it{this, in_place_index<0u>, ranges::begin(get<0>(views_))};
        it.template satisfy<0>();
        return it;
        // O(1) as sizeof...(Views) known at compile time
    }

    constexpr iterator<true> begin() const
        requires((range<const Views> && ...) && xo::concatable<const Views...>) //
    {
        iterator<true> it{this, in_place_index<0u>, ranges::begin(get<0>(views_))};
        it.template satisfy<0>();
        return it;
    }

    constexpr auto end() requires(!(__simple_view<Views> && ...)) {
        using LastView = xo::back<Views...>;
        if constexpr (common_range<LastView>) {
            constexpr auto N = sizeof...(Views);
            return iterator<false>{this, in_place_index<N - 1>, ranges::end(get<N - 1>(views_))};
        } else if constexpr (random_access_range<LastView> && sized_range<LastView>) {
            constexpr auto N = sizeof...(Views);
            return iterator<false>{
                this, in_place_index<N - 1>,
                ranges::begin(get<N - 1>(views_)) + ranges::size(get<N - 1>(views_))};
        } else {
            return default_sentinel;
        }
    }

    constexpr auto end() const requires(range<const Views>&&...) {
        using LastView = xo::back<const Views...>;
        if constexpr (common_range<LastView>) {
            constexpr auto N = sizeof...(Views);
            return iterator<true>{this, in_place_index<N - 1>, ranges::end(get<N - 1>(views_))};
        } else if constexpr (random_access_range<LastView> && sized_range<LastView>) {
            constexpr auto N = sizeof...(Views);
            return iterator<true>{
                this, in_place_index<N - 1>,
                ranges::begin(get<N - 1>(views_)) + ranges::size(get<N - 1>(views_))};
        } else {
            return default_sentinel;
        }
    }

    constexpr auto size() requires(sized_range<Views>&&...) {
        return apply(
            [](auto... sizes) {
                using CT = make_unsigned_t<common_type_t<decltype(sizes)...>>;
                return (CT{0} + ... + CT{sizes});
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
concat_view(R&&...) -> concat_view<views::all_t<R>...>;


// range adaptor object:

namespace views {
namespace xo {
class concat_fn {
  public:
    // [TODO] empty arg isn't valid. no meaningful value type? decide this for good.
    constexpr void operator()() const = delete;

    // single arg == all
    template <viewable_range V>
    constexpr auto operator()(V&& v) const
        noexcept(noexcept(std::views::all(static_cast<V&&>(v)))) {
        return std::views::all(static_cast<V&&>(v));
    }

    template <input_range... V>
    requires(sizeof...(V) > 1) && ranges::xo::concatable<all_t<V&&>...> &&
        (viewable_range<V> && ...) //
        constexpr auto
        operator()(V&&... v) const { // noexcept(noexcept(concat_view{static_cast<V&&>(v)...})) {
        return concat_view{static_cast<V&&>(v)...};
    }
};
} // namespace xo

inline constexpr xo::concat_fn concat;
} // namespace views


} // namespace std::ranges


#endif