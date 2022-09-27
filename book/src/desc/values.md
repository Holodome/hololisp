# Values

> By 'value' we understand minimal instance of hololisp object that language can operate on.

Hololisp currently supports several types of values:
* conses
* symbols
* numbers
* functions (and macros)
* nil
* true
* C FFI function
* env

## NAN-boxing 

Hololisp uses [nan-boxing](https://leonardschuetz.ch/blog/nan-boxing/) along with [pointer tagging](https://en.wikipedia.org/wiki/Tagged_pointer). These techniques help to store different types of data in a single 64-bit unit of storage. hololisp practically benefits from this idea, being able to store handles to values in a single 64-bit integer.

All modern operating systems use only of 48 bits of program address space. Remaining 16 bits can be used to store any additional information that can be easily stripped with a mask leaving a bare pointer. This observation is a core idea behind pointer tagging, however, this idea can be extended to allow storing IEEE754 double-precision numbers in a same unit of storage due to the fact that IEEE754 defines special kind of number - NAN.
NAN numbers are defined using 11 bits of number, leave 52 bits of mantissa and 1 bit of sign without value. By a very neat coincidence, the bits occupied by NAN mask are unused in a pointer.

NAN-boxing is a common name for using bitwise representation of double to store all non-NAN real numbers, and use 53 other bits of double for storing additional information, for example, for storing pointers.
NAN-boxing is used by Hololisp along with pointer tagging to store 4 kinds of values in a single u64 storage unit:

    number;
    nil;
    true;
    object pointer.

where object pointer is a pointer to heap-allocated variable-sized storage for all not covered hololisp value kinds like conses, symbols and other.

These techniques help to store different types of data in a single 64-bit unit of storage. hololisp practically benefits from this idea, being able to store handles to values in a single 64-bit integer.