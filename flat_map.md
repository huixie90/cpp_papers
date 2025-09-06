---
title: "flat_meow Fixes"
document: P3567R1
date: 2025-09-06
audience: LEWG, LWG
author:
  - name: Hui Xie
    email: <hui.xie1990@gmail.com>
  - name: Louis Dionne
    email: <ldionne.2@gmail.com>
  - name: Arthur O'Dwyer
    email: <arthur.j.odwyer@gmail.com>
toc: true
---

# Revision History

## R1

- Wording Fixes

# Abstract

This paper proposes a subset of the fixes in [@P2767R2] to `flat_map`, `flat_multimap`, 
`flat_set`, and `flat_multiset` based on libc++'s implementation.

# Introduction

[@P2767R2] proposed a set of changes to `flat_map`, `flat_multimap`, `flat_set`, and `flat_multiset`,  based on the author's
implementation experience. However, the paper contains not only some straight forward
fixes, but also some design changes, which did not get consensus in previous meetings.
We (libc++) have recently released the `flat_map` implementation. After consulting the
author of the original paper [@P2767R2], we decided to create a new paper which contains
a subset of the non-controversial fixes based on our implementation experience.



# LWG 4000: `flat_map::insert_range` Fix

As stated in [@LWG4000], `flat_map::insert_range` has an obvious bug. But for some reason,
this LWG issue was not moved forward, possibly due to the existence of [@P2767R2].
The fix is twofold: first, we use `value_type` explicitly to make sure that `e` is a `std::pair`,
and we move to `ranges::for_each` for consistency with the rest of the `flat_map` specification.

## Wording

Change [flat.map.modifiers]{.sref} paragraph 12 to:

```cpp
template<@*container-compatible-range*@<value_type> R>
  void insert_range(R&& rg);
```

:::bq

[12]{.pnum} *Effects*: Adds elements to `c` as if by:

