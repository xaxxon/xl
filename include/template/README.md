## String template library.

### Simple use

Anything within double curly braces is looked up by name and replaced with the corresponding value

Template: I went to the {{LOCATION}} and saw {{THING}}

Substitutions: {{"LOCATION", "store"}, {"THING", "my friend"}}

### Array of values for the same template

Supports taking a template string and filling it out for each value in a container (such as std::vector) and joining them
with a custom string.

Pseudo-code:

I looked at the painting and saw the following colors: {{COLORS|ColorTemplate|, }}

ColorTemplate: "{{ADJECTIVE}} {{COLOR}}"

Color {
  string adjective;
  string color;
}

vector<Color> colors{{"bright", "purple"}, {"dark", "green"}};

=> I looked at the painting and saw the following colors: bright purple, dark green

### Performance

Templates are 'compiled' into chunks for increased speed when used multiple times.  Compilation happens on first use
if not requested earlier.
