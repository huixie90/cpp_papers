---
title: "`zip()` Should Be an Infinite Range"
document: DXXXXR0
date: 2025-05-31
audience: SG9, LEWG
author:
  - name: Hui Xie
    email: <hui.xie1990@gmail.com>
  - name: Tristan Melen
    email: <>
toc: true
---

# Revision History

## R0

- Initial revision.

# Abstract

This paper proposes a fix that makes the `views::zip()` an infinite range `views::repeat(tuple())` instead of the current `views::empty<tuple<>>`


# Design Considerations

## Rationale for the Current `zip()` Design

### P2321R2

According to [@P2321R2], the reason for the current behaviour of `zip()` was:

> As in range-v3, zipping nothing produces an `empty_view` of the appropriate type.

Searching through the historical commits in range-v3, the behaviour of `zip()` was added in this commit [@range-v3].

In this commit, both nullary `cartesian_product()` and `zip()` were added, both yield an empty range, and both are incorrect. And the rationale for this commit was

> This allows us to remove the nasty MSVC special case for empty `cartesian_products`.

In conclusion, this behaviour did not seem to be a result of careful design, but just chosen to work around a "nasty MSVC special case".

### P2540R1

The discussion on the [@reflector] indicated that `cartesian_product()` should be `single(tuple())` and `zip()` should be `repeat(tuple())`.

Luckily, [@P2540R1] fixed `cartesian_product`. So it no longer follows the "range-v3" behaviour but the correct one `views::single(tuple())`.

However, the paper intentionally left `zip`'s behaviour unchanged, after . 

The main reason behind this decision was

> In particular, `zip` has the property that it is the inner join of the indexed sets, and is the main diagonal of the Cartesian product. However, the identity element for `zip` is `repeat(tuple<>)`, the infinite range of repeated empty tuples. 
> If we allowed zip of an empty range of ranges to be its identity element, we would be introducing an inconsistency into the system, where two different formulations of notionally the same thing produces different answers. 

However, the authors believe that the above conclusion is flawed.

TODO: explain the main diagonal and identity yield same results

## TODO
- explain any_of/all_of's identity
- explain cartesian_product
- explain the identify component of "minimum" operation should be infinity
- we prefer the result to be infinite range. if this cannot be agreed on, at the minimum, zip() should be ill-formed

## Other Languages

### Julia

Julia's `zip()` produces an infinite range of empty tuples, which is in line with the authors' view.

### Rust

Rust's compiler actively rejects `multizip` or `izip!` calls with no argument. This is much better than giving the wrong results.

### Java, C#, Haskell

These languages either only supports a zip with two ranges, or provide zip3, zip4, ... functions for more ranges, possibly due to
lack of support for variadic arguments. However, they don't provide a zip0, for a good reason.

### Python

Python got it wrong too. `zip()` is an empty iterable. It is unclear what drives the design decision on this. The author has asked this question
on StackOverflow's Python community long time ago [@stackoverflow], but there is no satisfying answer.

## An Infinite Range is not a `ranges::range`

Whether an infinite range is a `ranges::range` is out of the scope of this paper. After all, this paper only delegates this special case of `views::zip` to `views::repeat` without inventing anything new. Whatever fix that is needed to fix `ranges::range` concept to work nicely with infinite ranges will fix all the existing views, i.e. `views::iota(0)`, `views::repeat(x)` and so on in one go.

## Conclusion

The authors propose to change the current incorrect results of `zip()` to `repeat(tuple())`. If the resistance of this change is too significant, the authors believe a compromise is to make `zip()` ill-formed.


# Implementation Experience


# Wording

Modify [range.zip.overview]{.sref} section (2.1) as

- (2.1) [`auto(views​::​empty<tuple<>>)`]{.rm} [`views::repeat(tuple())`]{.add} if `Es` is an empty pack,


## Feature Test Macro

Bump the FTM `__cpp_lib_ranges_zip`

---
references:
  - id: stackoverflow
    citation-label: stackoverflow
    title: "Why does Python `zip()` yield nothing when given no iterables?"
    URL: https://stackoverflow.com/questions/71561715/why-does-python-zip-yield-nothing-when-given-no-iterables

  - id: range-v3
    citation-label: range-v3
    title: "[zip] Correctly zip no ranges"
    URL: https://github.com/ericniebler/range-v3/commit/ef9a650d4da87f02c5c079055e09017825c92fb3

  - id: reflector
    citation-label: reflector
    title: "[isocpp-lib-ext] zip and cartesian_product base case"
    URL: https://lists.isocpp.org/lib-ext/2022/01/22023.php

  - id: libcxx
    citation-label: libcxx
    title: "Implementation of zip() in libc++"
    author:
      - family: Xie
        given: Hui
    URL: todo
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
