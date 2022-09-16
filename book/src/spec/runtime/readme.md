# Runtime

Despite hololisp being interpreted language, it does compile to bytecode prior to execution, similar to [Lua](https://www.lua.org/). This allows for much faster code execution than naive AST walker. Additionally, it allows doing several operations that require control over program execution, like performing optimizations (tail call unwinding) and doing garbage collection (it is possible to do in ast walker using very nasty hacks).

