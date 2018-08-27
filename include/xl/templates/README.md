## String Template Library

Create complex formatted strings based on a template string which is comprised of literal strings and substitution definitions.

The two primary object types are `Template` and `Provider`.  

### Template

A template is the blueprint for how to build the final string.  It contains both literal
strings as well as substutition definitions.

### Provider

A provider is the data used while processing the substitution definitions
in the template.   

### My First Template

    Template tmpl("I walked to {{LOCATION}} to meet my friend {{NAME}}");
    
    auto provider = make_provider(
        std::pair("LOCATION", "park"),
        std::pair("NAME", "Dan")
    );
    
    auto final_string = tmpl.fill(provider);
    

### Template String Syntax

Anything not a substitution definition is passed through to the final string literally.
Substitutions are defined as everything between balanced pairs of {{ and }}'s.  

#### Substitution Definition

A substitution is everything between balanced pairs of {{ and }}'s.   

The format of a substitution is:

* Opening delimiter: {{
* Optional leading flags
* Substitution name 
* Optional join string 
* `|` Pipe
* Optional middle flags
* Context-sensitive data
* Closing delimiter: }}



### Leading Flags

Available leading flags (each is optional, but if present must be in this
order):

* optional `<` to ignore empty substitutions
* optional `!` to insert a named template here


### Middle Flags

Available middle flags (each is optional, but if present must be in this order):

* optional `!` following data is an inline template
* optional second `!` the inline template begins on the next line


## Providers

Substitution data can be just about anything.

* No requirements for Copyable
* Callbacks for creating more expensive data, on demand
* Iterable containers
* User-defined types (with either an associated member or free function to create substitution mappings)

Further Provider specializations may be added to the xl::templates namespace
to extend the general functionality of the implementation, such as for supporting
custom smart-pointer types, for example.

### get_provider

A user-defined type may specify how to produce a Provider object via a member instance function,
a free function, or a static function within an explicitly provided class.

Free/static functions have the signature:

    std::unique_ptr<Provider_Interface> get_provider(T &);
    
Member functions have the signature:

    std::unique_ptr<Provider_Interface> get_provider();
    
Note: ProviderPtr is a typedef for std::unique_ptr<Provider_Interface>

### Multiple values for the same template

A template can be automatically expanded for each element of a container (such as a vector).

A 'join string' will be inserted between each element.   The default string is a newline
but can be specified in the substitution definition:

    {{VECTOR_OF_DATA_NAME% join string |!inline template}}
    
for a vector with 3 elements will generate

`inline template join string inline template join string inline template`
    
    
### Join String
When performing multiple replacements, a `join string` may be specified.  This
string will be placed between each replacement.

To put `, ` between each replacement

    {{container%, |!{{value}}}}
               ^

The join string may be specified to be a leading join string by prefixing a 
second `%` before it.

To put `, ` before the first replacement and between each subsequent replacement:

    {{container%%, |!{{value}}}}
               ^^

### Ignore leading/trailing template if replacement is empty

If the replacement string is empty, the leading, trailing, or both parts of the
same line may be ignored as well.

Assuming name is empty, everything until after the subsitution will be ignored

    this will be ignored {{<name}} this will NOT be ignored
                           ^ 
      
    this will NOT be ignored {{name>}} this will be ignored
                                   ^
                                   
    this will be ignored {{<name>}} this will be ignored
                           ^    ^
   
For a more complicated replacement, the `<` and `>` always go at the very
beginning and end of the substitution:

    {{<name%, |!inline template>}}
      ^                        ^
      
Lines above and below the replacement aren't ignored.  

For multiple substitutions on the same line, a `>` on the preceding substitution
will be prioritized over `<` on a subsequent substitution on the same line.  

   

### Performance

Templates are 'compiled' into chunks for increased speed when used multiple times.  Compilation happens on first use
if not requested earlier.  However, there are numerous opportunities for further optimization. 



## Detailed Substitution Description


### Comments

