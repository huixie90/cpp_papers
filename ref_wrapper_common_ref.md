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

::: cmptable

## Before

```cpp
static_assert(same_as<
                common_reference_t<int&, reference_wrapper<int>>,
                int>);

static_assert(same_as<
                common_reference_t<int&, reference_wrapper<int>&>,
                int>);
```

## After

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
`views::join_with` and `views::concat` (Proposed in P2542R2) with a range of
`Foo`s and a single `Foo`. The user passes in a `reference_wrapper` to avoid
copies, because of performance reasons, or simply because copying is not the
right semantic in the application. However, due to both range adaptors use
`common_reference_t` in their implementations, the results are ranges of
temporary copies of `Foo`s.

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

In [functional.syn]{.sref}, at the end of the section

```cpp
// [refwrap], reference_­wrapper
```


Add the following

```cpp
template<class T, template<class> class TQual, template<class> class UQual>
struct basic_common_reference<T, reference_wrapper<T>, TQual, UQual> {
    using type = std::common_reference_t<TQual<T>, T&>;
};

template<class T, template<class> class TQual, template<class> class UQual>
struct basic_common_reference<reference_wrapper<T>, T, TQual, UQual> {
    using type = std::common_reference_t<UQual<T>, T&>;
};
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
