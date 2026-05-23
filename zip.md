---
title: "`zip()` Should Be Ill-formed"
document: P4243R0
date: 2025-05-31
audience: SG9, LEWG
author:
  - name: Hui Xie
    email: <hui.xie1990@gmail.com>
  - name: Tristan Melen
    email: <tpmelen@hotmail.com>
toc: true
---

# Abstract

This paper proposes a fix that makes the `views::zip()` ill-formed instead of the current `views::empty<tuple<>>`.

# Design Considerations

The authors believe that the correct result of `zip()` is an infinite range `views::repeat(tuple<>())`. However, because
infinite ranges sometimes cause confusions to some users, we believe it should be ill-formed. The current `views::empty<tuple<>>`
is mathematically incorrect.

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

However, the paper intentionally left `zip`'s behaviour unchanged.

The main reason behind this decision was

> In particular, `zip` has the property that it is the inner join of the indexed sets, and is the main diagonal of the Cartesian product. However, the identity element for `zip` is `repeat(tuple<>)`, the infinite range of repeated empty tuples.
> If we allowed zip of an empty range of ranges to be its identity element, we would be introducing an inconsistency into the system, where two different formulations of notionally the same thing produces different answers.

However, the authors believe that the above conclusion is flawed. First of all, main diagonal does not exist for zero-dimension. Python `numpy.diagonal` only accepts arrays with more than two dimensions. The author of that paper seemed to extend the concept of main diagonal to zero dimensions without specifying how.
. When it is zero dimension, `all_of` yields always `true`, therefore an infinite range is correct. Howedndex[0], uthors of this paper no longer purindex[0] == index[1] == index[2] == ...fusions to users. A compiler error is more appropriate.

## Identity

There are precedences of returning identity of an operation when there is no input in the library

- `std::all_of` of an empty range returns `true`, because the identity of operation `&&` is `true`
- `std::any_of` of an empty range returns `false`, because the identity of operation `||` is `false`
- `std::conjunction` is `true_type`
- `std::disjunction` is `false_type`
- `std::views::cartesian_product` is `views::single<std::tuple()>`. The reason why the `size` of the result is `1` is because the identity of multiplication is `1`

## Other Languages

### Julia

Julia's `zip()` produces an infinite range of empty tuples, which is in line with the authors' view in mathematically sense.

### Haskell

Haskell's `ZipList`'s `pure` implementation is [@haskell_source]

```haskell
pure x = ZipList (repeat x)
```

This is also what authors expected.

Haskell documentation [@haskell_doc] clearly stated that

> The only way to ensure `zipWith ($) fs` never removes elements is making `fs` infinite.

### Rust

Rust's compiler actively rejects `multizip` or `izip!` calls with no argument. This is much better than giving the wrong results.

### Java, C\#

These languages either only supports a zip with two ranges, or provide zip3, zip4, ... functions for more ranges, possibly due to
lack of support for variadic arguments. However, they don't provide a zip0, for a good reason.

### Python

`zip()` is an empty iterable.

Before Python 2.4, `zip()` raises an `TypeError`. And Python 2.4 changed its behaviour to return an empty list.
The rational of the change can be found here [@pep0201]. The motivating example is

```python
date, rain, high, low = zip(*csv.reader(file("weather.csv")))
print "Total rainfall", sum(rain)
```

When the csv file has no data, instead of raising an exception, it is useful to have the above code just work.

The only problem of this motivating example is that, it does not work.

```bash
Traceback (most recent call last):
  File "<python-input-17>", line 2, in <module>
    date, rain, high, low = zip(*csv.reader(file))
    ^^^^^^^^^^^^^^^^^^^^^
ValueError: not enough values to unpack (expected 4, got 0)
```

The `csv.reader` reads the file into list of rows, where each row is a list of elements: one for each column. `*rows` unpack the rows to the arguments to `zip`, then `date, rain, high, low = zip(*rows)` unpacks the zipped iterable's elements, so effectively `date`, `rain`, `high`, and `low` all represents a column. The user confidently wrote this code because they know that the csv file has 4 columns.

Now the motivation of changing `zip()` from `TypeError` to `[]` is that "it would be nice it still works if the csv file has no records".
The problem is that the above code assumes that the `len(zip(*rows))` is 4 so it unpacks to 4 variables. But `zip(*[])`
yields zero element thus the program raises a `ValueError`.

