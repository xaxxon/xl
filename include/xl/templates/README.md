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
    
   

### Performance

Templates are 'compiled' into chunks for increased speed when used multiple times.  Compilation happens on first use
if not requested earlier.  However, there are numerous opportunities for further optimization. 

