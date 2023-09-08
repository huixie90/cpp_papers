---
title: "`views::concat`"
document: P2542R3
date: 2023-06-07
audience: SG9, LEWG
author:
  - name: Hui Xie
    email: <hui.xie1990@gmail.com>
  - name: S. Levent Yilmaz
    email: <levent.yilmaz@gmail.com>
toc: true
---

# Revision History

## R3

- Redesigned `iter_swap`
- Relaxed `random_access_range` constraints
- Fixed conversions of `difference` types
- Various wording fixes

## R2

- Adding extra semantic constraints in the concept `concat-indirectly-readable`
  to prevent non-equality-preserving behaviour of `operator*` and `iter_move`.

## R1

- Removed the `common_range` support for underlying ranges that are
  `!common_range && random_access_range && sized_range`.

- Introduced extra exposition concepts to simplify the wording that defines
  `concatable`.

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
std::print("[{:n}, {:n}, {:n}, {:n}, {}]\n", 
            v1, v2, v3, a, s); 
// output:  [1, 2, 3, 4, 5, @**,**@ 6, 7, 8, 9]
```

## After

```cpp
std::vector<int> v1{1,2,3}, v2{4,5}, v3{};
std::array  a{6,7,8};
auto s = std::views::single(9);
std::print("{}\n", 
            std::views::concat(v1, v2, v3, a, s)); 
// output:  [1, 2, 3, 4, 5, 6, 7, 8, 9]
```

---

```cpp
class Foo;

class Bar{
public:
  const Foo& getFoo() const;
};

class MyClass{
  std::vector<Foo> foos_;
  Bar bar_;
public:
  auto getFoos () const{
    std::vector<std::reference_wrapper<Foo const>> fooRefs;
    fooRefs.reserve(foos_.size() + 1);

    for (auto const& f : foos_) {
      fooRefs.emplace_back(f);
    }
    fooRefs.emplace_back(bar_.getFoo());
    return fooRefs;
  }
};

// user
for(const auto& foo: myClass.getFoos()){
  // `foo` is std::reference_wrapper<Foo const>, not simply a Foo
  // ...
}
```

```cpp
class Foo;

class Bar{
public:
  const Foo& getFoo() const;
};

class MyClass{
  std::vector<Foo> foos_;
  Bar bar_;
public:
  auto getFoos () const{
    return std::views::concat(
      foos_,
      std::views::single(std::cref(bar_))
      | std::views::transform(&Bar::getFoo)
    );
  }
};

