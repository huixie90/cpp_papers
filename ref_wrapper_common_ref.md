---
title: "`common_reference_t` of `reference_wrapper` Should Be a Reference Type"
document: D2655R2
date: 2023-01-29
audience: SG9, LEWG
author:
  - name: Hui Xie
    email: <hui.xie1990@gmail.com>
  - name: S. Levent Yilmaz
    email: <levent.yilmaz@gmail.com>
toc: true
---

# Revision History
## R2

- Address issues with `common_reference` of any cv-qualified proxy types.
- Replaced Wording paragraphs with code.

## R1

- Added reasons why the result should be `T&`
- Support `const` and `volatile`
- Support derive-base conversions

## R0

- Initial revision.

# Abstract

This paper proposes a fix that makes the `common_reference_t<T&, reference_wrapper<T>>` a reference type `T&`.

# Motivation and Examples

C++20 introduced the meta-programming utility `common_reference`
[meta.trans.other]{.sref} in order to programmatically determine a common
reference type to which one or more types can be converted or bounded.

The precise rules are rather convoluted, but roughly speaking, for given two
non-reference types `X` and `Y`, `common_reference<X&, Y&>` is equivalent to the
expression `decltype(false ? declval<X&>() : declval<Y&>())` provided it is
valid. And if not, then user or a library is free to specialize the
`basic_common_reference` trait for any given type(s). (Two such specializations
are provided by the standard library, namely, for `std::pair` and `std::tuple`
which map `common_reference` to their respective elements.) And if no such
specialization exists, then the result is `common_type<X,Y>`.


The canonical use of `reference_wrapper<T>` is its being a surrogate for `T&`.
So it might be surprising to find out the following:

```cpp
int i = 1, j = 2;
std::reference_wrapper<int> jr = j; // ok - implicit constructor
int & ir = std::ref(i); // ok - implicit conversion
int & r = false ? i : std::ref(j); // error - conditional expression is ambiguous.
```

The reason for the error is not because `i` and `ref(j)`, an `int&` and a
`reference_wrapper<int>`, are incompatible. It is because they are too
compatible! Both types can be converted to one another, so the type of the
ternary expression is ambiguous.

Hence, per the current rules of `common_reference`, given lack of a specialization,
the evaluation falls back to `common_type<T, reference_wrapper<T>>`,
whose `::type` is valid and equal to `T`. In other words, the `common_reference`
type trait utility determines that the reference type to which both `T&` and
a `reference_wrapper<T>` can bind is a prvalue `T`!

The authors believe this current determination logic for `common_reference` for
an lvalue reference to a type `T` and its `reference_wrapper<T>` is merely an
accident, and is incompatible with the canonical purpose of the `reference_wrapper`.
The answer should have been `T&`.  (There is no ambiguity with the `common_reference`
of a prvalue T and reference_wrapper<T>, since former is convertible to latter,
but not vice versa).

This article proposes an update to the standard which would change the behavior
of `common_reference` to evaluate as `T&` given `T&` and an a `reference_wrapper<T>`,
commutatively. Any evolution to implicit conversion semantics of `reference_wrapper`,
or of the ternary operator for that matter, is out of the question. Therefore,
the authors propose to implement this change via providing a partial specialization of
`basic_common_reference` trait.



Below are some motivating examples:

::: cmptable

## C++20

```cpp
static_assert(same_as<
                common_reference_t<int&, reference_wrapper<int>>,
                int>);

static_assert(same_as<
                common_reference_t<int&, reference_wrapper<int>&>,
                int>);
```

## Proposed

```cpp
static_assert(same_as<
                common_reference_t<int&, reference_wrapper<int>>,
                int&>);

static_assert(same_as<
                common_reference_t<int&, reference_wrapper<int>&>,
                int&>);
```

---

