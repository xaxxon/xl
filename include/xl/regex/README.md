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

#### Running Regex Again on Unmatched Portion of Source String

To find the next match in a given string, use `RegexResult::next`.   This will run the
same regex (make sure it's still in scope) on the `suffix()` of the given results object.

    auto result1 = xl::Regex(".").matcH("abc"); // result1[0] is "a"
    auto result2 = result1.next(); // result2[0] is "b"
    auto result3 = result2.next(); // result3[0] is "c"
    auto result4 = result3.next(); // result4 is false, since there was no additional match
    
    
### Regex Result Range Loop

Putting a regex match in a range loop will run the regex over the portion of the 
original string after the previous successful match and stop when no match is available

    for (auto & match : Regex(".").match("abc")) {
        std::cout << match[0] << std::endl;
    }
    
will print out

    a
    b
    c
    

### Regex Replace

Calling `replace()` instead of `match()` on a Regex will return a string with the matching
sections of the string replaced by the specified replacement string.   

Using std::regex calls std::regex_replace, so all feature support is defined by that function.

Using PCRE, the values `$1`-`$9` are substituted for the matching captured reference.  Only 
the matches 1 through 9 are currently supported, though support for additional ways to 
specify captures may be added in the future.

    xl::Regex("([abc])(.)").replace("a1b2c3 d4e5f6", "$2$1"); // returns "1a2b3c d4e5f6"
    
