%{

/*
	Author: Michal Kukowski
 	email: michalkukowski10@gmail.com
*/

#include "../include/parser.tab.h"
#include <parser_helper.h>
#include <common.h>

#define YY_NO_INPUT
#define YY_NO_UNPUT

static int yywrap(void);

static uint64_t lines = 1;

#define YY_LEX(__token__) \
	do{ \
		YY_LOG("[LEX]\tSTR: %s\tTOKEN: %s\n", yytext, #__token__); \
		\
		if( asprintf(&yylval.ptoken.str, "%s", yytext) == -1) \
				YY_ERROR("[LEX]asprintf error\n", 1, ""); \
		\
		yylval.ptoken.line = lines; \
		\
		if( strcmp(#__token__, "YY_NUM") == 0) \
			yylval.ptoken.val = atoll(yytext); \
		\
		return __token__; \
	}while(0)


%}

WHITE_C 				[ \t]

%x COMMENT

%%

\{               			{
								YY_LOG("[LEX]\tBEGIN COMMENT\n", "");
								BEGIN(COMMENT);
							}

<COMMENT>\}      			{
								YY_LOG("[LEX]\tEND COMMENT\n", "");
								BEGIN(INITIAL);
							}

<COMMENT>\n					{
								YY_LOG("[LEX]\tCOMMENT\tNEW LINE\t Lines: %ju\n", lines);
								++lines;
							}

<COMMENT>[^\\{]|[^\}]|[^\n]	{ YY_LOG("[LEX]\tCOMMENT\t%s\n",yytext); }

{WHITE_C}+
^{WHITE_C}*\n				{
								YY_LOG("[LEX]\tNEW LINE\tLines: %ju\n", lines);
								++lines;
 							}

\n							{
								YY_LOG("[LEX]\tNEW LINE\tLines: %ju\n", lines);
								++lines;
 							}

\;							{ YY_LEX(YY_SEMICOLON); }

\+							{ YY_LEX(YY_ADD); }
\-							{ YY_LEX(YY_SUB); }
\/							{ YY_LEX(YY_DIV); }
\%							{ YY_LEX(YY_MOD); }
\*							{ YY_LEX(YY_MULT); }

\:\=						{ YY_LEX(YY_ASSIGN); }

\=							{ YY_LEX(YY_EQ); }
\<\>						{ YY_LEX(YY_NE); }
\<							{ YY_LEX(YY_LT); }
\>							{ YY_LEX(YY_GT); }
\<\=						{ YY_LEX(YY_LE); }
\>\=						{ YY_LEX(YY_GE); }


VAR							{ YY_LEX(YY_VAR); }


"BEGIN"						{ YY_LEX(YY_BEGIN); }
END							{ YY_LEX(YY_END); }


READ						{ YY_LEX(YY_READ); }
WRITE						{ YY_LEX(YY_WRITE); }


SKIP						{ YY_LEX(YY_SKIP); }


FOR							{ YY_LEX(YY_FOR); }
FROM						{ YY_LEX(YY_FROM); }
TO							{ YY_LEX(YY_TO); }
DOWNTO						{ YY_LEX(YY_DOWNTO); }
ENDFOR						{ YY_LEX(YY_ENDFOR); }


WHILE						{ YY_LEX(YY_WHILE); }
DO							{ YY_LEX(YY_DO); }
ENDWHILE					{ YY_LEX(YY_ENDWHILE); }


IF							{ YY_LEX(YY_IF); }
THEN						{ YY_LEX(YY_THEN); }
ELSE						{ YY_LEX(YY_ELSE); }
ENDIF						{ YY_LEX(YY_ENDIF); }

\[							{ YY_LEX(YY_L_BRACKET); }
\]							{ YY_LEX(YY_R_BRACKET); }


[_a-z]+						{ YY_LEX(YY_VARIABLE); }

[0-9]+						{ YY_LEX(YY_NUM); }

.							{ YY_LEX(YY_ERROR); }

%%

int yywrap(void)
{
	return 1;
}
