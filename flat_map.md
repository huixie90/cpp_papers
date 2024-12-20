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
`flat_set`, and `flat_multiset` based on the libc++'s implementation.

# Introduction

[@P2767R2] proposed a set of fixes to `flat_map`, `flat_multimap`, `flat_set`, and `flat_multiset`,  based on the author's
implementation experience. However, the paper contains not only some straight forward
fixes, but also some design changes, which did not get consensus in previous meetings.
We (libc++) have recently released the `flat_map` implementation. After consulting the
author of the original paper [@P2767R2], we decided to create a new paper which contains
a subset of the non-controversial fixes based on our implementation experience.



# LWG 4000: `flat_map::insert_range` Fix

As stated in [@LWG4000], `flat_map::insert_range` has an obvious bug. But for some reason,
this LWG issue was not moved forward even with lots of P0 vote, possibly due to the existence of [@P2767R2].

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

`flat_meow::swap` is currently specified as unconditionally `noexcept`. In case underlying container's `swap`
throws an exception, implementations have to either

1. Swallow the exception and restore invariants. Practically speaking, this means clearing both containers.
2. `std::terminate`

1 is extremely bad because users can silently get empty containers after a `swap` without knowing it. 2 is extremely harsh.

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

The multi-element insertion API consists of two sets of the overloads:

- A set of overloads that take any ranges
- A set of overloads that take `sorted_unique` ranges

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

[link to P2767](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2767r2.html#wording-set-insert-range)

Lots of the wording is optimizations QOI?

# Move from comparators

[link to P2767](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2767r2.html#move-from-comparator)

I found it very confusing and leads to implementation weirdness
[godbolt](https://godbolt.org/z/vernPe7xx)

# Stable sorting

[link to P2767](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2767r2.html#stable-sorting)

I asked MSVC STL , they said they just use plain `sort`, which makes me think maybe
we should do the same (that being said, it is not an obvious thing which might involve design discussions)

# zero initialization

[link to P2767](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2767r2.html#zero-initialization)

to consider

# Implementation Experience

This paper is based on our implementation in libc++.

## Feature Test Macro

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
