[Done] JW: “equavalent” -> “equivalent”
[Done] ??: “logcial” -> “logical”
[Done] TK: on p46: I wonder if we should swap the conditions
       JW: why does it make a difference?
       TK: we don’t need to instantiate the const-reference-t again and it can short-circuit
       JW: any objection to switching the order
[Done] TS: iter_reference_t<iterator> instead of concat-reference-t<...>?
       JW: yes, that should be the same condition
       TS: that is just stylistic
       JW: it uses less exposition-only stuff
       (swappable_with<iter_reference_t<iterator>, iter_reference_t<iterator>> && indirectly_swappable<iterator_t<maybe-const<Const, Views>>> && …)
[TODO] TK: there is a rewording in the chat avoiding N:
[TODO] “The exception specfication is equivalent to the logical AND of noexcept(ranges::swap(...)) and the following expression for every integer …”
[TODO] JW: I think I prefer that because we start with what we are saying
[TODO] TS: we need to reword that because std::get is actually throwing
[TODO] TS: we don’t care about std::get throwing here, though
[TODO] JW: I don’t have a quick solution to that! That will need some work off-line
[TODO] TS: we could use get_if but that is ugly: using a * and taking the address with addressof.
[TODO] TS: we could use declval and write it as a full expression
[TODO] TK: another suggestion in the chat
[TODO] JW: that would be a pack of types, i.e., we’d need to declval it - OK, now corrected:
[TODO] “noexcept(ranges::iter_swap(IT, IT)) for IT being lvalue of type iterator_t<maybe-const<Const, Views>> const, for each type in the pack.”
[TODO] BST: should we use “conjunction” instead of “logical AND”
[TODO] DK: I prefer logical and because I know what it means; conjunction I keep needing to look up
[TODO] TK: wording for p4 in the chat:
[TODO] “Exception specification is equivalent to:
[TODO] noexcept(ranges::swap(*x, *y)) && ... && noexcept(ranges::iter_swap(its, its))
[TODO] where its is a pack of lvalues of type iterator_t<maybe-const<Const, Views>> const respectively.”
[TODO] HX: in the previous wording all ranges needed to be sized, now the last range doesn’t need to be sized
[TODO] JW: the last range doesn’t have to be sized because we don’t skip to the end of it
[TODO] JW: is there an efficient way to get the last element of a pack
[TODO] SLY: there is a simple implementation in the paper, see https://godbolt.org/z/TG8MhbWqq
[TODO] TK: on concat-is-bidirectional: we check it twice
[TODO] TK: we have similar things in the cartesian_view: we could extract the logic for cartesian-is-common
[TODO] JW: that makes sense
[TODO] TS: (2.2.4) p5, p6 “offset + steps < …” doesn’t need a static_cast: you can do signed integer comparison with different sizes, i.e., this one doesn’t needs a cast
[TODO] TS: I think the advance-fwd thing also doesn’t need a static_cast: either it produces the right type or we are in integer land
[TODO] HX: I think the issue was that the conversion was explicit
[TODO] TS: only when you are possibly truncating but widening does not
[TODO] JW: but without the cast we may end up with the wrong type
[TODO] TS: they are on the same type
[TODO] TS: the two casts which are required is on the addition to the iterator
[TODO] JW: so remove for all that are not += and the equivalence a bit further on (-=)
[TODO] TK: begin()/end() use {} to initialize it: they should use ()
[TODO] JW: we only use {} if we really want to prevent narrowing conversions
[TODO] TS: on p19: the static_cast isn’t necessary: the conversion is implicit
[TODO] JW: it also lacks the >
[TODO] TS: on p34.1: I don’t think it needs a static_cast but it needs to say what type s is.
[TODO] TS: there is one s total not one s per integer
[TODO] TS: it is probably easier to just static_cast s
[TODO] TK: we may overflow: the difference type may not be able to hold that
[TODO] TS: if the difference type can’t represent the distance between two iterators that is already broken even without using -
[TODO] TS: on p34.3: the static_cast is unnecessary
[TODO] JW: same on p36
[TODO] TS: that has the same issue with having just one s and it doesn’t actually say what the type is
[TODO] TS: I don’t see why we say “denotes the difference” instead of using the expression
