# TODO

## Globals
- implment storage such that a `SymbolId` takes us to a heap cell
- Align VM globals ops `MKGLOBAL/LOADGLOBAL`
- Update codegen for `define` to emit the chosen global initialization sequence.
- Update codegen for global calls to use the global load op then `CALL`.
- Fix `emit_define` so any `MKCLOSURE` operand is a code address, not a `SymbolId`.
- Add a label/fixup pass so `define` can reference function entry addresses.
## Frontend
- Finish `letrec` desugaring in `Parser::create_letrec`.
- Decide/implement `set!` scoping behavior (if special-casing, add to `Scoper::resolve`).
- Add regression tests for `letrec` and `set!` parsing/lowering.
## Glue (Core -> VM)
- Implement codegen for `emit_var`, `emit_apply`, `emit_lambda`, and scoped `emit_define`.
- Add label/fixup pass so `define` and lambdas can reference code addresses.
- Decide how locals vs globals are represented in codegen (stack slot vs heap cell).
- Add `set!` codegen to emit the right store op for globals/locals.
## VM
- Finish `Cell`/heap model so captured vars and globals are stored consistently.
- Fix `MKCLOSURE` capture storage to match the `Cell` representation.
- Add globals ops (`GLOAD`/`GSTORE` or `LOADGLOBAL`/`MKGLOBAL`) and wire them in.
- Add a mutation/store op for `set!` and implement it in the VM.
