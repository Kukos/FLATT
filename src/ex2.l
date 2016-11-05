%{
	/*
		Author: Michal Kukowski
	 	email: michalkukowski10@gmail.com
	*/

#include <stdio.h>

#ifndef BOOL
	#define BOOL unsigned char
	#define TRUE 1
	#define FALSE 0
#endif

static BOOL print_new_line = FALSE;

int yywrap(void);
int yylex(void);

%}

%x SINGLE_COMMENT
%x SINGLE_COMMENT_BL

%x MULTI_COMMENT
%x MULTI_COMMENT_BL

%x STRING

%%

\"									{
										BEGIN(STRING);
										printf("%s",yytext);
									}

<STRING>[^\"]*						printf("%s",yytext);

<STRING>\"							{
										BEGIN(INITIAL);
										printf("%s",yytext);
									}

^"//"								BEGIN(SINGLE_COMMENT_BL);
<SINGLE_COMMENT_BL>\\\n				print_new_line = TRUE;
<SINGLE_COMMENT_BL>\n				{
										if(print_new_line)
											printf("\n");

										print_new_line = FALSE;
										BEGIN(INITIAL);
									}
<SINGLE_COMMENT_BL>[^\\]|[^\n]
"//"								BEGIN(SINGLE_COMMENT);
<SINGLE_COMMENT>\\\n
<SINGLE_COMMENT>\n					{
										printf("\n");
										BEGIN(INITIAL);
									}
<SINGLE_COMMENT>[^\\]|[^\n]

^"/*"								BEGIN(MULTI_COMMENT_BL);
<MULTI_COMMENT_BL>"*/"				BEGIN(INITIAL);
<MULTI_COMMENT_BL>[^\*]|[^/]

"/*"								BEGIN(MULTI_COMMENT);
<MULTI_COMMENT>"*/"					{
										printf("\n");
										BEGIN(INITIAL);
									}
<MULTI_COMMENT>[^\*]|[^/]

%%

int yywrap(void)
{
	return 1;
}

int main(void)
{
	yylex();

	return 1;
}