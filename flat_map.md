---
title: "flat_meow Fixes"
document: PXXXXR0
date: 2024-12-15
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

## R0

- Initial revision.

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
template<container-compatible-range<value_type> R>
  void insert_range(R&& rg);
```

:::bq

[12]{.pnum} *Effects*: Adds elements to `c` as if by:

```cpp
@[`for (const auto& e : rg) {`]{.rm}@
@[`ranges::for_each(rg, [&c](value_type e) {`]{.add}@
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
void swap(flat_map& y) @[`noexcept;`]{.rm}@
  @[`noexcept(is_nothrow_swappable_v<key_container_type> &&`]{.add}@
           @[`is_nothrow_swappable_v<mapped_container_type> &&`]{.add}@
           @[`is_nothrow_swappable_v<key_compare>);`]{.add}@

// ...

friend void swap(flat_map& x, flat_map& y) @[`noexcept`]{.rm}@
  @[`noexcept(is_nothrow_swappable_v<key_container_type> &&`]{.add}@
           @[`is_nothrow_swappable_v<mapped_container_type> &&`]{.add}@
           @[`is_nothrow_swappable_v<key_compare>)`]{.add}@
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

friend void swap(flat_multimap& x, flat_multimap& y) @[`noexcept`]{.rm}@
  @[`noexcept(is_nothrow_swappable_v<key_container_type> &&`]{.add}@
           @[`is_nothrow_swappable_v<mapped_container_type> &&`]{.add}@
           @[`is_nothrow_swappable_v<key_compare>)`]{.add}@
  { x.swap(y); }
```

Change [flat.set.defn]{.sref} as follows:

```cpp
void swap(flat_set& y) @[`noexcept;`]{.rm}@
  @[`noexcept(is_nothrow_swappable_v<container_type> &&`]{.add}@
           @[`is_nothrow_swappable_v<key_compare>);`]{.add}@

// ...

friend void swap(flat_set& x, flat_set& y) @[`noexcept`]{.rm}@
  @[`noexcept(is_nothrow_swappable_v<container_type> &&`]{.add}@
           @[`is_nothrow_swappable_v<key_compare>)`]{.add}@
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
void swap(flat_multiset& y) @[`noexcept;`]{.rm}@
  @[`noexcept(is_nothrow_swappable_v<container_type> &&`]{.add}@
           @[`is_nothrow_swappable_v<key_compare>);`]{.add}@

// ...

friend void swap(flat_multiset& x, flat_multiset& y) @[`noexcept`]{.rm}@
  @[`noexcept(is_nothrow_swappable_v<container_type> &&`]{.add}@
           @[`is_nothrow_swappable_v<key_compare>)`]{.add}@
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
template<container-compatible-range<value_type> R>
  void insert_range(R&& rg);
@[`template<container-compatible-range<value_type> R>`]{.add}@
  @[`void insert_range(sorted_unique_t, R&& rg);`]{.add}@
```

Add a new entry to [flat.map.modifiers]{.sref} after paragraph 14:

```cpp
template<container-compatible-range<value_type> R>
  void insert_range(sorted_unique_t, R&& rg);
```

:::bq

[15]{.pnum} *Effects*: Adds elements to `c` as if by:

```cpp
ranges::for_each(rg, [&c](value_type e) {
  c.keys.insert(c.keys.end(), std::move(e.first));
  c.values.insert(c.values.end(), std::move(e.second));
});
```

Then, merges the newly inserted sorted range and the sorted range of pre-existing elements into a single sorted range; and finally erases the duplicate elements as if by:

```cpp
auto zv = views::zip(c.keys, c.values);
auto it = ranges::unique(zv, key_equiv(compare)).begin();
auto dist = distance(zv.begin(), it);
c.keys.erase(c.keys.begin() + dist, c.keys.end());
c.values.erase(c.values.begin() + dist, c.values.end());
```

[16]{.pnum} *Complexity*: Linear in *N*, where *N* is `size()` after the operation.

[17]{.pnum} *Remarks*: Since this operation performs an in-place merge, it may allocate memory.

:::

### `flat_multimap`

Change [flat.multimap.defn]{.sref} as follows:

```cpp
template<class InputIterator>
  void insert(InputIterator first, InputIterator last);
