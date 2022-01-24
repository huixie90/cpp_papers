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

This paper proposes `views::concat` as very briefly introduced in
Section 4.7 of [@P2214R1]. It is a view factory that takes an
arbitrary number of ranges as an argument list, and provides a view that starts
at the first element of the first range, ends at the last element of the last
range, with all range elements sequenced in between respectively in the order
given in the arguments, effectively concatenating, or chaining together the
argument ranges.

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
public:
  auto getFoos () const{
    std::vector<std::reference_wrapper<Foo const>> fooRefs;
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
auto fooRefs = myClass.getFoos(); // need extra named value
for(const auto& fooRef: fooRefs | views::filter(pred)){
  // `foo` is std::reference_wrapper<Foo const>
  // ...
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

The first example shows that dealing with multiple ranges require care even in
the simplest of cases: The "Before" version manually concatenates all the
ranges in a formatting string, but empty ranges aren't handled and the result
contains an extra comma and a space. With `concat_view`, empty ranges are
handled per design, and the construct expresses the intent cleanly and
directly.

In the second example, the user has a class composed of fragmented and indirect
data and attempts to implement a member function that provides a range-like
view to all this data sequenced together in some order, and without creating
any copies. The "Before" implementation is needlessly complex, creates a
temporary container, and is not even returning a view. `concat_view` based
implementation is a neat one-liner as this proposed view squarely satisfies this
very use case.


# Design

This is a generator factory as described in [@P2214R1] Section 4.7. As such, it
can not be piped to. It takes the list of ranges to concatenate as arguments to
`ranges::concat_view` constructor, or to `ranges::views::concat` customization
point object.

## Reference type of a `concat_view`

In order to adapt the ranges as a sequence of a same type, the references of
these ranges must be convertible to a common type. For the standards
implementation we propose this requirement be imposed as a constraint on the
arguments of the `concat_view`. (This is an improvement on the existing
implementation provided by [@rangev3], which does not have this as a constraint,
albeit yields a hard compilation error when constructing its iterators.)

This requirement is explicitly formulated in terms of `std::common_reference_t`
of the argument ranges. See the [Wording](#class-template-concat_view-range.concat.view).
This is for practical utility and ease of adoption, and
happens to capture a majority of the use cases.

### Future `concat_view`s for different reference types

There are some potentially important use cases that are left out by the above
common reference formulation, and by this proposal:

1. Concatenating ranges of unrelated types into a sequence of common arbitrary
type. For instance, given a range of `int`s and a range of `string`s, provide a
view of `string`s where `int`s are transformed via `std::itoa`.

2. Concatenate ranges of different subclasses, into a range of their common
base. (Note that, `common_reference` formulation potentially yields a very
surprising counter-intuitive behavior: `concat_view` becomes either valid, if
the first range is that of the common base of the rest, or ill-formed by any
other ordering.)

3. Concatenate ranges of unrelated types into a range of a `std::variant` of
their reference types (proxied for lvalues).

The authors envision that the first one of these cases can be satisfied with a
`concat_transform`, and the second and third with a `concat_as`. Note that,
while `concat_transform(f, r...)` would be semantically equivalent to
`concat(transform(f, r)...)`, it would be best implemented as a full view for
reasons similar to what is described in [@P2214R1] Section 4.1. The last two of
these cases can be satisfied with `concat_as<RefType>`. Again,
`concat_transform` can be reused where `f` is simply a cast. However, a first
class support for a common case such as this may have desirable benefits.
(Note: a more general `concat(as<RefType>(r)...)` may also explored, introducing
the `transform` derivative `as_view` as a generalized version of `as_const` of
[@P2278R1].)

## Zero or one view

- The result of `concat`ing zero range can be an empty range,
but the element type of such range is unspecified.
Therefore `concat`ing zero range is made ill-formed in this proposal.

- The result of `concat`ing one range should be the range itself,
therefore this paper proposes that it returns `views::all(r)`.

## Is `begin` *O*(1)?

<!-- TODO: should we simplify this and related O(N) discussions by arguing that
N is a constant? -->
<!-- I think we should mention that it is O(N) in terms of number
of input range like zip_view, and O(1) in terms of number of elements in the
range unlike filtered_view. Feel free to simply it. Arguing N is a compile time
constant might not be enough (well std::array<t, N> is not a view). I think the
key might be that it is not linear to the number of elements inside the ragnes
-->

On creating the `begin` iterator of the `concat_view`, the position of the
iterator needs to point to the first element of the first non empty range, if
there is any. If the number of input ranges is `N`, `ranges::begin` can be
called `N` times at maximum and the equality comparison operator with the
sentinel can be called `N - 1` times at maximum. However, unlike
`filtered_view` where the number of comparison is linear to the number of
elements in the range at runtime, `concat_view`'s comparison is bounded by the
number of inputs `N`, which is known at compile time, and it is not affected by
the number of elements in each input ranges. `concat_view` is more similar to
`zip_view`, where `ranges::begin` is always called `N` times. Therefore,
`concat_view`'s `begin` function is constant time.


## Borrowed vs Cheap Iterator

`concat_view` can be designed to be a `borrowed_range`, if all the underlying
ranges are. However, this requires the iterator implementation to contain a
copy of all iterators and sentinels of all underlying ranges at all times (just
like that of `views::zip` [@P2321R2]). On the other hand (unlike `views::zip`),
a much cheaper implementation can satisfy all the proposed functionality
provided it is permitted to be unconditionally not borrowed. This
implementation would maintain only a single active iterator at a time and
simply refers to the parent view for the bounds. 

Experience shows the borrowed-ness of `concat` is not a major requirement; and
the existing implementation in [@rangev3] seems to have picked the cheaper
alternative. We follow suit in this proposal.

## Common Range

<!-- TODO: is this needed? -->
<!-- TODO: I think it is good to keep this section. It took me few iterations 
     to figure out this exact condition. At the beginning I thought the requirement
     was that "every" underlying range has to model common_range -->

`concat_view` can be `common_range` if the last underlying range models either

- `common_range`, or
- `random_access_range && sized_range`

## Bidirectional Range

`concat_view` can model `bidirectional_range` if the underlying ranges satisfy the following conditions:

- Every underlying range models `bidirectional_range`  
- If the iterator is at nth range's begin position, after `operator--` it should go to (n-1)th's range's end-1 position. This means, the (n-1)th range has to support either
  - `common_range && bidirectional_range`, so the position can be reached by `--ranges::end(n-1th range)`, assuming n-1th range is not empty, or
  - `random_access_range && sized_range`, so the position can be reached by `ranges::begin(n-1th range) + (ranges::size(n-1th range) - 1)` in constant time, assuming n-1th range is not empty.
  
  Note that the last underlying range does not have to satisfy this constraint because n-1 can never be the last underlying range. If neither of the two constraints is satisfied, in theory we can cache the end-1 position for every single underlying range inside the `concat_view` itself. But the authors do not consider this type of ranges as worth supporting bidirectional

In the `cancat` implementation in [@rangev3], `operator--` is only constrained on all underlying ranges being `bidirectional_range` on the declaration, but its implementation is using `ranges::next(ranges::begin(r), ranges::end(r))` which implicitly requires random access to make the operation constant time. So it went with the second constraint. In this paper, both are supported.

## Random Access Range

`concat_view` can be `random_access_range` if all the underlying ranges model `random_access_range` and `sized_range`.

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
  template <input_range... Views>
    requires @_see below_@
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
    using @_tuple-or-pair_@ = @_see-below_@;                     // exposition only

    template<class F, class Tuple>
    constexpr auto @_tuple-transform_@(F&& f, Tuple&& tuple) { // exposition only
        return apply([&]<class... Ts>(Ts&&... elements) {
            return @_tuple-or-pair_@<invoke_result_t<F&, Ts>...>(
                invoke(f, std::forward<Ts>(elements))...
            );
        }, std::forward<Tuple>(tuple));
    }

    template<class F, class Tuple>
    constexpr void @_tuple-for-each_@(F&& f, Tuple&& tuple) { // exposition only
        apply([&]<class... Ts>(Ts&&... elements) {
            (invoke(f, std::forward<Ts>(elements)), ...);
        }, std::forward<Tuple>(tuple));
    }
}
```

Given some pack of types `Ts`, the alias template `@_tuple-or-pair_@` is defined as follows:

1. If `sizeof...(Ts)` is `2`, `@_tuple-or-pair_@<Ts...>` denotes `pair<Ts...>`.
2. Otherwise, `@_tuple-or-pair_@<Ts...>` denotes `tuple<Ts...>`.

## `concat`

Add the following subclause to [range.adaptors]{.sref}.

### 24.7.? Concat view [range.concat] {-}

#### 24.7.?.1 Overview [range.concat.overview] {-}

[1]{.pnum} `concat_view` presents a `view` that concatenates all the underlying
ranges.

[2]{.pnum} The name `views::concat` denotes a customization point object
([customization.point.object]{.sref}). Given a pack of subexpressions `Es...`,
the expression `views::concat(Es...)` is expression-equivalent to

- [2.1]{.pnum} `views::all(Es...)` if `Es` is a pack with only one element
        and `views::all(Es...)` is a well formed expression,
- [2.2]{.pnum} otherwise, `concat_view(Es...)` if this expression is well formed,
- [2.3]{.pnum} otherwise, ill-formed.

[*Example:*

```cpp
std::vector<int> v1{1,2,3}, v2{4,5}, v3{};
std::array  a{6,7,8};
auto s = std::views::single(9);
for(auto&& i : std::views::concat(v1, v2, v3, a, s)){
  std::cout << i << ' '; // prints: 1 2 3 4 5 6 7 8 9 
}
```
- *end example*]

#### 24.7.?.2 Class template `concat_view` [range.concat.view] {-}

```cpp
namespace std::ranges {

  template <class... Rs>
  concept @_concatable_@ =                        // exposition only
    (convertible_to<range_reference_t<Rs>, 
      common_reference_t<range_reference_t<Rs>...>> && ...);

  template <class... Rs>
  concept @_concat-random-access_@ =              // exposition only
    ((random_access_range<Rs> && sized_range<Rs>)&&...);

  template <class R>
  concept @_constant-time-reversible_@ =          // exposition only
    (bidirectional_range<R> && common_range<R>) ||
    (sized_range<R> && random_access_range<R>);

  template <class... Rs>
  concept @_concat-bidirectional_@ = @_see below_@;   // exposition only

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
template <class... Rs>
concept @_concat-bidirectional_@ = @_see below_@;
```

[1]{.pnum} The pack `Rs...` models `@_concat-bidirectional_@` if,

* [1.1]{.pnum} Last element of `Rs...` models `bidirectional_range`,
* [1.2]{.pnum} And, all except the last element of `Rs...` model `@_constant-time-reversible_@`.

```cpp
constexpr explicit concat_view(Views... views);
```

[2]{.pnum} *Effects*: Initializes `@*views_*@` with `std::move(views)...`.

```cpp
constexpr @_iterator_@<false> begin() requires(!(@_simple-view_@<Views> && ...));
constexpr @_iterator_@<true> begin() const
  requires((range<const Views> && ...) && @_concatable_@<const Views...>);
```

[3]{.pnum} *Effects*: Let `@*is-const*@` be `true` for const-qualified overload, and `false` otherwise. Equivalent to:

```cpp
    @_iterator_@<@_is-const_@> it{this, in_place_index<0>, ranges::begin(get<0>(@*views_*@))};
    it.template @_satisfy_@<0>();
    return it;
```

```cpp
constexpr auto end() requires(!(@_simple-view_@<Views> && ...));
constexpr auto end() const requires(range<const Views>&&...);
```

[4]{.pnum} *Effects*: Let `@*is-const*@` be `true` for const-qualified overload, and `false` otherwise,
and let `@_last-view_@` be the last element of the pack `const Views...` for const-qualified overload,
and the last element of the pack `Views...` otherwise. Equivalent to:

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

[5]{.pnum} *Effects*: Equivalent to:

```cpp
    return apply(
        [](auto... sizes) {
            using CT = make_unsigned_t<common_type_t<decltype(sizes)...>>;
            return (CT{0} + ... + CT{sizes});
        },
        @_tuple-transform_@(ranges::size, @*views_*@));
```

#### 24.7.?.3 Class concat_view::iterator [range.concat.iterator] {-}

```cpp
namespace std::ranges{

  template <input_range... Views>
    requires (view<Views> && ...) && (sizeof...(Views) > 0) &&
              @_concatable_@<Views...>
  template <bool Const>
  class concat_view<Views...>::@_iterator_@ {
  
  public:
    using reference = common_reference_t<range_reference_t<@_maybe-const_@<Const, Views>>...>;
    using value_type = remove_cvref_t<reference>;
    using difference_type = common_type_t<range_reference_t<@_maybe-const_@<Const, Views>>...>;
    using iterator_concept = @*see below*@;
    using iterator_category = @*see below*@;                  // not always present.

  private:
    using @*base_iter*@ =                                     // exposition only
      variant<iterator_t<@_maybe-const_@<Const, Views>>...>;
    
    @_maybe-const_@<Const, concat_view>* @*parent_*@ = nullptr;   // exposition only
    @*base_iter*@ @*it_*@ = @*base_iter*@();                          // exposition only

    friend class @*iterator*@<!Const>;
    friend class concat_view;

    template <std::size_t N>
    constexpr void @_satisfy_@();                             // exposition only

    template <std::size_t N>
    constexpr void @_prev_@();                                // exposition only

    template <std::size_t N>
    constexpr void @_advance_fwd_@(difference_type offset, difference_type steps); // exposition only

    template <std::size_t N>
    constexpr void @_advance_bwd_@(difference_type offset, difference_type steps); // exposition only

  public:

    @_iterator_@() requires(default_initializable<iterator_t<@_maybe-const_@<Const, Views>>>&&...) =
        default;

    template <class... Args>
    explicit constexpr @_iterator_@(
                @_maybe-const_@<Const, concat_view>* parent,
                Args&&... args) 
        requires constructible_from<@*base_iter*@, Args&&...>;

    constexpr @_iterator_@(@_iterator_@<!Const> i) 
        requires Const &&
        (convertible_to<iterator_t<Views>, iterator_t<@_maybe-const_@<Const, Views>>>&&...);

    constexpr reference operator*() const;

    constexpr @_iterator_@& operator++();

    constexpr void operator++(int);

    constexpr @_iterator_@ operator++(int) 
        requires(forward_range<@_maybe-const_@<Const, Views>>&&...);
    
    constexpr @_iterator_@& operator--() 
        requires @_concat-bidirectional_@<@_maybe-const_@<Const, Views>...>;

    constexpr @_iterator_@ operator--(int) 
        requires @_concat-bidirectional_@<@_maybe-const_@<Const, Views>...>

    constexpr @_iterator_@& operator+=(difference_type n) 
        requires @_concat-random-access_@<@_maybe-const_@<Const, Views>...>;

    constexpr @_iterator_@& operator-=(difference_type n) 
        requires @_concat-random-access_@<@_maybe-const_@<Const, Views>...>;

    constexpr reference operator[](difference_type n) const
        requires @_concat-random-access_@<@_maybe-const_@<Const, Views>...>;

    friend constexpr bool operator==(const @_iterator_@& x, const @_iterator_@& y)
        requires(equality_comparable<iterator_t<@_maybe-const_@<Const, Views>>>&&...);

    friend constexpr bool operator==(const @_iterator_@& it, default_sentinel_t);

    friend constexpr bool operator<(const @_iterator_@& x, const @_iterator_@& y)
        requires(random_access_range<@_maybe-const_@<Const, Views>>&&...);

    friend constexpr bool operator>(const @_iterator_@& x, const @_iterator_@& y)
        requires(random_access_range<@_maybe-const_@<Const, Views>>&&...);

    friend constexpr bool operator<=(const @_iterator_@& x, const @_iterator_@& y)
        requires(random_access_range<@_maybe-const_@<Const, Views>>&&...);

    friend constexpr bool operator>=(const @_iterator_@& x, const @_iterator_@& y)
        requires(random_access_range<@_maybe-const_@<Const, Views>>&&...);

    friend constexpr auto operator<=>(const @_iterator_@& x, const @_iterator_@& y)
        requires((random_access_range<@_maybe-const_@<Const, Views>> &&
         three_way_comparable<@_maybe-const_@<Const, Views>>)&&...);

    friend constexpr @_iterator_@ operator+(const @_iterator_@& it, difference_type n)
        requires @_concat-random-access_@<@_maybe-const_@<Const, Views>...>;

    friend constexpr @_iterator_@ operator+(difference_type n, const @_iterator_@& it)
        requires @_concat-random-access_@<@_maybe-const_@<Const, Views>...>;

    friend constexpr @_iterator_@ operator-(const @_iterator_@& it, difference_type n)
        requires @_concat-random-access_@<@_maybe-const_@<Const, Views>...>;

    friend constexpr difference_type operator-(const @_iterator_@& x, const @_iterator_@& y) 
        requires @_concat-random-access_@<@_maybe-const_@<Const, Views>...>;

    friend constexpr difference_type operator-(const @_iterator_@& x, default_sentinel_t) 
        requires @_concat-random-access_@<@_maybe-const_@<Const, Views>...>;

    friend constexpr difference_type operator-(default_sentinel_t, const @_iterator_@& x) 
        requires @_concat-random-access_@<@_maybe-const_@<Const, Views>...>;

    // TODO: iter_swap and iter_move
  };

}
```

[1]{.pnum} `@_iterator_@::iterator_concept` is defined as follows:

- [1.1]{.pnum} If `@_concat-random-access_@<@_maybe-const_@<Const, Views>...>` is modeled, then `iterator_concept` denotes `random_access_iterator_tag`.
- [1.2]{.pnum} Otherwise, if `@_concat-bidirectional_@<@_maybe-const_@<Const, Views>...>` is modeled, then `iterator_concept` denotes `bidrectional_iterator_tag`.
- [1.3]{.pnum} Otherwise, if `(forward_range<@_maybe-const_@<Const, Views>> && ...)` is modeled, then `iterator_concept` denotes `forward_iterator_tag`.
- [1.4]{.pnum} Otherwise, `iterator_concept` denotes `input_iterator_tag`.

[2]{.pnum} The member typedef-name `iterator_category` is defined if and only if `(forward_range<@_maybe-const_@<Const, Views>>&&...)` is modeled. In that case, `iterator::iterator_category` is defined as follows:

- [2.1]{.pnum} If `is_lvalue_reference<reference>` is `true`, then `iterator_category` denotes `iterator_concept`
- [2.2]{.pnum} Otherwise, `iterator_category` denotes `input_iterator_tag`

```cpp
template <std::size_t N>
constexpr void @_satisfy_@();                             // exposition only
```

[3]{.pnum} *Effects*: Equivalent to:

```cpp
    if constexpr (N != (sizeof...(Views) - 1)) {
        if (get<N>(@*it_*@) == ranges::end(get<N>(@*parent_*@->@*views_*@))) {
            @*it_*@.template emplace<N + 1>(ranges::begin(get<N + 1>(@*parent_*@->@*views_*@)));
            @*satisfy*@<N + 1>();
        }
    }
```

```cpp
template <std::size_t N>
constexpr void @_prev_@();                                // exposition only
```

[4]{.pnum} *Effects*: Equivalent to:

```cpp
    if constexpr (N == 0) {
        --get<0>(@*it_*@);
    } else {
        if (get<N>(@*it_*@) == ranges::begin(get<N>(@*parent_*@->@*views_*@))) {
            using prev_view = @_maybe-const_@<Const, tuple_element_t<N - 1, tuple<Views...>>>;
            if constexpr (common_range<prev_view>) {
                @*it_*@.template emplace<N - 1>(ranges::end(get<N - 1>(@*parent_*@->@*views_*@)));
            } else {
                @*it_*@.template emplace<N - 1>(
                    ranges::next(ranges::begin(get<N - 1>(@*parent_*@->@*views_*@)),
                                 ranges::size(get<N - 1>(@*parent_*@->@*views_*@))));
            }
            @_prev_@<N - 1>();
        } else {
            --get<N>(@*it_*@);
        }
    }
```

```cpp
template <std::size_t N>
constexpr void @_advance_bwd_@(difference_type offset, difference_type steps); // exposition only
```

[5]{.pnum} *Effects*: Equivalent to:

```cpp
    if constexpr (N == sizeof...(Views) - 1) {
        get<N>(@*it_*@) += steps;
    } else {
        auto n_size = ranges::size(get<N>(@*parent_*@->@*views_*@));
        if (offset + steps < static_cast<difference_type>(n_size)) {
            get<N>(@*it_*@) += steps;
        } else {
            @*it_*@.template emplace<N + 1>(ranges::begin(get<N + 1>(@*parent_*@->@*views_*@)));
            advance_fwd<N + 1>(0, offset + steps - n_size);
        }
    }
```

```cpp
template <std::size_t N>
constexpr void @_advance_bwd_@(difference_type offset, difference_type steps); // exposition only
```

[6]{.pnum} *Effects*: Equivalent to:

```cpp
    if constexpr (N == 0) {
        get<N>(@*it_*@) -= steps;
    } else {
        if (offset >= steps) {
            get<N>(@*it_*@) -= steps;
        } else {
            @*it_*@.template emplace<N - 1>(ranges::begin(get<N - 1>(@*parent_*@->views_)) +
                                        ranges::size(get<N - 1>(@*parent_*@->views_)));
            advance_bwd<N - 1>(
                static_cast<difference_type>(ranges::size(get<N - 1>(@*parent_*@->views_))),
                steps - offset);
        }
    }
```

```cpp
template <class... Args>
explicit constexpr @_iterator_@(
            @_maybe-const_@<Const, concat_view>* parent,
            Args&&... args) 
    requires constructible_from<@*base_iter*@, Args&&...>;
```

[7]{.pnum} *Effects*: Initializes `@*parent_*@` with `parent`, and initializes `@*it_*@` with `std::forward<Args>(args)...`.

```cpp
constexpr @_iterator_@(@_iterator_@<!Const> i) 
    requires Const &&
    (convertible_to<iterator_t<Views>, iterator_t<@_maybe-const_@<Const, Views>>>&&...);
```

[8]{.pnum} *Effects*: Initializes `@*parent_*@` with `i.@*parent_*@`, and initializes `@*it_*@` with `std::move(i.@*it_*@)`.

```cpp
constexpr reference operator*() const;
```

[9]{.pnum} *Effects*: Equivalent to:

```cpp
    return std::visit([](auto&& it) -> reference { 
        return *it; }, @*it_*@);
```

```cpp
constexpr @_iterator_@& operator++();
```

[10]{.pnum} *Effects*: Let `@*i*@` be `@*it_.index()*@`. Equivalent to:

```cpp
    ++get<@*i*@>(@*it_*@);
    @*satisfy*@<@*i*@>();
    return *this;
```

```cpp
constexpr void operator++(int);
```

[11]{.pnum} *Effects*: Equivalent to:

```cpp
    ++*this;
```

```cpp
constexpr @_iterator_@ operator++(int) 
    requires(forward_range<@_maybe-const_@<Const, Views>>&&...);
```

[12]{.pnum} *Effects*: Equivalent to:

```cpp
    auto tmp = *this;
    ++*this;
    return tmp;
```

```cpp
constexpr @_iterator_@& operator--() 
    requires @_concat-bidirectional_@<@_maybe-const_@<Const, Views>...>;
```

[13]{.pnum} *Effects*: Let `@*i*@` be `@*it_.index()*@`. Equivalent to:

```cpp
    @*prev*@<@*i*@>();
    return *this;
```

```cpp
constexpr @_iterator_@ operator--(int) 
    requires @_concat-bidirectional_@<@_maybe-const_@<Const, Views>...>
```

[14]{.pnum} *Effects*: Equivalent to:

```cpp
    auto tmp = *this;
    --*this;
    return tmp;
```

```cpp
constexpr @_iterator_@& operator+=(difference_type n) 
    requires @_concat-random-access_@<@_maybe-const_@<Const, Views>...>;
```

[15]{.pnum} *Effects*: Let `@*i*@` be `@*it_.index()*@`. Equivalent to:

```cpp
    if(n > 0) {
      @*advance_fwd*@<@*i*@>(get<@*i*@>(@*it_*@) - ranges::begin(get<@*i*@>(@*parent_*@->@*views_*@)), n);
    } else if (n < 0) {
      @*advance_bwd*@<@*i*@>(get<@*i*@>(@*it_*@) - ranges::begin(get<@*i*@>(@*parent_*@->@*views_*@)), -n);
    }
    return *this;
```

```cpp
constexpr @_iterator_@& operator-=(difference_type n) 
    requires @_concat-random-access_@<@_maybe-const_@<Const, Views>...>;
```

[16]{.pnum} *Effects*: Equivalent to:

```cpp
    *this += -n;
    return *this;
```

```cpp
constexpr reference operator[](difference_type n) const
    requires @_concat-random-access_@<@_maybe-const_@<Const, Views>...>;
```

[17]{.pnum} *Effects*: Equivalent to:

```cpp
    return *((*this) + n);
```

```cpp
friend constexpr bool operator==(const @_iterator_@& x, const @_iterator_@& y)
    requires(equality_comparable<iterator_t<@_maybe-const_@<Const, Views>>>&&...);
```

[18]{.pnum} *Effects*: Equivalent to:

```cpp
    return x.@*it_*@ == y.@*it_*@;
```

```cpp
friend constexpr bool operator==(const @_iterator_@& it, default_sentinel_t);
```

[19]{.pnum} *Effects*: Equivalent to:

```cpp
    constexpr auto last_idx = sizeof...(Views) - 1;
    return it.@*it_*@.index() == last_idx &&
           get<last_idx>(it.@*it_*@) == ranges::end(get<last_idx>(it.@*parent_*@->@*views_*@));
```

```cpp
friend constexpr bool operator<(const @_iterator_@& x, const @_iterator_@& y)
    requires(random_access_range<@_maybe-const_@<Const, Views>>&&...);
```

[20]{.pnum} *Effects*: Equivalent to:

```cpp
    return x.@*it_*@ < y.@*it_*@;
```

```cpp
friend constexpr bool operator>(const @_iterator_@& x, const @_iterator_@& y)
    requires(random_access_range<@_maybe-const_@<Const, Views>>&&...);
```

[21]{.pnum} *Effects*: Equivalent to:

```cpp
    return y < x;
```

```cpp
friend constexpr bool operator<=(const @_iterator_@& x, const @_iterator_@& y)
    requires(random_access_range<@_maybe-const_@<Const, Views>>&&...);
```

[22]{.pnum} *Effects*: Equivalent to:

```cpp
    return !(y < x);
```

```cpp
friend constexpr bool operator>=(const @_iterator_@& x, const @_iterator_@& y)
    requires(random_access_range<@_maybe-const_@<Const, Views>>&&...);
```

[23]{.pnum} *Effects*: Equivalent to:

```cpp
    return !(x < y);
```

```cpp
friend constexpr auto operator<=>(const @_iterator_@& x, const @_iterator_@& y)
    requires((random_access_range<@_maybe-const_@<Const, Views>> &&
     three_way_comparable<@_maybe-const_@<Const, Views>>)&&...);
```

[24]{.pnum} *Effects*: Equivalent to:

```cpp
    return x.@*it_*@ <=> y.@*it_*@;
```

```cpp
friend constexpr @_iterator_@ operator+(const @_iterator_@& it, difference_type n)
    requires @_concat-random-access_@<@_maybe-const_@<Const, Views>...>;
```

[25]{.pnum} *Effects*: Equivalent to:

```cpp
    return @_iterator_@{it} += n;
```

```cpp
friend constexpr @_iterator_@ operator+(difference_type n, const @_iterator_@& it)
    requires @_concat-random-access_@<@_maybe-const_@<Const, Views>...>;
```

[26]{.pnum} *Effects*: Equivalent to:

```cpp
    return it + n;
```

```cpp
friend constexpr @_iterator_@ operator-(const @_iterator_@& it, difference_type n)
    requires @_concat-random-access_@<@_maybe-const_@<Const, Views>...>;
```

[27]{.pnum} *Effects*: Equivalent to:

```cpp
    return @*iterator*@{it} -= n;
```

```cpp
friend constexpr difference_type operator-(const @_iterator_@& x, const @_iterator_@& y) 
    requires @_concat-random-access_@<@_maybe-const_@<Const, Views>...>;
```

[28]{.pnum} *Effects*: Let `@*i~x~*@` denote `x.@*it_*@.index()` and `@*i~y~*@` denote `y.@*it_*@.index()`

- [28.1]{.pnum} if `@*i~x~*@ > @*i~y~*@`, let `@*d~y~*@` denote the distance from `get<@*i~y~*@>(y.@*it_*@)` to the end of `get<@*i~y~*@>(y.@*parent_*@.@*views_*@)`,
`@*d~x~*@` denote the distance from the begin of `get<@*i~x~*@>(x.@*parent_*@.@*views_*@)` to `get<@*i~x~*@>(x.@*it_*@)`. For every integer `@*i~y~*@ < @*i*@ < @*i~x~*@`, let `s` denote the sum of the sizes of all the ranges `get<@*i*@>(x.@*parent_*@.@*views_*@)` if there is any, and `0` otherwise, equivalent to

```cpp
    return @*d~y~*@ + s + @*d~x~*@;
```

- [28.2]{.pnum} otherwise, if `@*i~x~*@ < @*i~y~*@`, equivalent to:

```cpp
    return -(y - x);
```

- [28.3]{.pnum} otherwise, equivalent to:

```cpp
    return get<@*i~x~*@>(x.@*it_*@) - get<@*i~y~*@>(y.@*it_*@);
```

```cpp
friend constexpr difference_type operator-(const @_iterator_@& x, default_sentinel_t) 
    requires @_concat-random-access_@<@_maybe-const_@<Const, Views>...>;
```

[29]{.pnum} *Effects*: Let `@*i~x~*@` denote `x.@*it_*@.index()`, `@*d~x~*@` denote the distance from `get<@*i~x~*@>(x.@*it_*@)` to the end of `get<@*i~x~*@>(x.@*parent_*@.@*views_*@)`. For every integer `@*i~x~*@ < @*i*@ < sizeof...(Views)`, let `s` denote the sum of the sizes of all the ranges `get<@*i*@>(x.@*parent_*@.@*views_*@)` if there is any, and `0` otherwise, equivalent to

```cpp
    return -(@*d~x~*@ + s);
```

```cpp
friend constexpr difference_type operator-(default_sentinel_t, const @_iterator_@& x) 
    requires @_concat-random-access_@<@_maybe-const_@<Const, Views>...>;
```

[30]{.pnum} *Effects*: Equivalent to:

```cpp
      return -(x - default_sentinel);
```

```cpp
  // [TODO]
    friend constexpr auto iter_move(const iterator& i) noexcept(see below);
    friend constexpr void iter_swap(const iterator& l, const iterator& r) noexcept(see below)
        requires (indirectly_swappable<iterator_t<maybe-const<Const, First>>> && ... &&
            indirectly_swappable<iterator_t<maybe-const<Const, Views>>>);

```

## Feature Test Macro

Add the following macro definition to [version.syn]{.sref}, header `<version>` synopsis, with the value selected by the editor to reflect the date of adoption of this paper:

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
