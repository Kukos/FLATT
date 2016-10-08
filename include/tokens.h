#ifndef TOKENS_H
#define TOKENS_H

/*
    Tokens definision for compiler

    Author: Michal Kukowski
    email: michalkukowski10@gmail.com
*/

#include <common.h>
#include <asm.h>

typedef struct token_id
{
    uint8_t undefined;
    uint8_t read;
    uint8_t write;
    uint8_t skip;
    uint8_t add;
    uint8_t sub;
    uint8_t div;
    uint8_t mod;
    uint8_t mult;
    uint8_t eq;
    uint8_t ne;
    uint8_t lt;
    uint8_t gt;
    uint8_t le;
    uint8_t ge;
    uint8_t for_inc;
    uint8_t for_dec;
    uint8_t end_for;
    uint8_t while_loop;
    uint8_t end_while;
    uint8_t cond;
    uint8_t if_cond;
    uint8_t else_cond;
    uint8_t end_if;

}token_id;

/* READ or WRITE */
typedef struct token_io
{
    Value *res;

    uint8_t op;

}token_io;

/* SKIP ELSE ENDIF ENDWHILE ENDFOR */
typedef struct token_guard
{
    uint8_t type;

}token_guard;

typedef struct token_expr
{
    uint8_t op;
    Value *left;
    Value *right;
}token_expr;

/* ASSIGN */
typedef struct token_assign
{
    /* res := left op right */
    Value *res;
    token_expr *expr;

}token_assign;

/* COND in WHILE and IF */
typedef struct token_cond
{
    /* left R right */
    Value *left;
    Value *right;

    uint8_t r;
}token_cond;

/* IF */
typedef struct token_if
{
    token_cond *cond;

}token_if;

/* WHILE */
typedef struct token_while
{
    token_cond *cond;

}token_while;

/* FOR INC AND FOR DEC */
typedef struct token_for
{
    uint8_t type;

    Value *iterator;
    Value *begin_value;
    Value *end_value;
}token_for;

#define TOKEN_TYPE      uint8_t
#define TOKENS_TYPE_CNT 8
#define UNDEFINED       0
#define TOKEN_IO        1
#define TOKEN_GUARD     2
#define TOKEN_EXPR      3
#define TOKEN_ASSIGN    4
#define TOKEN_COND      5
#define TOKEN_IF        6
#define TOKEN_WHILE     7
#define TOKEN_FOR       8



/* Generic Token */
typedef struct Token
{
    TOKEN_TYPE type;
    union
    {
        token_io        *io;
        token_guard     *guard;
        token_assign    *assign;
        token_if        *if_cond;
        token_while     *while_loop;
        token_for       *for_loop;

    }body;

}Token;

extern token_id tokens_id;

/*
    Get str from operation type

    PARAMS
    @IN op - operation type

    RETURN:
    const pointer to const string iff success
    NULL iff failure

*/
__inline__ const char const *op_get_str(uint8_t op)
{
    if(op == tokens_id.add)
        return "+";
    else if(op == tokens_id.sub)
        return "-";
    else if(op == tokens_id.div)
        return "/";
    else if(op == tokens_id.mod)
        return "%";
    else if(op == tokens_id.mult)
        return "*";
    else
        return NULL;
}

/*
    Get str from relation type

    PARAMS
    @IN r - relation type

    RETURN:
    const pointer to const string iff success
    NULL iff failure

*/
__inline__ const char const *rel_get_str(uint8_t r)
{
    if(r == tokens_id.eq)
        return "=";
    else if(r == tokens_id.ne)
        return "<>";
    else if(r == tokens_id.lt)
        return "<";
    else if(r == tokens_id.le)
        return "<=";
    else if(r == tokens_id.gt)
        return ">";
    else if(r == tokens_id.ge)
        return ">=";
    else
        return NULL;
}