```cpp
class MyClass {
    vector<vector<Foo>> foos_;
    Foo delimiter_;

   public:
    void f() {
        auto r = views::join_with(foos_, 
                   views::single(std::ref(delimiter_)));
        for (auto&& foo : r) {
          // foo is a temporary copy
        }
    }
};
```

```cpp
class MyClass {
    vector<vector<Foo>> foos_;
    Foo delimiter_;

   public:
    void f() {
        auto r = views::join_with(foos_, 
                   views::single(std::ref(delimiter_)));
        for (auto&& foo : r) {
          // foo is a reference to the original element
        }
    }
};
```

---

```cpp
class MyClass {
    vector<Foo> foo1s_;
    Foo foo2_;

   public:
    void f() {
        auto r = views::concat(foo1s_, 
                   views::single(std::ref(foo2_)));
        for (auto&& foo : r) {
          // foo is a temporary copy
        }
    }
};
```

```cpp
class MyClass {
    vector<Foo> foo1s_;
    Foo foo2_;

   public:
    void f() {
        auto r = views::concat(foo1s_, 
                   views::single(std::ref(foo2_)));
        for (auto&& foo : r) {
          // foo is a reference to the original element
        }
    }
};
```

:::

In the second and the third example, the user would like to use
`views::join_with` and `views::concat` [@P2542R2], respectively, with a range of
`Foo`s and a single `Foo` for which they use a `reference_wrapper` to avoid
copies. Both of the range adaptors rely on `common_reference_t` in their
respective implementations (and specifications). As a consequence, the
counter-intuitive behavior manifests as shown, where the resultant views'
reference type is a prvalue `Foo`. There does not seem to be any way for the
range adaptor implementations to account for such use cases in isolation.

# Design

## Why Should the Result be `T&` and not `reference_wrapper<T>`

As they can both be converted to each other, the result of `common_reference_t`
can be either of them in theory. However, the authors believe that the users
would expect the result to be `T&`. Given the following example,

```cpp
auto r = views::concat(foos, 
           views::single(std::ref(foo2_)));
for (auto&& foo : r) {
  foo = anotherFoo;
}
```

If the result is `reference_wrapper<T>`, the assignment inside the for loop
would simply rebind the `reference_wrapper` to a different instance. On the
other hand, if the result is `T&`, the assignment would call the copy assignment
operator of the original `foo`s. The authors believe that the latter design is 
the intent of code and is the natural choice.

## Alternatives Considered

The following are some of the alternatives that considered originally. But later
dropped in favor of the one discussed in the next section.

### Option 1: Support Exact Same Type with CV-Ref Variations

One option would be to provide customisations for only `reference_wrapper<T>`
and cv-ref `T`. Note that this version is rather restrictive:

```cpp
template <class T, class U, template <class> class TQual,
          template <class> class UQual>
    requires std::same_as<T, remove_cv_t<U>>
struct basic_common_reference<T, reference_wrapper<U>, TQual, UQual> {
    using type = common_reference_t<TQual<T>, U&>;
};

template <class T, class U, template <class> class TQual,
          template <class> class UQual>
    requires std::same_as<remove_cv_t<T>, U>
struct basic_common_reference<reference_wrapper<T>, U, TQual, UQual> {
    using type = common_reference_t<T&, UQual<U>>;
};
```

### Option 2: Treat `reference_wrapper<T>` as `T&`

This options completely treats `reference_wrapper<T>` as `T&` and delegates
`common_reference<reference_wrapper<T>, U>` to the `common_reference<T&, U>`.
Therefore, it would support any conversions (including derived-base conversion)
that `T&` can do.

```cpp
template <class T, class U, template <class> class TQual, template <class> class UQual>
    requires requires { typename common_reference<TQual<T>, U&>::type; }
struct basic_common_reference<T, reference_wrapper<U>, TQual, UQual> {
    using type = common_reference_t<TQual<T>, U&>;
};

template <class T, class U, template <class> class TQual, template <class> class UQual>
    requires requires { typename common_reference<T&, UQual<U>>::type; }
struct basic_common_reference<reference_wrapper<T>, U, TQual, UQual> {
    using type = common_reference_t<T&, UQual<U>>;
};
```

