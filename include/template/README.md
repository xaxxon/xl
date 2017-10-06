String template library.

Template: I went to the {LOCATION} and saw {THING}

Substitutions: {{"LOCATION", "store"}, {"THING", "my friend"}}

Supports taking a template string and filling it out for each value in a container (such as std::vector) and joining them
with a custom string.

Pseudo-code:

I looked at the painting and saw the following colors: {COLORS|ColorTemplate|, }

ColorTemplate: "{ADJECTIVE} {COLOR}"

Color {
  string adjective;
  string color;
}

vector<Color> colors{{"bright", "purple"}, {"dark", "green"}};

=> I looked at the painting and saw the following colors: bright purple, dark green

Templates are 'compiled' into chunks for increased speed when used multiple times.  Compilation happens on first use
if not requested earlier.
