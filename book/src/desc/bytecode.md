# Bytecode

It is said that one who works on certain level of abstraction needs to know what happens below and above it.

Hololisp compiles to custom bytecode format prior to execution. It provides with minimal set of instructions needed to execute lisp code.

Each bytecode unit corresponds to a function or to a file context. Bytecode is a series on 1-byte bytecode ops with optional data with a
list of constant values used in this function.

> Bytecode is used in languages that don't compile to machine code (C). For example, Python compiles to bytecode just
> before the execution (not to be confused with [JIT](https://en.wikipedia.org/wiki/Just-in-time_compilation)). Java
> compiles to bytecode [ahead of time](https://en.wikipedia.org/wiki/Just-in-time_compilation).

## Why one needs bytecode in interpreter?

Programming language interpreters can be constructed perfectly fine without the need to compile to some intermediate
format before execution. This could drastically simplify interpreter code design, but is not
comparable in terms of performance to bytecode-based virtual machines, and even remotely to JIT compilers.

Benefits of bytecode interpreters:

* Better CPU instruction cache locality (fewer jumps)
* Size of stack is not limited by system
* Easier implementation of garbage-collecting algorithms
* Possibility of optimizations
* Faster

## Stack-based virtual machine

Hololisp uses stack-based virtual machine. Stack is a LIFO structure, where values can be pushed on the top and popped
from the top. Bytecode instructions operate the stack, either pushing values on it, popping from it, or using values
stored on top.

> Stack-based virtual machines are often contrasted
> with [Register-based virtual machines](https://www.quora.com/What-are-register-based-virtual-machines), which usually
> offer better performance and are used in, for example, [Lua since version 5.0](https://www.lua.org/doc/sblp2005.pdf).
> However, they are not suitable for Lisp interpreters.

## Bytecode instructions

Bytecode uses minimal set of instructions in order to simplify virtual machine and compiler design.

| Opcode   | Description                               |
|----------|-------------------------------------------|
| END      | Return from current function              |
| NIL      | Push nil on stack                         |
| TRUE     | Push true on stack                        |
| CONST    | Load and push constant on stack           |
| APPEND   | Append value to linked list               |
| POP      | Pop value from stack                      |
| FIND     | Find variable with name loaded from stack |
| CALL     | Call function                             |
| MBTRCALL | May-be-tail-recursive call                |
| JN       | Jump if nil                               |
| LET      | Create variable binding                   |
| PUSHENV  | Create new lexical env                    |
| POPENV   | Pop lexical env                           |
| CAR      | Get car of cons                           |
| CDR      | Get cdr of cons                           |
| SETCAR   | Set car of cons                           |
| SETCDR   | Set cdr of cons                           |
| MAKEFUN  | Create closure                            |

Instruction set was made as simple as possible, while supporting complex features
like [tail call optimizations](https://en.wikipedia.org/wiki/Tail_call) and closures.

### CONST

*CONST* instruction loads value from function constant list and pushes it on stack. 2 bytes of bytecode after
instruction opcode encode u16 value in big-endian format. This is decoded by VM when executing and used as an
index to constant array. Constants are symbols, numbers and functions.

Bytes of *CONST* bytecode instruction:

| CONST | idx >> 8 | idx & 0xFF |
|-------|----------|------------|

where idx is a 16-bit unsigned integer.

### APPEND, conses and lists

Linked lists is the only data structure supported by hololisp natively. Linked list is constructed from pairs, called
conses, where first element of a pair (car) is a pointer to data and second (cdr) is pointer to next cons or nil.
Although list conses can be useful on their own, they are most commonly used to create lists, for example, when passing
arguments to a function call.

Constructing linked list is usually done with pointers to two values: list head and list tail. Appending cons to list
end then can be written as follows:

```c
void append(cons **head, cons **tail, cons *value) {
    if (head == NULL) {
        *head = *tail = value;
    } else {
        (*tail)->cons = value;
        *tail = value;
    }
}
```

*APPEND* instruction is the only bytecode instruction able to create conses. When virtual machine meets *APPEND*
instruction, it expects following structure of virtual machine stack:

| List head | List tail | Value |
|-----------|-----------|-------|

Vm creates cons, setting car of it to 'value'. Then does exactly what is described in C code example.

After all elements have been appended to list, we need to pop list tail from stack.

Creating list example:

```
// creates (0 1) and leaves it on stack
NIL
NIL 
CONST 0 
APPEND
CONST 1 
APPEND
POP
```

To create a cons which is not a linked list node, *SETCDR* is needed.

### CAR, CDR, SETCAR, SETCDR

Conses can be manipulated with *CAR*, *CDR*, *SETCAR* and *SETCDR* instructions.

*CAR* and *CDR* pop cons from stack and push either car or cdr of it back on stack. This model of execution is useful
for implementing infamous [*cadadr*](https://franz.com/support/documentation/ansicl.94/dictentr/carcdrca.htm) and
friends.

*SETCAR* and *CDR* expect to find cons on stack and value next to it that needs to be set either to car or cdr of that
cons. They are also can be used to create non-list cons pairs:

```
// creates (0 . 1) and leaves it on stack
NIL
NIL
CONST 0
APPEND
POP
CONST 1
SETCDR
```

As it can be noticed, creating a standalone cons pair is quite verbose in bytecode terms. However, this is uncommon
operation and unlikely to be a performance bottleneck.

### PUSHENV, POPENV, LET, FIND, envs and variables

hololisp supports lexical scoping with *PUSHENV*, *POPENV* and *LET* instructions. *PUSHENV* and *POPENV* maintain stack
of lexical scopes, either creating new empty env or popping last and using previous. *LET* instruction is used to
associate symbol with a value in corrent lexical scope (env on top of stack). For example, all of these instructions are
used when implementing 'let' form:

```
(let ((a 1))
  ...)
```

will translate to:

```
PUSHENV
CONST a 
CONST 1
LET 
...
POPENV
```

*FIND* instruction is used to perform a 'find' on env stack. It uses symbol and tries to find a variable with that name,
search starting from the top of the stack going to the bottom.

Hololisp uses cons pairs to store values in env. Car of a pair is variable name and cdr is variable value. This allows
very easy manipulating of variables internally, without need to use additional instruction. For example,
to modify variable value we can just modify cdr of its slot in env.

```
(define x 1)
(set! x 2)
;; x is now 2!
```

will translate to

```
CONST x 
CONST 1
LET 

CONST x
FIND 
CONST 2
SETCDR
```

On the other hand, this approach forces use of *CDR* after each *FIND* in order to use variable value.

### JN and branching

Branching is done using single instruction: *JN*. *JN* means 'jump if nil'. When this instruction is encountered,
vm fetches top element from stack and decodes jump offset. Jump offset is a 16-bit unsigned integer, encoded in the same
way as *CONST*'s.

| JN  | offset >> 8 | offset & 0xFF |
|-----|-------------|---------------|

Unconditional jumps are implemented using *NIL* instruction before *JN*. Because creating a nil and pushing it on stack
is very cheap, there is no performance penalty. KISS in all its glory.

```
(if t 1 0)
```

will translate to

```
    TRUE 
    JN else
    CONST 1
    NIL 
    JN end
else:
    CONST 1
end:
    END
```

### MAKEFUN, CALL, MBTRCALL and functions

Functions are essential to hololisp. There are two types of functions: C FFI calls and calls within language
boundaries (*regular* functions).

C FFI calls are special feature of compiler to allow definitions of external functions. For example, virtual machine has
no ability to add numbers on its own and uses C FFI to add it to language.

Regular functions can be defined using two compiler-builtin forms: *define* and *lambda*. The only difference between
them is that functions created using *define* can be tail-optimized.

All functions in hololisp are closures. Thus, they capture the scope in which they were created and allow referencing
variables from there, even if they are currently out of scope.

Functions are stored as hololisp values in current bytecode list of locations. *MAKEFUN* instruction can be used to
create a closure - copy function definition from constant list. Current env pointer is stored to create function copy -
thus creating a closure.

Both C FFI and regular functions can be called with *CALL* instructions. All that does is fetches callable and argument
list from stack and either calls C function pointer or calls hololisp function by manipulating virtual call stack.

Hololisp also supports [tail recursion](https://en.wikipedia.org/wiki/Tail_call) optimization. Compiler marks calls that
are *last* in function bytecode as *BMTRCALL*'s. When virtual machine meets *BMTRCALL* it checks if function being
invoked is the same as current - and if they are the same reuses current call stack frame instead of creating new one.
