%{
#include <iostream>
using namespace std;
#define YY_DECL extern "C" int yylex()

#include "substitution.h"
#include "template_parser.tab.h"  // to get the token types that we return

%}
%%
[ \t\n]           ;
\{\{                {cout << "open" << endl; return SUBSTITUTION_OPEN;}
\}\}                {cout << "close" << endl; return SUBSTITUTION_CLOSE;}
[a-zA-Z0-9]+      {
	// we have to copy because we can't rely on yytext not changing underneath us:
	cout << "lexer string: " << yytext << endl;
	yylval.sval = strdup(yytext);
	return STRING;
}
.                ;
%%