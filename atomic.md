---
title: "Proposed Resolution for NB Comment GB13-309 `atomic_ref<T>` is not convertible to `atomic_ref<const T>`"
document: P3860R1
date: 2025-11-03
audience: LEWG, LWG
author:
  - name: Hui Xie
    email: <hui.xie1990@gmail.com>
  - name: Damien Lebrun-Grandié
    email: <lebrungrandt@ornl.gov>
toc: true
---

# Revision History
## R1
- Added `noexcept` specifier.

## R0
- Initial revision


# Abstract

This paper proposes a converting constructor for `atomic_ref` as the resolution for UK NB Comment GB13-309.

# Introduction

The UK NB Comment GB13-309 pointed out that `atomic_ref<T>` is not convertible to `atomic_ref<const T>`. [@P3323R1] added cv qualifiers to `atomic` and `atomic_ref`. However, the conversion constructor of `atomic_ref` between different cv qualifiers is overlooked.

# Wording

Change [atomics.ref.generic.general]{.sref} as follows:

```cpp
    constexpr explicit atomic_ref(T&);
    constexpr atomic_ref(const atomic_ref&) noexcept;
    @[`template <class U>`]{.add}@
    @[`constexpr atomic_ref(const atomic_ref<U>&) noexcept;`]{.add}@
```

Add a new entry to [atomics.ref.ops]{.sref} after paragraph 8:

:::add

```cpp
template <class U>
constexpr atomic_ref(const atomic_ref<U>& ref) noexcept;
```

[9]{.pnum} *Contraints*:

- [9.1]{.pnum} `is_same_v<remove_cv_t<T>, remove_cv_t<U>>` is `true`, and

- [9.2]{.pnum} `is_convertible_v<U*, T*>` is `true`

[10]{.pnum} *Postconditions*: `*this` references the object referenced by `ref`.

:::

# Implementation Experience

[@P3323R1] is currently being implemented in libc++ and adding the proposed constructor is under way.

# Feature Test Macro

[@P3323R1] does not seem to mention Feature Test Macro

<style>
.bq{
    display: block;
    margin-block-start: 1em;
    margin-block-end: 1em;
    margin-inline-start: 40px;
    margin-inline-end: 40px;
}
</style>
