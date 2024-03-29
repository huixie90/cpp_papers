# Done

- [Done] 5.2 needs to go
- [Done] we don't need 24.7.?.1 p2.3 and strick "if this expression is well formed" from 2.2
- [Done] in 24.7.?.2 add exposition-only for these concepts
- [Done] we need to require the entire expression to be equality preserving: we could add CC's code
- [Done] on p3 the pack needs to be expanded as I put into the chat
- [Done] remove the requires from the default constructor an remove the initialization of views_
- [Done] in the size the make_unsigned_t should be make_unsigned_like_t as it could be integer-like
- [Done] on size() the {} need to be ()
- [Done] also CT(0) isn't needed - the sequence is always non-empty
- [Done] value_type could use concat_value_t
- [Done] we don't need the friends
- [Done] the requires on the default ctor isn't needed
- [Done] and remove initialization from base-iter()
- [Done] op-- is missing a ;
- [Done] there is iterator const& which should be const iterator& (this is editorial)
- [Done] 2.1 should say is_reference_v
- [Done] all the other bullets need to start with "otherwise"
- [Done] you should use ranges::distance and then you don't need them
- [Done] instead of ranges::size this should be ranges::distance and we don't need the static_cast.
- [Done] all the steps need to cast to that iterator's difference_type
- [Done] al the steps need to be cast to range::diffence_type
- [Done] the use of maybe-const on the requirement of the constructor: I think we removed them elsewhere as we Const is true
- [Done] OK, we can keep it as is but I prefer <
- [Done] for op*() p10 can use concat_reference_t
- [Done] we don't need the preconditions: they come from the "Equivalent to:" => strike p13 and p15
- [Done] you need p17 but you can rid of p19
- [Done] we should spell out what reference is in 2.1
- [Done] mattermost: CC: we should ask the aurthors to remove the section numbers  (replaced "27.4" with "?.?")
- [Done] (EXTRA TODO) Use std::print instead of std::cout in the example.
- [Done] for the op++ and the others we should use the random-access from zip_view
  - Hui: zip_view's op++ requires all-random-access but we need random_access + sized
  - Levent: Right. Also, do they mean "op+"?
  - Hui: moved `all-random-access` to [range.adaptor.helpers] and reuse
- [Done] the concepts from zip_view [rang.zip.iterator] need to move to a different section: move to [range.utility.helper]
  - Hui: I think they refer to all-random-access, all-bidirectional and all-forward. In practice, we can only use all-forward. Not sure if it is worth
  - Levent: Agree. Also Casey says "this is a change which can be done outside this paper", anyway.
  - Hui: Moved
- [Done] (1.3) use all_forward
  - Hui: We could. but it seems to be more effort to reuse this simple concepts than it saves. what do you think?
  - Levent: I am mildly for it: It is good for a standard-wide glue. I suggest we walk the extra (micro-)mile.
- [Done] the things which use the general concepts are replaced: op++, op<, etc.
  - Hui: Not sure if I understand this. I guess they could either mean
    - 1. we could replace `random_access_range<maybe-const<Const, Views>>&&...` with `all-random-access`, or
    - 2. some of our ops requires `concat-random-access` but others requires `random_access_range<maybe-const<Const, Views>>&&...`. We should be consistent?
  - Levent: For 2, our use isn't accidental: operator< etc can be implemented
    without size. So, for those, we can use `all-random-access`. Which is also
    what TK was saying (only the "relational operators"). I think we can also
    update `concat-random-access`'s def'n to use `all-random-access` for bonus.

