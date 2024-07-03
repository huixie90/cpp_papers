---
title: "`any_view`"
document: PXXXXR0
date: 2024-07-06
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
- (well, we also have `copyable_function`)

For `any_view`, it is much much more complex than that:

- Is it an `input_range`, `forward_range`, `bidirectional_range`, `random_access_range`, or a `contiguous_range` ?
- Is the range `copyable` ?
- Is it a `sized_range` ?
- Is it a `borrowed_range` ?
- Is it a `common_range` ?
- What is the `range_reference_t` ?
- What is the `range_value_t` ?
- What is the `range_rvalue_reference_t` ?
- What is the `range_difference_t` ?
- Is the `range` const-iterable?
- Is the iterator `copyable` for `input_iterator`?
- Is the iterator equality comparable for `input_iterator`?
- sized_sentinel_for<S, I> ?

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

### Interface Design

The proposed template declaration is:

```cpp
enum class category
{
    none = 0,
    input = 1,
    forward = 3,
    bidirectional = 7,
    random_access = 15,
    contiguous = 31,
    mask = contiguous,
    sized = 32,
    borrowed = 64,
    common = 128,
    move_only_view = 256
};

template <class Ref, 
          category Cat = category::input,
          class Value = decay_t<Ref>,
          class RValueRef = add_rvalue_reference_t<remove_reference_t<Ref>>,
          class Diff = ptrdiff_t>
class any_view;
```

#### Considerations

##### Should the first argument be `Ref` or `Value`?

###### Option 1

If the first template parameter is `Ref`,

```cpp
template <class Ref, 
          category Cat = category::input,
          class Value = decay_t<Ref>>
```

For a range with a reference to `int`, one would write

- `any_view<int&>`

And for a `const` reference to `int`, one would write

- `any_view<const int&>`

In case of a generator range, e.g a `transform_view` which generates pr-value `int`, the usage would be

- `any_view<int>`

###### Option 2

If the first template parameter is `Value`,

```cpp
template <class Value, 
          category Cat = category::input,
          class Ref = Value&>
```

For a range with a reference to `int`, it would be less verbose

- `any_view<int>`

However, in order to have a `const` reference to `int`, one would have to explicitly specify the `Ref`, i.e.

- `any_view<int, category::input, const int&>`

This is a bit verbose. In case of a generator range, one would need to explicitly specify the `Ref` as well

- `any_view<int, category::input, int>`

###### Conclusion

There is no decision yet, this is open for discussion. For non-const reference case, Option 2 is less verbose, but for const reference case, Option 1 is less verbose.

##### `constexpr` Support

`constexpr` is not supported due to the implementation of SBO. There is no way, with the current working draft, to construct an object of different type on a `unsigned char[N]` or `std::byte[N]` buffer in `constexpr` context.

##### Move-only `view` Support

Move-only `view` is worth supporting as we generally support them in `ranges`. We propose to have a configuration template parameter `category::move_only_view` to make the `any_view` conditionally move-only. This removes the need to have another type `move_only_any_view` as we did for `move_only_function`.

We also propose that by default, `any_view` is copyable and to make it move-only, the user needs to explicitly provide this template parameter `category::move_only_view`.

##### Move-only `iterator` Support

In this proposal, `any_view::iterator` will be an exposition-only type. It is not worth making this `iterator` configurable. If the `iterator` is only `input_iterator`, we can also make it a
move-only iterator. There is no need to make them copyable. Existing algorithms that take "input only" iterators already know that they cannot copy them.

##### Is `any_view<T, category::contiguous>` Needed ?

`contiguous_range` is still useful to support even though we have already `std::span`. But `span` is non-owning and `any_view` owns the underlying `view`.

##### Is `any_view` const-iterable?

We cannot make `any_view` unconditionally const-iterable. Because if we did, `views` with cache-on-begin, like `filter_view`, `drop_while_view` can no longer fit in `any_view`.

One option is to make `any_view` conditionally const-iterable, via a configuration template parameter. However, this would make the whole interface super complicated. We will need to duplicate all the configuration template parameters, because those types, like `Ref` and `RValueRef` can be different between `const` and `non-const`.