If the first thing in a substitution is a `#` then the substitution will be ommitted
from the final results.  Note this may have a (very) small performance penalty because a 
dummy substitution is still created.  

    {{#This is a comment}}
    

### Contingent Data

Static text before or after a substitution may be omitted if the substitution
itself results in an empty string.

A leading `<` will omit static text on the same line before the substitution and a
trailing `>` will omit static text on the same line after the substitution.  The
omitted text will stop at another substitution if there is another on the same
line.  


    static text before substitution {{<name>}} static text after substitution
    
if `name` substitutes to an empty string, this whole line will be blank.
    
#### Note: if two substitutions on the same line 'point' at each other, the one on
the left one (pointing right) has priority

    A {{<1>}} B {{<2>}} C
    
If 1 is empty and 2=>2, this results in `2 C`.

If 1=>1 and 2 is empty, this results in `A 1 B_` 

(note the trailing space, shown with an underscore for emphasis)

#### Note: << and >> will grab everything until it reaches a substitution (including comment substitutions)

    ONE
    
    A {{<<empty_substitution>>}} B
    
    TWO
    
results in an empty string


### Inline Template

A template may be specified inside another template.  This will use the inline
template to for the Provider named in the substitution.

{{provider_name|!{{field1}} `-._.-' {{field2}}}}

A substitution with `field1 => one` and `field2 => two` results in:

    one `-._.-' two
    
#### Note: placing two `!`s will start the inline template on the next line

It is often useful to keep the template horizontally aligned, so by starting the
inline template with `!!`, everything from the `!!` to the end of that line is 
ignored.

        Line one
        {{line_number_list|!!
        Line {{number}}}}
        Line LAST
    
with `line_number_list => vector<string>{"two", "three"}` results in:

        Line one
        Line two
        Line three    
        Line LAST
    
But notice how it's clear that all the `Line` parts line up, as compared to:

        Line one
        {{line_number_list|!    Line {{number}}}}
        Line LAST
    
This example above will generate the same output, but makes it harder to visually
verify that the output will be aligned.

### Join String

When a template used multiple times for a single substitution, a string may be 
specified to be placed between the resulting text from each substitution.  
Everything between a `%` and the `|` will be placed between each substitution.

{{provider_name% $$|!{{string}}}}

A substitution with `vector<string>{"a", "b", "c"}` will result in:

    a $$b $$c
    
#### Note: placing two `%`s will include the join string before the first 
substitution as well.  Changing the above example to:

    {{provider_name%% $$|!{{string}}}}

will result in:

    _$$a $$b $$c
    
(note the leading space - shown with underscore for emphasis)

This is useful when adding to a list with fixed elements:

    A, B, C{{rest_of_list%%, |{{name}}}}
    
Substituted with `vector<string>{"D", "E", "F"}` results in:

    A, B, C, D, E, F
    
(note the comma between C and D which wouldn't be present if the substitution
only had one `%`)


### Escaping Special Characters

Putting a backslash before a character in a substitution will result in it not
being considered as a special character for starting/stopping part of the 
substitution.

For example, to have a `|` in your join string, you must put a backslash before it
otherwise it will be seen as the end of your join string.

    {{name%\||!inline template{{value}}}}

or to have a `%` in your substitution name,

    {{name\%}}
    
or to put `}}` in your inline template:

    {{name|!} \}} \}\}\}}}
    
The example above creates an inline template of `} }} }}}`



### Addressing Nested Providers with X.Y.Z notation

If a provider provides another provider, it can be addressed from the current template
by putting a `.` between the names.   Note that a `.` may never be a valid part of a 
single name.  

    {{has_a_provider_in_it|!{{name_of_provider|!{{some_value}} {{some_other_value}}}}}}

can be written as:

    {{{{has_a_provider_in_it.name_of_provider.some_value}} {{has_a_provider_in_it.name_of_provider.some_other_value}}}}
    
or

    {{has_a_provider_in_it|!{{name_of_provider.some_value}} {{name_of_provider.some_other_value}}}}}}
    
This works with containers as well, but you lose the ability to select a non-default
join string between them.  It will always be `\n`

    {{vector_of_providers|!{{some_value}}}}
    
can be written as:

    {{vector_of_providers.some_value}}
    


## Template Design Info

A Template object is the entire template string as presented to the API

A Substitution is the bit within a {{...}} and can contain Template objects
which contain subsequent Substitution objects.   A complex name Substitution
may also be broken down into implicit Template objects

{{a.  b.  c|{{foo}} }}

{{a|{{b.  c|{{foo}} }} }}

{{a|{{b|{{c|{{foo}} }} }} }}


## FAQ

### How do I associate some contingent literal text with the substitution on the left and other literal text with the substitution on the right?

Put an empty comment `{{#}}` substitution at the dividing point:

    {{not_empty>}} this shows {{#}} this doesn't show{{<empty}}
    
 