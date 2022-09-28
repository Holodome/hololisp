# Values

> By 'value' we understand minimal instance of hololisp object that language can operate on.

Hololisp currently supports several types of values:
* conses;
* symbols;
* numbers;
* functions (and macros);
* nil;
* true;
* C FFI function;
* env.

## NAN-boxing 

Hololisp uses [nan-boxing](https://leonardschuetz.ch/blog/nan-boxing/) along with [pointer tagging](https://en.wikipedia.org/wiki/Tagged_pointer). These techniques help to store different types of data in a single 64-bit unit of storage. hololisp practically benefits from this idea, being able to store handles to values in a single 64-bit integer.

All modern operating systems use only of 48 bits of program address space. Remaining 16 bits can be used to store any additional information that can be easily stripped with a mask leaving a bare pointer. This observation is a core idea behind pointer tagging, however, this idea can be extended to allow storing IEEE754 double-precision numbers in a same unit of storage due to the fact that IEEE754 defines special kind of number - NAN.
NAN numbers are defined using 11 bits of number, leave 52 bits of mantissa and 1 bit of sign without value. By a very neat coincidence, the bits occupied by NAN mask are unused in a pointer.

NAN-boxing is a common name for using bitwise representation of double to store all non-NAN real numbers, and use 53 other bits of double for storing additional information, for example, for storing pointers.
NAN-boxing is used by Hololisp along with pointer tagging to store 4 kinds of values in a single u64 storage unit:

* number;
* nil;
* true;
* object pointer.

where object pointer is a pointer to heap-allocated variable-sized storage for all not covered hololisp value kinds like conses, symbols and other.

These techniques help to store different types of data in a single 64-bit unit of storage. hololisp practically benefits from this idea, being able to store handles to values in a single 64-bit integer.

## Conses

Cons is a hololisp object that can hold two values. First element of a pair is called 'car', and second is called 'cdr'. These names are historical in lisp dialects.

Conses are frequently used to represent linked lists, where 'car' means data element of list node, and 'cdr' means next list node.

Linked lists consisting of conses are frequently called just 'lists' and are use throughout the language. For example, function arguments are passed in a list. 

Linked lists are naturally flexible in use but offer constrained performance due to big number of heap allocations involved and O(n) time complexity of random access. hololisp tries to be idiomatic lisp dialect and does not offer generic array with O(1) random access complexity.

```scheme 
(define pair (cons 1 2))
; (define pair '(1 . 2)) - same as above 

(print (car pair)) ; 1 
(print (cdr pair)) ; (2)

(define lst (list 1 2 3 4))
(print (car lst))  ; 1 
(print (cdr list)) ; (2 3 4)

```

## Symbol

Symbol is a special object not frequently found in languages other that lisp dialects. Symbol is a string consisting of restricted set of characters. Symbols have a property that they evaluate to a value they are bound to. They are used to access variables and functions. 

```scheme
(define symb 'value)
(print symb)  ; value 
(print 'symb) ; symb
```

## Number 

Hololisp numbers are IEEE754 doubles. Operations on numbers are done using builtin forms.
