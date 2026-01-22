Closures are made up out of practically two components:
* Capture Analysis
* Runtime Code envs

In compile time, we perform the capture analysis, where we identify the free variables of the closure. At compile time (code generation) when the closure is created we push to the VM heap the closure location of the code and the variables that were captured. Then on call, we go into the heap and retrieve the variable, load the free variables and execute code as usual.
