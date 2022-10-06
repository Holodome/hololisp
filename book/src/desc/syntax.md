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

