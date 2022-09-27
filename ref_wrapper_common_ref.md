---
title: "`common_reference_t` of `reference_wrapper` Should Be a Reference Type"
document: PXXXXX
date: 2022-09-12
audience: SG9, LEWG
author:
  - name: Hui Xie
    email: <hui.xie1990@gmail.com>
  - name: S. Levent Yilmaz
    email: <levent.yilmaz@gmail.com>
toc: true
---

# Revision History

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
`basic_­common_­reference` trait for any given type(s). Two such specializations
are provided by the standard library, namely, for `std::pair` and `std::tuple`
(which map `common_reference` to their respective elements). And if no such
specialization exists, then the result is `common_type<X,Y>`.


The canonical use of `reference_wrapper<T>` is being a surrogate for `T&`. So it
might be surprising to find out the following:

```cpp
int i = 1, j = 2;
std::reference_wrapper<int> jr = j; // ok - implicit constructor
int & ir = std::ref(i); // ok - implicit conversion
int & r = false ? i : std::ref(j); // error!
```

The reason for the error is not because `i` and `ref(j)` (an `int&` and a
`reference_wrapper<int>`) are incompatible. It is because they are too
compatible: Both types can be converted to one another, so the type of the
ternary expression is ambiguous.



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

The first example shows that currently the `common_reference_t` of `T&` and
`reference_wrapper<T>` yields a value type `T`, rather than a reference type.

In the second and the third example, the user would like to use
`views::join_with` and `views::concat` [@P2542R2], respectively, with a range of
`Foo`s and a single `Foo` for which they use a `reference_wrapper` to avoid
copies. Both range adaptors happen to rely on `common_reference_t` in their
implementations. This is the reason why the counter-intuitive behavior manifests
as shown, where the resultant range's reference type is a prvalue `Foo`. There
does not seem to be any way for the range adaptor implementations to account for
such use cases.

# Design

## Why Is `common_reference_t<T&, reference_wrapper<T>>` Not `T&`

The definition of `common_reference` is complicated. See details in
[meta.trans.other]{.sref}. Most part of the logic depends on the type of the
ternary expression `b ? x : y` if it is well formed. However, in the case of
`reference_wrapper`, the following code isn't valid

```cpp
int i = 5;
using t = decltype(false ? i : reference_wrapper<int>(i)); // error
```

The problem is that `int&` can be implicitly converted to
`reference_wrapper<int>` and `reference_wrapper<int>` can also be implicitly
converted to `int&`. There is no way to tell which conversion is better so the
ternary expression is not well formed.

Next `common_reference` checks if `basic_common_reference` is specialised and
the answer is no in this case.

Then `common_reference` falls back to `common_type`. And `common_type` would
`decay` the inputs so we will never have a reference type result.

## The Fix

As `common_reference` allows users to customise its behaviour by specialising the
template `basic_­common_­reference`, it is straightforward to customise
`reference_wrapper` through `basic_­common_­reference`.

# Wording

Modify [functional.syn]{.sref} to add to the end of `reference_wrapper` section:

:::add

> ```
> template<class T, template<class> class TQual, template<class> class UQual>
> struct basic_common_reference<T, reference_wrapper<T>, TQual, UQual>;
> ```

:::

Add the following subclause to [refwrap]{.sref}:

#### 22.10.6.? `common_reference` related specialization [refwrap.common.ref] {-}

```
template<class T, template<class> class TQual, template<class> class UQual>
struct basic_common_reference<T, reference_wrapper<T>, TQual, UQual> {
    using type = std::common_reference_t<TQual<T>, T&>;
};

template<class T, template<class> class TQual, template<class> class UQual>
struct basic_common_reference<reference_wrapper<T>, T, TQual, UQual> {
    using type = std::common_reference_t<UQual<T>, T&>;
};
```

<style>
.bq{
    display: block;
    margin-block-start: 1em;
    margin-block-end: 1em;
    margin-inline-start: 40px;
    margin-inline-end: 40px;
}
</style>
