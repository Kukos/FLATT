
%{

/*
    Author: Michal Kukowski
    email: michalkukowski10@gmail.com
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <darray.h>

#ifndef BOOL
    #define BOOL char
    #define TRUE 1
    #define FALSE 0
#endif

/* lexer build in function */
void yyerror(const char *msg);
int yylex(void);
int yyparse(void);

/*
	change every number to number with oposite sign
	 number -->  -number
	-number -->  number

	PARAMS:
	@IN str - input string
	@OUT neg_str - pointer to new string with changes

	RETURN:
	0 if success
	Non-zero value if failure
*/
int str_neg(const char *str, char **neg_str);

static BOOL was_error = FALSE;
static char* temp_str;

#define ERROR(msg) \
	do { \
		perror(msg); \
		exit(1); \
	}while(0)

#define YY_ERROR(msg) \
    do { \
        yyerror(msg); \
        yyerrok; \
        was_error = TRUE; \
    }while(0)

#define FREE(PTR) \
	do { \
		free(PTR); \
		PTR = NULL; \
	}while(0)

%}

%code requires
{
    typedef struct Expr_token
    {
        char *str;
        int val;
    }Expr_token;
}

%union
{
    Expr_token expr_token;
}

%token 	END_LINE
%token 	NUM
%token 	UNKNOWN_CHAR
%left 	ADD SUB
%left 	MULT DIV MOD
%left 	POW
%left 	NEG
%token 	L_BRACKET R_BRACKET

%type <expr_token> 	expr NUM

%%

calc:
	%empty
	| calc line
;

line:
      expr[res] END_LINE	            {
                                            if(! was_error )
                                                printf("\t%s\n\t= %d\n", $res.str, $res.val);

											was_error = FALSE;
											FREE($res.str);
                                        }
	| expr error END_LINE               {   yyerrok; }
	| error END_LINE                    {   yyerrok; }
	| END_LINE
;

expr[res]:
      NUM[num]                          {
                                            $res.val = $num.val;
                                            if( asprintf(&$res.str, "%s",$num.str) == -1)
												ERROR("asprintf error\n");
                                        }
    | expr[left] ADD expr[right]        {
                                            $res.val = $left.val + $right.val;
                                            if( asprintf(&$res.str, "%s %s +",$left.str, $right.str) == -1)
												ERROR("asprintf error\n");

											FREE($left.str);
											FREE($right.str);
                                        }
    | expr[left] SUB expr[right]        {
                                            $res.val = $left.val - $right.val;
                                            if( asprintf(&$res.str, "%s %s -",$left.str, $right.str) == -1)
												ERROR("asprintf error\n");

											FREE($left.str);
											FREE($right.str);

                                        }
    | expr[left] DIV expr[right]        {
                                            if( ! $right.val)
                                                YY_ERROR("Division by 0\n");
                                            else
                                            {
                                                $res.val = $left.val / $right.val;
                                                if( asprintf(&$res.str, "%s %s /", $left.str, $right.str) == -1)
													ERROR("asprintf error\n");

												FREE($left.str);
												FREE($right.str);
                                            }
                                        }
    | expr[left] MOD expr[right]        {
                                            if( ! $right.val)
                                                YY_ERROR("Division by 0\n");
                                            else
                                            {
                                                $res.val = $left.val % $right.val;
                                                if( asprintf(&$res.str, "%s %s %%", $left.str, $right.str) == -1)
													ERROR("asprintf error\n");

												FREE($left.str);
												FREE($right.str);
                                            }
                                        }
    | expr[left] MULT expr[right]       {
                                            $res.val = $left.val * $right.val;
                                            if( asprintf(&$res.str, "%s %s *", $left.str, $right.str) == -1)
												ERROR("asprintf error\n");

											FREE($left.str);
											FREE($right.str);
                                        }
    | expr[left] POW expr[right]        {
                                            $res.val = (int)pow((double)$left.val, (double)$right.val);
                                            if( asprintf(&$res.str, "%s %s ^", $left.str, $right.str) == -1)
												ERROR("asprintf error\n");

											FREE($left.str);
											FREE($right.str);
                                        }
	| SUB expr[right] %prec NEG         {
                                            $res.val = -$right.val;
											if( str_neg($right.str,&temp_str) )
												ERROR("str_neg error\n");

                                            if( asprintf(&$res.str, "%s",temp_str) == -1)
												ERROR("asprintf error\n");
											FREE($right.str);
											FREE(temp_str);
                                        }
    | L_BRACKET expr[middle] R_BRACKET  {
                                            $res.val = $middle.val;
                                            if( asprintf(&$res.str, "%s",$middle.str) == -1)
												ERROR("asprintf error\n");

											FREE($middle.str);
                                        }
	| UNKNOWN_CHAR                      {
											$res.str = NULL;
											YY_ERROR("Unknown character\n");
										}
;

%%

int str_neg(const char *str, char **neg_str)
{

/* private define */
#define INSERT_STRING(DA, STR, L, R, i) \
	do{ \
		for(i = L; i < R; ++i) \
			if( darray_insert(DA,(void*)&STR[i]) ) \
				return 1; \
	}while(0)

	int left;
	int right;
	int len;
	int i;
	int __i;

	Darray *darray;

	char minus = '-';
	char *temp_str;

	if(str == NULL)
		return 1;

	left = 0;
	right = 0;
	i = 0;
	__i = 0;
	len = strlen(str);

	darray = darray_create(UNSORTED, 0, sizeof(char), NULL);
	if( darray == NULL )
		return 1;

	for(i = 0; i < len; ++i)
	{
		/* find number */
		while( i < len && ! isdigit(str[i]) )
			++i;

		right = i;

		/* we have number */
		if( i < len )
		{
			/* we have negative number */
			if( i > 0 && str[i - 1] == '-' )
			{
				INSERT_STRING(darray, str, left, right - 1, __i);
			}
			else
			{
				INSERT_STRING(darray, str, left, right, __i);
				if( darray_insert(darray, (void*)&minus) )
					return 1;
			}
		}

		/* write number */
		while( i < len && isdigit(str[i]) )
			if(darray_insert(darray,(void*)&str[i++]))
				return 1;

		/* don't save last pos */
		if(i < len)
			left = i;
	}

	/* write last package iff we write anything prev */
	if( left )
		INSERT_STRING(darray, str, left, len, __i);

	/* copy string from darray to dest */
	temp_str = (char*)malloc(sizeof(char) * (darray->num_entries + 1));
	if(temp_str == NULL)
		return 1;

	if(memcpy(temp_str, darray->array, darray->num_entries * darray->size_of) == NULL)
		return 1;

	temp_str[darray->num_entries] = '\0';

	darray_destroy(darray);
	*neg_str = temp_str;

/* destroy private define */
#undef INSERT_STRING
	return 0;
}

void yyerror(const char *msg)
{
    printf("Error!\t%s\n",msg);
}

int main()
{
    return yyparse();
}