For simplicity, the authors propose to make `any_view` unconditionally non-const-iterable.

##### `common_range` support

Unconditionally makes `any_view` a `common_range` is not an option. This would exclude majority of the stl `view`s. Add a configuration template parameter to make `any_view` conditionally `common_range` is an overkill. After all, if the user do needs `common_range`, they can do `my_any_view | views::common`. What is more, it would adds substantial complexity to make `any_view` a `common_range`. The authors believe it is not worth to add the complexity to have
`common_range` support.

##### `borrowed_range` support

It is simple enough to have `borrowed_range` support:

- 1. Add a template configuration parameter
- 2. Specialise the `enable_borrowed_range` if the template parameter is set to `true`

Therefore the author added the conditional `borrowed_range` support. However, `borrowed_range` is not a very useful concept in general. So this is open for discussion.

##### Valueless state of `any_view`

It is proposed to support strong exception guarantee, i.e. if swap/copy/move assignment fails, the two `any_view` objects need to be in their original states. If the underlying's move constructor is not `noexcept`, the only way to achieve the guarantee is not calling the move constructor, i.e. making the objects on the heap and these operations would just swap pointers.
And as a result, the "move-from" `any_view` object will be in a valueless state.

##### `any_view` move constructor cannot guarantee underlying's move constructor is called

This is an implementation detail. When object is on the heap, move constructor would just assign pointers and no move is happening.

##### ABI Stability

The interface needs to get right at the very beginning. It is very difficult to have incremental evolution. Adding any features into `any_view` would likely to change the implementation's vtable.

### Performance

One of the major concerns of using type erased type is the performance cost of `virtual`/indirect function calls. With `any_view`, every iteration will have three `virtual`/indirect function calls:

```cpp
++it;
it != last;
*it;
```

#### Micro benchmark : `vector` vs `any_view` on purely iterating

Purely profile the iteration between `std::vector` and `any_view`

```cpp
  std::vector v = std::views::iota(0, state.range(0)) | std::ranges::to<std::vector>();
  for (auto _ : state) {
    for (auto i : v) {
      benchmark::DoNotOptimize(i);
    }
  }
```

vs

```cpp
  std::vector v = std::views::iota(0, state.range(0)) | std::ranges::to<std::vector>();
  std::ranges::any_view<int&> av(std::views::all(v));
  for (auto _ : state) {
    for (auto i : av) {
      benchmark::DoNotOptimize(i);
    }
  }
```

##### -O0

```bash
Benchmark                                           Time             CPU      Time Old      Time New       CPU Old       CPU New
--------------------------------------------------------------------------------------------------------------------------------
[BM_vector vs. BM_AnyView]/1024                  +3.4488         +3.4487         10423         46371         10418         46347
[BM_vector vs. BM_AnyView]/2048                  +3.3358         +3.3375         21318         92432         21301         92396
[BM_vector vs. BM_AnyView]/4096                  +3.4224         +3.4237         41864        185137         41834        185061
[BM_vector vs. BM_AnyView]/8192                  +3.4665         +3.4665         83019        370802         82986        370659
[BM_vector vs. BM_AnyView]/16384                 +3.4586         +3.4574        166596        742785        166536        742319
[BM_vector vs. BM_AnyView]/32768                 +3.4413         +3.4416        333311       1480349        333151       1479723
[BM_vector vs. BM_AnyView]/65536                 +3.4166         +3.4154        667125       2946432        666900       2944657
[BM_vector vs. BM_AnyView]/131072                +3.4295         +3.4305       1335405       5915230       1334717       5913487
[BM_vector vs. BM_AnyView]/262144                +3.4320         +3.4329       2665004      11811264       2663916      11808776
OVERALL_GEOMEAN                                  +3.4278         +3.4281             0             0             0             0

```

##### -O2

