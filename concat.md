---
title: "`views::concat`"
document: PXXXXR0
date: 2021-01-06
audience: SG9, LEWG
author:
  - name: Hui Xie
    email: <hui.xie1990@gmail.com>
  - name: S. Levent Yilmaz
    email: <levent.yilmaz@gmail.com>
toc: true
---

# Revision History

This is the initial revision.

# Abstract

This paper proposes the range adaptor `views::concat` as very briefly introduced in [@P2214R1] Section 4.7. It is a view factory that takes an arbitrary number of ranges as an argument list, and provides a view that starts at the first element of the first range, ends at the last element of the last range, with all range elements sequenced in between respectively in the order given in the arguments, effectively concatenating, or chaining together the argument ranges.

# Motivation and Examples

::: cmptable

## Before

```cpp
std::vector<int> v1{1,2,3}, v2{4,5}, v3{};
std::array  a{6,7,8};
int s = 9;
std::cout << std::format("[{:n}, {:n}, {:n}, {:n}, {}]\n", 
                v1, v2, v3, a, s); 
// output:  [1, 2, 3, 4, 5, , 6, 7, 8, 9]
```

## After

```cpp
std::vector<int> v1{1,2,3}, v2{4,5}, v3{};
std::array  a{6,7,8};
auto s = std::views::single(9);
std::cout << std::format("{}\n", 
                std::views::concat(v1, v2, v3, a, s)); 
// output:  [1, 2, 3, 4, 5, 6, 7, 8, 9]
```

---

```cpp
class Foo;

class Bar{
  const Foo& getFoo() const;
};

class MyClass{
  Foo foo_;
  std::vector<Bar> bars_;

  auto getFoos () const{
    std::vector<std::reference_wrapper<Foo const>> fooRefs{};
    fooRefs.reserve(bars_.size() + 1);
    fooRefs.push_back(std::cref(foo_));
    std::ranges::transform(bars_, std::back_inserter(bars), 
          [](const Bar& bar){
            return std::cref(bar.getFoo());
          });
    return fooRefs;
  }
};

// user
// the following line erros because getFoos() is prvalue
// for(const auto& foo: myClass.getFoos() | views::filter(pred))
auto fooRefs = myClass.getFoos();
for(const auto& foo: fooRefs | views::filter(pred)){
  // use foo.get()
}
```

```cpp
class Foo;

class Bar{
  const Foo& getFoo() const;
};

class MyClass{
  Foo foo_;
  std::vector<Bar> bars_;

  auto getFoos () const{
    using views = std::views;
    return views::concat(views::single(std::cref(foo_), 
        bars_ | views::transform(&Bar::getFoo)));
  }
};

// user
for(const auto& foo: myClass.getFoos() | views::filter(pred)){
  // use foo
}
```

:::

The first example shows that for a simple task like printing, doing it manually is error prone. The "Before" version manually concatinates all the ranges in a formatting string, but the result contains an extra comma and space because it doesn't handle the empty vector `v3` correctly. With `concat`, the user doesn't need to care about emptiness and simply uses the default range formatter to print.

The second example shows a function that tries to return a concatenated range from a single value and transformed range. One requirement is that the result range represents the original elements instead of copies.
In the "Before" version, the user has to create a vector and push elements from different places into it. Since the requirement is that the result shouldn't be copies, it wraps the elements with `reference_wrapper`. With `concat`, it is simple. Note that in the "Before" version, the result is still a container. Sometimes this is a problem because the interface might require the result to be a `view`.

# Design

This is a generator factory as described in [@P2214R1] Section 4.7. As such, it can not be piped to. It takes the list of ranges to concatenate as arguments to `ranges::concat_view` constructor, or to `ranges::views::concat` customization point object.

## Constrain on concatability

TODO:

- explain how this is an improvement over range-v3
- see if we can fix or somehow address that `common_reference_t` isn't a perfect solution, and misses some interesting cases. E.g.  `concat( b, d1, d2 )` works but `concat(d1,d2,b)` does not, where `D1` and `D2` are subclasses of `B`, but obviously `B&` would be a valid `reference_t` of the `concat_view`. (This can leads to long discussion with no conclusion, what about move this bullet point after the "Wording" section)

## Zero or one view

TODO:

- explain zero is invalid and one can be `all_t`

## Is `begin` Constant Time?

## Borrowed vs Cheap Iterator

A `concat` view can be designed to be a borrowed range, if all underlying ranges are. However, this requires the iterator implementation to contain a copy of all iterators and sentinels of all underlying ranges at all times (just like that of `views::zip` [@P2321R2]). On the other hand, a cheaper implementation that simply refers to the parent view can be used to satisfy all of the proposed functionality, if it is permitted to be not borrowed. Experience shows the borrowed-ness of `concat` is not a major requirement, and the existing implementation in [@rangev3] seems to have picked that latter alternative. We do so as such in this proposal.

## Common Range

`concat_view` can be `common_range` if the last underlying range models either

- `common_range`, or
- `random_access_range && sized_range`

## Bidirectional Range

`concat_view` can be `bidirectional_range` if the underlying ranges satisfy the following conditions:

- Every underlying range models `bidirectional_range`  
- If the iterator is at nth range's begin position, after `operator--` it should go to (n-1)th's range's end-1 position. This means, the (n-1)th range has to support either
  - `common_range && bidirectional_range`, so the position can be reached by `--ranges::end(n-1th range)`, assuming n-1th range is not empty, or
  - `random_access_range && sized_range`, so the position can be reached by `ranges::begin(n-1th range) + (ranges::size(n-1th range) - 1)` in constant time, assuming n-1th range is not empty.
  
  Note that the last underlying range does not have to satisfy this constraint because n-1 can never be the last underlying range. If neither of the two constraints is satisfied, in theory we can cache the end-1 position for every single underlying range inside the `concat_view` itself. But the authors do not consider this type of ranges as worth supporting bidirectional

In the `cancat` implementation in [@rangev3], `operator--` is only constrained on all underlying ranges being `bidirectional_range` on the declaration, but its implementation is using `ranges::next(ranges::begin(r), ranges::end(r))` which implicitly requires random access to make the operation constant time. So it went with the second constraint. In this paper, both are supported.

## Random Access Range

TODO:

- figure out exact condition. my guess is it needs to be all_random_access && (all_sized || all_common)

## Sized Range

`concat_view` can be `sized_range` if all the underlying ranges model `sized_range`

## Implementation experience

`views::concat` has been implemented in [@rangev3], with equivalent semantics as proposed here. We also have implemented a version that directly follows the proposed wording below without issue [@ours].

# Wording

## Addition to `<ranges>`

Add the following to [ranges.syn]{.sref}, header `<ranges>` synopsis:

```cpp
// [...]
namespace std::ranges {
  // [...]

  // [range.concat], concat view
  template <class... Ts>
  concept @_concatable_@ = @_see below_@; // exposition only

  template <input_range... Views>
    requires (view<Views> && ...) && (sizeof...(Views) > 0) &&
              @_concatable_@<Views...>
  class concat_view;

  namespace views {
    inline constexpr @_unspecified_@ concat = @_unspecified_@;
  }

}
```

## Range adaptor helpers [range.adaptor.helpers]

This paper applies the same following changes as in [@P2374R3]. If [@P2374R3] is merged into the standard, the changes in this section can be dropped.

New section after Non-propagating cache [range.nonprop.cache]{.sref}. Move the definitions of `@_tuple-or-pair_@`, `@_tuple-transform_@`, and `@_tuple-for-each_@` from Class template `zip_view` [range.zip.view]{.sref} to this section:

