---
title: "`any_view`"
document: PXXXXR0
date: 2024-05-27
audience: SG9, LEWG
author:
  - name: Hui Xie
    email: <hui.xie1990@gmail.com>
  - name: S. Levent Yilmaz
    email: <levent.yilmaz@gmail.com>
  - name: Louis Dionne
    email: <ldionne.2@gmail.com>
toc: true
---

# Revision History

## R0

- Initial revision.

# Abstract

This paper proposes a new type-erased `view`: `any_view`.

# Motivation and Examples

From C++20, a lot of `view`s have been introduced into the standard library.
With these `view`s, it is quite easy to create a range of objects. For example,

```cpp
// in MyClass.hpp
class MyClass{
  std::unordered_map<Key, Widget> widgets_;
public:
  auto getWidgets () const {
    return widgets_ | std::views::values
                    | std::views::filter(myFilter);
  }

  // other members
};
```

This works if one writes everything in the header. However, in practice,
in user's non-templated code bases, headers usually contain the declarations,
and implementation details are hidden in the implementation cpp files:

```cpp
// in MyClass.hpp
class MyClass{
  std::unordered_map<Key, Widget> widgets_;
public:
  /* what should be the return type? */ getWidgets() const;

  // other members
};

// in MyClass.cpp

/* what should be the return type? */ MyClass::getWidgets() const {
    return widgets_ | std::views::values
                    | std::views::filter(myFilter);
}
```

However, it is almost impossible to spell the correct type of the return value.
And in fact, to allow the flexibility of future changes, we don't actually
want to spell that particular type of the `view`. We need some type-erased helper
that can easily written in the header and can accept any concrete type of `view`.

There is something very similar: Lambdas are extremely useful but one cannot
spell their types. When we need a type in the API boundary, we often use the type-erased
type `std::function`.

Prior to C++20, that return type is often `std::vector<Widget>`, which enforces ownership.
This also enforces implementations to make copy of all the `Widget`s. On the other hand,
the caller may not care about the ownership at all and all it wants is to iterate through them.

After C++20, that return type is sometimes `std::span<Widget>`, which explicitly says
the caller does not want the ownership. However, one major caveat is that this enforces
contiguous memory. As a result, implementations cannot return the `view` pipelines as
shown in the example.

This paper proposes a new type-erased view `any_view` so that the above function's return type
can be spelled as `any_view<const Widget&>`.

# Design

## What Parameters can Users Configure?

Let's take `std::function` as an example. Its interface seems extremely simple:
the `operator()` and users only need to configure the return type and argument
types. Well, it is a bit more than that:

- Is it `copyable`?
- Does it own the function

After answering all these questions we ended up with three types now:

- `function`
- `move_only_function`
- `function_ref`

For `any_view`, it is much much more complex than that:

- Is it an `input_range`, `forward_range`, `bidirectional_range`, `random_access_range`, or a `contiguous_range` ?
- Is the range `copyable` ?
- Is the iterator `copyable` ?
- Is it a `sized_range` ?
- Is it a `borrowed_range` ?
- Is it a `common_range` ?
- What is the `range_reference_t` ?
- What is the `range_value_t` ?
- What is the `range_rvalue_reference_t` ?
- What is the `range_difference_t` ?

We can easily get combinatorial explosion of types if we follow the same approach of `std::function`. So let's look at the prior arts.

### BOOST.Range `boost::ranges::any_range`

Here is the type declaration

```cpp
template<
    class Value
  , class Traversal
  , class Reference
  , class Difference
  , class Buffer = any_iterator_default_buffer
>
class any_range;
```

It asks users to put `range_reference_t`, `range_value_t` and `range_difference_t`. `Traversal` is equivalent to `iterator_concept` so it decides the traversal category of the range. It does not need
to configure `copyable`, `borrowed_range` and `common_range` because all BOOST.Range ranges are `copyable`, `borrowed_range` and `common_range`. `sized_range` and `range_rvalue_reference_t` are not
considered.

### range-v3 `ranges::views::any_view`

Here is the type declaration

```cpp
enum class category
{
    none = 0,
    input = 1,
    forward = 3,
    bidirectional = 7,
    random_access = 15,
    mask = random_access,
    sized = 16,
};

template<typename Ref, category Cat = category::input>
struct any_view;
```

Here `Cat` handles both the traversal category and `sized_range`. `Ref` is the `range_reference_t`. It
does not allow users to configure the `range_value_t`, `range_difference_t`, `borrowed_range` and `common_range`. `copyable` is mandatory in range-v3.

# Implementation Experience

# Wording

## Feature Test Macro

```

---
references:
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