```bash
Benchmark                                           Time             CPU      Time Old      Time New       CPU Old       CPU New
--------------------------------------------------------------------------------------------------------------------------------
[BM_vector vs. BM_AnyView]/1024                 +14.8383        +14.8421           315          4991           315          4989
[BM_vector vs. BM_AnyView]/2048                 +14.9416        +14.9453           632         10075           632         10071
[BM_vector vs. BM_AnyView]/4096                 +15.1943        +15.2000          1231         19942          1231         19936
[BM_vector vs. BM_AnyView]/8192                 +15.1609        +15.1626          2465         39835          2464         39820
[BM_vector vs. BM_AnyView]/16384                +13.8958        +13.8949          5386         80235          5384         80196
[BM_vector vs. BM_AnyView]/32768                +13.8638        +13.8647         10720        159341         10714        159264
[BM_vector vs. BM_AnyView]/65536                +13.6891        +13.6912         21772        319807         21758        319659
[BM_vector vs. BM_AnyView]/131072               +13.5340        +13.5338         44363        644768         44335        644359
[BM_vector vs. BM_AnyView]/262144               +13.5374        +13.5384         87600       1273476         87558       1272956
OVERALL_GEOMEAN                                 +16.0765        +16.0789             0             0             0             0
```

`any_view` is 3 - 16 times slower on iteration than `std::vector`. Yes, 3
virtual function calls vs `std::vector`, what can you expect? But this benchmark
is not a realistic use case. No one would create a `vector`, immediately create
a type erased wrapper `any_view` that wraps it and then iterate through it. The
same way, no one would create a lambda, immediately create a `std::function`
then call it.

#### Slightly more realistic case: A view pipeline vs `any_view`

Since `any_view` is most likely used in an ABI boundary, the benchmark will separate the creation of the view in a different TU. We will compare using the "`view` pipeline directly" with using `any_view` that wraps the `view` pipeline.

Consider the following case :

```cpp
// hpp file
struct Widget {
  std::string name;
  int size;
};

struct UI {
  std::vector<Widget> widgets_;

  ??? getWidgetNames() const;
};

// cpp file
??? UI::getWidgetNames() const {
  return widgets_ | std::views::filter([](const Widget& widget) {
           return widget.size > 10;
         }) |
         std::views::transform(&Widget::name);
}
```

The implementation of creation of the results of `getWidgetNames` is hidden in a separate translation unit.

And this is what we are going to measure

```cpp
  lib::UI ui{global_widgets | std::views::take(state.range(0)) |
              td::ranges::to<std::vector>()};
  for (auto _ : state) {
    for (auto const& name : ui.getWidgetNames()) {
      benchmark::DoNotOptimize(const_cast<std::string&>(name));
    }
  }
```

In first case, we use the `view` directly. It is tedious to spell the type, and it is impossible to use lambda now because the function type is part of the result type. We ended up writing something like this

```cpp
// hpp file
struct UI {
  std::vector<Widget> widgets_;
  // cannot use lambda because we need to spell the type of the view
  struct FilterFn {
    bool operator()(const Widget&) const;
  };

  struct TransformFn {
    const std::string& operator()(const Widget&) const;
  };
  std::ranges::transform_view<
      std::ranges::filter_view<std::ranges::ref_view<const std::vector<Widget>>,
                               FilterFn>,
      TransformFn>
  getWidgetNames() const;
};

// cpp file
bool UI::FilterFn::operator()(const Widget& widget) const {
  return widget.size > 10;
}

const std::string& UI::TransformFn::operator()(const Widget& widget) const {
  return widget.name;
}

std::ranges::transform_view<
    std::ranges::filter_view<std::ranges::ref_view<const std::vector<Widget>>,
                             UI::FilterFn>,
    UI::TransformFn>
UI::getWidgetNames() const {
  return widgets_ | std::views::filter(UI::FilterFn{}) |
         std::views::transform(UI::TransformFn{});
}
```

With using `any_view`, the interface looks much simpler

```cpp
// hpp file
struct UI {
  std::vector<Widget> widgets_;

  std::ranges::any_view<const std::string&> getWidgetNames() const;
};

// cpp file
std::ranges::any_view<const std::string&> UI::getWidgetNames() const {
  return widgets_ | std::views::filter([](const Widget& widget) {
           return widget.size > 10;
         }) |
         std::views::transform(&Widget::name);
}
```

