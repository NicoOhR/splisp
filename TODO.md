# TODO

## Globals
- Add VM global table: map `SymbolId -> heap_idx` for global closures.
- Add `GLOAD`/`GSTORE` opcodes (or equivalent) to access globals.
- Update codegen for `define`: `MKCLOSURE <code_addr>` then `GSTORE <symbol_id>`.
- Update codegen for global function calls: `GLOAD <symbol_id>` then `CALL`.
- Fix `emit_define`: `MKCLOSURE` operand should be a code address, not `SymbolId`.
- Add a label/fixup pass so `define` can reference function entry addresses.
- (Optional) Add debug map `pc -> source line` separate from control-flow labels.

## Define
- Allow for internal defines