Immediately, it run into ambiguous specialisation problems for the following example

```cpp
common_reference_t<reference_wrapper<int>, reference_wrapper<int>>;
```

A quick fix is to add another specialisation

```cpp
template <class T, class U, template <class> class TQual, template <class> class UQual>
    requires requires { typename common_reference<T&, U&>::type; }
struct basic_common_reference<reference_wrapper<T>, reference_wrapper<U>, TQual, UQual> {
    using type = common_reference_t<T&, U&>;
};
```

However, this has some recursion problems.

```cpp
common_reference_t<reference_wrapper<reference_wrapper<int>>,
                   reference_wrapper<int>&>;
```

The user would expect the above expression to yield `reference_wrapper<int>&>`.
However it yields `int&` due to the recursion logic in the specialisation.

And even worse,

```cpp
common_reference_t<reference_wrapper<reference_wrapper<int>>,
                   int&>;
```

The above expression would also yield `int&` due to the recursion logic, even
though the nested `reference_wrapper` is not `convertible_to<int&>`.

The rational behind this option is that `reference_wrapper<T>` behaves exactly
the same as `T&`. But does it?

There is conversion from `reference_wrapper<T>` to `T&`, and if the result
requires another conversion, the language does not allow `reference_wrapper<T>`
to be converted to the result.



This would cover majority of the use cases. However, this does not cover the
derive-base conversions, i.e.
`common_reference_t<reference_wrapper<Derived>, Base&>>`. This is a valid use
case and the authors believe that it is important to support it.

## Supporting All Compatible Conversions (Option 3)

The above exposure can be extrapolated to any *cv*-qualified or other cross-type
compatible conversions. That is, if `common_reference_t<U, V>` exists then
`common_reference_t<reference_wrapper<U>, V>` and
`common_reference_t<U, reference_wrapper<V>>` should also exist and be equal to
it, *given* the only additional requirement that `reference_wrapper<U>` or
`reference_wrapper<V>`, respectively, can be also implicitly converted to
`common_reference_t<U,V>`. This statement only applies when the evaluation of
`common_reference_t` falls through to `basic_common_reference` (see next section).


The authors propose to support such behavior by allowing
`basic_common_reference` specialization to delegate the result to that of the
`common_reference_t` of the wrapped type with the other non-wrapper argument.
Furthermore, impose additional constraints on this specialization
to make sure that the `reference_wrapper` is convertible to this result. 

In order to support commutativity, we need to introduce two separate
specializations, and further constrain them to be mutually exclusive in order avoid
ambiguity.

Finally, we have to explicitly disable the edge cases with nested
`reference_wrapper`s since, while `reference_wrapper<reference_wrapper<T>>` is
not `convertible_to<T&>`

## Supporting *cv*-qualified `reference_wrapper` and Other Proxy types

### The Issue with *cv*-qualified Proxy Types

As implied in the previous sections, the rules of the `common_reference` trait
are such that any `basic_common_reference` specialization is consulted only if
some ternary expression of the pair of arguments is ill-formed (see
[meta.trans.other]{.sref}/5.3.1).

More precisely, that ternary expression is denoted by `@*COMMON-REF*@(T1, T2)`,
where `T1` and `T2` are the two arguments of the trait, and `@*COMMON-REF*@` is
a complex macro defined in [meta.trans.other]{.sref}/2. For the cases where both
`T1` and `T2` are l-value references, their `@*COMMON-REF*@` is the union of
their cv-qualifiers applied to both. For example, given `T1` is `const X&` and
`T2` is `Y&` (where `X` and `Y` are non-reference types), the evaluated
expression is `decltype(false ? xc : yc)` where `xc` and `yc` are `const X&` and
`const Y&`, respectively. Note that, the union of cv-qualifiers is `const` and
it is applied to *both arguments* even though originally `T2` is a non-`const`
reference.

