#ifndef LIBCPP__RANGE_CONCAT_HPP
#define LIBCPP__RANGE_CONCAT_HPP

#include <concepts>
#include <functional>
#include <ranges>
#include <tuple>
#include <variant>

#include "utils.hpp"

namespace std::ranges {

namespace xo { // exposition only things (and persevering face)

template <bool Const, class... Views>
concept all_random_access = (random_access_range<__maybe_const<Const, Views>> && ...);

template <class... Views>
concept concat_can_prev = ((bidirectional_range<Views> &&
                            (common_range<Views> ||
                             (random_access_range<Views> && sized_range<Views>))) &&
                           ...);

template <bool Const, class... Views>
concept concat_bidirectional = concat_can_prev<__maybe_const<Const, Views>...>;

template <bool Const, class... Views>
concept all_forward = (forward_range<__maybe_const<Const, Views>> && ...);

template <bool Const, typename... Views>
constexpr auto iterator_concept_test() {
    if constexpr (all_random_access<Const, Views...>) {
        return random_access_iterator_tag{};
    } else if constexpr (concat_bidirectional<Const, Views...>) {
        return bidirectional_iterator_tag{};
    } else if constexpr (all_forward<Const, Views...>) {
        return forward_iterator_tag{};
    } else {
        return input_iterator_tag{};
    }
}


template <size_t N, typename... T>
using Nth = tuple_element_t<N, tuple<T...>>;

template <typename... T>
using back = tuple_element_t<sizeof...(T) - 1, tuple<T...>>;


template <typename... T>
concept concatable =
    (convertible_to<range_reference_t<T>, common_reference_t<range_reference_t<T>...>> && ...);


template <size_t N, typename F>
constexpr auto visit_i(size_t idx, F&& f) {
    if (idx == N) {
        return static_cast<F&&>(f)(integral_constant<size_t, N>{});
    } else {
        if constexpr (N != 0) {
            return visit_i<N - 1>(idx, static_cast<F&&>(f));
        }
    }
}

} // namespace xo


// clang-format off
// [TODO] constrain less and allow just a `view`? (i.e. including output_range in the mix - need an example)
template <input_range... Views>
    requires (view<Views>&&...) && (sizeof...(Views) > 1) && xo::concatable<Views...>  
class concat_view : public view_interface<concat_view<Views...>> {
    // clang-format on
    tuple<Views...> views_ = tuple<Views...>(); // exposition only

    template <bool Const>
    class iterator {
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
                    // we are at the position Nth range's begin, by doing -- we are going to
                    // N-1th range's end-1
                    using prev_view = __maybe_const<Const, xo::Nth<N - 1, Views...>>;
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

        decltype(auto) get_parent_views() const { return parent_->views_; }

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
            auto visitor = [this]<size_t N>(integral_constant<size_t, N>) {
                ++get<N>(it_);
                this->satisfy<N>();
            };
            xo::visit_i<sizeof...(Views) - 1>(it_.index(), visitor);
            return *this;
        }

        constexpr void operator++(int) { ++*this; }

        constexpr iterator operator++(int) requires xo::all_forward<Const, Views...> {
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
            auto visitor = [this]<size_t N>(integral_constant<size_t, N>) { this->prev<N>(); };
            xo::visit_i<sizeof...(Views) - 1>(it_.index(), visitor);
            return *this;
        }

        constexpr iterator operator--(int) requires xo::concat_bidirectional<Const, Views...> {
            auto tmp = *this;
            --*this;
            return tmp;
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
    };


  public:
    constexpr concat_view() requires(default_initializable<Views>&&...) = default;

    constexpr explicit concat_view(Views... views)
        : views_{static_cast<Views&&>(views)...} {}

    // used exposition only concepts simple-view defined here:
    // http://eel.is/c++draft/ranges#range.utility.helpers (we can reuse in the spec)
    constexpr iterator<false> begin() requires(!(__simple_view<Views> && ...)) {
        iterator<false> it{this, in_place_index<0u>, ranges::begin(get<0>(views_))};
        it.template satisfy<0>();
        return it;
        // O(1) as sizeof...(Views) known at compile time
    }

    constexpr iterator<true> begin() const
        requires((range<const Views> && ...) && xo::concatable<const Views...>) {
        iterator<true> it{this, in_place_index<0u>, ranges::begin(get<0>(views_))};
        it.template satisfy<0>();
        return it;
    }

    constexpr auto end() requires(!(__simple_view<Views> && ...)) {
        if constexpr (common_range<xo::back<Views...>>) {
            constexpr auto N = sizeof...(Views);
            return iterator<false>{this, in_place_index<N - 1>, ranges::end(get<N - 1>(views_))};
        } else {
            return default_sentinel;
        }
    }

    constexpr auto end() const requires(range<const Views>&&...) {
        if constexpr (common_range<xo::back<const Views...>>) {
            constexpr auto N = sizeof...(Views);
            return iterator<true>{this, in_place_index<N - 1>, ranges::end(get<N - 1>(views_))};
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