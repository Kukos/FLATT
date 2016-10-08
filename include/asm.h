#ifndef ASM_H
#define ASM_H

/*
    File contains asmebler definisions described by Maciek Gebala

    Author: Michal Kukowski
    email: michalkukowski10@gmail.com
*/

#include <common.h>
#include <darray.h>

/* extern definisions of structures from arch.h */
struct Register;
struct mem_chunk;

#define VAR_TYPE    uint8_t
#define UNDEFINED   0
#define CONST_VAL   1
#define VARIABLE    2
#define VAR_NORMAL  3
#define VAR_ARR     4
#define VALUE       5
#define ARRAY       6
#define BIG_ARRAY   7
#define BIG_CONST   8
#define LOOP_VAR    9

/***** MNEMONICS *****/

typedef struct Mnemonics
{
    char *get;
    char *put;
    char *load;
    char *store;
    char *add;
    char *sub;
    char *copy;
    char *shr;
    char *shl;
    char *inc;
    char *dec;
    char *zero;
    char *jump;
    char *jzero;
    char *jodd;
    char *halt;

}Mnemonics;

extern const Mnemonics mnemonics;

/***** OPCODES *****/

typedef struct Opcodes
{
    uint8_t get;
    uint8_t put;
    uint8_t load;
    uint8_t store;
    uint8_t add;
    uint8_t sub;
    uint8_t copy;
    uint8_t shr;
    uint8_t shl;
    uint8_t inc;
    uint8_t dec;
    uint8_t zero;
    uint8_t jump;
    uint8_t jzero;
    uint8_t jodd;
    uint8_t halt;

}Opcodes;

extern const Opcodes opcodes;


/***** VARIABLES *****/

/*
    Const value is not a const variable !!!!
    hardcoded i.e 100, 100000, 50
*/
typedef struct const_value
{
    /* after static analisys const can be >= 64bits */
    VAR_TYPE type;

    /* in spec we know that const value has only 64bits */
    uint64_t value;

    mpz_t big_value;

}const_value;

/* Array for us is not a pointer */
typedef struct Array
{
    /* name is unique  */
    char *name;

    /* array need to be defined with static const_value length so max is 64bits */
    uint64_t len;

    /* address */
    mpz_t addr;

    VAR_TYPE type;

    /* array of var_arr */
    Darray *arr;

    struct mem_chunk *chunk;

}Array;

/*
    Normal variable
*/
typedef struct var_normal
{
    /* name is unique, this is variable key */
    char *name;

    /* variable has "inf" bits */
    mpz_t value;

    /* i.e a + b, a * b + c / 2 */
    char *symbolic_value;

    uint8_t is_symbolic:1;
    uint8_t padding:7;

}var_normal;

/*
    Special variable it is part of array
*/
typedef struct var_arr
{
    /* pointer has normal value and offset of parent */
    var_normal *var;

    /*  iif t[10], t[1000]*/
    uint64_t offset;

    /* iff t[i] [j] */
    var_normal *var_offset;

    /* ptr to Array  */
    Array *arr;

}var_arr;

/*
    Variable is a normal  variable or part of array
*/
typedef struct Variable
{
    VAR_TYPE type;

    union
    {
        var_normal  *var;
        var_arr     *arr;

    }body;

}Variable;

/*
    type value is a const_value or variable
*/
typedef struct Value
{
    VAR_TYPE type;

    union
    {
        const_value *cv;
        Variable    *var;
    }body;

    /* iff variable is in reg */
    struct Register *reg;

    struct mem_chunk *chunk;

}Value;


/*
    Create const_value

    PARAMS
    @IN val - value

    RETURN:
    %NULL iff failure
    %Pointer iff success
*/
const_value *const_value_create(uint64_t val);

/*
    Create big const value

    PARAMS
    @IN val - big value

    RETURN:
    %NULL iff failure
    %Pointer iff success
*/
const_value *const_bigvalue_create(mpz_t val);