// user
for(const auto& foo: myClass.getFoos()){
  // use foo, which is Foo const &
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
data. They want to implement a member function that provides a range-like view
to all this data sequenced together in some order, and without creating any
copies. The "Before" implementation is needlessly complex and creates a
temporary container with a potentially problematic indirection in its value type. 
`concat_view` based implementation is a neat one-liner.


# Design

This is a generator factory as described in [@P2214R1] Section 4.7. As such, it
can not be piped to. It takes the list of ranges to concatenate as arguments to
`ranges::concat_view` constructor, or to `ranges::views::concat` customization
point object.

## `concatable`-ity of ranges

Adaptability of any given two or more distinct `range`s into a sequence that
itself models a `range`, depends on the compatibility of the reference and the
value types of these ranges. A precise formulation is made in terms of
`std::common_reference_t` and `std::common_type_t`, and is captured by the
exposition only concept `concatable`. See
[Wording](#concatable-definition). Proposed `concat_view`
is then additionally constrained by this concept. (Note that, this is an
improvement over [@rangev3] `concat_view` which lacks such constraints, and
fails with hard errors instead.)

### `reference`

The `reference` type is the `common_reference_t` of all underlying range's
`range_reference_t`. In addition, as the result of `common_reference_t` is not
necessarily a reference type, an extra constraint is needed to make sure that
each underlying range's `range_reference_t` is convertible to that common
reference.

### `value_type`

To support the cases where underlying ranges have proxy iterators, such as
`zip_view`, the `value_type` cannot simply be the `remove_cvref_t` of the
`reference` type, and it needs to respect underlying ranges' `value_type`.
Therefore, in this proposal the `value_type` is defined as the `common_type_t`
of all underlying range's `range_value_t`.

### `range_rvalue_reference_t`

To make `concat_view`'s iterator's `iter_move` behave correctly for the cases
where underlying iterators customise `iter_move`, such as `zip_view`,
`concat_view` has to respect those customizations. Therefore, `concat_view`
requires `common_reference_t` of all underlying ranges's
`range_rvalue_reference_t` exist and can be converted to from each underlying
range's `range_rvalue_reference_t`.

### `indirectly_readable`

In order to make `concat_view` model `input_range`, `reference`, `value_type`,
and `range_rvalue_reference_t` have to be constrained so that the iterator of
the `concat_view` models `indirectly_readable`.

### Mixing ranges of references with ranges of prvalues

In the following example,

```cpp
std::vector<std::string> v = ...;
auto r1 = v | std::views::transform([](auto&& s) -> std::string&& {return std::move(s);});
auto r2 = std::views::iota(0, 2)
            | std::views::transform([](auto i){return std::to_string(i)});
auto cv = std::views::concat(r1, r2);
auto it = cv.begin();
*it; // first deref
*it; // second deref
```

`r1` is a range of which `range_reference_t` is `std::string&&`, while `r2`'s
`range_reference_t` is `std::string`. The `common_reference_t` between these two
`reference`s would be `std::string`. After the "first deref", even though the
result is unused, there is a conversion from `std::string&&` to `std::string`
when the result of underlying iterator's `operator*` is converted to the
`common_reference_t`. This is a move construction and the underlying `vector`'s
element is modified to a moved-from state. Later when the "second deref" is
called, the result is a moved-from `std::string`. This breaks the "equality
preserving" requirements.

Similar to `operator*`, `ranges::iter_move` has this same issue, and it is more
common to run into this problem. For example,

```cpp
std::vector<std::string> v = ...;
auto r = std::views::iota(0, 2)
           | std::views::transform([](auto i){return std::to_string(i)});
auto cv = std::views::concat(v, r);
auto it = cv.begin();
std::ranges::iter_move(it); // first iter_move
std::ranges::iter_move(it); // second iter_move
```

`v`'s `range_rvalue_reference_t` is `std::string&&`, and `r`'s
`range_rvalue_reference_t` is `std::string`, the `common_reference_t` between
them is `std::string`. After the first `iter_move`, the underlying `vector`'s
first element is moved to construct the temporary `common_reference_t`, aka
`std::string`. As a result, the second `iter_move` results in a moved-from state
`std::string`. This breaks the "non-modifying" equality-preserving contract in
`indirectly_readable` concept.

A naive solution for this problem is to ban all the usages of mixing ranges of
references with ranges of prvalues. However, not all of this kind of mixing are
problematic. For example, `concat`ing a range of `std::string&&` with a range of
prvalue `std::string_view` is OK, because converting `std::string&&` to
`std::string_view` does not modify the `std::string&&`. In general, it is not
possible to detect whether the conversion from `T&&` to `U` modifies `T&&`
through syntactic requirements. Therefore, the authors of this paper propose to
use the "equality-preserving" semantic requirements of the requires-expression
and the notational convention that constant lvalues shall not be modified in a
manner observable to equality-preserving as defined in
[concepts.equality]{.sref}. See
[Wording](#concat-indirectly-readable-definition).

### Unsupported Cases and Potential Extensions for the Future

Common type and reference based `concatable` logic is a practical and convenient
solution that satisfies the motivation as outlined, and is what the authors
propose in this paper. However, there are several potentially important use
cases that get left out:

1. Concatenating ranges of different subclasses, into a range of their common
   base.
2. Concatenating ranges of unrelated types into a range of a user-determined
   common type, e.g. a `std::variant`.

Here is an example of the first case where `common_reference` formulation can
manifest a rather counter-intuitive behavior: Let `D1` and `D2` be two types
only related by their common base `B`, and `d1`, `d2`, and `b` be some range of
these types, respectively. `concat(b, d1, d2)` is a well-formed range of `B` by
the current formulation, suggesting such usage is supported. However, a mere
reordering of the sequence, say to `concat(d1, d2, b)`, yields one that is
not.

The authors believe that such cases should be supported, but can only be done so
via an adaptor that needs at least one explicit type argument at its interface.
A future extension may satisfy these use cases, for example a `concat_as` view,
or by even generally via an `as` view that is a type-generalized
version of the `as_const` view of [@P2278R1].

## Zero or one view

- No argument `views::concat()` is ill-formed. It can not be a `views::empty<T>`
  because there is no reasonable way to determine an element type `T`.
- Single argument `views::concat(r)` is expression equivalent to
  `views::all(r)`, which intuitively follows.

## Hidden *O(N)* time complexity for *N* adapted ranges

Time complexities as required by the `ranges` concepts are formally expressed
with respect to the total number of elements (the size) of a given range, and
not to the statically known parameters of that range. Hence, the complexity of
`concat_view` or its iterators' operations are documented to be constant time,
even though some of these are a linear function of the number of ranges it
concatenates which is a statically known parameter of this view.

Some examples of these operations for `concat_view` are copy, `begin` and
`size`, and its iterators' increment, decrement, advance, distance. This
characteristic (but not necessarily the specifics) are very much similar to the
other n-ary adaptors like `zip_view` [@P2321R2] and `cartesian_view`
[@P2374R3].


## Borrowed vs Cheap Iterator

`concat_view` can be designed to be a `borrowed_range`, if all the underlying
ranges are. However, this requires the iterator implementation to contain a copy
of all iterators and sentinels of all underlying ranges at all times (just like
that of `views::zip` [@P2321R2]). On the other hand (unlike `views::zip`), a
much cheaper implementation can satisfy all the proposed functionality provided
it is permitted to be unconditionally not borrowed. This implementation would
maintain only a single active iterator at a time and simply refers to the parent
view for the bounds.

Experience shows the borrowed-ness of `concat` is not a major requirement; and
the existing implementation in [@rangev3] seems to have picked the cheaper
alternative. This paper proposes the same.

## Common Range

`concat_view` can be `common_range` if the last underlying range models `common_range`

## Bidirectional Range

`concat_view` can model `bidirectional_range` if the underlying ranges satisfy
the following conditions:

- Every underlying range models `bidirectional_range`  
- If the iterator is at nth range's begin position, after `operator--` it should
  go to (n-1)th's range's end-1 position. This means, the (n-1)th range has to
  support either
  - `common_range && bidirectional_range`, so the position can be reached by
    `--ranges::end(n-1th range)`, assuming n-1th range is not empty, or
  - `random_access_range && sized_range`, so the position can be reached by
    `ranges::begin(n-1th range) + (ranges::size(n-1th range) - 1)` in constant
    time, assuming n-1th range is not empty.
  
  Note that the last underlying range does not have to satisfy this constraint
  because n-1 can never be the last underlying range. If neither of the two
  constraints is satisfied, in theory we can cache the end-1 position for every
  single underlying range inside the `concat_view` itself. But the authors do
  not consider this type of ranges as worth supporting bidirectional

In the `concat` implementation in [@rangev3], `operator--` is only constrained
on all underlying ranges being `bidirectional_range` on the declaration, but its
implementation is using `ranges::next(ranges::begin(r), ranges::end(r))` which
implicitly requires random access to make the operation constant time. So it
went with the second constraint. In this paper, both are supported.

## Random Access Range

`concat_view` can model `random_access_range` if the underlying ranges satisfy
the following conditions:

- Every underlying range models `random_access_range`  
- All except the last range model `sized_range`

## Sized Range

`concat_view` can be `sized_range` if all the underlying ranges model
`sized_range`

## `iter_swap` Customizations

### Option 1: Delegate to the Underlying `iter_swap`

This option was originally adopted in the paper. If all the combinations of the
underlying iterators model `indirectly_swappable`, then
`ranges::iter_swap(x, y)` could delegate to the underlying iterators

```cpp
std::visit(ranges::iter_swap, x.it_, y.it_);
```

This would allow swapping elements across heterogeneous ranges, which is very
powerful. However, `ranges::iter_swap` is too permissive. Consider the following
example:

```cpp
int v1[] = {1, 2};
long v2[] = {2147483649, 4};
auto cv = std::views::concat(v1, v2);
auto it1 = cv.begin();
auto it2 = cv.begin() + 2;
std::ranges::iter_swap(it1, it2);
```

Delegating
`ranges::iter_swap(const concat_view::iterator&, const concat_view::iterator&)` to
`ranges::iter_swap(int*, long*)` in this case would result in a tripple-move and
leave `v1[0]` overflowed.

Another example discussed in SG9 mailing list is

```cpp
std::string_view v1[] = {"aa"};
std::string v2[] = {"bbb"};
auto cv = std::views::concat(v1, v2);
auto it1 = cv.begin();
auto it2 = cv.begin()+1;
std::ranges::iter_swap(it1, it2);
```

`ranges::iter_swap(string_view*, string*)` in this case would result in dangling
reference due to the tripple move, which is effectively

```cpp
void iter_swap_impl(string_view* x, string* y)
{
  std::string tmp(std::move(*y));
  *y = std::move(*x);
  *x = std::move(tmp); // *x points to local string tmp
}
```

It turned out that allowing swapping elements across heterogeneous ranges could
result in lots of undesired behaviours.

### Option 2: Do not Provide the Customization

If the `iter_swap` customization is removed, the above examples are no longer an
issue because `ranges::iter_swap` would be ill-formed. This is because
`iter_reference_t<concat_view::iterator>` are prvalues in those cases.

For the trivial cases where underlying ranges share the same `iter_reference_t`,
`iter_value_t` and `iter_rvalue_reference_t`, the genereated tripple-move swap
would just work.

However, all the user `iter_swap` customizations will be ignored, even if the
user tries to `concat` the same type of ranges with `iter_swap` customizations.

### Option 3: Delegate to the Underlying `iter_swap` Only If They Have the Same Type

This option was suggested by Tomasz in the SG9 mailing list. The idea is to

- 1. use `ranges::iter_swap(x.it_, y.it_)` if they have the same underlying iterator type
- 2. otherwise, `ranges::swap(*x, *y)` if it is valid

The issues decribed in Option 1 are avoided because we only delegate to
underlying `iter_swap` if they have the same underlying iterators.

The authors believe that this apporach is a good compromise thus it is adopted
in this revision.

## Implementation Experience

`views::concat` has been implemented in [@rangev3], with equivalent semantics as
proposed here. The authors also implemented a version that directly follows the
proposed wording below without any issue [@ours].

# Wording

## Addition to `<ranges>`

Add the following to [ranges.syn]{.sref}, header `<ranges>` synopsis:

```cpp
// [...]
namespace std::ranges {
  // [...]

  // [range.concat], concat view
  template <input_range... Views>
    requires @*see below*@
  class concat_view;

  namespace views {
    inline constexpr @_unspecified_@ concat = @_unspecified_@;
  }

}
```

## Move exposition-only utilities

Move `@*all-random-access*@`, `@*all-bidirectional*@`, and `@*all-forward*@` from [range.zip.iterator]{.sref} to [range.adaptor.helpers]{.sref}.

Add the following to [range.adaptor.helpers]{.sref}

```diff
namespace std::ranges {
// [...]
+ template<bool Const, class... Views>
+   concept @*all-random-access*@ =                 // exposition only
+     (random_access_range<@*maybe-const*@<Const, Views>> && ...);
+ template<bool Const, class... Views>
+   concept @*all-bidirectional*@ =                 // exposition only
+     (bidirectional_range<@*maybe-const*@<Const, Views>> && ...);
+ template<bool Const, class... Views>
+   concept @*all-forward*@ =                       // exposition only
+     (forward_range<@*maybe-const*@<Const, Views>> && ...);
// [...]
}
```

Remove the following from [range.zip.iterator]{.sref}

```diff
namespace std::ranges {
- template<bool Const, class... Views>
-   concept @*all-random-access*@ =                 // exposition only
-     (random_access_range<@*maybe-const*@<Const, Views>> && ...);
- template<bool Const, class... Views>
-   concept @*all-bidirectional*@ =                 // exposition only
-     (bidirectional_range<@*maybe-const*@<Const, Views>> && ...);
- template<bool Const, class... Views>
-   concept @*all-forward*@ =                       // exposition only
-     (forward_range<@*maybe-const*@<Const, Views>> && ...);
// [...]
}
```

## `concat`

Add the following subclause to [range.adaptors]{.sref}.

### ?.?.? Concat view [range.concat] {-}

#### ?.?.?.1 Overview [range.concat.overview] {-}

[1]{.pnum} `concat_view` presents a `view` that concatenates all the underlying
ranges.

[2]{.pnum} The name `views::concat` denotes a customization point object
([customization.point.object]{.sref}). Given a pack of subexpressions `Es...`,
the expression `views::concat(Es...)` is expression-equivalent to

- [2.1]{.pnum} `views::all(Es...)` if `Es` is a pack with only one element
        and `views::all(Es...)` is a well formed expression,
- [2.2]{.pnum} otherwise, `concat_view(Es...)`

\[*Example:*
```cpp
std::vector<int> v1{1,2,3}, v2{4,5}, v3{};
std::array  a{6,7,8};
auto s = std::views::single(9);
for(auto&& i : std::views::concat(v1, v2, v3, a, s)){
  std::cout << i << ' '; // prints: 1 2 3 4 5 6 7 8 9 
}
```
- *end example*\]

#### ?.?.?.2 Class template `concat_view` [range.concat.view] {-}

```cpp
namespace std::ranges {

  template <class... Rs>
  using @*concat-reference-t*@ = common_reference_t<range_reference_t<Rs>...>; // exposition only

  template <class... Rs>
  using @*concat-value-t*@ = common_type_t<range_value_t<Rs>...>; // exposition only

  template <class... Rs>
  using @*concat-rvalue-reference-t*@ = common_reference_t<range_rvalue_reference_t<Rs>...>; // exposition only

  template <class... Rs>
  concept @*concat-indirectly-readable*@ = @*see below*@; // exposition only

  template <class... Rs>
  concept @_concatable_@ = @*see below*@;             // exposition only

  template <bool Const, class... Rs>
  concept @_concat-is-random-access_@ = @*see below*@;   // exposition only

  template <class R>
  concept @_constant-time-reversible_@ =          // exposition only
    (bidirectional_range<R> && common_range<R>) ||
    (sized_range<R> && random_access_range<R>);

  template <bool Const, class... Rs>
  concept @_concat-is-bidirectional_@ = @*see below*@;   // exposition only

  template <input_range... Views>
    requires (view<Views> && ...) && (sizeof...(Views) > 0) &&
              @_concatable_@<Views...>
  class concat_view : public view_interface<concat_view<Views...>> {
    tuple<Views...> @*views_*@;                   // exposition only

    template <bool Const>
    class @_iterator_@;                           // exposition only

  public:
    constexpr concat_view() = default;

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
concept @*concat-indirectly-readable*@ = @*see below*@; // exposition only
```

:::bq

[1]{.pnum} []{#concat-indirectly-readable-definition} The exposition-only
`@*concat-indirectly-readable*@` concept is equivalent to:

```cpp
template <class Ref, class RRef, class It>
concept @*concat-indirectly-readable-impl*@ =  // exposition only
requires (const It it) { 
  { *it } -> convertible_to<Ref>;
  { ranges::iter_move(it) } -> convertible_to<RRef>;
};

template <class... Rs>
concept @*concat-indirectly-readable*@ = // exposition only
  common_reference_with<@*concat-reference-t*@<Rs...>&&, 
                        @*concat-value-t*@<Rs...>&> &&
  common_reference_with<@*concat-reference-t*@<Rs...>&&, 
                        @*concat-rvalue-reference-t*@<Rs...>&&> &&
  common_reference_with<@*concat-rvalue-reference-t*@<Rs...>&&, 
                        @*concat-value-t*@<Rs...> const&> &&
  (@*concat-indirectly-readable-impl*@<@*concat-reference-t*@<Rs...>, 
                                   @*concat-rvalue-reference-t*@<Rs...>, 
                                   iterator_t<Rs>> && ...);
```

:::

```cpp
template <class... Rs>
concept @_concatable_@ = @*see below*@; // exposition only
```

:::bq

[2]{.pnum} []{#concatable-definition} The exposition-only `@_concatable_@`
concept is equivalent to:

```cpp
template <class... Rs>
concept @_concatable_@ = requires { // exposition only
  typename @*concat-reference-t*@<Rs...>;
  typename @*concat-value-t*@<Rs...>;
  typename @*concat-rvalue-reference-t*@<Rs...>;
} && @*concat-indirectly-readable*@<Rs...>;
```

:::

```cpp
template <bool Const, class... Rs>
concept @_concat-is-random-access_@ = @*see below*@; // exposition only
```

:::bq

[3]{.pnum} Let `Fs` be the pack that consists of all elements of `Rs` except the last, then `@_concat-is-random-access_@` is equivalent to:

```cpp
template <bool Const, class... Rs>
concept @_concat-is-random-access_@ = // exposition only
   @*all-random-access*@<Const, Rs...> &&
   (sized_range<@*maybe-const*@<Const, Fs>> && ...);
```

:::

```cpp
template <bool Const, class... Rs>
concept @_concat-is-bidirectional_@ = @*see below*@; // exposition only
```

:::bq

[4]{.pnum} Let `V` be the last element of `Rs`, and `Fs` be the pack that consists of all elements of `Rs` except `V`, then `@_concat-is-bidirectional_@` is equivalent to:

```cpp
template <bool Const, class... Rs>
concept @_concat-is-bidirectional_@ = // exposition only
  (bidirectional_range<@*maybe_const*@<Const, V>> && ... && @*constant-time-reversible*@<@*maybe_const*@<Const, Fs>>);
```

:::

```cpp
constexpr explicit concat_view(Views... views);
```

:::bq

[5]{.pnum} *Effects*: Initializes `@*views_*@` with `std::move(views)...`.

:::

```cpp
constexpr @_iterator_@<false> begin() requires(!(@_simple-view_@<Views> && ...));
constexpr @_iterator_@<true> begin() const
  requires((range<const Views> && ...) && @_concatable_@<const Views...>);
```

:::bq

[6]{.pnum} *Effects*: Let `@*is-const*@` be `true` for the const-qualified overload,
and `false` otherwise. Equivalent to:

```cpp
@_iterator_@<@_is-const_@> it{this, in_place_index<0>, ranges::begin(get<0>(@*views_*@))};
it.template @_satisfy_@<0>();
return it;
```

:::

```cpp
constexpr auto end() requires(!(@_simple-view_@<Views> && ...));
constexpr auto end() const requires(range<const Views>&&...);
```

:::bq

[7]{.pnum} *Effects*: Let `@*is-const*@` be `true` for the const-qualified overload,
and `false` otherwise, and let `@_last-view_@` be the last element of the pack
`const Views...` for the const-qualified overload, and the last element of the pack
`Views...` otherwise. Equivalent to:

```cpp
if constexpr (common_range<@_last-view_@>) {
    constexpr auto N = sizeof...(Views);
    return @_iterator_@<@_is-const_@>{this, in_place_index<N - 1>, 
                      ranges::end(get<N - 1>(@*views_*@))};
} else {
    return default_sentinel;
}
```

:::

```cpp
constexpr auto size() requires(sized_range<Views>&&...);
constexpr auto size() const requires(sized_range<const Views>&&...);
```

:::bq

[8]{.pnum} *Effects*: Equivalent to:

```cpp
return apply(
    [](auto... sizes) {
        using CT = @*make-unsigned-like-t*@<common_type_t<decltype(sizes)...>>;
        return (CT(sizes) + ...);
    },
    @_tuple-transform_@(ranges::size, @*views_*@));
```

:::

#### ?.?.?.3 Class concat_view::iterator [range.concat.iterator] {-}

```cpp
namespace std::ranges{

  template <input_range... Views>
    requires (view<Views> && ...) && (sizeof...(Views) > 0) &&
              @_concatable_@<Views...>
  template <bool Const>
  class concat_view<Views...>::@_iterator_@ {
  
  public:
    using iterator_category = @*see below*@;                  // not always present.
    using iterator_concept = @*see below*@;
    using value_type = @*concat-value-t*@<@_maybe-const_@<Const, Views>...>;
    using difference_type = common_type_t<range_difference_t<@_maybe-const_@<Const, Views>>...>;

  private:
    using @*base-iter*@ =                                     // exposition only
      variant<iterator_t<@_maybe-const_@<Const, Views>>...>;
    
    @_maybe-const_@<Const, concat_view>* @*parent_*@ = nullptr;   // exposition only
    @*base-iter*@ @*it_*@;                                        // exposition only

    template <std::size_t N>
    constexpr void @_satisfy_@();                             // exposition only

    template <std::size_t N>
    constexpr void @_prev_@();                                // exposition only

    template <std::size_t N>
    constexpr void @_advance-fwd_@(difference_type offset, difference_type steps); // exposition only

    template <std::size_t N>
    constexpr void @_advance-bwd_@(difference_type offset, difference_type steps); // exposition only

    template <class... Args>
    explicit constexpr @_iterator_@(@_maybe-const_@<Const, concat_view>* parent, Args&&... args) 
        requires constructible_from<@*base-iter*@, Args&&...>;  // exposition only

  public:

    @_iterator_@() = default;

    constexpr @_iterator_@(@_iterator_@<!Const> i) 
        requires Const && (convertible_to<iterator_t<Views>, iterator_t<const Views>> && ...);

    constexpr decltype(auto) operator*() const;

    constexpr @_iterator_@& operator++();

    constexpr void operator++(int);

    constexpr @_iterator_@ operator++(int) 
        requires @*all-forward*@<Const, Views...>;
    
    constexpr @_iterator_@& operator--() 
        requires @_concat-is-bidirectional_@<Const, Views...>;

    constexpr @_iterator_@ operator--(int) 
        requires @_concat-is-bidirectional_@<Const, Views...>;

    constexpr @_iterator_@& operator+=(difference_type n) 
        requires @_concat-is-random-access_@<Const, Views...>;

    constexpr @_iterator_@& operator-=(difference_type n) 
        requires @_concat-is-random-access_@<Const, Views...>;

    constexpr decltype(auto) operator[](difference_type n) const
        requires @_concat-is-random-access_@<Const, Views...>;

    friend constexpr bool operator==(const @_iterator_@& x, const @_iterator_@& y)
        requires(equality_comparable<iterator_t<@_maybe-const_@<Const, Views>>>&&...);

    friend constexpr bool operator==(const @_iterator_@& it, default_sentinel_t);

    friend constexpr bool operator<(const @_iterator_@& x, const @_iterator_@& y)
        requires @*all-random-access*@<Const, Views...>;

    friend constexpr bool operator>(const @_iterator_@& x, const @_iterator_@& y)
        requires @*all-random-access*@<Const, Views...>;

    friend constexpr bool operator<=(const @_iterator_@& x, const @_iterator_@& y)
        requires @*all-random-access*@<Const, Views...>;

    friend constexpr bool operator>=(const @_iterator_@& x, const @_iterator_@& y)
        requires @*all-random-access*@<Const, Views...>;

    friend constexpr auto operator<=>(const @_iterator_@& x, const @_iterator_@& y)
        requires (@*all-random-access*@<Const, Views...> &&
         (three_way_comparable<@_maybe-const_@<Const, Views>> &&...));

    friend constexpr @_iterator_@ operator+(const @_iterator_@& it, difference_type n)
        requires @_concat-is-random-access_@<Const, Views...>;

    friend constexpr @_iterator_@ operator+(difference_type n, const @_iterator_@& it)
        requires @_concat-is-random-access_@<Const, Views...>;

    friend constexpr @_iterator_@ operator-(const @_iterator_@& it, difference_type n)
        requires @_concat-is-random-access_@<Const, Views...>;

    friend constexpr difference_type operator-(const @_iterator_@& x, const @_iterator_@& y) 
        requires @_concat-is-random-access_@<Const, Views...>;

    friend constexpr difference_type operator-(const @_iterator_@& x, default_sentinel_t) 
        requires @*see below*@;

    friend constexpr difference_type operator-(default_sentinel_t, const @_iterator_@& x) 
        requires @*see below*@;

    friend constexpr decltype(auto) iter_move(const iterator& it) noexcept(@*see below*@);

    friend constexpr void iter_swap(const iterator& x, const iterator& y) noexcept(@*see below*@)
        requires @*see below*@;
  };

}
```

[1]{.pnum} `@_iterator_@::iterator_concept` is defined as follows:

- [1.1]{.pnum} If `@_concat-is-random-access_@<Const, Views...>`
  is modeled, then `iterator_concept` denotes `random_access_iterator_tag`.
- [1.2]{.pnum} Otherwise, if
  `@_concat-is-bidirectional_@<Const, Views...>` is modeled, then
  `iterator_concept` denotes `bidirectional_iterator_tag`.
- [1.3]{.pnum} Otherwise, if `@*all-forward*@<Const, Views...>` is modeled, then
  `iterator_concept` denotes `forward_iterator_tag`.
- [1.4]{.pnum} Otherwise, `iterator_concept` denotes `input_iterator_tag`.

[2]{.pnum} The member typedef-name `iterator_category` is defined if and only if
`@*all-forward*@<Const, Views...>` is modeled. In that case,
`iterator::iterator_category` is defined as follows:

- [2.1]{.pnum} If `is_reference_v<@*concat-reference-t*@<@_maybe-const_@<Const, Views>...>>` is `false`, then
  `iterator_category` denotes `input_iterator_tag`
- [2.2]{.pnum} Otherwise, let `Cs` denote the pack of types
  `iterator_traits<iterator_t<@*maybe-const*@<Const, Views>>>::iterator_category...`.
  - [2.2.1]{.pnum} If
    `(derived_from<Cs, random_access_iterator_tag> && ...) && @_concat-is-random-access_@<Const, Views...>`
    is true, `iterator_category` denotes `random_access_iterator_tag`.
  - [2.2.2]{.pnum} Otherwise, if
    `(derived_from<Cs, bidirectional_iterator_tag> && ...) && @_concat-is-bidirectional_@<Const, Views...>`
    is true, `iterator_category` denotes `bidirectional_iterator_tag`.
  - [2.2.3]{.pnum} Otherwise, If `(derived_from<Cs, forward_iterator_tag> && ...)` is true, `iterator_category` denotes `forward_iterator_tag`.
  - [2.2.4]{.pnum} Otherwise, `iterator_category` denotes `input_iterator_tag`.

```cpp
template <std::size_t N>
constexpr void @_satisfy_@();                             // exposition only
```

:::bq

[3]{.pnum} *Effects*: Equivalent to:

```cpp
if constexpr (N < (sizeof...(Views) - 1)) {
    if (get<N>(@*it_*@) == ranges::end(get<N>(@*parent_*@->@*views_*@))) {
        @*it_*@.template emplace<N + 1>(ranges::begin(get<N + 1>(@*parent_*@->@*views_*@)));
        @*satisfy*@<N + 1>();
    }
}
```

:::

```cpp
template <std::size_t N>
constexpr void @_prev_@();                                // exposition only
```

:::bq

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

:::

```cpp
template <std::size_t N>
constexpr void @_advance-fwd_@(difference_type offset, difference_type steps); // exposition only
```

:::bq

[5]{.pnum} *Effects*: Equivalent to:

```cpp
using underlying_diff_type = iter_difference_t<variant_alternative_t<N, @*base-iter*@>>;
if constexpr (N == sizeof...(Views) - 1) {
    get<N>(@*it_*@) += static_cast<underlying_diff_type>(steps);
} else {
    auto n_size = ranges::distance(get<N>(@*parent_*@->@*views_*@));
    if (offset + steps < static_cast<difference_type>(n_size)) {
        get<N>(@*it_*@) += static_cast<underlying_diff_type>(steps);
    } else {
        @*it_*@.template emplace<N + 1>(ranges::begin(get<N + 1>(@*parent_*@->@*views_*@)));
        @*advance-fwd*@<N + 1>(0, offset + steps - static_cast<difference_type>(n_size));
    }
}
```

:::

```cpp
template <std::size_t N>
constexpr void @_advance-bwd_@(difference_type offset, difference_type steps); // exposition only
```

:::bq

[6]{.pnum} *Effects*: Equivalent to:

```cpp
using underlying_diff_type = iter_difference_t<variant_alternative_t<N, @*base-iter*@>>;
if constexpr (N == 0) {
    get<N>(@*it_*@) -= static_cast<underlying_diff_type>(steps);
} else {
    if (offset >= steps) {
        get<N>(@*it_*@) -= static_cast<underlying_diff_type>(steps);
    } else {
        auto prev_size = ranges::distance(get<N - 1>(@*parent_*@->@*views_*@));
        @*it_*@.template emplace<N - 1>(ranges::begin(get<N - 1>(@*parent_*@->@*views_*@)) + prev_size);
        @*advance-bwd*@<N - 1>(
            static_cast<difference_type>(prev_size),
            steps - offset);
    }
}
```

:::

```cpp
template <class... Args>
explicit constexpr @_iterator_@(
            @_maybe-const_@<Const, concat_view>* parent,
            Args&&... args) 
    requires constructible_from<@*base-iter*@, Args&&...>; // exposition only
```

:::bq

[7]{.pnum} *Effects*: Initializes `@*parent_*@` with `parent`, and initializes
`@*it_*@` with `std::forward<Args>(args)...`.

:::

```cpp
constexpr @_iterator_@(@_iterator_@<!Const> i) 
    requires Const &&
    (convertible_to<iterator_t<Views>, iterator_t<const Views>>&&...);
```

:::bq

[8]{.pnum} *Effects*: Initializes `@*parent_*@` with `i.@*parent_*@`, and
initializes `@*it_*@` with `std::move(i.@*it_*@)`.

:::

```cpp
constexpr decltype(auto) operator*() const;
```

:::bq

[9]{.pnum} *Preconditions*: `@*it_*@.valueless_by_exception()` is `false`.

[10]{.pnum} *Effects*: Equivalent to:

```cpp
using reference = @*concat-reference-t*@<@_maybe-const_@<Const, Views>...>;
return std::visit([](auto&& it) -> reference { 
    return *it; }, @*it_*@);
```

:::

```cpp
constexpr @_iterator_@& operator++();
```

:::bq

[11]{.pnum} *Preconditions*: `@*it_*@.valueless_by_exception()` is `false`.

[12]{.pnum} *Effects*: Let `@*i*@` be `@*it_*@.index()`. Equivalent to:

```cpp
++get<@*i*@>(@*it_*@);
@*satisfy*@<@*i*@>();
return *this;
```

:::

```cpp
constexpr void operator++(int);
```

:::bq

[13]{.pnum} *Effects*: Equivalent to:

```cpp
++*this;
```

:::

```cpp
constexpr @_iterator_@ operator++(int) 
    requires @*all-forward*@<Const, Views...>;
```

:::bq

[14]{.pnum} *Effects*: Equivalent to:

```cpp
auto tmp = *this;
++*this;
return tmp;
```

:::

```cpp
constexpr @_iterator_@& operator--() 
    requires @_concat-is-bidirectional_@<Const, Views...>;
```

:::bq

[15]{.pnum} *Preconditions*: `@*it_*@.valueless_by_exception()` is `false`.

[16]{.pnum} *Effects*: Let `@*i*@` be `@*it_*@.index()`. Equivalent to:

```cpp
@*prev*@<@*i*@>();
return *this;
```

:::

```cpp
constexpr @_iterator_@ operator--(int) 
    requires @_concat-is-bidirectional_@<Const, Views...>;
```

:::bq

[17]{.pnum} *Effects*: Equivalent to:

```cpp
auto tmp = *this;
--*this;
return tmp;
```

:::

```cpp
constexpr @_iterator_@& operator+=(difference_type n) 
    requires @_concat-is-random-access_@<Const, Views...>;
```

:::bq

[18]{.pnum} *Preconditions*: `@*it_*@.valueless_by_exception()` is `false`.

[19]{.pnum} *Effects*: Let `@*i*@` be `@*it_*@.index()`. Equivalent to:

```cpp
if(n > 0) {
  @*advance-fwd*@<@*i*@>(static_cast<difference_type(get<@*i*@>(@*it_*@) - ranges::begin(get<@*i*@>(@*parent_*@->@*views_*@))), n);
} else if (n < 0) {
  @*advance-bwd*@<@*i*@>(static_cast<difference_type(get<@*i*@>(@*it_*@) - ranges::begin(get<@*i*@>(@*parent_*@->@*views_*@))), -n);
}
return *this;
```

:::

```cpp
constexpr @_iterator_@& operator-=(difference_type n) 
    requires @_concat-is-random-access_@<Const, Views...>;
```

:::bq

[20]{.pnum} *Effects*: Equivalent to:

```cpp
*this += -n;
return *this;
```

:::

```cpp
constexpr decltype(auto) operator[](difference_type n) const
    requires @_concat-is-random-access_@<Const, Views...>;
```

:::bq

[21]{.pnum} *Effects*: Equivalent to:

```cpp
return *((*this) + n);
```

:::

```cpp
friend constexpr bool operator==(const @_iterator_@& x, const @_iterator_@& y)
    requires(equality_comparable<iterator_t<@_maybe-const_@<Const, Views>>>&&...);
```

:::bq

[22]{.pnum} *Preconditions*: `x.@*it_*@.valueless_by_exception()` and
`y.@*it_*@.valueless_by_exception()` are each `false`.

[23]{.pnum} *Effects*: Equivalent to:

```cpp
return x.@*it_*@ == y.@*it_*@;
```

:::

```cpp
friend constexpr bool operator==(const @_iterator_@& it, default_sentinel_t);
```

:::bq

[24]{.pnum} *Preconditions*: `it.@*it_*@.valueless_by_exception()` is `false`.

[25]{.pnum} *Effects*: Equivalent to:

```cpp
constexpr auto last_idx = sizeof...(Views) - 1;
return it.@*it_*@.index() == last_idx &&
       get<last_idx>(it.@*it_*@) == ranges::end(get<last_idx>(it.@*parent_*@->@*views_*@));
```

:::

```cpp
friend constexpr bool operator<(const @_iterator_@& x, const @_iterator_@& y)
    requires @*all-random-access*@<Const, Views...>;
friend constexpr bool operator>(const @_iterator_@& x, const @_iterator_@& y)
    requires @*all-random-access*@<Const, Views...>;
friend constexpr bool operator<=(const @_iterator_@& x, const @_iterator_@& y)
    requires @*all-random-access*@<Const, Views...>;
friend constexpr bool operator>=(const @_iterator_@& x, const @_iterator_@& y)
    requires @*all-random-access*@<Const, Views...>;
friend constexpr auto operator<=>(const @_iterator_@& x, const @_iterator_@& y)
   requires (@*all-random-access*@<Const, Views...> &&
    (three_way_comparable<@_maybe-const_@<Const, Views>> &&...));
```

:::bq

[26]{.pnum} *Preconditions*: `x.@*it_*@.valueless_by_exception()` and
`y.@*it_*@.valueless_by_exception()` are each `false`.

[27]{.pnum} Let `@*op*@` be the operator.

[28]{.pnum} *Effects*: Equivalent to:

```cpp
return x.@*it_*@ @*op*@ y.@*it_*@;
```

:::

```cpp
friend constexpr @_iterator_@ operator+(const @_iterator_@& it, difference_type n)
    requires @_concat-is-random-access_@<Const, Views...>;
```

:::bq

[29]{.pnum} *Preconditions*: `it.@*it_*@.valueless_by_exception()` is `false`.

[30]{.pnum} *Effects*: Equivalent to:

```cpp
return @_iterator_@{it} += n;
```

:::

```cpp
friend constexpr @_iterator_@ operator+(difference_type n, const @_iterator_@& it)
    requires @_concat-is-random-access_@<Const, Views...>;
```

:::bq

[31]{.pnum} *Effects*: Equivalent to:

```cpp
return it + n;
```

:::

```cpp
friend constexpr @_iterator_@ operator-(const @_iterator_@& it, difference_type n)
    requires @_concat-is-random-access_@<Const, Views...>;
```

:::bq

[32]{.pnum} *Effects*: Equivalent to:

```cpp
return @*iterator*@{it} -= n;
```

:::

```cpp
friend constexpr difference_type operator-(const @_iterator_@& x, const @_iterator_@& y) 
    requires @_concat-is-random-access_@<Const, Views...>;
```

:::bq

[33]{.pnum} *Preconditions*: `x.@*it_*@.valueless_by_exception()` and
`y.@*it_*@.valueless_by_exception()` are each `false`.

[34]{.pnum} *Effects*: Let `@*i~x~*@` denote `x.@*it_*@.index()` and `@*i~y~*@`
denote `y.@*it_*@.index()`

- [34.1]{.pnum} if `@*i~x~*@ > @*i~y~*@`, let `@*d~y~*@` denote the distance
  from `get<@*i~y~*@>(y.@*it_*@)` to the end of
  `get<@*i~y~*@>(y.@*parent_*@.@*views_*@)`, `@*d~x~*@` denote the distance from
  the begin of `get<@*i~x~*@>(x.@*parent_*@.@*views_*@)` to
  `get<@*i~x~*@>(x.@*it_*@)`. For every integer `@*i~y~*@ < @*i*@ < @*i~x~*@`,
  let `s` denote the sum of the sizes of all the ranges
  `get<@*i*@>(x.@*parent_*@.@*views_*@)` if there is any, and `0` otherwise,
  equivalent to

  ```cpp
  return static_cast<difference_type>(@*d~y~*@) + s + static_cast<difference_type>(@*d~x~*@);
  ```

- [34.2]{.pnum} otherwise, if `@*i~x~*@ < @*i~y~*@`, equivalent to:

  ```cpp
  return -(y - x);
  ```

- [34.3]{.pnum} otherwise, equivalent to:

  ```cpp
  return static_cast<difference_type>(get<@*i~x~*@>(x.@*it_*@) - get<@*i~y~*@>(y.@*it_*@));
  ```

:::

```cpp
friend constexpr difference_type operator-(const @_iterator_@& x, default_sentinel_t) 
    requires @*see below*@;
```

:::bq

[35]{.pnum} *Preconditions*: `x.@*it_*@.valueless_by_exception()` is `false`.

[36]{.pnum} *Effects*: Let `@*i~x~*@` denote `x.@*it_*@.index()`, `@*d~x~*@`
denote the distance from `get<@*i~x~*@>(x.@*it_*@)` to the end of
`get<@*i~x~*@>(x.@*parent_*@.@*views_*@)`. For every integer
`@*i~x~*@ < @*i*@ < sizeof...(Views)`, let `s` denote the sum of the sizes of
all the ranges `get<@*i*@>(x.@*parent_*@.@*views_*@)` if there is any, and `0`
otherwise, equivalent to

```cpp
return -(static_cast<difference_type>(@*d~x~*@) + static_cast<difference_type>(s));
```

[37]{.pnum} *Remarks*: Let `V` be the last element of the pack `Views`, the expression in the requires-clause is equivalent to:

```cpp
(@_concat-is-random-access_@<Const, Views...> && sized_range<@*maybe-const*@<Const, V>>)
```

:::

```cpp
friend constexpr difference_type operator-(default_sentinel_t, const @_iterator_@& x) 
    requires @*see below*@;
```

:::bq

[38]{.pnum} *Effects*: Equivalent to:

```cpp
return -(x - default_sentinel);
```

[39]{.pnum} *Remarks*: Let `V` be the last element of the pack `Views`, the expression in the requires-clause is equivalent to:

```cpp
(@_concat-is-random-access_@<Const, Views...> && sized_range<@*maybe-const*@<Const, V>>)
```

:::

```cpp
friend constexpr decltype(auto) iter_move(const iterator& it) noexcept(@*see below*@);
```

:::bq

[40]{.pnum} *Preconditions*: `it.@*it_*@.valueless_by_exception()` is `false`.

[41]{.pnum} *Effects*: Equivalent to:

```cpp
return std::visit(
    [](const auto& i) ->
        @*concat-rvalue-reference-t*@<@_maybe-const_@<Const, Views>...> { 
        return ranges::iter_move(i);
    },
    it.@*it_*@);
```

[42]{.pnum} *Remarks*: The exception specification is equivalent to:

```cpp
((is_nothrow_invocable_v<decltype(ranges::iter_move), 
                           const iterator_t<@_maybe-const_@<Const, Views>>&> &&
  is_nothrow_convertible_v<range_rvalue_reference_t<@_maybe-const_@<Const, Views>>,
                             common_reference_t<range_rvalue_reference_t<
                               @_maybe-const_@<Const, Views>>...>>) &&...)
```

:::

```cpp
friend constexpr void iter_swap(const iterator& x, const iterator& y) noexcept(@*see below*@)
    requires @*see below*@;
```

:::bq

[43]{.pnum} *Preconditions*: `x.@*it_*@.valueless_by_exception()` and
`y.@*it_*@.valueless_by_exception()` are each `false`.

[44]{.pnum} *Effects*: Equivalent to:

```cpp
std::visit(
    [&](const auto& it1, const auto& it2) {
        if constexpr (is_same_v<decltype(it1), decltype(it2)>) {
            ranges::iter_swap(it1, it2);
        } else {
            ranges::swap(*x, *y);
        }
    },
    x.@*it_*@, y.@*it_*@);
```

[45]{.pnum} *Remarks*: Let `N` be the logcial `AND` of the following expressions:

```cpp
noexcept(ranges::iter_swap(std::get<@*i*@>(x.@*it_*@), std::get<@*i*@>(y.@*it_*@)))
```

for every integer 0 <= `@*i*@` < `sizeof...(Views)`, the exception specification is equavalent to

```cpp
noexcept(ranges::swap(*x, *y)) && N
```

[46]{.pnum} *Remarks*: The expression in the requires-clause is equivalent to

```cpp
(indirectly_swappable<iterator_t<@*maybe-const*@<Const, Views>>> && ... &&
 swappable_with<@*concat-reference-t*@<@*maybe-const*@<Const, Views>...>,
                @*concat-reference-t*@<@*maybe-const*@<Const, Views>...>>)
```

:::

## Feature Test Macro

Add the following macro definition to [version.syn]{.sref}, header `<version>`
synopsis, with the value selected by the editor to reflect the date of adoption
of this paper:

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


<style>
.bq{
    display: block;
    margin-block-start: 1em;
    margin-block-end: 1em;
    margin-inline-start: 40px;
    margin-inline-end: 40px;
}
</style>
