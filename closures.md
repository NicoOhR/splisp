Closures are made up out of practically two components:
* Capture Analysis
* Runtime Code envs

In compile time, we perform the capture analysis, where we identify the free variables of the closure. At compile time (code generation) when the closure is created we push to the VM heap the closure location of the code and the variables that were captured. Then on call, we go into the heap and retrieve the variable, load the free variables and execute code as usual.

Current VM stack convention:

* Before `mkclosure <code_addr>`: `... captured_1 captured_2 ... captured_n n`
* After `mkclosure <code_addr>`: `... closure_handle`
* Before `call`: `... closure_handle`
* After `call`: `... captured_1 captured_2 ... captured_n` and `pc = code_addr`

`mkclosure` uses move-capture semantics. The captured values are removed from the
data stack and stored in the heap environment. `call` clones those captured
values back onto the data stack, preserving their original order.
