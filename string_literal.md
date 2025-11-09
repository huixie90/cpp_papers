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

## Deprecate String Literals as non-`viewable_range` or `view::all`

The goal is to make the usage ill-formed (with diagnostic). The usual
procedure to invalidate existing valid code is going through the deprecation
path. An obvious choice to make string literals not usable with ranges library
is to make it not a `viewable_range`. However, this can't achieved by using
`[[deprecated]]` attribute. Another solution is to have a new overload in
`views::all` and mark that overload `[[deprecated]]`

## Make `view::all` to return `string_view` for String Literals



## How to detect a string literal

`"ab"` has the same type as `const char c[] = {'a', 'b', 'c'}`. There is no simple
way to detect it just through the type system.


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
