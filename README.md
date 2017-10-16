## XL (xaxxon's library)

A bunch of general-purpose C++ utilities used across my different projects.

### Regexer

std::regex and PCRE agnostic regex library

### Template

String template substitution engine

### AlignedString

Cache friendly 16/64-byte aligned string with vectorized implementations of common functions

### zstring_view

std::string_view-like class which guarantees its contents to be NUL-terminated.   Useful for function parameters
which require a NUL terminated string to be able to take char* or std::string inputs without making a copy.

### Library extensions

Convenient additions to std::algorithm which operate on entire containers instead of having to pass begin and end.
Some handy type traits as well.

### Log

Logging library which calls user defined callbacks when data is provided.   Has log levels and subjects which can be
turned on/off at runtime.
