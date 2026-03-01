# TODO

## Globals
- Add VM global table: map `SymbolId -> heap_idx` for global closures.
- Add `GLOAD`/`GSTORE` opcodes (or equivalent) to access globals.
- Update codegen for `define`: `MKCLOSURE <code_addr>` then `GSTORE <symbol_id>`.
- Update codegen for global function calls: `GLOAD <symbol_id>` then `CALL`.
- Fix `emit_define`: `MKCLOSURE` operand should be a code address, not `SymbolId`.
- Add a label/fixup pass so `define` can reference function entry addresses.
- (Optional) Add debug map `pc -> source line` separate from control-flow labels.

## set! scoping
- In `Scoper::resolve`, treat `(set! name expr)` as:
  - Resolve `name` like any other symbol in the current scope chain.
  - Recurse into `expr`.
- Ensure the `set!` keyword itself is not resolved as a variable.

## Parser/keyword updates
- Ensure `set!` is in the parser keyword table so it is not parsed as a normal symbol.
