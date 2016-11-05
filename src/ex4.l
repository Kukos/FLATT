%{
/*
	Author: Michal Kukowski
 	email: michalkukowski10@gmail.com
*/

#include <stdio.h>
#include <stack.h>
#include <stdlib.h>
#include <math.h>

#define STACK_CLEANUP(S) \
	do{ S->darray->num_entries = 0; }while(0)

#define STACK_ERROR(msg) \
	do{ printf("%s",msg); BEGIN(ERROR); STACK_CLEANUP(stack); }while(0)

#define BASIC_OP(stack,arg1,arg2,res,op) \
	do \
	{\
		if(stack_pop(stack,(void*)&arg1) || stack_pop(stack,(void*)&arg2))\
			STACK_ERROR("Not enough arguments\n");\
\
		res = arg2 op arg1;\
		if(stack_push(stack,(void*)&res)) \
			STACK_ERROR("stack_push error\n");\
	}\
	while(0)

int yywrap(void);
int yylex(void);

static int arg1;
static int arg2;
static int res;

static Stack *stack;

%}

WHITE_C				[ \t]

%x	ERROR

%%

{WHITE_C}
\n      			{
							if(stack->darray->num_entries > 1)
								printf("Not enough math operators!\n");
							else
							{
								if(stack_pop(stack,(void*)&res))
									printf("stack_pop error\n");
								else
									printf("= %d\n",res);
							}

							STACK_CLEANUP(stack);
					}

\-?[0-9]+			{
						arg1 = atoi(yytext);
						if( stack_push(stack,(void*)&arg1))
							STACK_ERROR("stack_push error\n");
					}

\+					BASIC_OP(stack,arg1,arg2,res,+);

\-					BASIC_OP(stack,arg1,arg2,res,-);

\*					BASIC_OP(stack,arg1,arg2,res,*);

\/					BASIC_OP(stack,arg1,arg2,res,/);

\%					BASIC_OP(stack,arg1,arg2,res,%);

\^					{
						if(stack_pop(stack,(void*)&arg1) || stack_pop(stack,(void*)&arg2))
							STACK_ERROR("Not enough arguments\n");

						res = (int)pow((int)arg2,(int)arg1);

						if(stack_push(stack,(void*)&res))
							STACK_ERROR("stack_push error\n");
					}

.					STACK_ERROR("Unknown character!\n");

<ERROR>[^\n]
<ERROR>\n			{
						STACK_CLEANUP(stack);
						BEGIN(INITIAL);
					}
%%

int yywrap(void)
{
	return 1;
}

int main(void)
{
    stack = stack_create(sizeof(int));
	yylex();

    stack_destroy(stack);

	return 0;
}
