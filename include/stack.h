#ifndef STACK_H
#define STACK_H

#include"darray.h"

/*
    Author: Michal Kukowski
    email: michalkukowski10@gmail.com

    Stack using dynamic array
*/



#ifndef BOOL
    #define BOOL unsigned char
    #define TRUE 1
    #define FALSE 0
#endif

typedef struct Stack
{
    Darray *darray;

}Stack;


/*
    Create stack of typee TYPE

    PARAMS
    @OUT PTR - pointer to stack
    @IN TYPE - type
*/
#define STACK_CREATE(PTR,TYPE) do{ PTR = stack_create(sizeof(TYPE)); } while(0);

/*
    Create stack

    PARAMS
    @IN size_of - size of element of stack

    RETURN:
    NULL if failure
    Pointer if success
*/
Stack *stack_create(int size_of);

/*
    Destroy stack

    PARAMS
    @IN / OUT stack - pointer to pointer to struct

    RETURN
    This is a void function
*/
void stack_destroy(Stack *stack);

/*
    Push value on stack

    PARAMS:
    @IN stack - pointer to stack
    @IN val - value

    RETURN:
    0 if success
	Non-zero value if failure
*/
int stack_push(Stack *stack,void *val);

/*
    pop value from stack

    PARAMS:
    @IN stack - pointer to stack
    @OUT val - value

    RETURN:
    0 if success
	Non-zero value if failure
*/
int stack_pop(Stack *stack,void *val);

/*
    Get top value from stack

    PARAMS:
    @IN stack - pointer to stack
    @OUT val - value

    RETURN:
  	0 if success
	Non-zero value if failure
*/

int stack_get_top(Stack *stack,void *val);

/*
    PARAMS
    @IN stack - pointer to stack

    RETURN:
    TRUE if stack is empty
    FALSE if is not empty of failure
*/
BOOL stack_is_empty(Stack *stack);

/*
    Convert stack to array

    PARAMS
    @IN stack - pointer to stack
    @OUT array - array
    @OUT size - size of array

	RETURN:
	0 if success
	Non-zero value if failure
*/
int stack_to_array(Stack *stack,void *array,int *size);

#endif