/*
    Create IO token

    PARAMS
    @IN type - token type READ or WRITE
    @IN val - pointer to value

    RETURN:
    NULL iff failure
    Pointer iff success
*/
token_io *token_io_create(uint8_t type, Value *val) __nonull__(2);

/*
    Destroy IO token

    PARAMS
    @IN token - pointer to IO token

    RETURN:
    This is void function
*/
void token_io_destroy(token_io *token) __nonull__(1);

/*
    print on stdout token info

    PARAMS
    @IN token - pointer to token

    RETURN:
    This is void function
*/
void token_io_print(token_io *token) __nonull__(1);

/*
    Get token string

    PARAMS
    @IN token - pointer to token

    RETURN:
    Allocated token string iff success
    NULL iff failure
*/
char *token_io_str(token_io *token) __nonull__(1);

/*
    Create GUARD token

    PARAMS
    @IN type - token type  SKIP ELSE ENDIF ENDWHILE ENDFOR

    RETURN:
    NULL iff failure
    Pointer iff success
*/
token_guard *token_guard_create(uint8_t type);

/*
    Destroy GUARD token

    PARAMS
    @IN token - pointer to GUARD token

    RETURN:
    This is void function
*/
void token_guard_destroy(token_guard *token) __nonull__(1);

/*
    print on stdout token info

    PARAMS
    @IN token - pointer to token

    RETURN:
    This is void function
*/
void token_guard_print(token_guard *token) __nonull__(1);

/*
    Get token string

    PARAMS
    @IN token - pointer to token

    RETURN:
    Allocated token string iff success
    NULL iff failure
*/
char *token_guard_str(token_guard *token) __nonull__(1);

/*
    Create EXPR token

    at least @left and @OP needed

    @left @OP @right

    PARAMS
    @IN op - operation ADD SUB MULT MOD DIV
    @IN left - pointer to left value
    @IN right - pointer to right value

    RETURN:
    NULL iff failure
    Pointer iff success
*/
token_expr *token_expr_create(uint8_t op, Value *left, Value *right) __nonull__(2);


/*
    Destroy EXPR token

    PARAMS
    @IN token - pointer to EXPR token

    RETURN:
    This is void function
*/
void token_expr_destroy(token_expr *token) __nonull__(1);

/*
    print on stdout token info

    PARAMS
    @IN token - pointer to token

    RETURN:
    This is void function
*/
void token_expr_print(token_expr *token) __nonull__(1);

/*
    Get token string

    PARAMS
    @IN token - pointer to token

    RETURN:
    Allocated token string iff success
    NULL iff failure
*/
char *token_expr_str(token_expr *token) __nonull__(1);

/*
    Create ASSIGN token

    @res := @left @OP @right;

    PARAMS
    @IN res - pointer to res Value
    @IN expr - pointer to expr token

    RETURN:
    NULL iff failure
    Pointer iff success
*/
token_assign *token_assign_create(Value *res, token_expr *expr) __nonull__(1, 2);

/*
    Destroy ASSIGN token

    PARAMS
    @IN token - pointer to ASSIGN token

    RETURN:
    This is void function
*/
void token_assign_destroy(token_assign *token) __nonull__(1);

/*
    print on stdout token info

    PARAMS
    @IN token - pointer to token

    RETURN:
    This is void function
*/
void token_assign_print(token_assign *token) __nonull__(1);

/*
    Get token string

    PARAMS
    @IN token - pointer to token

    RETURN:
    Allocated token string iff success
    NULL iff failure
*/
char *token_assign_str(token_assign *token) __nonull__(1);

/*
    Create COND token

    @left @R @right

    PARAMS
    @IN r - relation = <> <  >  <=  >=
    @IN left - left value
    @IN right - right value

    RETURN:
    NULL iff failure
    Pointer iff success
*/
token_cond *token_cond_create(uint8_t r, Value *left, Value *right) __nonull__(2, 3);

