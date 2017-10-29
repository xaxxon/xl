## String template library.

### Simple use

Anything within double curly braces is looked up by name and replaced with the corresponding value

Template: I went to the {{LOCATION}} and saw {{THING}}

Substitutions: {{"LOCATION", "store"}, {"THING", "my friend"}}

### Flexible

Substitution data can be anything.

* No requirements for Copyable
* Callbacks for creating more expensive data, on demand
* Iterable containers
* User-defined types (with either an associated member or free function to create substitution mappings)

### Array of values for the same template

A template can be automatically expanded for each element of a container (such as a vector).  A string, such
as a comma or newline, can be specified for insertion between each expansion.

### In-Line or Out-of-Line Sub-Templates

Templates can refer to other templates, which may be specified by name or be placed in-line in another template.

### Performance

Templates are 'compiled' into chunks for increased speed when used multiple times.  Compilation happens on first use
if not requested earlier.

### Unicode Support

No.