- [Done] we should use a single definition for the relational operators similar to basic_const_iterator [const.iterator.ops] p19
  - Hui: I just learned we can specify that way. Not sure if you like it?
  - Levent: Love it. (https://eel.is/c++draft/const.iterators.ops#17 right?)

- [Done] mattermost: TZ/CC: wording the concat-bidirectional
  - Hui: From the chat history they don't seem to be happy about our wording and had some alternative wordings in the chat
  - Levent: Leave this to next round?
  - H: I am bit confused with "Model" vs "Model and Satisfy"...
  - L: I am not sure where we pulled this from. I found this: https://github.com/cplusplus/draft/blob/1be4801ac1f90aca9a8f5804a48e8bcd082f5bb9/papers/n4821.md?plain=1#L564
    It seems if we simply say "models" that should suffice here.

- [Done] when doing zip_view and cartesian_product we added maybe_const so we don't need to do that everywhere and we should do that here, too
  - Hui: ?
  - Levent: I think they're talking about the original local util (e.g.
    https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1035r3.html#zip_viewrs-iterator-exposition-only)
    becoming a standard exposition symbol (https://eel.is/c++draft/ranges in
    synopsis) after zip and cartesian_product was in. Wants to make sure we
    don't define it again. (But we don't. So...)
  - Hui: Maybe they meant that we should add `Const` as template parameter for concepts like `concat-random-access` and `concat-bidirectional`, like what they did for `all-random-access` etc. So that we don't repeat the `maybe_const` in the every usage of `concat-random-access`? What do you think?
  - Levent: Makes sense. Agree Also the naming can be more consistent with https://eel.is/c++draft/range.cartesian#view, 
  `concat-is-random-access` instead of `concat-random-access` etc.
  - Levent: Also, instead of `Rs` as the name of the pack, we should use `Views`. But careful if we really mean View and not Range
    in all these.

- [Done] that "expect" needs to be "except"
  - Hui: could not find any expect
  - Levent: I checked earliest revisions. No "expect" indeed. Not sure what this was.
  - Hui: I found that in the mattermost chat, Tomasz suggested wording for concat-bidi which has expect. It should refer to that. I also used his suggested wording now

# Todos

- [TODO] do we want to avoid emplacing ranges we don't need to use?
  - Hui: Good point. I think this refers to `advance_fwd` and `advance_bwd`. Maybe we can do something smarter.
  - Levent: Oof. Sure, empty subranges. Let the compiler authors do the optim?
    (not sure how refined these "equivalent to:" snippets should be)
  - Hui: at the moment, if we do `i + n`, if `n` is large enough to skip several ranges in between, we currently specify that it needs to be emplaced for every single begin iterators of the ranges in between, which could be skipped. Not sure how we can do this with an elegant way

- [TODO] explain why this is over-constrained
  - Hui: Not sure what this refers to
  - Levent: Discussion was that only the first N-1 random access ranges also
    need to be sized. Last one does not. And that is our current
    over-constraint. If this is true, then we can use a macro similar to
    `concat-bidirectional`. But I think we thought about this and decided that
    we need the last one sized also. I didn't remember then, and for sure don't
    remember now :P
  - Hui: Indeed I think the last range does not need to be sized with careful implementation. If you look at the implementation (and the spec) for `x - y`, we calculate all the sizes. But the last value is not used. We could avoid it by carefully doing the bound checking, which is a bit more complicated. In addition, in `x - default_sentinel`, we have to use the last range's size if it is not `common_range`. But this operation (`x - default_sentinel`) is not required for `random_access`

- [TODO] put the front matter after the iterator_category
  - Hui: I think this refers to all the preconditions about variant's valueless state. It is good idea but I don't know how to do it. I mean, how to specify what operations are allowed and what are not. Need prio examples.
  - Levent: We don't have any operations that are allowed. (no?)
  - Hui: Well the generated operations like the destructor and the assignment operators should be allowed. (no?)
  - Right. And for the friend members it gets tricky to rely only on front matter. I actually attempted a version, but
    it looked uglier in the end. Not sure about this one.


- [TODO] I think the authors need to think about iter_swap and provide rational
  - Hui: sg9?
  - Levent: Let's just remove it.
  - Hui: I thought Tim brought another issue that even the default behaviour (without our specialisation) of `iter_swap` is still problematic for `concat(strings, string_views)`.
  https://godbolt.org/z/49esvec96
  - Slightly modified example: https://godbolt.org/z/na8haa3Ex
  
# Full transcript

