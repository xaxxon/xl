## AlignedString

High performance string class with over-aligned string types to allow vector optimizations.

### Memory Layouts

There are two buffer types provided - a static, in-object buffer and a dynamic, out-of-object buffer.  
All strings are NUL terminated.

### Static buffer

The static buffer has its length in the type.   It can hold strings up to `length-1` characters (not including the NUL
terminator).   String length must be a multiple of 16.  String lengths which are a multiple of 64 have different performance
characteristics due to processing 64 bytes at a time instead of 16.   

The `sizeof` the object is exactly the same as the `size` of the string.  This means you can get 4 strings of length 16 in
a cache line.

### Dynamic buffer

The alignment (either 16 or 64) is specified in the type.  The string length can be any size, but the capacity of the buffer
will always be a multiple of the alignment.   Alignment of 64 has different performance characteristics than 16 due to 
processing 64 bytes at a time instead of 16.   

The length of the string is artificially limited to a 32bit size in order to have the `sizeof` the string (not including
the dynamic buffer) to be no larger than 16 so 4 objects can fit on to a single 64-byte cache line.   


### Performance

64-byte size/alignment strings will process 64 bytes at a time for comparison/search operations.   For example, with 2 very
long dynamic buffer strings, if the strings are the same, comparing for equality is faster on the 64 byte variety.  However,
if they differ on the first character, the 16 byte version is faster because it only checks 16 bytes instead of 64 bytes.
