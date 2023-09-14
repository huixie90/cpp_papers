- [TODO] TK: common-arg is very generic (it is exposition-only); can we give a different name?
	 JW: people should think of a better name - we don’t do that now
- [DONE] JW: maybe_const should be hyphenated
- [TODO] TK: [on advance-bwd] we always compute the end but we don’t need to do that for common; in Cartesian product there is something like that; the question is whether we want to do it here, too?
         JW: anybody want to have that replaced by the tool from Cartesian?
         JW: implementations can do that anyway
         TK: equivalent => the + is observable
         TS: the random access and not common is the odd one: we could require the range to just give it to you
         TS: the treatment is inconsistent for this type of ranges
         JW: so we would have another constexpr if ?
         TK: yes and use the helper from Cartesian
         HX: what do you about bidir ranges: also do the common thing?
         TS: yes
         HX: we could but I want to point out that such kind of ranges exist like iota(0, 1, l) is such a range
         TK: in the prev() we already have the relevant helper function
         TS: the original case is already contrived
         TS: if you want your bound to a different type there is a chance that your increment will never actually hit the bound
         TS: I don’t think we need to bend over backwards to accommodate common everywhere
         TK: we can make this range random access if it the underlying are random access and sized
         TK: why would we impose a requirement which isn’t needed
         TS: you don’t need random access but you can work around that
         JW: is this an SG9 discussion? That seems to be a design question
- [TODO] TK: same discussion: the ranges::end(...) isn’t required to be sized, i.e., we need to use a common range again
         TK: I guess, that is the reason why we shouldn’t bother
         JG: aside from that point, let’s check if we are happy with the changes
         TK: it should be back to the prose if we need the end
         JM: but we don’t know if there is an end
         TK: then it is up to the implementation to do it properly: it is always possible to do it
         JM: does the upcoming common-range discussion affect the correctness?
         TK: yes, if it is a common-range then that is the case
-        HX: [op-] We rephrased the whole specification
- [DONE] JG: I think we wanted to have the “let” refer to the sum
         JM: the other operator the same: the “for every …” should be after the let
         JM: and we need another earmark
-        HX: next change [iter_swap]
- [NAD]  JM: isn’t there iterator_reference_t instead of iterator_t
         DK: the (its, its) is odd
         JM: it says afterwards but it unclear what the respectively refers to
         JW: the views is the unexpanded pack; if we have better wording use it, otherwise leave it
- [Done] JM: why is there are const at the end?
         JW: we normally have west-const

- [Done] JW: based on the chat the short-circuiting in the requires clause doesn’t happen
         JW: TS what do you think of TK’s proposal in the chat? This way we could avoid the fold-expression if the first bit fails