template<class InputIterator>
  void insert(sorted_equivalent_t, InputIterator first, InputIterator last);
template<container-compatible-range<value_type> R>
  void insert_range(R&& rg);
@[`template<container-compatible-range<value_type> R>`]{.add}@
  @[`void insert_range(sorted_equivalent_t, R&& rg);`]{.add}@
```

### `flat_set`

Change [flat.set.defn]{.sref} as follows:

```cpp
template<class InputIterator>
  void insert(InputIterator first, InputIterator last);
template<class InputIterator>
  void insert(sorted_unique_t, InputIterator first, InputIterator last);
template<container-compatible-range<value_type> R>
  void insert_range(R&& rg);
@[`template<container-compatible-range<value_type> R>`]{.add}@
  @[`void insert_range(sorted_unique_t, R&& rg);`]{.add}@
```

Add a new entry to [flat.set.modifiers]{.sref} after paragraph 12:

```cpp
template<container-compatible-range<value_type> R>
  void insert_range(sorted_unique_t, R&& rg);
```

:::bq

[13]{.pnum} *Effects*: Equivalent to `insert_range(rg)`.

[14]{.pnum} *Complexity*: Linear in *N*, where *N* is `size()` after the operation.

:::

### `flat_multiset`

Change [flat.multiset.defn]{.sref} as follows:

```cpp
template<class InputIterator>
  void insert(InputIterator first, InputIterator last);
template<class InputIterator>
  void insert(sorted_equivalent_t, InputIterator first, InputIterator last);
template<container-compatible-range<value_type> R>
  void insert_range(R&& rg);
@[`template<container-compatible-range<value_type> R>`]{.add}@
  @[`void insert_range(sorted_equivalent_t, R&& rg);`]{.add}@
```

# `flat_set::insert_range`

The current specification for `flat_set::insert_range` seems to unnecessarily pessimize by forcing
copies of the elements:

```cpp
template<container-compatible-range<value_type> R>
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
template<container-compatible-range<value_type> R>
  void insert_range(R&& rg);