```cpp
namespace std::ranges {
    template <class... Ts>
    using tuple-or-pair = see-below;                     // exposition only

    template<class F, class Tuple>
    constexpr auto tuple-transform(F&& f, Tuple&& tuple) { // exposition only
        return apply([&]<class... Ts>(Ts&&... elements) {
            return tuple-or-pair<invoke_result_t<F&, Ts>...>(
                invoke(f, std::forward<Ts>(elements))...
            );
        }, std::forward<Tuple>(tuple));
    }

    template<class F, class Tuple>
    constexpr void tuple-for-each(F&& f, Tuple&& tuple) { // exposition only
        apply([&]<class... Ts>(Ts&&... elements) {
            (invoke(f, std::forward<Ts>(elements)), ...);
        }, std::forward<Tuple>(tuple));
    }
}
```

Given some pack of types `Ts`, the alias template `@_tuple-or-pair_@` is defined as follows:

1. If `sizeof...(Ts)` is `2`, `@_tuple-or-pair_@<Ts...>` denotes `pair<Ts...>`.
2. Otherwise, `@_tuple-or-pair_@<Ts...>` denotes `tuple<Ts...>`.

## `concat_view`

Add the following subclause to [range.adaptors]{.sref}.

### 24.7.? Concat view [range.concat] {-}

#### 24.7.?.1 Overview [range.concat.overview] {-}

`concat_view` presents a `view` that concatinates all the underlying ranges.

The name `views::concat` denotes a customization point object ([customization.point.object]{.sref}). Given a pack of subexpressions `Es...`, the expression `views::concat(Es...)` is expression-equivalent to

- `views::all(Es...)` if `Es` is a pack with only one element,
- otherwise, `concat_view(Es...)` if the expression is valid,
- otherwise, ill-formed

*[Example:*

```cpp
std::vector<int> v1{1,2,3}, v2{4,5}, v3{};
std::array  a{6,7,8};
auto s = std::views::single(9);
for(auto&& i : std::views::concat(v1, v2, v3, a, s)){
  std::cout << i << ' ';
}
// prints 1 2 3 4 5 6 7 8 9 
```

- *end example]*

#### 24.7.?.2 Class template `concat_view` [range.concat.view] {-}

```cpp
namespace std::ranges{
 
  template <class... T>
  concept @_concatable_@ =                        // exposition only
    (convertible_to<range_reference_t<T>, 
      common_reference_t<range_reference_t<T>...>> && ...);

  template <bool Const, class... Ts>
  concept @_concat-random-access_@ =              // exposition only
    ((random_access_range<@_maybe-const_@<Const, Ts>> &&
      sized_range<@_maybe-const_@<Const, Ts>>)&&...);

  template <class... T>
  using @_back_@ =                                // exposition only
    tuple_element_t<sizeof...(T) - 1, tuple<T...>>;

  template <bool... b, size_t... I>
  consteval bool @_all-but-last_@                 // exposition only
    (index_sequence<I...>) {
      return ((I == sizeof...(I) - 1 || b) && ...);
  }

  template <class R>
  concept @_constant-time-reversible_@ =          // exposition only
    (bidirectional_range<R> && common_range<R>) ||
    (sized_range<R> && random_access_range<R>);

  template <bool Const, class... Ts>
  concept @_concat-bidirectional_@ =              // exposition only
    @_all-but-last_@<@_constant-time-reversible_@<
      @_maybe-const_@<Const, Ts>>...>(index_sequence_for<Ts...>{}) && 
    bidirectional_range<@_back_@<@_maybe-const_@<Const, Ts>...>>;


  template <input_range... Views>
    requires (view<Views> && ...) && (sizeof...(Views) > 0) &&
              @_concatable_@<Views...>
  class concat_view : public view_interface<concat_view<Views...>> {
    tuple<Views...> @*views_*@ = tuple<Views...>(); // exposition only

    template <bool Const>
    class @_iterator_@;                           // exposition only

  public:
    constexpr concat_view() requires(default_initializable<Views>&&...) = default;

    constexpr explicit concat_view(Views... views);

    constexpr @_iterator_@<false> begin() requires(!(@_simple-view_@<Views> && ...));

    constexpr @_iterator_@<true> begin() const
      requires((range<const Views> && ...) && @_concatable_@<const Views...>);

    constexpr auto end() requires(!(@_simple-view_@<Views> && ...));

    constexpr auto end() const requires(range<const Views>&&...);

    constexpr auto size() requires(sized_range<Views>&&...);

    constexpr auto size() const requires(sized_range<const Views>&&...);
  };

  template <class... R>
  concat_view(R&&...) -> concat_view<views::all_t<R>...>;
}
```

