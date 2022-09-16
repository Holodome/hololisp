# Syntax

hololisp is based on [s-expressions](https://en.wikipedia.org/wiki/S-expression). S-expressions are defined recursively as a list of items enclosed in parentheses.

## Lexical grammar

```
token:
    symbol
    number
    dot
    lparen
    rparen
    quote
    
symbol:
    symbol-nondigit
    symbol symbol-nondigit
    symbol digit
    digit
    symbol dot
    dot symbol
    
symbol-nondigit: one of
    a b c d e f g h i j k l m
    n o p q r s t u v w x y z
    A B C D E F G H I J K L M
    N O P Q R S T U V W X Y Z
    * / @ $ % ^ & _ = < > ~ ? 
    ! [ ] { } 
    sign
   
digit: one of
    0 1 2 3 4 5 6 7 8 9 
    
sign:
    + -
    
dot: 
    .
    
number:
    sign number-consant
    number-constant
    
number-consant:
    digit 
    number-constant digit
   
lparen:
    (
    
rparen:
    )
    
quote:
    '
```

Lexer is coded using FSM based on two-action loop parsing and finaliziation of token after that. For more information please refer to [Some Strategies For Fast Lexical Analysis when Parsing Programming Languages](https://nothings.org/computer/lexing.html).

```c
// hll_compiler.c

const char *token_start = cursor;
enum hll_lexer_state state = HLL_LEX_START;
do {
    // Basic state machine-based lexing.
    // It has been decided to use character equivalence classes in hope that
    // lexical grammar stays simple. In cases like C grammar, there are a
    // lot of special cases that are better be handled by per-codepoint
    // lookup.
    enum hll_lexer_equivalence_class eqc = get_equivalence_class(*cursor++);
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

## Syntax

```
list:
    nil
    lparen list-body rparen
    
list-body:
    atom
    list-body atom
    list-body dot atom
    
atom:
    list
    symbol
    number
    quote atom
    
nil:
    lparen rparen
    
t:
    symbol "t"
```

Parsing is done using naive recursive-descent parses that builds AST.