##### -O0

```bash
Benchmark                                                        Time             CPU      Time Old      Time New       CPU Old       CPU New
---------------------------------------------------------------------------------------------------------------------------------------------
[BM_RawPipeline vs. BM_AnyViewPipeline]/1024                  +0.4290         +0.4291         78469        112130         78435        112090
[BM_RawPipeline vs. BM_AnyViewPipeline]/2048                  +0.4051         +0.4050        159225        223729        159161        223625
[BM_RawPipeline vs. BM_AnyViewPipeline]/4096                  +0.3568         +0.4021        331276        449466        320471        449331
[BM_RawPipeline vs. BM_AnyViewPipeline]/8192                  +0.4022         +0.4030        639566        896817        639056        896623
[BM_RawPipeline vs. BM_AnyViewPipeline]/16384                 +0.4148         +0.4144       1267196       1792804       1266743       1791639
[BM_RawPipeline vs. BM_AnyViewPipeline]/32768                 +0.4293         +0.4287       2522849       3606022       2522004       3603164
[BM_RawPipeline vs. BM_AnyViewPipeline]/65536                 +0.4199         +0.4201       5078713       7211428       5076977       7209978
[BM_RawPipeline vs. BM_AnyViewPipeline]/131072                +0.4170         +0.4170      10142694      14372118      10139299      14367292
[BM_RawPipeline vs. BM_AnyViewPipeline]/262144                +0.4358         +0.4357      20386564      29270816      20381118      29260958
OVERALL_GEOMEAN                                               +0.4120         +0.4172             0             0             0             0
```

##### -O2

```bash
Benchmark                                                        Time             CPU      Time Old      Time New       CPU Old       CPU New
---------------------------------------------------------------------------------------------------------------------------------------------
[BM_RawPipeline vs. BM_AnyViewPipeline]/1024                  +0.8066         +0.8064          3504          6330          3503          6327
[BM_RawPipeline vs. BM_AnyViewPipeline]/2048                  +0.7136         +0.7134          7339         12576          7335         12568
[BM_RawPipeline vs. BM_AnyViewPipeline]/4096                  +0.6746         +0.6748         14841         24853         14835         24846
[BM_RawPipeline vs. BM_AnyViewPipeline]/8192                  +0.6424         +0.6423         30177         49563         30163         49537
[BM_RawPipeline vs. BM_AnyViewPipeline]/16384                 +0.6538         +0.6539         60751        100468         60720        100427
[BM_RawPipeline vs. BM_AnyViewPipeline]/32768                 +0.6524         +0.6521        121345        200514        121303        200404
[BM_RawPipeline vs. BM_AnyViewPipeline]/65536                 +0.6582         +0.6579        240378        398604        240326        398440
[BM_RawPipeline vs. BM_AnyViewPipeline]/131072                +0.6861         +0.6860        484220        816458        484055        816109
[BM_RawPipeline vs. BM_AnyViewPipeline]/262144                +0.6234         +0.6235        991733       1609940        991406       1609560
OVERALL_GEOMEAN                                               +0.6782         +0.6782             0             0             0             0
```

This is slightly better now. It is about 40% - 70% slower on iteration. Although
it is still not very realistic. Nobody is spelling the concrete type of a view
pipeline. It is tedious to write and also it makes the implementation not
flexible. Any changes in the implementation would result in changing the type in
the header, which defeats the purpose of hiding implementation details in a TU.

#### Much more realistic case: A copy of `vector<string>` vs `any_view`

For situation like this, in most code bases in the wild, the interface is
probably returning a `std::vector<std::string>`, i.e. using `std::vector` as a
type erasure tool, with the cost of copying all the elements.

```cpp
// hpp file
struct UI {
  std::vector<Widget> widgets_;

  std::vector<std::string> getWidgetNames() const;
};

// cpp file
std::vector<std::string> UI::getWidgetNames() const {
  return widgets_ | std::views::filter([](const Widget& widget) {
           return widget.size > 10;
         }) |
         std::views::transform(&Widget::name) | std::ranges::to<std::vector>();
}
```


