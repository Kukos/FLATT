#ifndef PARSER_HELPER_H
#define PARSER_HELPER_H

/*
    Definision of external parser code

    Author: Michal Kukowski
    email: michalkukowski10@gmail.com
*/

#include <log.h>
#include <asm.h>
#include <filebuffer.h>
#include <compiler.h>
#include <arraylist.h>
#include <avl.h>

/* avl with Pvars */
extern Avl *variables;

/* array with code split at lines */
extern Darray *code_lines;

/* buffer for input code file */
extern file_buffer *fb_input;

/*
    MAIN PARSER FUNCTION

    PARAMS
    @IN file - file with source code
    @OUT out_tokens - list with tokens

    RETURN:
    0 iff failure
    1 iff success
*/
int parse(const char *file, Arraylist **out_tokens) __nonull__(1, 2);

/* DEBUG_MODE FOR PARSER */
#ifdef YY_DEBUG_MODE
    #define YY_LOG(msg, ...) \
    do{ \
        __log__(msg, ##__VA_ARGS__); \
    }while(0)
#else
    #define YY_LOG(msg, ...)
#endif

#ifdef YY_TRACE_MODE
    #define YY_TRACE(...) \
    do{ \
        __trace_call__(__TRACE__, ##__VA_ARGS__); \
    }while(0)
#else
    #define YY_TRACE(...)
#endif

#ifdef YY_DEBUG_MODE
    #define YY_ERROR(msg, errno, ...) \
    do{ \
        __log__(__ERROR__); \
        __log__("\t"); \
        __log__(msg, ##__VA_ARGS__); \
        \
        return errno; \
    }while(0)
#else
    #define YY_ERROR(msg, errno, ...)
#endif

#define PTOKEN_VAR  0
#define PTOKEN_ARR  1
#define PTOKEN_IT   2

#define IS_VAR_SET(pvar)    (pvar->set ? 1 : 0)
#define IS_VAR_USE(pvar)    (pvar->used ? 1 : 0)

#define VAR_SET(pvar)       (pvar->set = 1)
#define VAR_USE(pvar)       (pvar->used = 1)


typedef struct parser_token
{
    char *str;
    uint64_t val;
    uint64_t line;
}parser_token;

typedef struct Pvar
{
    /* is variable set ? */
    uint8_t set:1;

    /* is variable used ? */
    uint8_t used:1;

    uint8_t type:2;

    uint8_t padding:4;

    char *name;

    uint64_t array_len;

}Pvar;

typedef struct Pvalue
{
    Value *val;
    uint64_t line;

}Pvalue;

typedef struct Pexpr
{
    token_expr *expr;
    uint64_t line;
}Pexpr;

typedef struct Pcond
{
    token_cond *cond;
    uint64_t line;
}Pcond;

/*
    Create parser variable

    PARAMS
    @IN name
    @IN array - 0 if not 1 if yes
    @IN array_len - array len if normal put 0

    RETURN:
    NULL iff failure
    Pointer iff success
*/
Pvar *pvar_create(char *name, uint8_t type, uint64_t array_len) __nonull__(1);

/*
    Destroy Parser variable

    PARAMS
    @IN pvar - pointer to Pvar

    RETURN:
    This is void function
*/
void pvar_destroy(Pvar *pvar) __nonull__(1);

/*
    Standart cmp function

    PARAMS
    @IN pvar1 - Parser var 1
    @IN pvar2 - Parser var 2

    RETURN:
    -1 iff pvar1 < pvar2
    0 iff pvar1 == pvar2
    1 iff pvar1 > pvar2
*/
int pvar_cmp(void *pvar1, void *pvar2) __nonull__(1, 2);

/*
    Get pvar from varibales by name

    PARAM
    @IN name - var name

    RETURN
    NULL if var with name doesn't exist
    Pointer if exists
*/
Pvar *pvar_get_by_name(char *name) __nonull__(1);

/*
    declare variable or array

    PARAMS
    @IN type - var type 0 normal 1 array
    @IN token - pointer to token

    RETURN:
    This is void function

*/
void declare(uint8_t type, parser_token *token, uint64_t array_len) __nonull__(2);

/*
    Undeclare variable or array

    PARAMS
    @IN name - var name

    RETURN:
    This is void function
*/
void undeclare(char *name) __nonull__(1);

/*
    check variable declaration

    PARAMS
    @IN name - var name

    RETURN:
    TRUE iff var is declared
    FALSE iff var is undeclared
*/
BOOL is_declared(char *name) __nonull__(1);

/*
    CHECK initialized var

    PARAMS
    @IN name - var name

    RETURN:
    FALSE if var is not set
    TRUE if var is set
*/
BOOL is_set(char *name) __nonull__(1);

/*
    Set "initialized" flag

    PARAMS
    @IN name - var name

    RETURN:
    This is void function
*/
void set(char *name) __nonull__(1);

/*
    Check usefull of variable

    PARAMS
    @IN name - var name

    RETURN:
    FALSE if var is not used
    TRUE if var is used
*/
BOOL is_used(char *name) __nonull__(1);

/*
    Set "used" flag

    PARAMS
    @IN name - var name

    RETURN:
    This is void function
*/
void use(char *name) __nonull__(1);

/*
    Check correctnes of using this var: a[X] iff is array
                                        a iff is normal var

    PARAMS
    @IN name - var name
    @IN use_type - using type

    RETURN:
    FALSE if var is not use correctly
    TRUE if var is use correctly
*/
BOOL is_correct_use(char *name, uint8_t use_type) __nonull__(1);

/*
    Check if offset of array is out of array mem range

    PARAMS
    @IN name - array name
    @IN offset - array offset

    RETURN:
    FALSE iff offset is in mem range
    TRUE iff offset is out of mem range

*/
BOOL is_arr_out_of_range(char *name, uint64_t offset) __nonull__(1);


/*
    GET string ( name or const_val to str) of pval

    PARAMS
    @IN val - pointer to val

    RETURN:
    NULL iff failure
    New string iff success ( don't forget to free )
*/
char *pval_str(Pvalue *val) __nonull__(1);

/*
    Check using of this value

    PARAMS
    @IN val - pointer to Pval
    @IN fl - first line in code scope
    @IN ll - last line in code scope

    RETURN:
    This is a void function

    IFF something is wrong function exit with 1 code
*/
void pval_check_use(Pvalue *val, uint64_t fl, uint64_t ll) __nonull__(1);

/*
    Check init if variable

    PARAMS
    @IN val - pointer to Pval
    @IN fl - first line in code scope
    @IN ll - last line in code scope

    RETURN:
    This is a void function

    IFF something is wrong function exit with 1 code
*/
void pval_check_init(Pvalue *val, uint64_t fl, uint64_t ll) __nonull__(1);


/*
    Check correctnes of cond

    PARAMS
    @IN cond - pointer to Pcond

    RETURN:
    This is void function
*/
void check_cond(Pcond *cond) __nonull__(1);

/*
    Set to use variables from cond

    PARAMS
    @IN cond - pointer to Pcond

    RETURN:
    This is void function
*/
void use_cond(Pcond *cond) __nonull__(1);

/*
    check using iterator, we can't overwrite iterator, so
    READ it and it := ... trigger error

    PARAMS
    @IN it - for loop iterator
    @IN token - token
    @IN fl - first line in code scope
    @IN ll - last line in code scope

    RETURN:
    This is void funciton
*/
void check_use_it(Value *it, Token *token, uint64_t fl, uint64_t ll) __nonull__(1, 2);

/*
    Split string into array of string and write to arr

    PARAMS
    @IN str - string
    @IN delimeter - delimeter
    @IN arr - dynamic array

    RETURN
    0 iff success
    Non-zero value if failure
*/
int strspl(char *str, const char delimeter, Darray *arr) __nonull__(1, 3);

/*
    Print error msg and lines in code

    PARAMS
    @IN fl - first line in code with error
    @IN ll - last line in code with error
    @IN msg - error msg
    @IN ... - va_args to error msg

    RETURN:
    This is void function
*/
void print_err(uint64_t fl, uint64_t ll, const char *msg, ...) __nonull__(3);

#endif
