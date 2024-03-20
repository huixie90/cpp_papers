# [range.concat.overview]

- CPO remove `and views::all(Es...) is a well formed expression`
- Example uses `std::print` in a loop

# [range.concat.view]

- p.4. change concat-is-bidirectional to match style of concat-is-random-access
- p.7 Use pack indexing

# [range.concat.iterator]

- All `get` to `std::get`
- All `std::size_t` to `size_t`
- Remove p.29 Precondition.  (and renumbering the rest)
- P30/32 (new 29/31): make temp copy
- P34/36 (new 33/35): "for every integer" change
- P37/P39 (new 36/38): "requires changes for `i - default_sentinel`"