##### -O0

```bash
Benchmark                                                       Time             CPU      Time Old      Time New       CPU Old       CPU New
--------------------------------------------------------------------------------------------------------------------------------------------
[BM_VectorCopy vs. BM_AnyViewPipeline]/1024                  -0.5376         -0.5376        238558        110316        238396        110242
[BM_VectorCopy vs. BM_AnyViewPipeline]/2048                  -0.5110         -0.5110        454350        222187        454157        222104
[BM_VectorCopy vs. BM_AnyViewPipeline]/4096                  -0.4868         -0.4869        886121        454774        885773        454530
[BM_VectorCopy vs. BM_AnyViewPipeline]/8192                  -0.4766         -0.4769       1729318        905041       1728626        904303
[BM_VectorCopy vs. BM_AnyViewPipeline]/16384                 -0.4834         -0.4834       3462454       1788737       3461093       1788080
[BM_VectorCopy vs. BM_AnyViewPipeline]/32768                 -0.4858         -0.4729       7006102       3602475       6830520       3600306
[BM_VectorCopy vs. BM_AnyViewPipeline]/65536                 -0.4777         -0.4776      13741174       7176723      13734490       7175337
[BM_VectorCopy vs. BM_AnyViewPipeline]/131072                -0.4792         -0.4792      27501856      14321826      27494923      14318104
[BM_VectorCopy vs. BM_AnyViewPipeline]/262144                -0.4838         -0.4835      55950048      28883803      55912545      28879083
OVERALL_GEOMEAN                                              -0.4917         -0.4903             0             0             0             0
```

##### -O2

```bash
Benchmark                                                       Time             CPU      Time Old      Time New       CPU Old       CPU New
--------------------------------------------------------------------------------------------------------------------------------------------
[BM_VectorCopy vs. BM_AnyViewPipeline]/1024                  -0.8228         -0.8228         35350          6264         35330          6262
[BM_VectorCopy vs. BM_AnyViewPipeline]/2048                  -0.8250         -0.8250         71983         12596         71953         12590
[BM_VectorCopy vs. BM_AnyViewPipeline]/4096                  -0.8320         -0.8320        148942         25018        148873         25005
[BM_VectorCopy vs. BM_AnyViewPipeline]/8192                  -0.8276         -0.8276        291307         50234        291198         50209
[BM_VectorCopy vs. BM_AnyViewPipeline]/16384                 -0.8304         -0.8304        590026        100058        589571        100020
[BM_VectorCopy vs. BM_AnyViewPipeline]/32768                 -0.8301         -0.8300       1175121        199685       1174459        199614
[BM_VectorCopy vs. BM_AnyViewPipeline]/65536                 -0.8297         -0.8298       2363963        402634       2363007        402209
[BM_VectorCopy vs. BM_AnyViewPipeline]/131072                -0.8340         -0.8340       4841300        803467       4838717        803175
[BM_VectorCopy vs. BM_AnyViewPipeline]/262144                -0.8463         -0.8463      10412999       1600341      10410152       1600078
OVERALL_GEOMEAN                                              -0.8310         -0.8310             0             0             0             0
```

Boom! `any_view` is 50% - 80% faster. In the test cases, 10% of the `Widget`s were filtered out by the filter pipeline and the `name` string;s length is randomly 0-30. So some of `string`s are in the SBO and some are allocated. Yes this code pattern is very common: making the code simple and clean at the cost of copying data, even though most of the callers don't need the ownership at all.

#### Some optimisations in the wild: `vector<reference_wrapper>` vs `any_view`

People who care about some level of the performance (and at the same time, need to keep a clean ABI boundary) sometimes make it return `std::vector<std::reference_wrapper<const std::string>>` to save the copy of those `string`s. (Note: `std::string_view` is not used in this example, because this example is to demonstrate a more generic case rather than just `std::string`.)