The origin and rationale for these contrived rules are rather obscure. But one
consequence in the context of this paper is that there are interesting edge
cases where the `basic_common_reference` treatment do not apply. Take,

```cpp
int i = 3;
const std::reference_wrapper<int> r = i;
int& j = r; // ok.
```

That is, the `const`-ness of `reference_wrapper<int>` itself does not change its
semantics, since it is just a proxy to an `int&`. So, it would be natural to
expect that `int&` should be the common reference of `int&` and `const
reference_wrapper<int>&`, since objects of both types can be assigned to an
`int&`.

However, because of the way `@*COMMON-REF*@` is defined, the evaluated ternary
expression is `decltype(false ? r : jc)`, where `jc` is `@**const**@ int&`. Lo
and behold, this expression is no longer ill-formed and evaluates to `const
int&` (the conversion direction is no longer ambiguous, since
`reference_wrapper<int>` can not be constructed from an `int const&`), and we
get:

```cpp
// in the current standard, as well as with this proposal:
static_assert(std::same_as<
    std::common_reference_t<
       const std::reference_wrapper<int>&,
       int&
       >,
    const int&  // not int& !!
>);
```

This issue exists not only in `reference_wrapper`, but any proxy-like types with
reference cast operators. For example

```cpp
struct A {};

struct B {
    operator A& () const;
};

static_assert(std::same_as<
    std::common_reference_t<A&, const B&>,
    const A&  // not A& !!
>);
```

### The Fix
  
As per SG9's direction, we'd like to fix this issue along with the
`basic_common_referece` treatment in this paper.

Going back to the example above, the builtin ternary operator `?:`
is of type `A&` as expected:

```cpp
A a;
const B b;

static_assert(std::same_as<
    decltype(false ? a : b),
    A&
>);
```

 And, as also explained above, becuase of @*COMMON-REF*@, `commono_reference`
 
  
 why does `common_reference` evaluate to `const A&`? Currently, the result of
`common_reference` of two types is decided in the following order:

- (1). `@*COMMON-REF*@`
- (2). `basic_common_reference`
- (3). Builtin ternary operator `?:`
- (4). `common_type_t`

Note that the origal problem this paper tries to solve is that
`common_reference_t<int&, reference_wrapper<int>>` is prvalue `int`. This is
because (1) and (3) are ill-formed because of the ambiguity and it falls back to
(4) which reproduces prvalue. The solution was to add (2) to make
`reference_wrapper<int>` behaves more like an `int&`

However, the second `const` issue, which is described in this section, is
because (1) is too greedy to pick it up, even though (2) or (3) would have produced
the desired results.

Before going to the details of `@*COMMON-REF*@`, let's see Tim Song's comments on `@*COMMON-REF*@`

> It's important that `common_reference<tuple<int>&, tuple<int>&>` remains
> `tuple<int>&` and not `tuple<int&>`, even though the obvious way of writing
> the `basic_common_reference` specialization for `tuple`s (which is also the
> one in the standard) would yield the latter. Having a layer for the
> trivial/obvious cases before we use the user-specializable component makes it
> more convenient for users, and more importantly, harder to get wrong. So I
> think `@*COMMON-REF*@` was probably not meant to deal with proxy references
> and user-defined conversions at all. It is used only when the types involved
> are reference types, but that doesn't make any sense if it were meant to
> handle things that are convertible to reference types.

The purpose of `@*COMMON-REF*@` is to deal with some trivial/obvious cases for
users who would like to write `basic_common_reference` for their types. This is
why `@*COMMON-REF*@` comes before `basic_common_reference`. These obvious cases
include `common_reference<T&, T&>` produces `T&` and
`common_reference<T&, const T&>` produces `const T&`. However, the way it is
defined also includes user defined conversions and these conversions are never
"trivial/obvious" cases and sometimes produces unexpected results.

