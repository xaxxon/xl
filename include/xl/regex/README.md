## Regex

This library makes it simple to use regular expressions and also provides a 
consistent (as much as possible) between std::regex and PCRE regular expression
libraries.

The match results object stores a copy of the string being matched against, so 
rvalue strings may be used without worry of whether the captured sub-patterns
will be valid when used later.  

A string literal suffix is provided for creating regular expressions, `_re`.

The function `regexer` takes a string and a regex (or string defining a regex)
and returns the matches from that combination.


### Results Object

Testing the results object as a boolean will return whether or not it represents
a match or whether the regex failed to match against the provided string.

For capturing sub-patterns, the value can be retrieved by index (starting at 1),
or (when named patterns are supported), by name.  

The results object stores a copy of the original string so as long as the results 
object is still around, the matching patterns may still be accessed.


### Regex Result Range Loop

Puttinga a regex match in a range loop will run the regex over the portion of the 
original string after the previous successful match and stop when no match is available

    for (auto & match : Regex(".").match("abc")) {
        std::cout << match[0] << std::endl;
    }
    
will print out

    a
    b
    c
    
    