```cpp
// hpp file
struct UI {
  std::vector<Widget> widgets_;

  std::vector<std::reference_wrapper<const std::string>> getWidgetNames() const;
};

// cpp file
std::vector<std::reference_wrapper<const std::string>> UI::getWidgetNames()
    const {
  return widgets_ | std::views::filter([](const Widget& widget) {
           return widget.size > 10;
         }) |
         std::views::transform(
             [](const Widget& widget) { return std::cref(widget.name); }) |
         std::ranges::to<std::vector>();
}
```

##### -O0

```bash
Benchmark                                                             Time             CPU      Time Old      Time New       CPU Old       CPU New
--------------------------------------------------------------------------------------------------------------------------------------------------
[BM_VectorRefWrapper vs. BM_AnyViewPipeline]/1024                  -0.3744         -0.3744        183525        114814        183467        114768
[BM_VectorRefWrapper vs. BM_AnyViewPipeline]/2048                  -0.3757         -0.3759        368639        230131        368529        229985
[BM_VectorRefWrapper vs. BM_AnyViewPipeline]/4096                  -0.3689         -0.3691        736658        464898        736390        464614
[BM_VectorRefWrapper vs. BM_AnyViewPipeline]/8192                  -0.3821         -0.3820       1497364        925176       1496487        924814
[BM_VectorRefWrapper vs. BM_AnyViewPipeline]/16384                 -0.3930         -0.3929       3004560       1823724       3002700       1823008
[BM_VectorRefWrapper vs. BM_AnyViewPipeline]/32768                 -0.3841         -0.3841       5914763       3642657       5911948       3641151
[BM_VectorRefWrapper vs. BM_AnyViewPipeline]/65536                 -0.3848         -0.3849      11823432       7273326      11820603       7270880
[BM_VectorRefWrapper vs. BM_AnyViewPipeline]/131072                -0.3864         -0.3863      23783665      14592934      23776966      14591191
[BM_VectorRefWrapper vs. BM_AnyViewPipeline]/262144                -0.3834         -0.3834      47455488      29259513      47445000      29253042
OVERALL_GEOMEAN                                                    -0.3815         -0.3815             0             0             0             0
```

##### -O2

```bash
Benchmark                                                             Time             CPU      Time Old      Time New       CPU Old       CPU New
--------------------------------------------------------------------------------------------------------------------------------------------------
[BM_VectorRefWrapper vs. BM_AnyViewPipeline]/1024                  +1.7685         +1.7690          2175          6022          2174          6020
[BM_VectorRefWrapper vs. BM_AnyViewPipeline]/2048                  +1.7366         +1.7368          4476         12248          4474         12244
[BM_VectorRefWrapper vs. BM_AnyViewPipeline]/4096                  +1.4270         +1.4270         10039         24363         10034         24354
[BM_VectorRefWrapper vs. BM_AnyViewPipeline]/8192                  +0.8925         +0.8927         25901         49018         25888         48999
[BM_VectorRefWrapper vs. BM_AnyViewPipeline]/16384                 +0.6378         +0.6378         59587         97590         59567         97558
[BM_VectorRefWrapper vs. BM_AnyViewPipeline]/32768                 +0.5174         +0.5179        129966        197216        129883        197145
[BM_VectorRefWrapper vs. BM_AnyViewPipeline]/65536                 +0.4826         +0.4826        265071        392994        264940        392799
[BM_VectorRefWrapper vs. BM_AnyViewPipeline]/131072                +0.4483         +0.4484        549000        795131        548800        794858
[BM_VectorRefWrapper vs. BM_AnyViewPipeline]/262144                +0.2827         +0.2827       1240717       1591496       1240390       1590989
OVERALL_GEOMEAN                                                    +0.8370         +0.8371             0             0             0             0
```

This set of results are not very consistent. Depending on the optimization level, `any_view` can be 30% faster or 80% slower.

#### Conclusion?

In the cases where type erasure is needed, the performance is not bad and sometimes even faster than the most common solution today : copying data into the `vector`.

Implementations can explore optimisations on algorithms, such as `for_each`, for special handling of `any_view`

# Implementation Experience

`any_view` has been implemented in [@rangev3], with equivalent semantics as
proposed here. The authors also implemented a version that directly follows the
proposed wording below without any issue [@ours].

# Wording

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
