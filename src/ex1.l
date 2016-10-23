%{
/*
	Author: Michal Kukowski
 	email: michalkukowski10@gmail.com
*/

#include <stdio.h>

int yywrap(void);
int yylex(void);

static long words = 0;
static long lines = 0;

%}

WHITE_C	[ \t]
WORD 	[^ \t\n]

%%
^{WHITE_C}+[\n]*	/* at the begining of line we dont need any white characters */
{WHITE_C}+$			/* at the end of line we dont need any white characters */
{WHITE_C}+	  		printf(" "); /* We cut off every white characters at the begining and end
							so now we have white characters in the middle, print single space instead */
^\n					/* ignore empty line */
\n					{
						printf("%s", yytext);
						++lines;
					}

{WORD}+				{
						printf("%s", yytext);
						++words;
					}
%%

int yywrap(void)
{
	return 1;
}

int main(void)
{
	yylex();
	printf("Lines: %ld\nWords: %ld\n",lines,words);

	return 0;
}
