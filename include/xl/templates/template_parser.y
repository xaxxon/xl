%{
#include <cstdio>
#include <iostream>
#include "substitution.h"
using namespace std;

// stuff from flex that bison needs to know about:
extern "C" int yylex();
extern "C" int yyparse();
extern "C" FILE *yyin;
 
void yyerror(const char *s);
%}

%union {
	char * sval;
}

// define the "terminal symbol" token types I'm going to use (in CAPS
// by convention), and associate each with a field of the union:
%token <sval> STRING
%token <sval> SUBSTITUTION_OPEN
%token <sval> SUBSTITUTION_CLOSE

%%
template:
    template STRING substitution {cout << "got recursive sub" << endl;}
    | STRING substitution
    | STRING
    | substitution

substitution:
    SUBSTITUTION_OPEN substition_body SUBSTITUTION_CLOSE
    
substitution_body:
    '#' SUBSTITION_CLOSE
    | IGNORE_EMPTY_BEFORE_MARKER after_ignore_empty_before_marker
    
after_ignore_empty_before_marker: 
    TEMPLATE_INSERTION_MARKER after_template_insertion_marker
    | after_template_insertion_marker
    
    

    

%%

int main(int, char**) {
	// open a file handle to a particular file:
	FILE *myfile = fopen("a.snazzle.file", "r");
	// make sure it is valid:
	if (!myfile) {
		cout << "I can't open a.snazzle.file!" << endl;
		return -1;
	}
	// set flex to read from it instead of defaulting to STDIN:
	yyin = myfile;
	
	// parse through the input until there is no more:
	do {
		yyparse();
	} while (!feof(yyin));
	
}

void yyerror(const char *s) {
	cout << "Parse error: " << s << endl;
	exit(-1);
}
