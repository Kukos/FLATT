#ifndef COMPILER_H
#define COMPILER_H

#include <common.h>
#include <tokens.h>
#include <arraylist.h>
#include <avl.h>
#include <stack.h>
#include <compiler_algo.h>
#include <arch.h>

/*
    Compiler for fake machine described by Maciek Gebala

    Author: Michal Kukowski
    email: michalkukowski10@gmail.com

*/

typedef struct Option
{
    uint8_t    wall:1;
    uint8_t    werr:1;
    uint8_t    optimal:2;
    uint8_t    tokens:1;
    uint8_t    padding:3;

    char *input_file;
    char *output_file;

}Option;

extern Option option;

extern Arraylist *asmcode;
extern Avl* compiler_variables;
extern Stack *labels;

typedef struct Cvar
{
    char *name;

    VAR_TYPE type;

    union
    {
        Array *arr;
        Value *val;
    }body;

    /* is in memory up to date value ? */
    uint8_t up_to_date  :1;
    uint8_t padding     :7;

}Cvar;

typedef struct Cfor
{
    Cvar *it; /* iterator, NULL if no needed */
    Cvar *hit; /* helper iterator */

    Register *it_reg; /* register for iterator NULL if no needed */
    Register *hit_reg; /* register for helper iterator */

    uint8_t for_type    :1;
    uint8_t it_needed   :1;
    uint8_t padding     :6;
}Cfor;

#define FOR_INC 0
#define FOR_DEC 1

/* start with | because is > than 'z' */
#define PTR_NAME        "|PTR"
#define TEMP_ADDR_NAME  "|TEMPADDR"
#define TEMP1_NAME      "|TEMP1"
#define TEMP2_NAME      "|TEMP2"
#define TEMP_DIV_HELPER "|TEMP_DIV_HELPER"

#define LABEL_FAKE      -1
#define LABEL_TRUE       1
#define LABEL_FALSE      2
#define LABEL_END        3
#define LABEL_RETURN     4

typedef struct Label
{
    Arraylist_node *ptr;
    int8_t type;

}Label;

/*
    Create Label

    PARAMS
    @IN ptr to line where we have code jump to LABEL
    @IN type - label type

    RETURN
    NULL iff failure
    Pointer to Label iff success
*/
Label *label_create(Arraylist_node *ptr, int8_t type) __nonull__(1);

/*
    Destroy Label

    PARAMS
    @IN label - pointer to Label

    RETURN
    This is void function
*/
void label_destroy(Label *label) __nonull__(1);

/*
    Convert normal value name to iterator name

    PARAMS
    @IN val - iterator value

    RETURN:
    NULL iff failure
    iterator name iff success

*/
__inline__ char *get_iterator_name(Value *val)
{
    char *valname = value_str(val);
    char *name;

    if(valname == NULL)
        return NULL;

    if(asprintf(&name, "%.*s", (int)strlen(valname) - 2, valname + 1) == -1)
        return NULL;

    FREE(valname);
    return name;
}

/*
    Local iterator is iterator in for loop not declared by user
    i.e FOR i FROM a TO b
    LOCAL_IT = b + 1 - a

    function convert it and end value to local it name

    PARAMS
    @IN it - iterator value
    @IN end - end value

    RETURN:
    NULL iff failure
    iterator name iff success
*/
__inline__ char *get_local_iterator_name(Value *it)
{
    int i;

    char *it_name = value_str(it);
    char *name;

    size_t len;

    if(it_name == NULL)
        return NULL;

    if(asprintf(&name, "T%.*s",(int)strlen(it_name) - 2, it_name + 1) == -1)
        return NULL;

    FREE(it_name);

    len = strlen(name);

    for(i = 0; i < len; ++i)
        name[i] = toupper(name[i]);

    return name;
}

/*
    Create cvar from structure str

    PARAMS
    @IN type - Var type
    @IN str - structrue Arrat or Value

    RETURN:
    NULL iff failure
    Pointer to Cavr iff success
*/
Cvar *cvar_create(VAR_TYPE type, void* str) __nonull__(2);

/*
    Destroy cvar NO FREE UNION BODY

    PARAMS
    @IN cvar - pointer to Cvar

    RETURN:
    This is void function
*/
void cvar_destroy(Cvar *cvar) __nonull__(1);