One possible solution is to disable `@*COMMON-REF*@` for all user defined
conversions and only allow "trivial" conversions like cv qualification and
derived-to-base conversions. Once `@*COMMON-REF*@` is disabled for user defined
conversions, for `common_reference_t<const reference_wrapper<int>&, int&>`, (2)
`basic_common_reference` would pick it up and produces the expected result
`int&`. For `common_reference_t<A&, const B&>`, (3) builtin ternary operator
`?:` would pick it up and produces the expected result `A&`.

Currently, the `@*COMMON-REF*@` is defined as:

> Given types `A` and `B`, let `X` be `remove_reference_t<A>`, let `Y` be
> `remove_reference_t<B>`, and let `@*COMMON-REF*@(A, B)` be:
>
> - (2.5) If `A` and `B` are both lvalue reference types, `@*COMMON-REF*@(A, B)`
> is `@*COND-RES*@(@*COPYCV*@(X, Y) &, @*COPYCV*@(Y, X) &)` if that type exists
> and is a reference type.
> - (2.6) Otherwise, let `C` be `remove_reference_t<@*COMMON-REF*@(X&, Y&)>&&`.
> If `A` and `B` are both rvalue reference types, `C` is well-formed, and
> `is_convertible_v<A, C> && is_convertible_v<B, C>` is true, then
> `@*COMMON-REF*@(A, B)` is `C`.
> - (2.7) Otherwise, let `D` be `@*COMMON-REF*@(const X&, Y&)`. If `A` is an
> rvalue reference and `B` is an lvalue reference and `D` is well-formed and
> `is_convertible_v<A, D>` is true, then `@*COMMON-REF*@(A, B)` is `D`.
> - (2.8) Otherwise, if `A` is an lvalue reference and B is an rvalue reference,
> then `@*COMMON-REF*@(A, B)` is `@*COMMON-REF*@(B, A)`.
> - (2.9) Otherwise, `@*COMMON-REF*@(A, B)` is ill-formed.

To reject user defined conversions, change (2.5) to additionally require
`is_convertible_v<X*, add_pointer_t<R>>` and
`is_convertible_v<Y*, add_pointer_t<R>>`, where `R` is the result
`@*COND-RES*@(@*COPYCV*@(X, Y) &, @*COPYCV*@(Y, X) &)`. This is enough to fix
the two example issues in this paper. But for consistency, it is better to add
the pointer converions requirements for (2.6) and (2.7) for rvalue reference
cases.


# Implementation Experience

- The authors implemented the proposed wording below without any issue[@ours].

- The authors also applied the proposed wording in LLVM's libc++ and all libc++ tests passed.[@libcxx]

# Wording

Modify [meta.trans.other]{.sref} section (2.5) to (2.9)

Given types `A` and `B`, let `X` be `remove_reference_t<A>`, let `Y` be
`remove_reference_t<B>`, and let `@*COMMON-REF*@(A, B)` be:

