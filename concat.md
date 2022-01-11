---
title: "`views::concat`"
document: PXXXXR0
date: 2021-01-06
audience: SG9, LEWG
author:
  - name: Hui Xie
    email: <hui.xie1990@gmail.com>
  - name: S. Levent Yilmaz
    email: <levent.yilmaz@gmail.com>
toc: true
---

# Revision History

This is the initial revision.

# Abstract

This paper proposes the range adaptor `views::concat` as very briefly introduced in [@P2214R1] Section 4.7. It is an adaptor that takes an arbitrary number of ranges as an argument list, and provides a view that starts at the first element of the first range, ends at the last element of the last range, with all range elements sequenced in between respectively in the order given in the arguments, effectively concatenating, or chaining together the argument ranges.

# Example

```cpp
std::vector v1{1,2,3}, v2{4,5}, v3{};
std::array  a{6,7,8};
fmt::print("{}\n", v | std::views::concat(v1, v2, v3, a)); // [1,2,3,4,5,6,7,8]
```

# Design

This adaptor is a generator factory as described in [@P2214R1] Section 4.7. As such, it can not be piped to. It takes the list of ranges to concatenate as arguments to `ranges::concat_view` constructor, or to `ranges::views::concat` range adaptor object.

## 

## Borrowed vs Cheap Iterator

A `concat` view can be designed to be a borrowed range, if all underlying ranges are. However, this requires the iterator implementation to contain a copy of all iterators and sentinels of all underlying ranges at all times (just like that of `views::zip` [@P2321R2]). On the other hand, a cheaper implementation that simply refers to the parent view can be used to satisfy all of the proposed functionality, if it is permitted to be not borrowed. Experience shows the borrowed-ness of `concat` is not a major requirement, and the existing implementation in [@rangev3] seems to have picked that latter alternative. We do so as such in this proposal.


## Implementation experience

`views::concat` has been implemented in [@rangev3], with equivalent semantics as proposed here. We also have implemented a version that directly follows the proposed wording below without issue [@ours].

# Wording

## Addition to `<ranges>`

Add the following to 24.2 [ranges.syn]{.sref}, header `<ranges>` synopsis:

```cpp
// [...]
namespace std::ranges {
  // [...]

  // [range.concat], concat view
  template <input_range... Views>
    requires (view<Views> && ...) && (sizeof...(Views) > 0)
  class concat_view;

  namespace views {
    inline constexpr @_unspecified_@ concat = @_unspecified_@;
  }

}
```

## `concat_view`

Add the following subclause to 24.7 [range.adaptors]{.sref}.

### 24.7.? Concat view [range.concat]

#### 24.7.?.1 Overview [range.concat.overview]

#### 24.7.?.2 Class template `concat_view` [range.concat.view]

#### 24.7.?.3 Class concat_view::iterator [range.concat.iterator]

#### 24.7.?.4 Class concat_view::sentinel [range.concat.sentinel]

4.7.?.2 Class template chunk_by_view [range.chunk.by.view]

## Feature Test Macro

Add the following macro definition to 17.3.2 [version.syn]{.sref}, header `<version>` synopsis, with the value selected by the editor to reflect the date of adoption of this paper:

```cpp
#define __cpp_lib_ranges_concat  20XXXXL // also in <ranges>
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