/*
    Destroy const_value

    PARAMS
    @IN cv - pointer to const_value

    RETURN:
    This is void function
*/
void const_value_destroy(const_value *cv) __nonull__(1);

/*
    print on stdout info about const_value

    PARAMS
    @IN cv - pointer to const value

    RETURN:
    This is a void function
*/
void const_value_print(const_value *cv) __nonull__(1);

/*
    Create normal variable

    PARAMS
    @IN name - variable name

    RETURN:
    %NULL iff failure
    %Pointer iff success
*/
var_normal *var_normal_create(const char *name) __nonull__(1);

/*
    Destroy normal variable

    PARAMS
    @IN var - pointer to variable

    RETURN:
    This is void function
*/
void var_normal_destroy(var_normal *var) __nonull__(1);

/*
    Simple cmp function ( name is a key )

    PARAMS
    @IN var1 - poiner to 1st var
    @IN var2 - pointer to 2nd var

    RETURN:
    -1 iff var1 < var2
    0 iff var1 == var2
    1 iff var1 > var2
*/
int var_normal_cmp(void *var1, void *var2) __nonull__(1, 2);

/*
    print on stdout info about var normal

    PARAMS
    @IN var - var normal

    RETURN:
    This is a void function
*/
void var_normal_print(var_normal *var) __nonull__(1);

/*
    From normal varibale create part of array

    PARAMS
    @IN var - pointer to normal variable
    @IN arr - pointer to array
    @IN offset - array offset

    RETURN:
    %NULL iff failure
    %Pointer iff success
*/
var_arr *var_arr_create(var_normal *var, Array *arr, uint64_t offset) __nonull__(1, 2);

/*
    Destroy part of array

    PARAMS
    @IN va - pointer to var_arr

    RETURN:
    This is void function
*/
void var_arr_destroy(var_arr *va) __nonull__(1);

/*
    Simple cmp function ( name is a key )

    PARAMS
    @IN va1 - poiner to 1st va
    @IN va2 - pointer to 2nd va

    RETURN:
    -1 iff va1 < va2
    0 iff va1 == va2
    1 iff va1 > va2
*/
int var_arr_cmp(void *va1, void *va2) __nonull__(1, 2);

/*
    print on stdout info about var arr

    PARAMS
    @IN va - var arr

    RETURN:
    This is a void function
*/
void var_arr_print(var_arr *va) __nonull__(1);

/*
    Create variable of type @type

    PARAMS
    @IN type - variable type
    @IN str - pointer to structure normal or array

    RETURN:
    %NULL iff failure
    %Pointer iff success
*/
Variable *variable_create(VAR_TYPE type, void *str) __nonull__(2);

/*
    Destroy variable

    PARAMS
    @IN var - pointer to variable

    RETURN:
    This is void function
*/
void variable_destroy(Variable *var) __nonull__(1);

/*
    Simple cmp function ( name is a key )

    PARAMS
    @IN var1 - poiner to 1st var
    @IN var2 - pointer to 2nd var

    RETURN:
    -1 iff var1 < var2
    0 iff var1 == var2
    1 iff var1 > var2
*/
int variable_cmp(void *var1, void *var2) __nonull__(1, 2);

/*
    Name getter

    PARAMS
    @IN var - pointer to var

    RETURN:
    NULL iff failure
    pointer to name iff success
*/
char *variable_get_name(Variable *var) __nonull__(1);

/*
    print on stdout info about variable

    PARAMS
    @IN var - pointer to Variable

    RETURN:
    This is a void function
*/
void variable_print(Variable *var) __nonull__(1);

/*
    Create value of TYPE @type

    PARAMS
    @IN type - value type
    @IN str - poinyer to structure Variable or const_value

    RETURN:
    %NULL iff failure
    %Pointer iff success
*/
Value *value_create(VAR_TYPE type, void *str) __nonull__(2);