- (2.5) [let `R` be `@*COND-RES*@(@*COPYCV*@(X, Y) &, @*COPYCV*@(Y, X) &)`.]{.add}If `A` and `B` are both lvalue reference types, [`@*COMMON-REF*@(A, B)`
is `@*COND-RES*@(@*COPYCV*@(X, Y) &, @*COPYCV*@(Y, X) &)` if that type exists
and is a reference type.]{.rm}[, `R` is well-formed, `R` is a reference type and `is_convertible_v<X*, add_pointer_t<R>> && is_convertible_v<Y*, add_pointer_t<R>>` is `true`, then `@*COMMON-REF*@(A, B)` is `R`.]{.add}
- (2.6) Otherwise, let `C` be `remove_reference_t<@*COMMON-REF*@(X&, Y&)>&&`.
If `A` and `B` are both rvalue reference types, `C` is well-formed, and
`is_convertible_v<A, C> && is_convertible_v<B, C>` [` && is_convertible_v<X*, add_pointer_t<C>> && is_convertible_v<Y*, add_pointer_t<C>>`]{.add} is true, then
`@*COMMON-REF*@(A, B)` is `C`.
- (2.7) Otherwise, let `D` be `@*COMMON-REF*@(const X&, Y&)`. If `A` is an
rvalue reference and `B` is an lvalue reference and `D` is well-formed and
`is_convertible_v<A, D>` [` && is_convertible_v<X*, add_pointer_t<D>> && is_convertible_v<Y*, add_pointer_t<D>>`]{.add} is true, then `@*COMMON-REF*@(A, B)` is `D`.
- (2.8) Otherwise, if `A` is an lvalue reference and B is an rvalue reference,
then `@*COMMON-REF*@(A, B)` is `@*COMMON-REF*@(B, A)`.
- (2.9) Otherwise, `@*COMMON-REF*@(A, B)` is ill-formed.


Modify [functional.syn]{.sref} to add to the end of `reference_wrapper` section:

:::add

```cpp
// @*[refwrap.common.ref] `common_reference` related specializations*@
template <class R, class T, template <class> class RQual, template <class> class TQual>
struct basic_common_reference<R, T, RQual, TQual>;

template <class T, class R, template <class> class TQual, template <class> class RQual>
struct basic_common_reference<T, R, TQual, RQual>;
```

:::

Add the following subclause to [refwrap]{.sref}:

#### ?.?.?.? `common_reference` related specializations [refwrap.common.ref] {-}

The `basic_common_reference` specializations should be constrained and defined as follows:

```cpp
template <class T>
inline constexpr bool @*is-ref-wrapper*@ = false; // exposition only

template <class T>
inline constexpr bool @*is-ref-wrapper*@<reference_wrapper<T>> = true;

template<class R, class T, class RQ, class TQ>
concept @*ref-wrap-common-reference-exists-with*@ = // exposition only
    @*is-ref-wrapper*@<R>
    && requires {
        typename common_reference_t<typename R::type&, TQ>;
    }
    && convertible_to<RQ, common_reference_t<typename R::type&, TQ>>
    ;

template <class R, class T, template <class> class RQual,  template <class> class TQual>
    requires(  @*ref-wrap-common-reference-exists-with*@<R, T, RQual<R>, TQual<T>> 
           && !@*ref-wrap-common-reference-exists-with*@<T, R, TQual<T>, RQual<R>>  )
struct basic_common_reference<R, T, RQual, TQual> {
    using type = common_reference_t<typename R::type&, TQual<T>>;
};

template <class T, class R, template <class> class TQual,  template <class> class RQual>
    requires(  @*ref-wrap-common-reference-exists-with*@<R, T, RQual<R>, TQual<T>> 
           && !@*ref-wrap-common-reference-exists-with*@<T, R, TQual<T>, RQual<R>>  )
struct basic_common_reference<T, R, TQual, RQual> {
    using type = common_reference_t<typename R::type&, TQual<T>>;
};
```

## Feature Test Macro

Add the following macro definition to [version.syn]{.sref}, header `<version>`
synopsis, with the value selected by the editor to reflect the date of adoption
of this paper:

```cpp
#define __cpp_lib_common_reference_reference_wrapper_specialized  20XXXXL // also in <functional>
```

---
references:
  - id: ours
    citation-label: ours
    title: "A proof-of-concept implementation of common_reference_t for reference_wrapper"
    author:
      - family: Xie
        given: Hui
      - family: Yilmaz
        given: S. Levent
    URL: https://github.com/huixie90/cpp_papers/tree/main/impl/ref_wrapper
  - id: libcxx
    citation-label: libcxx
    title: "Implementation of common_reference in libc++"
    author:
      - family: Xie
        given: Hui
      - family: Yilmaz
        given: S. Levent
    URL: https://reviews.llvm.org/D141200
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
