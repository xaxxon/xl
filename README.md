## XL (xaxxon's library)

A bunch of general-purpose C++ utilities used across my different projects.

### Regexer

std::regex and PCRE agnostic regex library with a very simple syntax.   
Supports matching and replacing with either regex library.  Documentation in subfolder

### Template

Powerful, expressive string template substitution engine.  Documentation in subfolder

### AlignedString

Cache friendly 16/64-byte aligned string with vectorized implementations of common functions

### zstring_view

std::string_view-like class which guarantees its contents to be NUL-terminated.   Useful for function parameters
which require a NUL terminated string to be able to take char* or std::string inputs without making a copy.

### Library extensions

Convenient additions to std::algorithm which operate on entire containers instead of having to pass begin and end.
Some handy type traits as well.  Also has many useful type traits, such as is_template_for_v<std::vector, std::vector<int>>

### Log

Logging library which calls user defined callbacks when data is provided.   Has log levels and subjects which can be
turned on/off at runtime.  When used in an upstream dependency, makes it very easy to hook into the logging framework of the executable (or downstream library).
Documentation in subfolder.

### magic_ptr

Smart pointer class which may or may not own the object it refers to.  Deleter is specified at runtime (like in shared_ptr), but magic_ptr is not copyable (like unique_ptr) and may inherit ownership from a unique_ptr.

### Json (requires PCRE library installed)

JSON-ish parser.  Attempts to comply with JSON5 as described here: https://github.com/json5/json5
Things like multi-line strings, comments, etc. 