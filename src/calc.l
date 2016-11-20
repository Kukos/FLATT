%{

/*
	Author: Michal Kukowski
 	email: michalkukowski10@gmail.com
*/

#include "../include/calc.tab.h"
#include <stdlib.h>

%}

WHITE_C 				[ \t]
UNKNOWN_C				[^ \t\n\+\-\*\/\%\^\(\)0-9]

%x COMMENT

%%

"#"                     BEGIN(COMMENT);
<COMMENT>\\\n
<COMMENT>\n             BEGIN(INITIAL);
<COMMENT>[^\\]|[^\n]

{WHITE_C}+

[0-9]+               	{
				            yylval.expr_token.val = atoi(yytext);
				            yylval.expr_token.str = yytext;
				            return NUM;
			            }

\\\n

\n			   			return END_LINE;
\+			            return ADD;
\-			           	return SUB;
\*						return MULT;
\/			            return DIV;
\%                      return MOD;
\^		         		return POW;
\(			            return L_BRACKET;
\)			            return R_BRACKET;
{UNKNOWN_C}+			return UNKNOWN_CHAR;

%%