/*
    Destroy Value

    PARAMS
    @IN val - pointer to value

    RETURN:
    This is void function
*/
void value_destroy(Value *val) __nonull__(1);

/*
    print on stdout info about Value

    PARAMS
    @IN val - pointer to Value

    RETURN:
    This is a void function
*/
void value_print(Value *val) __nonull__(1);

/*
    Get new string (allocated char*) of value

    PARAMS
    @IN val - pointer to val

    RETURN:
    String with value
*/
char *value_str(Value *val) __nonull__(1);

/*
    SET val in value

    PARAMS
    @IN value - pointer to value
    @IN val - value to set

    RETURN
    This is void function
*/
void value_set_val(Value *value, mpz_t val) __nonull__(1);

/*
    GET val from value

    PARAMS
    @IN value - pointer to value
    @OUT val - got value

    RETURN
    value

*/
int value_get_val(Value *value, mpz_t val) __nonull__(1);

/*
    Reset symbolic value ( free and set to null)

    PARAMS
    @IN val - value

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int value_reset_symbolic_value(Value *val) __nonull__(1);

/*
    Add symbolic value to old value

    PARAMS
    @IN val - value
    @IN str - symbolic string

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int value_add_symbolic(Value *val, char *str);

/*
    Check bigarray status

    PARAMS
    @IN val - value

    RETURN
    FALSE iff value isn't big array
    TRUE iff value is big array
*/
BOOL __inline__ value_is_big_array(Value *val)
{
    if(val == NULL)
        return FALSE;

    if(val->type == CONST_VAL)
        return FALSE;

    if(val->body.var->type == VAR_NORMAL)
        return FALSE;

    if(val->body.var->body.arr->arr->type == ARRAY)
        return FALSE;

    return TRUE;
}

/*
    Check symbolic state

    PARAMS
    @IN val - value

    RETURN
    FALSE iff value has normal value
    TRUE iff value has symbolic value
*/
BOOL __inline__ value_is_symbolic(Value *val)
{
    if(val == NULL)
        return FALSE;

    if(val->type == CONST_VAL)
        return FALSE;

    if(val->body.var->type == VAR_NORMAL)
        return val->body.var->body.var->is_symbolic;

    return val->body.var->body.arr->var->is_symbolic;
}

/* reset symbolic value state in value */
void __inline__ value_reset_symbolic_flag(Value *val)
{
    if(val == NULL)
        return;

    if(val->type == CONST_VAL)
        return;

    if(val->body.var->type == VAR_NORMAL)
        val->body.var->body.var->is_symbolic = 0;
    else
        val->body.var->body.arr->var->is_symbolic = 0;
}

/* set symbolic value */
void __inline__ value_set_symbolic_flag(Value *val)
{
    if(val == NULL)
        return;

    if(val->type == CONST_VAL)
        return;

    if(val->body.var->type == VAR_NORMAL)
        val->body.var->body.var->is_symbolic = 1;
    else
        val->body.var->body.arr->var->is_symbolic = 1;
}

/* reset flag and value */
int __inline__ value_reset_symbolic(Value *val)
{
    value_reset_symbolic_flag(val);

    if(value_reset_symbolic_value(val))
        ERROR("value_reset_symbolic_value error\n", 1, "");

    return 0;
}

/*
    Create new Value with all fields equal to src value

    PARAMS
    @IN dts - destination
    @IN src - source

    RETURN
    Non-zero value iff failure
    0 iff success
*/
int value_copy(Value **dst, Value *src) __nonull__(1, 2);

/*
    Create Array

    PARAMS
    @IN name - array name
    @IN len - array len

    RETURN:
    %NULL iff failure
    %Pointer iff success
*/
Array *array_create(const char *name, uint64_t len) __nonull__(1);

/*
    Destroy array

    PARAMS
    @IN array - pointer to array

    RETURN:
    This is void function
*/
void array_destroy(Array *array) __nonull__(1);

#endif