```cpp
constexpr explicit concat_view(Views... views);
```

[1]{.pnum} *Effects*: Initializes `@*views_*@` with `std::move(views)...`.

```cpp
constexpr @_iterator_@<false> begin() requires(!(@_simple-view_@<Views> && ...));
constexpr @_iterator_@<true> begin() const
  requires((range<const Views> && ...) && @_concatable_@<const Views...>);
```

[2]{.pnum} *Effects*: Let `@*is-const*@` be `true` for const-qualified overload, and `false` otherwise. Equivalent to:

```cpp
    @_iterator_@<@_is-const_@> it{this, in_place_index<0>, ranges::begin(get<0>(@*views_*@))};
    it.template @_satisfy_@<0>();
    return it;
```

```cpp
constexpr auto end() requires(!(@_simple-view_@<Views> && ...));
constexpr auto end() const requires(range<const Views>&&...);
```

[3]{.pnum} *Effects*: Let `@*is-const*@` be `true` for const-qualified overload, and `false` otherwise, and let `@_last-view_@` be `@_back_@<const Views...>` for const-qualified overload, and `@_back_@<Views...>` otherwise. Equivalent to:

```cpp
    if constexpr (common_range<@_last-view_@>) {
        constexpr auto N = sizeof...(Views);
        return @_iterator_@<@_is-const_@>{this, in_place_index<N - 1>, 
                          ranges::end(get<N - 1>(@*views_*@))};
    } else if constexpr (random_access_range<@_last-view_@> && sized_range<@_last-view_@>) {
        constexpr auto N = sizeof...(Views);
        return @_iterator_@<@_is-const_@>{
            this, in_place_index<N - 1>,
            ranges::begin(get<N - 1>(@*views_*@)) + ranges::size(get<N - 1>(@*views_*@))};
    } else {
        return default_sentinel;
    }
```

```cpp
constexpr auto size() requires(sized_range<Views>&&...);
constexpr auto size() const requires(sized_range<const Views>&&...);
```

[4]{.pnum} *Effects*: Equivalent to:

```cpp
    return apply(
        [](auto... sizes) {
            using CT = make_unsigned_t<common_type_t<decltype(sizes)...>>;
            return (CT{0} + ... + CT{sizes});
        },
        @_tuple_transform_@(ranges::size, @*views_*@));
```

#### 24.7.?.3 Class concat_view::iterator [range.concat.iterator] {-}


## Feature Test Macro

Add the following macro definition to 17.3.2 [version.syn]{.sref}, header `<version>` synopsis, with the value selected by the editor to reflect the date of adoption of this paper:

```cpp
#define __cpp_lib_ranges_concat  20XXXXL // also in <ranges>
```

---
references:
  - id: rangev3
    citation-label: range-v3
    title: "range-v3 library"
    author:
      - family: Niebler
        given: Eric
    URL: https://github.com/ericniebler/range-v3

  - id: ours
    citation-label: ours
    title: "A proof-of-concept implementation of views::concat"
    author:
      - family: Xie
        given: Hui
      - family: Yilmaz
        given: S. Levent
    URL: https://github.com/huixie90/cpp_papers/tree/main/impl/concat
---