This example uses `zip(*s)` to transpose a matrix. When the input, say, is `[[1,2], [3,4], [5,6]]`, it is clear that it is a
`3 x 2` matrix and the transpose result is a `2 x 3` matrix. And when the input is `*[]`, there is no way to know if this is a `0 x 1`, or `0 x 2`, or `0 x 3` ... matrix. `zip` makes an assumption that this is a `0 x 0` matrix, but unfortunately in this case, the user wants a `0 x 4` matrix.

This motivating example is not only "not working", but also a justification that the cardinality of `zip()` cannot be known.

#### Comments from Guido van Rossum (BDFL)

The author has contacted the creator of Python, Guido van Rossum, about the design decision of `zip()` and here is the reply from him about the design decision:

> To be honest, I don't recall how the decision around this detail went. I assume it was literally that people showed me the specific example given in the PEP and argued that it should work that way, and I found I didn't feel like arguing against the treatment of such a minor edge case.
>
> The edge case actually reminds me of the debate about the meaning of 0**0 (see Wikipedia).  I think the same reasoning may apply here? Python chose the more useful outcome, for some applications.
>
> In any case, if you want to change C++'s zip to make this an error, you have my blessing.

#### Comments from Barry Warsaw

The author has also contacted Barry Warsaw, the author of [@pep0201], who introduced `zip` to Python 2.0. And here is his reply:

> I don't remember the discussion leading to the change either, but the Zen of Python does favor practicality over purity.  Guido's recollection is likely right; folks had some real-world experience where it was more practical to return an empty sequence rather than have to write code to handle an exception.
>
> For example:
>
> ```python
> >>> def ab(*seq):
> ...     for a, b in zip(*seq):
> ...         print(a, b)
> ...
> >>> ab([1, 2, 3], [4, 5, 6])
> 1 4
> 2 5
> 3 6
> >>> ab()
> >>>
> ```
>
> Totally contrived of course, but it is nice at least here not to have to handle an exception.  Defining that as repeat(()) would also be much less useful as it would infinite loop, so you'd probably have to add some value checking before the zip() call to avoid that.

The authors of this C++ paper agree with the "Zen of Python", however, this example seems a bit contrived. The function `ab` accepts either exactly two lists, or exactly zero list. It still needs to handle the `ValueError` if the user passes any other number of lists, for example one list. See this truncated error:

```python
>>> ab([1,2])
ValueError: not enough values to unpack (expected 2, got 1)
```

And this very example would not work in C++'s `zip`, because C++ is a statically typed language, the line

```cpp
for (auto [a, b] : zip(rng...))
```

will fail to compile as we cannot bind `[a, b]` to a 0-tuple, even we define `zip` as an empty range, whereas in Python, the a sequence is empty, the body of the for loop is not evaluated.

### Conclusion of other languages

Majority of the languages do not support null-ary `zip`. Julia makes it an infinite range, which is more mathematically correct. Python makes it an empty range.
It is understandable in Python there can be practical use cases as in Python one can dynamically turn a `Tuple` into a `List`, and allows ill formed for loop body if the sequence is empty.

## Counter Arguments

There were discussions on the reflector and some of them are defending the current design `views::empty<std::tuple<>>`. And I will go through all of them I have seen on the reflector

### Consistency

> - zip(empty_range, empty_range, empty_range) is an empty range
> - zip(empty_range, empty_range) is an empty range
> - zip(empty_range) is an empty range
> - zip() is an empty range
>
> Consistency.

This example only makes sense if the input is "empty_range". Any other ranges will break this "consistency". For example

- zip(single_view, single_view, single_view) is range of size 1
- zip(single_view, single_view) is range of size 1
- zip(single_view) is range of size 1
- zip() is a range of empty range

Inconsistency.

This is actually a great example to prove that there is no way to have consistency in terms of the size of the result range.

### Inner product

> The compelling use case for zip() empty range is a fold over it:

>  ```cpp
> template <typename... Rng>
> constexpr int inner_product(Rng&&... rng)
> {
>      int result = 0;
>      for (auto tpl : std::views::zip(rng...))
>          std::apply([&](auto&&... x) {
>              result += (x * ... * 1);
>          }, tpl);
>      return result;
> }
> ```

