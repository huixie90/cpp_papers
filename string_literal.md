---
title: "String literals considered harmful in ranges"
document: DXXXXR0
date: 2025-11-09
audience: SG9
author:
  - name: Hui Xie
    email: <hui.xie1990@gmail.com>
  - name: Jonathan Müller
    email: <foonathan@jonathanmueller.dev>
toc: true
---

# Revision History

## R0

- Initial revision

# Abstract

This paper proposes to fix string literal support in ranges library

# Introduction

TODO: add the problem in `to_utfN` and why banning is only a partial solution


#### `ranges::empty`

```cpp
std::ranges::empty(""sv); // true
std::ranges::empty(""s);  // true
std::ranges::empty("");   // false
```

#### `ranges::size`

```cpp
std::ranges::size("abc"sv); // 3
std::ranges::size("abc"s);  // 3
std::ranges::size("abc");   // 4
```

#### `views::split/lazy_split`

```cpp
std::views::split("ab cd ef"sv, " "sv) // split into 3 elements
std::views::split("ab cd ef"sv, " "s)  // split into 3 elements
std::views::split("ab cd ef"sv, " ")   // split into 1 element
```

#### `views::concat`

```cpp
std::views::concat("ab"sv, "cd"sv); // abcd
std::views::concat("ab"s, "cd"s);   // abcd
std::views::concat("ab", "cd");     // ab\0cd\0
```

#### `views::reverse`

```cpp
"abc"sv | std::views::reverse;  // cba
"abc"s  | std::views::reverse;  // cba
"abc"   | std::views::reverse;  // \0cba
```

#### `views::transform`

```cpp
auto atoi_leetcoder = [](char c) -> int { return c - '0'; };

"123"sv | std::views::transform(atoi_leetcoder);  // [1,2,3]
"123"s  | std::views::transform(atoi_leetcoder);  // [1,2,3]
"123"   | std::views::transform(atoi_leetcoder);  // [1,2,3,-48]
```

#### `views::cartesian_product`

```cpp
std::views::cartesian_product("ab"sv, "cd"sv); // ac, ad, bc, bd
std::views::cartesian_product("ab"s, "cd"s);   // ac, ad, bc, bd
std::views::cartesian_product("ab", "cd");     // ac, ad, a\0, bc, bd, b\0, \0c, \0d, \0\0
```

# Proposed Solutions

## Deprecate `ranges::begin` and `ranges::end` for String Literals

We have tried to delete the `const char[N]` overload in `ranges::begin` to see the impact

```cpp
template <size_t _Np>
constexpr auto operator()(const char (&__t)[_Np]) const noexcept = delete;

```

Only 27 tests are failing

```
  llvm-libc++-shared.cfg.in :: std/containers/sequences/deque/deque.modifiers/append_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/containers/sequences/deque/deque.modifiers/assign_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/containers/sequences/deque/deque.modifiers/insert_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/containers/sequences/deque/deque.modifiers/prepend_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/containers/sequences/forwardlist/forwardlist.modifiers/assign_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/containers/sequences/forwardlist/forwardlist.modifiers/insert_range_after.pass.cpp
  llvm-libc++-shared.cfg.in :: std/containers/sequences/forwardlist/forwardlist.modifiers/prepend_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/containers/sequences/list/list.modifiers/append_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/containers/sequences/list/list.modifiers/assign_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/containers/sequences/list/list.modifiers/insert_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/containers/sequences/list/list.modifiers/prepend_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/containers/sequences/vector.bool/append_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/containers/sequences/vector.bool/assign_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/containers/sequences/vector.bool/insert_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/containers/sequences/vector/vector.modifiers/append_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/containers/sequences/vector/vector.modifiers/assign_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/containers/sequences/vector/vector.modifiers/insert_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/ranges/range.adaptors/range.join/lwg3698.pass.cpp
  llvm-libc++-shared.cfg.in :: std/ranges/range.adaptors/range.lazy.split/general.pass.cpp
  llvm-libc++-shared.cfg.in :: std/ranges/range.adaptors/range.lazy.split/view_interface.pass.cpp
  llvm-libc++-shared.cfg.in :: std/ranges/range.adaptors/range.split/ctor.range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/ranges/range.adaptors/range.split/general.pass.cpp
  llvm-libc++-shared.cfg.in :: std/strings/basic.string/string.cons/from_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/strings/basic.string/string.modifiers/string_append/append_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/strings/basic.string/string.modifiers/string_assign/assign_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/strings/basic.string/string.modifiers/string_insert/insert_range.pass.cpp
  llvm-libc++-shared.cfg.in :: std/strings/basic.string/string.modifiers/string_replace/replace_with_range.pass.cpp
```

And all the container tests are failing with the exact same reason: we have this helper in the container tests

```cpp
template <>
constexpr TestCase<char> EmptyContainer_OneElementRange<char>{.initial = {}, .index = 0, .input = "a", .expected = "a"};
```

The test utility has special code to remove the null terminator from the literal.

```cpp
  template <std::size_t N2>
  constexpr Buffer(const char (&input)[N2])
    requires std::same_as<T, char>
  {
    static_assert(N2 <= N);
    std::ranges::copy(input, data_);
    // Omit the terminating null.
    size_ = input[N2 - 1] == '\0' ? N2 - 1 : N2;
  }
```

If the tests were using `string_view`, the special null terminator removing code can also be removed.

And the failed tests in the `range.adaptor` are the tests that we explicitly lock down this null terminator behaviour.

It is unclear that whether or not an user is using string literal with the ranges library, with the intention that they
explicitly want the null terminator in the range.

## Deprecate String Literals as non-`viewable_range` or `view::all`


## Make `view::all` to return `string_view` for String Literals


## How to detect a string literal

`"ab"` has the same type as `const char c[] = {'a', 'b', 'c'}`. There is no simple
way to detect it just through the type system.

### Reflection

TODO: explain reflection can help with detecting string literals in constant expression. but not in general case

### `char` array can be initialised with string literals , but not another `char` array

Ed Catmur had proposed a way to detect a string literal from a `char` array on this Stackoverflow post
https://stackoverflow.com/a/75151972/10915786. That is, a `char` array can be initialised from a string literal
[dcl.init.string]{.sref}, but not from another `char` array.

```cpp
    using T = char[2];

    T x = {'a', '\0'};
    T y1 {"a"}; // OK
    T y2 {x};   // NOK
```

With this, one can create a concept that detects it as shown in Ed's example code.

```cpp
#define IS_SL(x) ([&]<class T = char>() { \
    return std::is_same_v<T const (&)[sizeof(x)], decltype(x)> and \
    requires { std::type_identity_t<T[sizeof(x) + 1]>{x}; }; }())
```


# Wording

# Implementation Experience

# Feature Test Macro


<style>
.bq{
    display: block;
    margin-block-start: 1em;
    margin-block-end: 1em;
    margin-inline-start: 40px;
    margin-inline-end: 40px;
}
</style>