```cpp
@[`for (const auto& e : rg) {`]{.rm}@
@[`ranges::for_each(rg, [&](value_type e) {`]{.add}@
  c.keys.insert(c.keys.end(), @[`std::move(`]{.add}@e.first@[`)`]{.add}@);
  c.values.insert(c.values.end(), @[`std::move(`]{.add}@e.second@[`)`]{.add}@);
}@[`);`]{.add}@
```

Then, sorts the range of newly inserted elements with respect to `value_comp()`; merges the resulting sorted range and the sorted range of pre-existing elements into a single sorted range; and finally erases the duplicate elements as if by:

```cpp
auto zv = views::zip(c.keys, c.values);
auto it = ranges::unique(zv, key_equiv(compare)).begin();
auto dist = distance(zv.begin(), it);
c.keys.erase(c.keys.begin() + dist, c.keys.end());
c.values.erase(c.values.begin() + dist, c.values.end());
```

:::

# `swap` Should be Conditionally `noexcept`

`flat_meow::swap` is currently specified as unconditionally `noexcept`. In case the underlying container's `swap`
throws an exception, implementations have to either

1. Swallow the exception and restore invariants. Practically speaking, this means clearing both containers.
2. `std::terminate`

The first option is extremely bad because users will silently get an empty `flat_meow` after a failed `swap`. The second option is extremely harsh.

Instead, making `swap` conditionally-`noexcept` allows us to propagate the exception (after restoring invariants).

## Wording

Change [flat.map.defn]{.sref} as follows:

```cpp
void swap(flat_map& y) noexcept@@[`(@*see below*@)`]{.add}@@;

// ...

friend void swap(flat_map& x, flat_map& y) noexcept@[`(noexcept(x.swap(y)))`]{.add}@
  { x.swap(y); }
```

Change [flat.map.modifiers]{.sref} paragraph 33:

```cpp
void swap(flat_map& y) @[`noexcept;`]{.rm}@
  @[`noexcept(is_nothrow_swappable_v<key_container_type> &&`]{.add}@
           @[`is_nothrow_swappable_v<mapped_container_type> &&`]{.add}@
           @[`is_nothrow_swappable_v<key_compare>);`]{.add}@
```

:::bq

[33]{.pnum} *Effects*: Equivalent to:

```cpp
ranges::swap(compare, y.compare);
ranges::swap(c.keys, y.c.keys);
ranges::swap(c.values, y.c.values);
```
:::

Change [flat.multimap.defn]{.sref} as follows:

```cpp
void swap(flat_multimap& y) @[`noexcept;`]{.rm}@
  @[`noexcept(is_nothrow_swappable_v<key_container_type> &&`]{.add}@
           @[`is_nothrow_swappable_v<mapped_container_type> &&`]{.add}@
           @[`is_nothrow_swappable_v<key_compare>);`]{.add}@

// ...

friend void swap(flat_multimap& x, flat_multimap& y) noexcept@[`(noexcept(x.swap(y)))`]{.add}@
  { x.swap(y); }
```

Change [flat.set.defn]{.sref} as follows:

```cpp
void swap(flat_set& y) noexcept@@[`(@*see below*@)`]{.add}@@;

// ...

friend void swap(flat_set& x, flat_set& y) noexcept@[`(noexcept(x.swap(y)))`]{.add}@
  { x.swap(y); }
```

Change [flat.set.modifiers]{.sref} paragraph 13:

```cpp
void swap(flat_set& y) @[`noexcept;`]{.rm}@
  @[`noexcept(is_nothrow_swappable_v<container_type> &&`]{.add}@
           @[`is_nothrow_swappable_v<key_compare>);`]{.add}@
```

:::bq

[13]{.pnum} *Effects*: Equivalent to:

```cpp
ranges::swap(compare, y.compare);
ranges::swap(c, y.c);
```
:::

Change [flat.multiset.defn]{.sref} as follows:

```cpp
void swap(flat_multiset& y) noexcept@@[`(@*see below*@)`]{.add}@@;

// ...

friend void swap(flat_multiset& x, flat_multiset& y) noexcept@[`(noexcept(x.swap(y)))`]{.add}@
  { x.swap(y); }
```

Change [flat.multiset.modifiers]{.sref} paragraph 9:

```cpp
void swap(flat_multiset& y) @[`noexcept;`]{.rm}@
  @[`noexcept(is_nothrow_swappable_v<container_type> &&`]{.add}@
           @[`is_nothrow_swappable_v<key_compare>);`]{.add}@
```

:::bq

[9]{.pnum} *Effects*: Equivalent to:

```cpp
ranges::swap(compare, y.compare);
ranges::swap(c, y.c);
```
:::


# Missing `insert_range(sorted_unique, rg)`

The multi-element insertion API consists of two sets of overloads:

- A set of overloads that take a potentially-unsorted range
- A set of overloads that take a `sorted_unique` range

```cpp
insert(first, last);                 // 1a
insert(il);                          // 2a
insert_range(rg);                    // 3a

insert(sorted_unique, first, last);  // 1b
insert(sorted_unique, il);           // 2b
insert_range(sorted_unique, rg)      // 3b is conspicuously missing.
```

However, the last overload `insert_range(sorted_unique, rg)` is missing. This could 
be an oversight in the original proposal. This is true for `flat_set` and `flat_map`.
Similarly, in `flat_multiset` and `flat_multimap`, the overload
`insert_range(sorted_equivalent, rg)` is also missing.

## Wording

### `flat_map`

Change [flat.map.defn]{.sref} as follows:

```cpp
template<class InputIterator>
  void insert(InputIterator first, InputIterator last);
template<class InputIterator>
  void insert(sorted_unique_t, InputIterator first, InputIterator last);
template<@*container-compatible-range*@<value_type> R>
  void insert_range(R&& rg);
@@[`template<@*container-compatible-range*@<value_type> R>`]{.add}@@
  @[`void insert_range(sorted_unique_t, R&& rg);`]{.add}@
```

Add a new entry to [flat.map.modifiers]{.sref} after paragraph 14:

:::add

```cpp
template<@*container-compatible-range*@<value_type> R>
  void insert_range(sorted_unique_t, R&& rg);
```

:::bq

[15]{.pnum} *Effects*: Equivalent to `ìnsert_range(rg)`.

[16]{.pnum} *Complexity*: Linear in *N*, where *N* is `size()` after the operation.

:::
:::

### `flat_multimap`

Change [flat.multimap.defn]{.sref} as follows:

```cpp
template<class InputIterator>
  void insert(InputIterator first, InputIterator last);
template<class InputIterator>
  void insert(sorted_equivalent_t, InputIterator first, InputIterator last);
template<@*container-compatible-range*@<value_type> R>
  void insert_range(R&& rg);
@@[`template<@*container-compatible-range*@<value_type> R>`]{.add}@@
  @[`void insert_range(sorted_equivalent_t, R&& rg);`]{.add}@
```

### `flat_set`

Change [flat.set.defn]{.sref} as follows:

```cpp
template<class InputIterator>
  void insert(InputIterator first, InputIterator last);
template<class InputIterator>
  void insert(sorted_unique_t, InputIterator first, InputIterator last);
template<@*container-compatible-range*@<value_type> R>
  void insert_range(R&& rg);
@@[`template<@*container-compatible-range*@<value_type> R>`]{.add}@@
  @[`void insert_range(sorted_unique_t, R&& rg);`]{.add}@
```

Add a new entry to [flat.set.modifiers]{.sref} after paragraph 12:

:::add

```cpp
template<@*container-compatible-range*@<value_type> R>
  void insert_range(sorted_unique_t, R&& rg);
```

:::bq

[13]{.pnum} *Effects*: Equivalent to `insert_range(rg)`.

[14]{.pnum} *Complexity*: Linear in *N*, where *N* is `size()` after the operation.

:::
:::

### `flat_multiset`

Change [flat.multiset.defn]{.sref} as follows:

```cpp
template<class InputIterator>
  void insert(InputIterator first, InputIterator last);
template<class InputIterator>
  void insert(sorted_equivalent_t, InputIterator first, InputIterator last);
template<@*container-compatible-range*@<value_type> R>
  void insert_range(R&& rg);
@@[`template<@*container-compatible-range*@<value_type> R>`]{.add}@@
  @[`void insert_range(sorted_equivalent_t, R&& rg);`]{.add}@
```

Change [flat.multiset.modifiers]{.sref} Paragraph 8 as follows:

```cpp
template<class InputIterator>
  constexpr void insert(sorted_equivalent_t, InputIterator first, InputIterator last);
```

:::bq

[7]{.pnum} *Effects*: Equivalent to `insert(first, last)`.

[8]{.pnum} *Complexity*: Linear [ in *N*, where *N* is `size()` after the operation]{.add}.
:::

Add a new entry to [flat.multiset.modifiers]{.sref} after Paragraph 8:

:::add

```cpp
template<@*container-compatible-range*@<value_type> R>
  void insert_range(R&& rg);
```

:::bq

[9]{.pnum} *Effects*: Adds elements to `c` as if by:

```cpp
ranges::for_each([&](auto&& e) {
  c.insert(c.end(), std::forward<decltype(e)>(e));
});
```

Then, sorts the range of newly inserted elements with respect to `compare`, and merges the resulting sorted range and the sorted range of pre-existing elements into a single sorted range.

[10]{.pnum} *Complexity*: *N* + *M*log*M*, where *N* is `size()` before the operation and *M* is `ranges::distance(rg)`.

[11]{.pnum} *Remarks*: Since this operation performs an in-place merge, it may allocate memory.

:::

```cpp
template<@*container-compatible-range*@<value_type> R>
  void insert_range(sorted_equivalent_t, R&& rg);
```

:::bq

[12]{.pnum} *Effects*: Equivalent to `insert_range(rg)`.

[13]{.pnum} *Complexity*: Linear in *N*, where *N* is `size()` after the operation.

:::
:::

# `flat_set::insert_range`

The current specification for `flat_set::insert_range` seems to unnecessarily pessimize by forcing
copies of the elements:

```cpp
template<@*container-compatible-range*@<value_type> R>
  void insert_range(R&& rg);
```

:::bq

[10]{.pnum} *Effects*: Adds elements to `c` as if by:

```cpp
for (const auto& e : rg) {
  c.insert(c.end(), e); // COPYING HERE
}
```

:::

We should allow implementations to move when they can.

## Wording

Change [flat.set.modifiers]{.sref} paragraph 10 as follows:

```cpp
template<@*container-compatible-range*@<value_type> R>
  void insert_range(R&& rg);
```

:::bq

[10]{.pnum} *Effects*: Adds elements to `c` as if by:

```cpp
@[`for (const auto& e : rg) {`]{.rm}@
@[`ranges::for_each(rg, [&](auto&& e) {`]{.add}@
  c.insert(c.end(), @[`std::forward<decltype(e)>(`]{.add}@e@[`)`]{.add}@);
}
```

Then, sorts the range of newly inserted elements with respect to compare; merges the resulting sorted range and the sorted range of pre-existing elements into a single sorted range; and finally erases all but the first element from each group of consecutive equivalent elements.

:::


# Underspecified special member functions

The special member functions for `flat_meow` are currently not specified explicitly. This means that an implementation
using e.g. `flat_map(flat_map&&) = default` would be conforming. However, such an implementation would not be correct
with respect to exception handling. Indeed, if an exception is thrown while moving from the incoming map, the incoming
map would be left in a potentially invalid state with respect to its invariants.

Note that the [blanket paragraph](http://eel.is/c++draft/container.adaptors#flat.map.overview-6) does not apply here, since we're concerned with the incoming `flat_map`'s
invariants, not `*this`'s invariants. We believe that the behavior of these special member functions must be specified explicitly, otherwise these constructors
are useless in any context where an exception can be thrown.

## Wording

### `flat_map` Wording

Change [flat.map.overview]{.sref} Paragraph 6 as follows:

:::bq

[6]{.pnum} If any member function in [flat.map.defn]{.sref} exits via an exception[,]{.add} the invariants are restored. [For the move constructor and move assignment operator, the invariants of the argument are also restored.]{.add}

[*Note 2*: This can result in the `flat_map` being emptied. — *end note*]

:::

Change [flat.map.defn]{.sref} as follows:

```cpp
// [flat.map.cons], constructors
flat_map() : flat_map(key_compare()) { }

@[`flat_map(const flat_map&);`]{.add}@
@[`flat_map(flat_map&&);`]{.add}@
@[`flat_map& operator=(const flat_map&);`]{.add}@
@[`flat_map& operator=(flat_map&&);`]{.add}@
```

### `flat_multimap` Wording

Change [flat.multimap.overview]{.sref} Paragraph 6 as follows:

:::bq

[6]{.pnum} If any member function in [flat.multimap.defn]{.sref} exits via an exception, the invariants are restored. [For the move constructor and move assignment operator, the invariants of the argument are also restored.]{.add}

[*Note 2*: This can result in the `flat_multimap` being emptied. — *end note*]

:::

Change [flat.multimap.defn]{.sref} as follows:

```cpp
// [flat.multimap.cons], constructors
flat_multimap() : flat_multimap(key_compare()) { }

@[`flat_multimap(const flat_multimap&);`]{.add}@
@[`flat_multimap(flat_multimap&&);`]{.add}@
@[`flat_multimap& operator=(const flat_multimap&);`]{.add}@
@[`flat_multimap& operator=(flat_multimap&&);`]{.add}@
```

### `flat_set` Wording

Change [flat.set.overview]{.sref} Paragraph 6 as follows:

:::bq

[6]{.pnum} If any member function in [flat.set.defn]{.sref} exits via an exception, the invariant is restored. [For the move constructor and move assignment operator, the invariant of the argument is also restored.]{.add}

[*Note 2*: This can result in the `flat_set`'s being emptied. — *end note*]

:::

Change [flat.set.defn]{.sref} as follows:

```cpp
// [flat.set.cons], constructors
flat_set() : flat_set(key_compare()) { }

@[`flat_set(const flat_set&);`]{.add}@
@[`flat_set(flat_set&&);`]{.add}@
@[`flat_set& operator=(const flat_set&);`]{.add}@
@[`flat_set& operator=(flat_set&&);`]{.add}@
```

### `flat_multiset` Wording

Change [flat.multiset.overview]{.sref} Paragraph 6 as follows:

:::bq

[6]{.pnum} If any member function in [flat.multiset.defn]{.sref} exits via an exception, the invariant is restored. [For the move constructor and move assignment operator, the invariant of the argument is also restored.]{.add}

[*Note 2*: This can result in the `flat_multiset`'s being emptied. — *end note*]

:::

Change [flat.multiset.defn]{.sref} as follows:

```cpp
// [flat.multiset.cons], constructors
flat_multiset() : flat_multiset(key_compare()) { }

@[`flat_multiset(const flat_multiset&);`]{.add}@
@[`flat_multiset(flat_multiset&&);`]{.add}@
@[`flat_multiset& operator=(const flat_multiset&);`]{.add}@
@[`flat_multiset& operator=(flat_multiset&&);`]{.add}@
```

# Implementation Experience

This paper is based on our implementation in libc++.

# Feature Test Macro

Bump `__cpp_lib_flat_map` and `__cpp_lib_flat_set` appropriately.


<style>
.bq{
    display: block;
    margin-block-start: 1em;
    margin-block-end: 1em;
    margin-inline-start: 40px;
    margin-inline-end: 40px;
}
</style>