/*
    Normal cmp function for cvar ( name is key )

    PARAMS
    @IN a - 1st cvar
    @IN b - 2nd cvar

    RETURN:
    -1 a  < b
    0  a == b
    1  a  > b
*/
int cvar_cmp(void *a, void *b) __nonull__(1, 2);

/*
    Get cvar by Value

    PARAMS
    @IN val - value

    RETURN
    NULL iff failure
    Cvar iff success
*/
Cvar *cvar_get_by_value(Value *val) __nonull__(1);

/*
    Get cvar by name

    PARAMS
    @IN name - cvar name

    RETURN
    NULL iff failure
    Cvar iff success
*/
Cvar *cvar_get_by_name(const char *name) __nonull__(1);

/*
    Check declaration of cvar by value

    PARAMS
    @IN val - value

    RETURN
    TRUE iff declared
    FALSE iff not declared
*/
BOOL cvar_is_declared_by_value(Value *val) __nonull__(1);

/*
    Check declaration of cvar by name

    PARAMS
    @IN val - value

    RETURN
    TRUE iff declared
    FALSE iff not declared
*/
BOOL cvar_is_declared_by_name(const char *name) __nonull__(1);

/*
    Find and set array to array member

    PARAMS
    @IN val - value

    RETURN
    This is void function
*/
__inline__ void set_array_to_vararr(Value *val)
{
    Cvar *cvar;

    if(val == NULL)
        return;

    if(val->type == CONST_VAL)
        return;

    if(val->body.var->type == VAR_NORMAL)
        return;

    cvar = cvar_get_by_value(val);
    val->body.var->body.arr->arr = cvar->body.arr;

}

/*
    Calculate maximum memory for loop variables
    for each FOR loop need ietrator and local copy of end value

    PARAMS
    @IN tokens - tokens list

    RETURN
    Number of memory chunks needed for loops
*/
uint64_t calc_loop_mem_section(Arraylist *tokens) __nonull__(1);

/*
    calc memory section and write result to mem struct

    PARAMS
    @IN vars - avl with pvars
    @IN tokens - tokens list

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int prepare_mem_sections(Avl *vars, Arraylist *tokens) __nonull__(1, 2);

/*
    Main compiler function,
    in this function we run parser, optimizer iff 0X > 0 and compiler

    PARAMS
    NO PARAMS

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int main_compile(void);

/*
    Set Fake label
*/
__inline__ int label_fake(void)
{
    Label *label;
    Arraylist_node *fake = *(&fake);


    label = label_create(fake, LABEL_FAKE);
    if(label == NULL)
        ERROR("label_create error\n", 1, "");

    LOG("CREATE FAKE LABEL WITH TYPE = %d\n",label->type);
    if(stack_push(labels, (void*)&label))
        ERROR("stack_push error\n", 1, "");

    return 0;
}

/* change last label (top on stack) to line */
__inline__ int label_to_line(uint64_t line)
{
    Label *label;
    char *old;

    if(stack_pop(labels, (void*)&label))
        ERROR("stack_pop error\n", 1, "");

    LOG("LABEL TO LINE WTIH TYPE = %d\n", label->type);
    if(label->type == LABEL_FAKE)
    {
        LOG("FAKE LABEL ONLY DESTROY\n", "");

        label_destroy(label);
        return 0;
    }

    old = *(char**)label->ptr->data;

    LOG("NORMAL LABEL CHANGE %s to...\n", old);
    if(asprintf(label->ptr->data, "%s%ju\n", old, line) == -1)
        ERROR("asprintf error\n", 1, "");

    LOG("... %s", *(char**)label->ptr->data);

    FREE(old);
    label_destroy(label);

    return 0;
}

/*
    Create cfor

    PARAMS
    @IN it - iterator ( might be NULL )
    @IN hit - helper iterator
    @IN it_reg - register for iterator ( might be NULL)
    @IN hit_reg - register for heleper iterator
    @IN type - for type

    RETURN
    NULL iff failure
    Pointer iff success
*/
Cfor *cfor_create(Cvar *it, Cvar *hit, Register *it_reg, Register *hit_reg, uint8_t for_type) __nonull__(2, 4);

/*
    Destroy cfor

    PARAMS
    @IN cfor - Cfor

    RETURN
    This is void function
*/
void cfor_destroy(Cfor *cfor) __nonull__(1);

#endif
