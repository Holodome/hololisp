# Compiler 

## Lexer 

Lexer is coded using FSM based on two-action loop parsing and finaliziation of token after that. For more information please refer to [Some Strategies For Fast Lexical Analysis when Parsing Programming Languages](https://nothings.org/computer/lexing.html).

```c
// hll_compiler.c

const char *token_start = cursor;
hll_lexer_state state = HLL_LEX_START;
do {
    // Basic state machine-based lexing.
    // It has been decided to use character equivalence classes in hope that
    // lexical grammar stays simple. In cases like C grammar, there are a
    // lot of special cases that are better be handled by per-codepoint
    // lookup.
    hll_lexer_equivalence_class eqc = get_equivalence_class(*cursor++);
    // Decides what state should go next based on current state and new
    // character.
    state = get_next_state(state, eqc);
    // If character we just parsed means that token is not yet started (for
    // example, if we encounter spaces), increment token start to adjust for
    // that. If it happens that we allow other non-significant characters,
    // this should better be turned into an array lookup.
    token_start += state == HLL_LEX_START;
} while (state < HLL_LEX_FIN);

switch (state) {
    // finalize ...
}
```

## Parser

## Bytecode generator 

## Diagnostics 

Generation of decent error messages is arguably the single most complicated part
of virtual machine. The difficulty comes from the need to minimize slowdown 
and additional memory footprint none of which are possible to do entirely.
