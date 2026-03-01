# TODO

## set! scoping
- Confirm `set!` does not introduce a new lexical scope.
- In `Scoper::resolve`, treat `(set! name expr)` as:
  - Resolve `name` like any other symbol in the current scope chain.
  - Recurse into `expr`.
- Ensure the `set!` keyword itself is not resolved as a variable.

## Parser/keyword updates
- Ensure `set!` is in the parser keyword table so it is not parsed as a normal symbol.