- JG: this is for C++26
- JG presenting
- TK: do we still use tie?
- TS: this paper doesn't actually use tuple or pair
- TS: 5.2 is already done
- JG: this came in with zip or so.preview
- TS: [action] 5.2 needs to go
- TK: what about concat of no views?
- JG: don't do that
- TS: ill-formed
- CC: [action] we don't need 24.7.?.1 p2.3 and strick "if this expression is well formed" from 2.2
- TK: do we want to allow concat_view of one view
- RD: the pre-matter does talk about 0 and 1 view and it is all_view
- TK: but I can still use concat_view explicitly with 1 object
- TS: that doesn't seem justified: you can get a take_view of a string_view and get a string_view back
- TK: do we have common_reference_t with single argument?
- TS: I'm pretty sure we do: we can have it with any number of argument > 0
- TS: we should change the example to use print instead of cout
- JG: is there something special about concat-random-access which we don't have under some other concept?
- CC: in general these concepts are models requirements and are needed when implementing the view
- TK: [action] when doing zip_view and cartesian_product we added maybe_const so we don't need to do that everywhere and we should do that here, too
- RD: do we have a recommended way to word the concepts like the concept-indriectly-readable
- RD: how consistent do we want these wordings to be be?
- RD: the approach used here is otherwise only by iota_view
- RD: there is "exposition-only" repeated for iota_view
- JG: [action] in 24.7.?.2 add exposition-only for these concepts
- JG: we tend to remove the section names
- TS: the concept uses static_cast - where do we require the implicit conversion work?
- TK: we required common_refernce_t but not common_reference_with
- CC: this is the impl-version; the other requires common_reference_with
- TS: we require each of the iterator types to have something convertible_to but I don't see we check that
- JG: so we have a required change
- TS: it should require convertible_to in addition to the two expressions
- TS: if you do things multiple times you end up getting totally nonsensical result
- TK: do you mean implicate conversions?
- CC: do we change to the requires expression to what I typed in the chat?
- TS: [action] we need to require the entire expression to be equality preserving: we could add CC's code
- TK: I have another variation how to spell that in the chat
- TS: we can do that, too - I don't mind how we spell it
- TK: I believe it is the same thing
- RD: [action] on p3 the pack needs to be expanded as I put into the chat
- TS: the types model a concept; this one is ugly
- TS: it needs to say both satisfies and models
- JG: why the requirement on 3.2?
- CC: we should add a "The" at the begining of 3.1
- CC: [action] that "expect" needs to be "except"
- TS: I'd like to say this in code instead of word - or pseudo code
- JG: TS's homework is to put suggested wording into the chat or send it to the author
- TK: for the end: the last element is always the last element of the pack and then we can apply constness when needed (see chat)
- TS: on the concept view: we don't need the default_initializable
- JG: [action] remove the requires from the default constructor an remove the initialization of views_
- TK: [action] in the size the make_unsigned_t should be make_unsigned_like_t as it could be integer-like
- RD: the as_const stuff is just missing an article in multiple places
- CC: [action] on size() the {} need to be ()
- TS: [action] also CT(0) isn't needed - the sequence is always non-empty
- TK: [action] value_type could use concat_value_t
- TS: the maybe-const needs to be there
- JG: unless we restructure as TK has mentioned
- TK: [action] we don't need the friends
- JG: do we need the = base-iter();?
- TK: it is a variant: it will construct the first element
- TK: [action] the requires on the default ctor isn't needed
- TK: [action] and remove initialization from base-iter()
- TS: it is weird that the default is tied to the first iterator in the range specifically
- DK: the default ctor doesn't mean anything
- TS: yes, it can only be compared to itself
- DK: so we could have a special variant member
- TS: it could use monostate instead of depending on the first iterator
- TS: I would add monostate to the end and initialize with that
- TK: but why at the end?
- TS: I think it is nicer at the end if we add monostate
- TK: I'd be more comfortable that it is default constructible if any iterator is default constructable
- TS: that is harder to implement; it is simple to add monostate and always use that
- TS: but then everything visiting the variant is harder - so I take that back: I think I can accept the weirdness
- RD: [action] op-- is missing a ;
- TK: [action] for the op++ and the others we should use the random-access from zip_view
- TS: [action] the concepts from zip_view [rang.zip.iterator] need to move to a different section: move to [range.utility.helper]
- TS: maybe we should promote [range.utility.helper] to something higher
- CC: seems odd that we have compile-time and run-time range helper; this is a change which can be done outside this paper
- TK: all the relational operators should use the random-access
- JG: [action] the things which use the general concepts are replaced: op++, op<, etc.
- CC: [action] there is iterator const& which should be const iterator& (this is editorial)
- CC: [action] (1.3) use all_forward
- TK: I thin we can remove parenthesis (2.2.1) around derived from and use binary version of ...
- TS: but then you need them at the end; I don't think is helpful
- TK: (2.2.3) needs to check forward_iterator
- TS: we are just making a C++17 forward into a C++20 input iterator - this is going to be pervasive
- TS: having the category lie within the type it isn't that bad
- TK: I would still prefer that not to lie if we can avoid it
- TS: this one only lies if the iterator lies to us
- TS: [action] 2.1 should say is_reference_v
- TS: [action] all the other bullets need to start with "otherwise"
- TK: I'd prefer using N < (sizeof...(Views)
- TS: you'd still need the -1
- TK: [action] OK, we can keep it as is but I prefer <
- RD: I put some formatting changes to the chat
- TS: on prev(): the block is cartesian-common-arg and use that here: we could get rid of the constexpr
- TS: [action] instead of ranges::size this should be ranges::distance and we don't need the static_cast
- TS: [action] all the steps need to cast to that iterator's difference_type
- TK: if we need to skip multiple ranges and we are emplacing all the iterators and this is observable
- TS: if we skip a range entirely we just need to call distance
- TS: [issue] do we want to avoid emplacing ranges we don't need to use?
- TK: this applies to both advance forward and backward
- TS: for advance backward we can also use the common logic
- CC: the static_cast are not needed
- TS: [action] you should use ranges::distance and then you don't need them
- TS: [action] al the steps need to be cast to range::diffence_type
- TK: the requirements for random access is too strict: we don't need sized for the last range
- TK: we could use the same macro as for bidirectional
- SLY: what TK brought we considered; I think that was discussed: there was a reason why we didn't go for that but I can't remember right now
- TK: we could look at op- between iterator which is related: this one works
- TK: the one with the sentinel need to require sized sentinel
- SLY: we are still over constrained for some reason
- TS: we don't need sized sentinel: we can get (it - begin)
- JG: [action] explain why this is over-constrained
- TK: if we think the last range is special for bidirectional it should be consistently special
- DK: can we figure that out off-line?
- JG: we will certainly need to see this paper again but let's go through all sections
- CC: [action] the use of maybe-const on the requirement of the constructor: I think we removed them elsewhere as we Const is true
- JG: did we have this in the synopsis as well?
- TK: [action] for op*() p10 can use concat_reference_t
- TS: [action] we don't need the preconditions: they come from the "Equivalent to:" => strike p13 and p15
- DK: also p17 and p19?
- TS: [action] you need p17 but you can rid of p19
- RD: where does "reference" come from?
- TS: [action] we should spell out what reference is in 2.1
- TK: in the op[] I'd prefer that we get an iterator i and then dereference *i and we don't say how we get it
- JG: are the precondition needed here?
- DK: I think when we have "Equivalent To" and it says on the equivalent things we can drop them
- CC: I'd prefer the precondition on the op==
- TS: can we drop the precondition and rather say in the front-matter say "if valueless_by_acception() is true the iterator is singular"
- CC: it isn't directly clear to me that I can't operate on singular iterators and they kind of override the generic requirements
- CC: we can do it at least with front matter for this iterator
- TS: if you think singular is insufficient, we can say that all operations on singular iterators for concat iterator undefined
- TS: [action] put the front matter after the iterator_category
- TK: [action] we should use a single definition for the relational operators similar to basic_const_iterator
- [const.iterator.ops] p19
- TK: I'll provide a better link
- TK: op- for default_sentinel/iterator looks odd
- TK: I would prefer to use this is the primary and the other direction negative; it isn't wrong though
- CC: iterator const& needs to be fixed to become const iterator&
- TK: if we factor maybe-const types in the exception specification should be better
- TK: do we want to compute the conditional noexcept on all combination?
- TS: that is quadratic
- CC: the visit is on the same complexity
- TK: I'm not sure if iter_swap behaves correctly when the common_reference_t is prvalues
- TS: is that an indirectly swappable thing?
- CC: this is a question about iter_swap not about this paper
- TK: over here we are hiding swapping iterators with different types behind an interface with one type
- TK: we should require the reference types to be swappable
- CC: that is circular
- TS: I think the authors need to think about iter_swap and provide rational