This example function implementation is actually incorrect mathematically. First of all, "inner product" space is $\Re^n \times \Re^n \rightarrow \Re$.
The function seems to generalise it into $\Re^n \times ... \times \Re^n \rightarrow \Re$. There is an important aspect: all the inputs are in the same space $R^n$.

There are two possible mathematical definition of this function

1. Generalised inner product, with a precondition that all input ranges must be of the same length `n`.

   With this definition, let's assume we are in $R^2$ space, and each vector has exactly two elements `(x, y)`. The result of `inner_product(r0, r1, r2, ...)` is

   `(x0 * x1 * x2 * ... * 1) + (y0 * y1 * y2 * ... * 1)`

   given `r0` is `(x0, y0)`,  `r1` is `(x1, y1)`, `r2` is `(x2, y2)`, ...

   and when the input is null-ary, the result is `1 + 1 = 2`. We can see that the function should return `2` instead of `0` when the problem space is $R^2$. Similarly, the result of this function in $R^n$ space should be `n`. The result of this function depends on the space we are in, i.e, the `n` in the precondition. We cannot infer `n` with
   null-ary inputs. Therefore, there isn't any meaningful definition
   for this function with null-ary inputs.

2. Generalised inner product of the first `n` elements in a vector, where `n` is the minimum length of all of the vectors.

   With this definition, the generalised inner product is $\sum_{i=0}^{minlength(rng...)} (x_{i,1} * x_{i,2} * ... * 1)$. When the input is null-ary, one way to define `minlength(rng...)` is to use its identity, and since the identity of `min` is infinity, the result should be `1 + 1 + 1 + ...`, which is $\infty$ instead of `0`. Or you can say the `min` does not make sense with no inputs and there is no definition for this function when there is no inputs. In fact, `std::min(initializer_list<T>)` follows this pattern. If the `initializer_list.size()` is `0`, the behaviour is undefined.

This "compelling use case of `zip()`" simply does not return a mathematically correct answer, no matter how we interpret its intent.

### There are no complaints of being empty range

> An infinite range is clearly mathematically the correct identity element. But that's entirely separate from the question of whether it's the correct programming solution. I have yet to see a compelling argument for why looping over `zip(rs...)` should degenerate into an infinite loop when an empty range is a perfectly reasonable, useful, and practical solution. Have people run into issues with either Python or C++ *not* providing an infinite range here? I would like to see concrete evidence thereof.

I think to answer "Have people run into issues with either Python or C++ *not* providing an infinite range here?", we should also answer "Have people used `zip` with no arguments" first. The author of this paper is no longer pursuing the "infinite range" option.

## Conclusion

The authors propose to change `zip()` to be ill-formed.

# Implementation Experience


# Wording

Modify [range.zip.overview]{.sref} section 2 as

The name views::zip denotes a customization point object ([customization.point.object]). Given a pack of subexpressions Es..., the expression views::zip(Es...) is expression-equivalent to

- (2.1) [`auto(views::empty<tuple<>>)` if `Es` is an empty pack, ]{.rm}
- (2.2) [otherwise, ]{.rm}`zip_view<views::all_t<decltype((Es))>...>(Es...)`.


## Feature Test Macro

Bump the FTM `__cpp_lib_ranges_zip`

---
references:
  - id: stackoverflow
    citation-label: stackoverflow
    title: "Why does Python `zip()` yield nothing when given no iterables?"
    URL: https://stackoverflow.com/questions/71561715/why-does-python-zip-yield-nothing-when-given-no-iterables

  - id: pep0201
    citation-label: pep0201
    title: "PEP 201 – Lockstep Iteration"
    URL: https://peps.python.org/pep-0201/#subsequent-change-to-zip

  - id: haskell_doc
    citation-label: haskell_doc
    title: "Haskell ZipList Documentation"
    URL: https://en.wikibooks.org/wiki/Haskell/Applicative_functors#ZipList

  - id: haskell_source
    citation-label: haskell_source
    title: "implementing `pure` in ZipList"
    URL: https://hackage.haskell.org/package/ghc-internal-9.1201.0/docs/src/GHC.Internal.Functor.ZipList.html

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