/*
    Destroy COND token

    PARAMS
    @IN token - pointer to COND token

    RETURN:
    This is void function
*/
void token_cond_destroy(token_cond *token) __nonull__(1);

/*
    print on stdout token info

    PARAMS
    @IN token - pointer to token

    RETURN:
    This is void function
*/
void token_cond_print(token_cond *token) __nonull__(1);

/*
    Get token string

    PARAMS
    @IN token - pointer to token

    RETURN:
    Allocated token string iff success
    NULL iff failure
*/
char *token_cond_str(token_cond *token) __nonull__(1);

/*
    Create IF token from COND token

    PARAMS
    @IN cond - pointer to token_cond

    RETURN:
    NULL iff failure
    Pointer iff success
*/
token_if *token_if_create(token_cond *cond) __nonull__(1);

/*
    Destroy IF token

    PARAMS
    @IN token - pointer to IF token

    RETURN:
    This is void function
*/
void token_if_destroy(token_if *token) __nonull__(1);

/*
    print on stdout token info

    PARAMS
    @IN token - pointer to token

    RETURN:
    This is void function
*/
void token_if_print(token_if *token) __nonull__(1);

/*
    Get token string

    PARAMS
    @IN token - pointer to token

    RETURN:
    Allocated token string iff success
    NULL iff failure
*/
char *token_if_str(token_if *token) __nonull__(1);

/*
    Create WHILE token from COND token

    PARAMS
    @IN cond - pointer to token_cond

    RETURN:
    NULL iff failure
    Pointer iff success
*/
token_while *token_while_create(token_cond *cond) __nonull__(1);

/*
    Destroy WHILE token

    PARAMS
    @IN token - pointer to WHILE token

    RETURN:
    This is void function
*/
void token_while_destroy(token_while *token) __nonull__(1);

/*
    print on stdout token info

    PARAMS
    @IN token - pointer to token

    RETURN:
    This is void function
*/
void token_while_print(token_while *token) __nonull__(1);

/*
    Get token string

    PARAMS
    @IN token - pointer to token

    RETURN:
    Allocated token string iff success
    NULL iff failure
*/
char *token_while_str(token_while *token) __nonull__(1);

/*
    Create FOR token

    FOR @it FROM @begin TO / DOWNTO @end

    PARAMS
    @IN type - for type TO ( inc ) DOWNTO ( dec )
    @IN it - for iterator
    @IN begin - start value
    @IN end - end value

    RETURN:
    NULL iff failure
    Pointer iff success
*/
token_for *token_for_create(uint8_t type, Value *it, Value *begin, Value *end) __nonull__(2, 3, 4);

/*
    Destroy FOR token

    PARAMS
    @IN token - pointer to FOR token

    RETURN:
    This is void function
*/
void token_for_destroy(token_for *token) __nonull__(1);

/*
    print on stdout token info

    PARAMS
    @IN token - pointer to token

    RETURN:
    This is void function
*/
void token_for_print(token_for *token) __nonull__(1);

/*
    Get token string

    PARAMS
    @IN token - pointer to token

    RETURN:
    Allocated token string iff success
    NULL iff failure
*/
char *token_for_str(token_for *token) __nonull__(1);

/*
    Create generic token from @token

    PARAMS
    @IN type - token type
    @IN token - pointer to @type token

    RETURN:
    NULL iff failure
    Pointer iff success
*/
Token *token_create(TOKEN_TYPE type, void *token) __nonull__(2);

/*
    Destroy generic  token

    PARAMS
    @IN token - pointer to generic token

    RETURN:
    This is void function
*/
void token_destroy(Token *token) __nonull__(1);

/*
    print on stdout token info

    PARAMS
    @IN token - pointer to token

    RETURN:
    This is void function
*/
void token_print(Token *token) __nonull__(1);

/*
    Get token string

    PARAMS
    @IN token - pointer to token

    RETURN:
    Allocated token string iff success
    NULL iff failure
*/
char *token_str(Token *token) __nonull__(1);

#endif