```

:::bq

[10]{.pnum} *Effects*: Adds elements to `c` as if by:

```cpp
for (@[`const auto&`]{.rm}@ @[`auto&&`]{.add}@ e : rg) {
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

> Note that the [blanket paragraph](http://eel.is/c++draft/container.adaptors#flat.map.overview-6) does not apply here, since we're concerned with the incoming `flat_map`'s
> invariants, not `*this`'s invariants.

We believe that the behavior of these special member functions must be specified explicitly, otherwise these constructors
are useless in any context where an exception can be thrown.

## Wording

### `flat_map` Wording

Change [flat.map.defn]{.sref} as follows:

```cpp
// [flat.map.cons], constructors
flat_map() : flat_map(key_compare()) { }

@[`flat_map(const flat_map&);`]{.add}@
@[`flat_map(flat_map&&);`]{.add}@
@[`flat_map& operator=(const flat_map&);`]{.add}@
@[`flat_map& operator=(flat_map&&);`]{.add}@
```

Add a new entry to [flat.map.cons]{.sref} at the beginning:

```cpp
flat_map(flat_map&& o);
```

:::bq

[1]{.pnum} *Effects*: Initialize `@*c*@` with `std::move(o.@*c*@)` and `@*compare*@` with `std::move(o.@*compare*@)`.
If the function exits via an exception, the invariants of `o` are restored.

:::

```cpp
flat_map& operator=(flat_map&& o);
```

:::bq

[2]{.pnum} *Effects*: Assigns `std::move(o.@*c*@)` to `@*c*@` and `std::move(o.@*compare*@)` to `@*compare*@`.
If the function exits via an exception, the invariants of `o` and `*this` are restored.

:::

> Drafting note: Our intent is not to mandate implementations to move or not move the comparator, but we are
> not certain how to word things such that both implementations are valid.

### `flat_multimap` Wording

Change [flat.multimap.defn]{.sref} as follows:

```cpp
// [flat.multimap.cons], constructors
flat_multimap() : flat_multimap(key_compare()) { }

@[`flat_multimap(const flat_multimap&);`]{.add}@
@[`flat_multimap(flat_multimap&&);`]{.add}@
@[`flat_multimap& operator=(const flat_multimap&);`]{.add}@
@[`flat_multimap& operator=(flat_multimap&&);`]{.add}@
```

Add a new entry to [flat.multimap.cons]{.sref} at the beginning:

```cpp
flat_multimap(flat_multimap&& o);
```

:::bq

[1]{.pnum} *Effects*: Initialize `@*c*@` with `std::move(o.@*c*@)` and `@*compare*@` with `std::move(o.@*compare*@)`.
If the function exits via an exception, the invariants of `o` are restored.

:::

```cpp
flat_multimap& operator=(flat_multimap&& o);
```

:::bq

[2]{.pnum} *Effects*: Assigns `std::move(o.@*c*@)` to `@*c*@` and `std::move(o.@*compare*@)` to `@*compare*@`.
If the function exits via an exception, the invariants of `o` and `*this` are restored.

:::

### `flat_set` Wording

Change [flat.set.defn]{.sref} as follows:

```cpp
// [flat.set.cons], constructors
flat_set() : flat_set(key_compare()) { }

@[`flat_set(const flat_set&);`]{.add}@
@[`flat_set(flat_set&&);`]{.add}@
@[`flat_set& operator=(const flat_set&);`]{.add}@
@[`flat_set& operator=(flat_set&&);`]{.add}@
```

Add a new entry to [flat.set.cons]{.sref} at the beginning:

```cpp
flat_set(flat_set&& o);
```

:::bq

[1]{.pnum} *Effects*: Initialize `@*c*@` with `std::move(o.@*c*@)` and `@*compare*@` with `std::move(o.@*compare*@)`.
If the function exits via an exception, the invariants of `o` are restored.

:::

```cpp
flat_set& operator=(flat_set&& o);
```

:::bq

[2]{.pnum} *Effects*: Assigns `std::move(o.@*c*@)` to `@*c*@` and `std::move(o.@*compare*@)` to `@*compare*@`.
If the function exits via an exception, the invariants of `o` and `*this` are restored.

:::

### `flat_multiset` Wording

Change [flat.multiset.defn]{.sref} as follows:

```cpp
// [flat.multiset.cons], constructors
flat_multiset() : flat_multiset(key_compare()) { }

@[`flat_multiset(const flat_multiset&);`]{.add}@
@[`flat_multiset(flat_multiset&&);`]{.add}@
@[`flat_multiset& operator=(const flat_multiset&);`]{.add}@
@[`flat_multiset& operator=(flat_multiset&&);`]{.add}@
```

Add a new entry to [flat.multiset.cons]{.sref} at the beginning:

```cpp
flat_multiset(flat_multiset&& o);
```

:::bq

[1]{.pnum} *Effects*: Initialize `@*c*@` with `std::move(o.@*c*@)` and `@*compare*@` with `std::move(o.@*compare*@)`.
If the function exits via an exception, the invariants of `o` are restored.

:::

```cpp
flat_multiset& operator=(flat_multiset&& o);
```

:::bq

[2]{.pnum} *Effects*: Assigns `std::move(o.@*c*@)` to `@*c*@` and `std::move(o.@*compare*@)` to `@*compare*@`.
If the function exits via an exception, the invariants of `o` and `*this` are restored.

:::

Note: We purposefully do not add `noexcept` specifiers to any of these member functions. Doing so is a complicated subject
that is the target of [LWG2227](http://wg21.link/LWG2227) and we would prefer to solve that problem separately. In practice,
implementations are free to strengthen `noexcept` specifications if they so desire.

# Implementation Experience

This paper is based on our implementation in libc++.

# Feature Test Macro


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
    title: "A proof-of-concept implementation of `any_view`"
    author:
      - family: Xie
        given: Hui
      - family: Yilmaz
        given: S. Levent
      - family: Louis
        given: Dionne
    URL: https://github.com/huixie90/cpp_papers/tree/main/impl/any_view
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
