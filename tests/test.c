#include <common.h>
#include <arch.h>
#include <asm.h>
#include <compiler.h>
#include <darray.h>
#include <avl.h>
#include <parser_helper.h>

/*
    TEST COMPILER CODE AND GENERATED CODE

    Author: Michal Kukowski
    email: michalkukowski10@gmail.com

    TEST

    Test description included in test name

    PARAMS
    NO PARAMS

    RETURN:
    %PASSED iff passed
    %FAILED iff failed
*/

#define COMP_EXEC "./compiler.out"
#define INT_EXEC "./interpreter-cln.out"

#define PASSED 0
#define FAILED 1

#define TEST(func) \
    do{ \
        if(func) \
            printf("[TEST]\t%s\t%sFAILED%s\n", #func, RED, RESET); \
        else \
            printf("[TEST]\t%s\t%sPASSED%s\n", #func, GREEN, RESET); \
    }while(0)

static int test_create_variables(void);
static int test_big_array_create(void);
static int test_cmp_variables(void);

static int test_create_tokens(void);
static int test_tokens_str(void);

static int test_pump_algo(void);
static int test_mult_algo(void);

static int test_memory_managment(void);
static int test_regs_managment1(void);
static int test_regs_managment2(void);
static int test_regs_managment3(void);

static int test_gramma_errors(void);
static int test_semantic_errors(void);

static int test_correct_semantic(void);

static int test_correct_tokens(void);

static int test_compiled_code(void);
static int test_gebala_code(void);
static int test_gotfryd_code(void);
static int test_gotfryd_code2(void);

void run(void);

static int test_create_variables(void)
{
    const_value *cv;
    const_value *cv2;
    var_normal *vn;
    var_normal *vn2;
    var_arr *va;
    Variable *var;
    Value *val;
    Array *arr;

    Darray_iterator it;
    uint64_t i;

    mpz_t bigvalue;

    cv = const_value_create(1000ull);
    if(cv == NULL)
        return FAILED;

    if(cv->value != 1000ull || cv->type != CONST_VAL)
        return FAILED;

    mpz_init(bigvalue);
    mpz_set_str(bigvalue, "100000000000000000000000000000", 10);

    cv2 = const_bigvalue_create(bigvalue);
    if(cv2 == NULL)
        return FAILED;

    if(cv2->type != BIG_CONST || mpz_cmp(bigvalue, cv2->big_value))
        return FAILED;

    vn = var_normal_create("a");
    if(vn == NULL)
        return FAILED;

    if(strcmp(vn->name,"a"))
        return FAILED;

    vn2 = var_normal_create("b");
    if(vn2 == NULL)
        return FAILED;

    if(strcmp(vn2->name,"b"))
        return FAILED;

    arr = array_create("t", 100ull);
    if(arr == NULL)
        return FAILED;

    if(strcmp(arr->name, "t") || arr->len != 100ull)
        return FAILED;

    for(darray_iterator_init(arr->arr, &it, ITI_BEGIN), i = 0; \
        ! darray_iterator_end(&it); \
        darray_iterator_next(&it), ++i)
    {
        darray_iterator_get_data(&it, (void*)&va);
        if(va->arr != arr || i != va->offset || strcmp(va->var->name, arr->name))
            return FAILED;

        if(va != ((var_arr**)it.array)[it.index])
            return FAILED;
    }

    va = var_arr_create(vn2, arr, 200ull);
    if(va == NULL)
        return FAILED;

    if(va->arr != arr || va->var != vn2 || va->offset != 200ull)
        return FAILED;

    var = variable_create(VAR_NORMAL, vn);
    if(var == NULL)
        return FAILED;

    if(var->type != VAR_NORMAL || var->body.var != vn)
        return FAILED;

    val = value_create(CONST_VAL, cv);
    if(val == NULL)
        return FAILED;

    if(val->type != CONST_VAL || val->body.cv != cv)
        return FAILED;

    /* Destroy vn2 too */
    var_arr_destroy(va);

    const_value_destroy(cv2);

    /* destroy vn too */
    variable_destroy(var);

    /* destroy cv too */
    value_destroy(val);

    array_destroy(arr);

    mpz_clear(bigvalue);

    return PASSED;
}

static int test_big_array_create(void)
{
    uint64_t len;
    Array *arr;

    len = ARRAY_MAX_LEN;

    arr = array_create("t", len);
    if(arr == NULL)
        return FAILED;

    if(arr->type != ARRAY)
        return FAILED;

    array_destroy(arr);

    len = 1ull << 40;
    arr = array_create("t", len);
    if(arr == NULL)
        return FAILED;

    if(arr->type != BIG_ARRAY)
        return FAILED;

    array_destroy(arr);

    return PASSED;
}

static int test_cmp_variables(void)
{
    var_normal *vn1;
    var_normal *vn2;

    var_arr *va1;
    var_arr *va2;

    /* to avoid warning */
    Array *arr = *(&arr);

    Variable *var1;
    Variable *var2;

    vn1 = var_normal_create("a");
    if(vn1 == NULL || strcmp(vn1->name, "a"))
        return FAILED;

    vn2 = var_normal_create("b");
    if(vn2 == NULL || strcmp(vn2->name, "b"))
        return FAILED;

    if(var_normal_cmp((void*)&vn1,(void*)&vn2) != -1)
        return FAILED;

    va1 = var_arr_create(vn1, arr, 1);
    if(va1 == NULL)
        return FAILED;

    va2 = var_arr_create(vn2, arr, 2);
    if(va1 == NULL)
        return FAILED;

    if(var_arr_cmp((void*)&va1, (void*)&va2) != -1)
        return FAILED;

    var1 = variable_create(VAR_ARR, va1);
    if(var1 == NULL)
        return FAILED;

    var2 = variable_create(VAR_ARR, va2);
    if(var2 == NULL)
        return FAILED;

    if(variable_cmp((void*)&var1, (void*)&var2) != -1)
        return FAILED;

    /* destroy va_arr and va_normal too */
    variable_destroy(var1);
    variable_destroy(var2);

    return PASSED;
}

static int test_create_tokens(void)
{
    token_io        *tio;
    token_expr      *texpr;
    token_assign    *tass;
    token_cond      *tcond1;
    token_cond      *tcond2;
    token_for       *tfor;
    token_guard     *tguard;
    token_if        *tif;
    token_while     *twhile;

    int i;
#define N 11
#define M 6

    Value *values[N];
    Variable *vars[N];
    var_normal *vns[N];

    Token *tokens[M];

    for(i = 0; i < N; ++i)
    {
        vns[i] = var_normal_create("var");
        if(vns[i] == NULL)
            return FAILED;

        vars[i] = variable_create(VAR_NORMAL, vns[i]);
        if( vars[i] == NULL)
            return FAILED;

        values[i] = value_create(VARIABLE, vars[i]);
        if(values[i] == NULL)
            return FAILED;
    }

    i = 0;

    tio = token_io_create(tokens_id.read, values[i]);
    if(tio == NULL || tio->op != tokens_id.read || tio->res != values[i])
        return FAILED;

    ++i;

    texpr = token_expr_create(tokens_id.add, values[i], values[i + 1]);
    if(texpr == NULL || texpr->op != tokens_id.add ||
        texpr->left != values[i] || texpr->right != values[i + 1] )
        return FAILED;

    i += 2;

    tass = token_assign_create(values[i], texpr);
    if(tass == NULL || tass->res != values[i] || tass->expr != texpr )
        return FAILED;

    ++i;

    tcond1 = token_cond_create(tokens_id.eq, values[i], values[i + 1]);
    if(tcond1 == NULL || tcond1->r != tokens_id.eq ||
        tcond1->left != values[i] || tcond1->right != values[i + 1])
        return FAILED;

    i +=2;

    tcond2 = token_cond_create(tokens_id.gt, values[i], values[i + 1]);
    if(tcond2 == NULL || tcond2->r != tokens_id.gt ||
        tcond2->left != values[i] || tcond2->right != values[i + 1])
        return FAILED;

    i += 2;

    tfor = token_for_create(tokens_id.for_inc, values[i], values[i + 1], values[i + 2]);
    if( tfor == NULL || tfor->type != tokens_id.for_inc ||
        tfor->iterator != values[i] || tfor->begin_value != values[i + 1] ||
        tfor->end_value != values[i + 2] )
        return FAILED;

    i += 2;

    tguard = token_guard_create(tokens_id.skip);
    if(tguard == NULL || tguard->type != tokens_id.skip)
        return FAILED;

    tif = token_if_create(tcond1);
    if(tif == NULL || tif->cond != tcond1)
        return FAILED;

    twhile = token_while_create(tcond2);
    if(twhile == NULL || twhile->cond != tcond2)
        return FAILED;

    tokens[0] = token_create(TOKEN_IO, tio);
    if( tokens[0] == NULL || tokens[0]->type != TOKEN_IO ||
        tokens[0]->body.io != tio)
        return FAILED;

    tokens[1] = token_create(TOKEN_ASSIGN, tass);
    if( tokens[1] == NULL || tokens[1]->type != TOKEN_ASSIGN ||
        tokens[1]->body.assign != tass)
        return FAILED;

    tokens[2] = token_create(TOKEN_GUARD, tguard);
    if( tokens[2] == NULL || tokens[2]->type != TOKEN_GUARD ||
        tokens[2]->body.guard != tguard)
        return FAILED;

    tokens[3] = token_create(TOKEN_IF, tif);
    if( tokens[3] == NULL || tokens[3]->type != TOKEN_IF ||
        tokens[3]->body.if_cond != tif)
        return FAILED;

    tokens[4] = token_create(TOKEN_FOR, tfor);
    if( tokens[4] == NULL || tokens[4]->type != TOKEN_FOR ||
        tokens[4]->body.for_loop != tfor)
        return FAILED;

    tokens[5] = token_create(TOKEN_WHILE, twhile);
    if( tokens[5] == NULL || tokens[5]->type != TOKEN_WHILE ||
        tokens[5]->body.while_loop != twhile)
        return FAILED;

    for(i = 0; i < M; ++i)
        token_destroy(tokens[i]);

    return PASSED;

#undef N
#undef M
}

static int test_tokens_str(void)
{

#define VALS    25
#define TOKENS  16
#define CONST   7
#define NORMAL  18
#define ARR     9
#define VARS    18
#define EXPR    5
#define COND    2

    token_io        *tio;
    token_io        *tio2;
    token_io        *tio3;

    token_guard     *guard;
    token_guard     *guard2;
    token_guard     *guard3;
    token_guard     *guard4;
    token_guard     *guard5;

    token_assign    *ass;
    token_assign    *ass2;
    token_assign    *ass3;
    token_assign    *ass4;
    token_assign    *ass5;

    token_if        *tif;

    token_while     *twhile;

    token_for       *tfor;

    const_value     *cv[CONST];
    var_normal      *vn[NORMAL];
    var_arr         *va[ARR];

    Variable        *vars[VARS];
    Value           *values[VALS];

    Token           *tokens[TOKENS];
    char            *str[TOKENS];

    Array           *fake = *(&fake);

    token_expr      *exprs[EXPR];
    token_cond      *conds[COND];

    const char *const expected[] =
    {
        /* IO  */
        "WRITE\t 1000 ",
        "READ\t a ",
        "WRITE\t t[ 10 ] ",
        /* GUARD */
        "SKIP",
        "ELSE",
        "ENDWHILE",
        "ENDIF",
        "ENDFOR",
        /* ASSIGN */
        " a  :=  10  +  15 ",
        " a  :=  b  *  3 ",
        " a  :=  t[ 0 ]  -  c ",
        " t[ 1 ]  :=  a  %  2 ",
        " t[ 2 ]  :=  t[ 1 ]  /  t[ 0 ] ",
        "IF\t 10  <>  20 ",
        "WHILE\t t[ 1 ]  >  t[ 2 ] ",
        "FOR  i  FROM  a  TO  t[ 1 ] "
    };

    int c_vals   = 0;
    int c_tokens = 0;
    int c_const  = 0;
    int c_normal = 0;
    int c_arr    = 0;
    int c_vars   = 0;
    int c_exprs  = 0;
    int c_conds  = 0;

    int i;

    /*  WRITE 1000 */
    cv[c_const] = const_value_create(1000);
    if(cv[c_const] == NULL)
        return FAILED;

    values[c_vals] = value_create(CONST_VAL, (void*)cv[c_const]);
    if(values[c_vals] == NULL)
        return FAILED;

    tio = token_io_create(tokens_id.write, values[c_vals]);
    if(tio == NULL)
        return FAILED;

    tokens[c_tokens] = token_create(TOKEN_IO, (void*)tio);
    if(tokens[c_tokens] == NULL)
        return FAILED;

    str[c_tokens] = token_str(tokens[c_tokens]);
    if(strcmp(str[c_tokens], expected[c_tokens]))
        return FAILED;

    ++c_const;
    ++c_vals;
    ++c_tokens;

    /* READ a */
    vn[c_normal] = var_normal_create("a");
    if(vn[c_normal] == NULL)
        return FAILED;

    vars[c_vars] = variable_create(VAR_NORMAL, (void*)vn[c_normal]);
    if(vars[c_vars] == NULL)
        return FAILED;

    values[c_vals] = value_create(VARIABLE, (void*)vars[c_vars]);
    if(values[c_vals] == NULL)
        return FAILED;

    tio2 = token_io_create(tokens_id.read, values[c_vals]);
    if(tio2 == NULL)
        return FAILED;

    tokens[c_tokens] = token_create(TOKEN_IO, (void*)tio2);
    if(tokens[c_tokens] == NULL)
        return FAILED;

    str[c_tokens] = token_str(tokens[c_tokens]);
    if(strcmp(str[c_tokens], expected[c_tokens]))
        return FAILED;

    ++c_normal;
    ++c_vars;
    ++c_vals;
    ++c_tokens;

    /* "WRITE\tt[ 10 ]"  */
    vn[c_normal] = var_normal_create("t");
    if(vn[c_normal] == NULL)
        return FAILED;

    va[c_arr] = var_arr_create(vn[c_normal], fake, 10);
    if(va[c_arr] == NULL)
        return FAILED;

    vars[c_vars] = variable_create(VAR_ARR, (void*)va[c_arr]);
    if(vars[c_vars] == NULL)
        return FAILED;

    values[c_vals] = value_create(VARIABLE, (void*)vars[c_vars]);
    if(values[c_vals] == NULL)
        return FAILED;

    tio3 = token_io_create(tokens_id.write, values[c_vals]);
    if(tio3 == NULL)
        return FAILED;

    tokens[c_tokens] = token_create(TOKEN_IO, (void*)tio3);
    if(tokens[c_tokens] == NULL)
        return FAILED;

    str[c_tokens] = token_str(tokens[c_tokens]);
    if(strcmp(str[c_tokens], expected[c_tokens]))
        return FAILED;

    ++c_normal;
    ++c_arr;
    ++c_vars;
    ++c_vals;
    ++c_tokens;

    /*  SKIP */
    guard = token_guard_create(tokens_id.skip);
    if(guard == NULL)
        return FAILED;

    tokens[c_tokens] = token_create(TOKEN_GUARD, (void*)guard);
    if(tokens[c_tokens] == NULL)
        return FAILED;

    str[c_tokens] = token_str(tokens[c_tokens]);
    if(strcmp(str[c_tokens], expected[c_tokens]))
        return FAILED;

    ++c_tokens;

    /* ELSE */
    guard2 = token_guard_create(tokens_id.else_cond);
    if(guard2 == NULL)
        return FAILED;

    tokens[c_tokens] = token_create(TOKEN_GUARD, (void*)guard2);
    if(tokens[c_tokens] == NULL)
        return FAILED;

    str[c_tokens] = token_str(tokens[c_tokens]);
    if(strcmp(str[c_tokens], expected[c_tokens]))
        return FAILED;

    ++c_tokens;

    /* ENDWHILE */
    guard3 = token_guard_create(tokens_id.end_while);
    if(guard3 == NULL)
        return FAILED;

    tokens[c_tokens] = token_create(TOKEN_GUARD, (void*)guard3);
    if(tokens[c_tokens] == NULL)
        return FAILED;

    str[c_tokens] = token_str(tokens[c_tokens]);
    if(strcmp(str[c_tokens], expected[c_tokens]))
        return FAILED;

    ++c_tokens;

    /* ENDIF */
    guard4 = token_guard_create(tokens_id.end_if);
    if(guard4 == NULL)
        return FAILED;

    tokens[c_tokens] = token_create(TOKEN_GUARD, (void*)guard4);
    if(tokens[c_tokens] == NULL)
        return FAILED;

    str[c_tokens] = token_str(tokens[c_tokens]);
    if(strcmp(str[c_tokens], expected[c_tokens]))
        return FAILED;

    ++c_tokens;

    /* ENDFOR */
    guard5 = token_guard_create(tokens_id.end_for);
    if(guard5 == NULL)
        return FAILED;

    tokens[c_tokens] = token_create(TOKEN_GUARD, (void*)guard5);
    if(tokens[c_tokens] == NULL)
        return FAILED;

    str[c_tokens] = token_str(tokens[c_tokens]);
    if(strcmp(str[c_tokens], expected[c_tokens]))
        return FAILED;

    ++c_tokens;

    /* a  :=  10  +  15 */
    vn[c_normal] = var_normal_create("a");
    if(vn[c_normal] == NULL)
        return FAILED;

    vars[c_vars] = variable_create(VAR_NORMAL, (void*)vn[c_normal]);
    if(vars[c_vars] == NULL)
        return FAILED;

    values[c_vals] = value_create(VARIABLE, (void*)vars[c_vars]);
    if(values[c_vals] == NULL)
        return FAILED;

    cv[c_const] = const_value_create(10);
    if(cv[c_const] == NULL)
        return FAILED;

    values[c_vals + 1] = value_create(CONST_VAL, (void*)cv[c_const]);
    if(values[c_vals + 1] == NULL)
        return FAILED;

    cv[c_const + 1] = const_value_create(15);
    if(cv[c_const + 1] == NULL)
        return FAILED;

    values[c_vals + 2] = value_create(CONST_VAL, (void*)cv[c_const + 1]);
    if(values[c_vals + 2] == NULL)
        return FAILED;

    exprs[c_exprs] = token_expr_create(tokens_id.add, values[c_vals + 1], values[c_vals + 2]);
    if(exprs[c_exprs] == NULL)
        return FAILED;

    ass = token_assign_create(values[c_vals], exprs[c_exprs]);
    if(ass == NULL)
        return FAILED;

    tokens[c_tokens] = token_create(TOKEN_ASSIGN, (void*)ass);
    if(tokens[c_tokens] == NULL)
        return FAILED;

    str[c_tokens] = token_str(tokens[c_tokens]);
    if(strcmp(str[c_tokens], expected[c_tokens]))
        return FAILED;

    ++c_normal;
    ++c_vars;
    c_vals += 3;
    c_const += 2;
    ++c_exprs;
    ++c_tokens;

    /* a  :=  b  *  3  */
    vn[c_normal] = var_normal_create("a");
    if(vn[c_normal] == NULL)
        return FAILED;

    vars[c_vars] = variable_create(VAR_NORMAL, (void*)vn[c_normal]);
    if(vars[c_vars] == NULL)
        return FAILED;

    values[c_vals] = value_create(VARIABLE, (void*)vars[c_vars]);
    if(values[c_vals] == NULL)
        return FAILED;

    vn[c_normal + 1] = var_normal_create("b");
    if(vn[c_normal + 1] == NULL)
        return FAILED;

    vars[c_vars + 1] = variable_create(VAR_NORMAL, (void*)vn[c_normal + 1]);
    if(vars[c_vars + 1] == NULL)
        return FAILED;

    values[c_vals + 1] = value_create(VARIABLE, (void*)vars[c_vars + 1]);
    if(values[c_vals + 1] == NULL)
        return FAILED;

    cv[c_const] = const_value_create(3);
    if(cv[c_const] == NULL)
        return FAILED;

    values[c_vals + 2] = value_create(CONST_VAL, (void*)cv[c_const]);
    if(values[c_vals + 2] == NULL)
        return FAILED;

    exprs[c_exprs] = token_expr_create(tokens_id.mult, values[c_vals + 1], values[c_vals + 2]);
    if(exprs[c_exprs] == NULL)
        return FAILED;

    ass2 = token_assign_create(values[c_vals], exprs[c_exprs]);
    if(ass2 == NULL)
        return FAILED;

    tokens[c_tokens] = token_create(TOKEN_ASSIGN, (void*)ass2);
    if(tokens[c_tokens] == NULL)
        return FAILED;

    str[c_tokens] = token_str(tokens[c_tokens]);
    if(strcmp(str[c_tokens], expected[c_tokens]))
        return FAILED;

    c_normal += 2;
    c_vars += 2;
    ++c_const;
    c_vals += 3;
    ++c_exprs;
    ++c_tokens;

    /*  a  :=  t[ 0 ]  -  c  */
    vn[c_normal] = var_normal_create("a");
    if(vn[c_normal] == NULL)
        return FAILED;

    vars[c_vars] = variable_create(VAR_NORMAL, (void*)vn[c_normal]);
    if(vars[c_vars] == NULL)
        return FAILED;

    values[c_vals] = value_create(VARIABLE, (void*)vars[c_vars]);
    if(values[c_vals] == NULL)
        return FAILED;

    vn[c_normal + 1] = var_normal_create("t");
    if(vn[c_normal + 1] == NULL)
        return FAILED;

    va[c_arr] = var_arr_create(vn[c_normal + 1], fake, 0);
    if(va[c_arr] == NULL)
        return FAILED;

    vars[c_vars + 1] = variable_create(VAR_ARR, (void*)va[c_arr]);
    if(vars[c_vars + 1] == NULL)
        return FAILED;

    values[c_vals + 1] = value_create(VARIABLE, (void*)vars[c_vars + 1]);
    if(values[c_vals + 1] == NULL)
        return FAILED;

    vn[c_normal + 2] = var_normal_create("c");
    if(vn[c_normal + 2] == NULL)
        return FAILED;

    vars[c_vars + 2] = variable_create(VAR_NORMAL, (void*)vn[c_normal + 2]);
    if(vars[c_vars + 2] == NULL)
        return FAILED;

    values[c_vals + 2] = value_create(VARIABLE, (void*)vars[c_vars + 2]);
    if(values[c_vals + 2] == NULL)
        return FAILED;

    exprs[c_exprs] = token_expr_create(tokens_id.sub, values[c_vals + 1], values[c_vals + 2]);
    if(exprs[c_exprs] == NULL)
        return FAILED;

    ass3 = token_assign_create(values[c_vals], exprs[c_exprs]);
    if(ass3 == NULL)
        return FAILED;

    tokens[c_tokens] = token_create(TOKEN_ASSIGN, (void*)ass3);
    if(tokens[c_tokens] == NULL)
        return FAILED;

    str[c_tokens] = token_str(tokens[c_tokens]);
    if(strcmp(str[c_tokens], expected[c_tokens]))
        return FAILED;

    c_normal += 3;
    c_vars += 3;
    c_vals += 3;
    ++c_arr;
    ++c_exprs;
    ++c_tokens;

    /*  t[ 1 ]  :=  a  %  2  */
    vn[c_normal] = var_normal_create("t");
    if(vn[c_normal] == NULL)
        return FAILED;

    va[c_arr] = var_arr_create(vn[c_normal], fake, 1);
    if(va[c_arr] == NULL)
        return FAILED;

    vars[c_vars] = variable_create(VAR_ARR, (void*)va[c_arr]);
    if(vars[c_vars] == NULL)
        return FAILED;

    values[c_vals] = value_create(VARIABLE, (void*)vars[c_vars]);
    if(values[c_vals] == NULL)
        return FAILED;

    vn[c_normal + 1] = var_normal_create("a");
    if(vn[c_normal + 1] == NULL)
        return FAILED;

    vars[c_vars + 1] = variable_create(VAR_NORMAL, (void*)vn[c_normal + 1]);
    if(vars[c_vars + 1] == NULL)
        return FAILED;

    values[c_vals + 1] = value_create(VARIABLE, (void*)vars[c_vars + 1]);
    if(values[c_vals + 1] == NULL)
        return FAILED;

    cv[c_const] = const_value_create(2);
    if(cv[c_const] == NULL)
        return FAILED;

    values[c_vals + 2] = value_create(CONST_VAL, (void*)cv[c_const]);
    if(values[c_vals + 2] == NULL)
        return FAILED;

    exprs[c_exprs] = token_expr_create(tokens_id.mod, values[c_vals + 1], values[c_vals + 2]);
    if(exprs[c_exprs] == NULL)
        return FAILED;

    ass4 = token_assign_create(values[c_vals], exprs[c_exprs]);
    if(ass4 == NULL)
        return FAILED;

    tokens[c_tokens] = token_create(TOKEN_ASSIGN, (void*)ass4);
    if(tokens[c_tokens] == NULL)
        return FAILED;

    str[c_tokens] = token_str(tokens[c_tokens]);
    if(strcmp(str[c_tokens], expected[c_tokens]))
        return FAILED;

    c_normal += 2;
    c_vars += 2;
    ++c_arr;
    ++c_const;
    c_vals += 3;
    ++c_exprs;
    ++c_tokens;

    /*  t[ 2 ]  :=  t[ 1 ]  /  t[ 0 ] */
    vn[c_normal] = var_normal_create("t");
    if(vn[c_normal] == NULL)
        return FAILED;

    va[c_arr] = var_arr_create(vn[c_normal], fake, 2);
    if(va[c_arr] == NULL)
        return FAILED;

    vars[c_vars] = variable_create(VAR_ARR, (void*)va[c_arr]);
    if(vars[c_vars] == NULL)
        return FAILED;

    values[c_vals] = value_create(VARIABLE, (void*)vars[c_vars]);
    if(values[c_vals] == NULL)
        return FAILED;

    vn[c_normal + 1] = var_normal_create("t");
    if(vn[c_normal + 1] == NULL)
        return FAILED;

    va[c_arr + 1] = var_arr_create(vn[c_normal + 1], fake, 1);
    if(va[c_arr + 1] == NULL)
        return FAILED;

    vars[c_vars + 1] = variable_create(VAR_ARR, (void*)va[c_arr + 1]);
    if(vars[c_vars + 1] == NULL)
        return FAILED;

    values[c_vals + 1] = value_create(VARIABLE, (void*)vars[c_vars + 1]);
    if(values[c_vals + 1] == NULL)
        return FAILED;

    vn[c_normal + 2] = var_normal_create("t");
    if(vn[c_normal + 2] == NULL)
        return FAILED;

    va[c_arr + 2] = var_arr_create(vn[c_normal + 2], fake, 0);
    if(va[c_arr + 2] == NULL)
        return FAILED;

    vars[c_vars + 2] = variable_create(VAR_ARR, (void*)va[c_arr + 2]);
    if(vars[c_vars + 2] == NULL)
        return FAILED;

    values[c_vals + 2] = value_create(VARIABLE, (void*)vars[c_vars + 2]);
    if(values[c_vals + 2] == NULL)
        return FAILED;

    exprs[c_exprs] = token_expr_create(tokens_id.div, values[c_vals + 1], values[c_vals + 2]);
    if(exprs[c_exprs] == NULL)
        return FAILED;

    ass5 = token_assign_create(values[c_vals], exprs[c_exprs]);
    if(ass5 == NULL)
        return FAILED;

    tokens[c_tokens] = token_create(TOKEN_ASSIGN, (void*)ass5);
    if(tokens[c_tokens] == NULL)
        return FAILED;

    str[c_tokens] = token_str(tokens[c_tokens]);
    if(strcmp(str[c_tokens], expected[c_tokens]))
        return FAILED;

    c_normal += 3;
    c_arr += 3;
    c_vars += 3;
    c_vals += 3;
    ++c_exprs;
    ++c_tokens;

    /* IF       10  <>  20 */
    cv[c_const] = const_value_create(10);
    if(cv[c_const] == NULL)
        return FAILED;

    values[c_vals] = value_create(CONST_VAL, (void*)cv[c_const]);
    if(values[c_vals] == NULL)
        return FAILED;

    cv[c_const + 1] = const_value_create(20);
    if(cv[c_const + 1] == NULL)
        return FAILED;

    values[c_vals + 1] = value_create(CONST_VAL, (void*)cv[c_const + 1]);
    if(values[c_vals + 1] == NULL)
        return FAILED;

    conds[c_conds] = token_cond_create(tokens_id.ne, values[c_vals], values[c_vals + 1]);
    if(conds[c_conds] == NULL)
        return FAILED;

    tif = token_if_create(conds[c_conds]);
    if(tif == NULL)
        return FAILED;

    tokens[c_tokens] = token_create(TOKEN_IF, (void*)tif);
    if(tokens[c_tokens] == NULL)
        return FAILED;

    str[c_tokens] = token_str(tokens[c_tokens]);
    if(strcmp(str[c_tokens], expected[c_tokens]))
        return FAILED;

    c_const += 2;
    c_vals += 2;
    ++c_conds;
    ++c_tokens;

    /* WHILE\t t[ 1 ]  >  t[ 2 ]  */
    vn[c_normal] = var_normal_create("t");
    if(vn[c_normal] == NULL)
        return FAILED;

    va[c_arr] = var_arr_create(vn[c_normal], fake, 1);
    if(va[c_arr] == NULL)
        return FAILED;

    vars[c_vars] = variable_create(VAR_ARR, (void*)va[c_arr]);
    if(vars[c_vars] == NULL)
        return FAILED;

    values[c_vals] = value_create(VARIABLE, (void*)vars[c_vars]);
    if(values[c_vals] == NULL)
        return FAILED;

    vn[c_normal + 1] = var_normal_create("t");
    if(vn[c_normal + 1] == NULL)
        return FAILED;

    va[c_arr + 1] = var_arr_create(vn[c_normal + 1], fake, 2);
    if(va[c_arr + 1] == NULL)
        return FAILED;

    vars[c_vars + 1] = variable_create(VAR_ARR, (void*)va[c_arr + 1]);
    if(vars[c_vars + 1] == NULL)
        return FAILED;

    values[c_vals + 1] = value_create(VARIABLE, (void*)vars[c_vars + 1]);
    if(values[c_vals + 1] == NULL)
        return FAILED;

    conds[c_conds] = token_cond_create(tokens_id.gt, values[c_vals], values[c_vals + 1]);
    if(conds[c_conds] == NULL)
        return FAILED;

    twhile = token_while_create(conds[c_conds]);
    if(twhile == NULL)
        return FAILED;

    tokens[c_tokens] = token_create(TOKEN_WHILE, (void*)twhile);
    if(tokens[c_tokens] == NULL)
        return FAILED;

    str[c_tokens] = token_str(tokens[c_tokens]);
    if(strcmp(str[c_tokens], expected[c_tokens]))
        return FAILED;

    c_normal += 2;
    c_arr += 2;
    c_vars += 2;
    c_vals += 2;
    ++c_tokens;
    ++c_conds;

    /* "FOR  i  FROM  a  TO  t[ 1 ]  */
    vn[c_normal] = var_normal_create("i");
    if(vn[c_normal] == NULL)
        return FAILED;

    vars[c_vars] = variable_create(VAR_NORMAL, (void*)vn[c_normal]);
    if(vars[c_vars] == NULL)
        return FAILED;

    values[c_vals] = value_create(VARIABLE, (void*)vars[c_vars]);
    if(values[c_vals] == NULL)
        return FAILED;

    vn[c_normal + 1] = var_normal_create("a");
    if(vn[c_normal + 1] == NULL)
        return FAILED;

    vars[c_vars + 1] = variable_create(VAR_NORMAL, (void*)vn[c_normal + 1]);
    if(vars[c_vars + 1] == NULL)
        return FAILED;

    values[c_vals + 1] = value_create(VARIABLE, (void*)vars[c_vars + 1]);
    if(values[c_vals + 1] == NULL)
        return FAILED;

    vn[c_normal + 2] = var_normal_create("t");
    if(vn[c_normal + 2] == NULL)
        return FAILED;

    va[c_arr] = var_arr_create(vn[c_normal + 2], fake, 1);
    if(va[c_arr] == NULL)
        return FAILED;

    vars[c_vars + 2] = variable_create(VAR_ARR, (void*)va[c_arr]);
    if(vars[c_vars + 2] == NULL)
        return FAILED;

    values[c_vals + 2] = value_create(VARIABLE, (void*)vars[c_vars + 2]);
    if(values[c_vals + 2] == NULL)
        return FAILED;

    tfor = token_for_create(tokens_id.for_inc, values[c_vals], values[c_vals + 1], values[c_vals + 2]);
    if(tfor == NULL)
        return FAILED;

    tokens[c_tokens] = token_create(TOKEN_FOR, (void*)tfor);
    if(tokens[c_tokens] == NULL)
        return FAILED;

    str[c_tokens] = token_str(tokens[c_tokens]);
    if(strcmp(str[c_tokens], expected[c_tokens]))
        return FAILED;

    c_normal += 3;
    c_vars += 3;
    ++c_arr;
    c_vals += 3;
    ++c_tokens;

 /* clean up, token clean sub tokens and values, so it clean all*/
    for(i = 0; i < c_tokens; ++i)
    {
        FREE(str[i]);
        token_destroy(tokens[i]);
    }

    return PASSED;

#undef VALS
#undef TOKENS
#undef CONST
#undef NORMAL
#undef ARR
#undef VARS
#undef EXPR
#undef COND
}

/* fake asm function */
#define ZERO(REG)       REG = 0
#define INC(REG)        ++REG
#define DEC(REG)        --REG
#define SHL(REG)        REG <<= 1
#define SHR(REG)        REG >>= 1
#define ADD(REG)        REG += MEM[R0]
#define SUB(REG)        REG -= MEM[R0]
#define LOAD(REG)       REG = MEM[R0]
#define STORE(REG)      MEM[R0] = REG

static int test_pump_algo(void)
{
#define BZERO(REG)  mpz_set_ui(REG, 0)
#define BINC(REG)   mpz_add_ui(REG, REG, 1)
#define BSHL(REG)   mpz_mul_2exp(REG, REG, 1)

#define PUMP(REG, VAL) \
    do{\
        int i = (sizeof(uint64_t) << 3) - 1; \
        \
        while( ! GET_BIT(VAL, i) && i) \
            --i; \
        \
        ZERO(REG); \
        for(; i > 0; --i) \
            if(GET_BIT(VAL, i)) \
            { \
                INC(REG); \
                SHL(REG); \
            } \
            else \
                SHL(REG); \
        \
        if( GET_BIT(VAL, i)) \
            INC(REG); \
    }while(0)

#define BPUMP(REG, VAL) \
    do{ \
        uint64_t i = mpz_sizeinbase(VAL, 2); \
        BZERO(REG); \
        \
        for(--i; i > 0; --i) \
            if(BGET_BIT(VAL, i)) \
            { \
                BINC(REG); \
                BSHL(REG); \
            } \
            else \
                BSHL(REG); \
        \
        if( BGET_BIT(VAL, i)) \
            BINC(REG); \
    }while(0)

    uint64_t expected1 = 0ull;
    uint64_t expected2 = 1ull;
    uint64_t expected3 = 123456678ull;
    uint64_t expected4 = 8732648721ull;
    uint64_t expected5 = 99999000000111ull;

    mpz_t bexpected1;
    mpz_t bexpected2;
    mpz_t bexpected3;
    mpz_t bexpected4;
    mpz_t bexpected5;

    uint64_t reg;
    mpz_t breg;

    PUMP(reg, expected1);
    if(reg != expected1)
        return FAILED;

    PUMP(reg, expected2);
    if(reg != expected2)
        return FAILED;

    PUMP(reg, expected3);
    if(reg != expected3)
        return FAILED;

    PUMP(reg, expected4);
    if(reg != expected4)
        return FAILED;

    PUMP(reg, expected5);
    if(reg != expected5)
        return FAILED;

    mpz_init(bexpected1);
    mpz_init(bexpected2);
    mpz_init(bexpected3);
    mpz_init(bexpected4);
    mpz_init(bexpected5);

    mpz_set_str(bexpected1, "0", 10);
    mpz_set_str(bexpected2, "1", 10);
    mpz_set_str(bexpected3, "389627835871234320", 10);
    mpz_set_str(bexpected4, "62137862187316782367862193879812739", 10);
    mpz_set_str(bexpected5, "99999999000000011111112222222333333388888844444777755552987387127391", 10);

    mpz_init(breg);

    BPUMP(breg, bexpected1);
    if(mpz_cmp(breg, bexpected1))
        return FAILED;

    BPUMP(breg, bexpected2);
    if(mpz_cmp(breg, bexpected2))
        return FAILED;

    BPUMP(breg, bexpected3);
    if(mpz_cmp(breg, bexpected3))
        return FAILED;

    BPUMP(breg, bexpected4);
    if(mpz_cmp(breg, bexpected4))
        return FAILED;

    BPUMP(breg, bexpected5);
    if(mpz_cmp(breg, bexpected5))
        return FAILED;

    mpz_clear(bexpected1);
    mpz_clear(bexpected2);
    mpz_clear(bexpected3);
    mpz_clear(bexpected4);
    mpz_clear(bexpected5);

    mpz_clear(breg);

    return PASSED;

#undef BZERO
#undef BINC
#undef BSHL

#undef PUMP
#undef BPUMP
}

static int test_mult_algo(void)
{
#define MULT(REG1, REG2, REG3) \
    do{ \
        ZERO(REG1); \
        if(REG2 == 0) \
            break; \
        if(REG3 == 0) \
            break; \
        if(REG2 <= REG3) \
        { \
            R0 = 2; \
            \
            while(REG2) \
            { \
                if(REG2 & 1) \
                { \
                    ADD(REG1); \
                    SHL(REG3); \
                    STORE(REG3); \
                    SHR(REG2); \
                } \
                else \
                { \
                    SHL(REG3); \
                    STORE(REG3); \
                    SHR(REG2); \
                } \
            } \
        } \
        else \
        { \
            R0 = 1; \
            \
            while(REG3) \
            { \
                if(REG3 & 1) \
                { \
                    ADD(REG1); \
                    SHL(REG2); \
                    STORE(REG2); \
                    SHR(REG3); \
                } \
                else \
                { \
                    SHL(REG2); \
                    STORE(REG2); \
                    SHR(REG3); \
                } \
            } \
        } \
\
    }while(0)

#define UPDATE_MEM \
    do{ \
        MEM[0] = reg1; \
        MEM[1] = reg2; \
        MEM[2] = reg3; \
    }while(0)

    uint64_t R0;
    uint64_t MEM[3];
    uint64_t reg1 = 0;
    uint64_t reg2 = 0;
    uint64_t reg3 = 0;

    memset(MEM, 0, sizeof(uint64_t) * 3);

    reg2 = 1000ull;
    reg3 = 0;

    UPDATE_MEM;
    MULT(reg1, reg2, reg3);

    if(reg1 != 0)
        return FAILED;

    reg2 = 0;
    reg3 = 1234ull;

    UPDATE_MEM;
    MULT(reg1, reg2, reg3);

    if(reg1 != 0)
        return FAILED;

    reg2 = 2;
    reg3 = 10;

    UPDATE_MEM;
    MULT(reg1, reg2, reg3);

    if(reg1 != 20)
        return FAILED;

    reg2 = 1234ull;
    reg3 = 3456ull;

    UPDATE_MEM;
    MULT(reg1, reg2, reg3);

    if(reg1 != 1234ull * 3456ull)
        return FAILED;

    reg2 = 123ull;
    reg3 = 321ull;

    UPDATE_MEM;
    MULT(reg1, reg2, reg3);

    if(reg1 != 123ull * 321ull)
        return FAILED;

    return PASSED;

#undef MULT
#undef UPDATE_MEM
}

#undef ZERO
#undef INC
#undef DEC
#undef SHL
#undef SHR
#undef ADD
#undef SUB
#undef LOAD
#undef STORE

static int test_memory_managment(void)
{
#define VARS        20
#define FORS        3
#define TOKENS      6
#define ITERATORS   3

    Pvar *vars[VARS];

    token_for *tfor[FORS];
    token_guard *guards[FORS];
    Token *token[TOKENS];

    var_normal *vn;
    Variable *var;
    Value *val;

    var_normal *vn_it[ITERATORS];
    Variable *var_it[ITERATORS];
    Value *val_it[ITERATORS];

    Avl *__variables;
    Arraylist *__tokens;

    int i;

    i = 0;

    __variables = avl_create(sizeof(Pvar*), pvar_cmp);
    if(__variables == NULL)
        return FAILED;

    /*
        Create some variables
        SUMMARY:

        VARS = 12
        ARR = 5  SIZE = 701900
        BIG = 3 SIZE = 4 * MAX + 100.010.000.000
    */
    vars[i] = pvar_create("a", PTOKEN_VAR, 0);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;

    ++i;

    vars[i] = pvar_create("b", PTOKEN_VAR, 0);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;

    ++i;

    vars[i] = pvar_create("c", PTOKEN_VAR, 0);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;
    ++i;

    vars[i] = pvar_create("d", PTOKEN_VAR, 0);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;
    ++i;

    vars[i] = pvar_create("e", PTOKEN_VAR, 0);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;

    ++i;

    vars[i] = pvar_create("t1", PTOKEN_ARR, ARRAY_MAX_LEN / 10);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;

    ++i;

    vars[i] = pvar_create("t2", PTOKEN_ARR, ARRAY_MAX_LEN / 3);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;

    ++i;

    vars[i] = pvar_create("t3", PTOKEN_ARR, ARRAY_MAX_LEN / 2);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;

    ++i;

    vars[i] = pvar_create("bt1", PTOKEN_ARR, ARRAY_MAX_LEN + 10000000);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;

    ++i;

    vars[i] = pvar_create("bt2", PTOKEN_ARR, ARRAY_MAX_LEN + 100000000000);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;

    ++i;

    vars[i] = pvar_create("f", PTOKEN_VAR, 0);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;


    ++i;

    vars[i] = pvar_create("g", PTOKEN_VAR, 0);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;

    ++i;

    vars[i] = pvar_create("h", PTOKEN_VAR, 0);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;

    ++i;

    vars[i] = pvar_create("ajiusdyhkujagshdjhsakjhjd", PTOKEN_VAR, 0);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;

    ++i;

    vars[i] = pvar_create("khjaskdhjkash____dkjashjdkasjd____e", PTOKEN_VAR, 0);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;

    ++i;

    vars[i] = pvar_create("tasdasdasdasdadada", PTOKEN_ARR, ARRAY_MAX_LEN - 1);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;

    ++i;

    vars[i] = pvar_create("tsadasdiuyeqwiuyeuiqwyeuiwq", PTOKEN_ARR, ARRAY_MAX_LEN / 5);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;

    ++i;

    vars[i] = pvar_create("sadasdg", PTOKEN_VAR, 0);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;

    ++i;

    vars[i] = pvar_create("qwhjekqwhke", PTOKEN_VAR, 0);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;

    ++i;

    vars[i] = pvar_create("btqweqwe2", PTOKEN_ARR, ARRAY_MAX_LEN << 1);
    if(vars[i] == NULL)
        return FAILED;

    if(avl_insert(__variables, (void*)&vars[i]))
        return FAILED;

    ++i;

    vn = var_normal_create("var");
    if(vn == NULL)
        return FAILED;

    var = variable_create(VAR_NORMAL, (void*)vn);
    if(var == NULL)
        return FAILED;

    val = value_create(VARIABLE, (void*)var);
    if(val == NULL)
        return FAILED;

    for(i = 0; i < FORS; ++i)
    {
        tfor[i] = token_for_create(tokens_id.for_inc, val, val, val);
        if(tfor[i] == NULL)
            return FAILED;

        guards[i] = token_guard_create(tokens_id.end_for);
        if(guards[i] == NULL)
            return FAILED;
    }

    /*  FOR
            FOR
            ENDFOR
        ENDFOR

        FOR
        ENDFOR
    */

    __tokens = arraylist_create(sizeof(Token*));
    if(__tokens == NULL)
        return FAILED;

    i = 0;

    token[i] = token_create(TOKEN_FOR, (void*)tfor[0]);
    if(token[i] == NULL)
        return FAILED;

    if(arraylist_insert_last(__tokens, (void*)&token[i]))
        return FAILED;

    ++i;

    token[i] = token_create(TOKEN_FOR, (void*)tfor[1]);
    if(token[i] == NULL)
        return FAILED;

    if(arraylist_insert_last(__tokens, (void*)&token[i]))
        return FAILED;

    ++i;

    token[i] = token_create(TOKEN_GUARD, (void*)guards[0]);
    if(token[i] == NULL)
        return FAILED;

    if(arraylist_insert_last(__tokens, (void*)&token[i]))
        return FAILED;

    ++i;

    token[i] = token_create(TOKEN_GUARD, (void*)guards[1]);
    if(token[i] == NULL)
        return FAILED;

    if(arraylist_insert_last(__tokens, (void*)&token[i]))
        return FAILED;

    ++i;

    token[i] = token_create(TOKEN_FOR, (void*)tfor[2]);
    if(token[i] == NULL)
        return FAILED;

    if(arraylist_insert_last(__tokens, (void*)&token[i]))
        return FAILED;

    ++i;

    token[i] = token_create(TOKEN_GUARD, (void*)guards[2]);
    if(token[i] == NULL)
        return FAILED;

    if(arraylist_insert_last(__tokens, (void*)&token[i]))
        return FAILED;

    board_init();

    /*
        SUMMARY:

        VARS = 12
        ARR = 5  SIZE = ARRAY_MAX_LEN / 10 + ARRAY_MAX_LEN / 3 + ARRAY_MAX_LEN / 2
                        + ARRAY_MAX_LEN - 1 + ARRAY_MAX_LEN / 5
        BIG = 3 SIZE = 4 * MAX + 100.010.000.000
    */
    if( prepare_mem_sections(__variables, __tokens) )
        return FAILED;

    /*
        OLD CONFIG
        if(memory->var_first_addr != 0)
            return FAILED;

        if(memory->var_last_addr != 11)
            return FAILED;

        if(memory->loop_var_first_addr != 12)
            return FAILED;

        if(memory->loop_var_last_addr != 15)
            return FAILED;

    */

    if(memory->loop_var_first_addr != 0)
        return FAILED;

    if(memory->loop_var_last_addr != 3)
        return FAILED;

    if(memory->var_first_addr != 4)
        return FAILED;

    if(memory->var_last_addr != 15)
        return FAILED;

    if(memory->arrays_first_addr != 16)
        return FAILED;

    if(memory->arrays_last_addr != (  ARRAY_MAX_LEN / 10 + ARRAY_MAX_LEN / 3
                                    + ARRAY_MAX_LEN / 2  + ARRAY_MAX_LEN - 1
                                    + ARRAY_MAX_LEN / 5) + 15 )
        return FAILED;

    if(mpz_cmp_ui(memory->big_arrays_first_addr,
                            (  ARRAY_MAX_LEN / 10 + ARRAY_MAX_LEN / 3
                             + ARRAY_MAX_LEN / 2  + ARRAY_MAX_LEN - 1
                             + ARRAY_MAX_LEN / 5) + 16) )
        return FAILED;

    /* alloc iterator */
    for(i = 0; i < ITERATORS; ++i)
    {
        vn_it[i] = var_normal_create("it");
        if(vn_it[i] == NULL)
            return FAILED;

        var_it[i] = variable_create(VAR_NORMAL, (void*)vn_it[i]);
        if(var_it[i] == NULL)
            return FAILED;

        val_it[i] = value_create(VARIABLE, (void*)var_it[i]);
        if(val_it[i] == NULL)
            return FAILED;
    }

    /* alloc 2x iterator in memory */
    my_malloc(memory, LOOP_VAR, (void*)val_it[0]);
    my_malloc(memory, LOOP_VAR, (void*)val_it[1]);

    if(memory->loop_var_allocated != 2)
        return FAILED;

    free_loop_section(memory);

    if(memory->loop_var_allocated != 0)
        return FAILED;

    my_malloc(memory, LOOP_VAR, (void*)val_it[2]);

    if(memory->loop_var_allocated != 1)
        return FAILED;

    board_destroy();

    for(i = 0; i < VARS; ++i)
        pvar_destroy(vars[i]);

    for(i = 0; i < TOKENS; ++i)
        FREE(token[i]);

    for(i = 0; i < FORS; ++i)
    {
        FREE(tfor[i]);
        token_guard_destroy(guards[i]);
    }

    value_destroy(val);

    avl_destroy(__variables);
    arraylist_destroy(__tokens);

    return PASSED;

#undef N
}

#define CREATE_VAR(i) \
    do{ \
        vn[i] = var_normal_create(names[i]); \
        if(vn[i] == NULL) \
            return FAILED; \
        \
        vars[i] = variable_create(VAR_NORMAL, (void*)vn[i]); \
        if(vars[i] == NULL) \
            return FAILED; \
        \
        vals[i] = value_create(VARIABLE, (void*)vars[i]); \
        if(vals[i] == NULL) \
            return FAILED; \
    }while(0);

#define SET_VAR_IN_REG(REG, VAL) \
    do{ \
        cpu->registers[REG]->val = vals[VAL]; \
        REG_SET_BUSY(cpu->registers[REG]); \
        vals[VAL]->reg = cpu->registers[REG]; \
    }while(0)

static int test_regs_managment1(void)
{

#define NUM_VARS    10
#define NUM_TOKENS  4

    /*
        SCENARIO:

        REGS:
        ADDR | a | b | f | g

        cur token:
        a := b + c;

        next tokens:
        b := f * d;
        WRITE g;
        a := e % c;
    */

    Token *tokens[NUM_TOKENS];
    Arraylist *list;

    token_assign    *ass1;
    token_assign    *ass2;
    token_assign    *ass3;

    token_expr      *expr1;
    token_expr      *expr2;
    token_expr      *expr3;

    token_io        *io;

    char *names[] = {"a" , "b", "c", "b", "f", "d", "g", "a", "e", "c"};
    var_normal *vn[NUM_VARS];
    Variable *vars[NUM_VARS];
    Value *vals[NUM_VARS];

    int i;
    int j = 0;
    int reg;

    /* Create variables */
    for(i = 0; i < ARRAY_SIZE(names); ++i)
        CREATE_VAR(i);

    i = 0;

    /* create tokens */
    list = arraylist_create(sizeof(Token*));
    if(list == NULL)
        return FAILED;

    expr1 = token_expr_create(tokens_id.add, vals[i + 1], vals[i + 2]);
    if(expr1 == NULL)
        return FAILED;

    ass1 = token_assign_create(vals[i], expr1);
    if(ass1 == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_ASSIGN, (void*)ass1);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    i += 3;
    ++j;

    expr2 = token_expr_create(tokens_id.mult, vals[i + 1], vals[i + 2]);
    if(expr2 == NULL)
        return FAILED;

    ass2 = token_assign_create(vals[i], expr2);
    if(ass2 == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_ASSIGN, (void*)ass2);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    i += 3;
    ++j;

    io = token_io_create(tokens_id.write, vals[i]);
    if(io == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_IO, (void*)io);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    ++i;
    ++j;

    expr3 = token_expr_create(tokens_id.mod, vals[i + 1], vals[i + 2]);
    if(expr3 == NULL)
        return FAILED;

    ass3 = token_assign_create(vals[i], expr3);
    if(ass3 == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_ASSIGN, (void*)ass3);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    i += 3;
    ++j;

    /*  init board */
    board_init();

    /*
        REGS
        ADDR | a | b | f | g

    */

    SET_VAR_IN_REG(1, 0);
    SET_VAR_IN_REG(2, 1);
    SET_VAR_IN_REG(3, 4);
    SET_VAR_IN_REG(4, 6);

    /*
        a = b + c, we use b and c

        CUR REGS:
        ADDR | a | b | f | g

        EXPECTED REGS:
        ADDR | c | b | f | g
    */

    REG_SET_IN_USE(cpu->registers[2]);

    /* get register for b, expected 2 */
    reg = get_register(list, 1, vals[1]);
    if(reg != 2)
        return FAILED;

    /* get register for c, expected 1 */
    reg = get_register(list, 1, vals[2]);
    if(reg != 1)
        return FAILED;

    /*
        b = f * d

        CUR REGS:
        ADDR | c | b | f | g

        EXPECTED REGS:
        ADDR | d | b | f | g

    */

    SET_VAR_IN_REG(1, 2);
    SET_VAR_IN_REG(2, 3);

    REG_SET_IN_USE(cpu->registers[2]);
    REG_SET_IN_USE(cpu->registers[3]);

    reg = get_register(list, 2, vals[3]);
    if(reg != 2)
        return FAILED;

    reg = get_register(list, 2, vals[4]);
    if(reg != 3)
        return FAILED;

    reg = get_register(list, 2, vals[5]);
    if(reg != 1)
        return FAILED;

    /*
        WRITE g

        CUR REGS:
        ADDR | d | b | f | g

        EXPECTED REGS:
        ADDR | d | b | f | g
    */

    SET_VAR_IN_REG(1, 5);

    REG_SET_IN_USE(cpu->registers[4]);

    reg = get_register(list, 3, vals[6]);
    if(reg != 4)
        return FAILED;

    /*
        b = f * d

        CUR REGS:
        ADDR | d | b | f | g

        EXPECTED REGS:
        ADDR | a | e | c | g
    */

    for(i = 0; i < REGS_NUMBER; ++i)
        REG_SET_BUSY(cpu->registers[i]);

    reg = get_register(list, 3, vals[7]);
    if(reg != 1)
        return FAILED;

    REG_SET_IN_USE(cpu->registers[1]);

    reg = get_register(list, 3, vals[8]);
    if(reg != 2)
        return FAILED;

    REG_SET_IN_USE(cpu->registers[2]);

    reg = get_register(list, 3, vals[9]);
    if(reg != 3)
        return FAILED;

    /* clean up */
    for(i = 0; i < NUM_TOKENS; ++i)
        token_destroy(tokens[i]);

    arraylist_destroy(list);
    board_destroy();

    return PASSED;
#undef NUM_VARS
#undef NUM_TOKENS
}

static int test_regs_managment2(void)
{
#define NUM_TOKENS  8
#define NUM_VARS    16

    /*
        SCENARIO:

        REGS:
        ADDR | - | - | - | -

        cur token:
        a := b / d

        next tokens:
        FOR i FROM b TO c DO
            READ d
            FOR j FROM d DOWNTO i DO
                f = b - e
            ENDFOR
        ENDFOR

        WRITE d
    */

    Token *tokens[NUM_TOKENS];
    Arraylist *list;

    token_assign    *ass1;
    token_assign    *ass2;

    token_expr      *expr1;
    token_expr      *expr2;

    token_io        *io1;
    token_io        *io2;

    token_for       *for1;
    token_for       *for2;

    token_guard     *end1;
    token_guard     *end2;

    char *names[] = {"a" , "b", "c", "i", "b", "c", "d", "j", "d", "i", "f", "b", "e", "d", "IC", "JI"};

    var_normal *vn[NUM_VARS];
    Variable *vars[NUM_VARS];
    Value *vals[NUM_VARS];

    int i;
    int j = 0;
    int reg;

    /* Create variables */
    for(i = 0; i < ARRAY_SIZE(names); ++i)
        CREATE_VAR(i);

    i = 0;

    /* create tokens */
    list = arraylist_create(sizeof(Token*));
    if(list == NULL)
        return FAILED;

    expr1 = token_expr_create(tokens_id.div, vals[i + 1], vals[i + 2]);
    if(expr1 == NULL)
        return FAILED;

    ass1 = token_assign_create(vals[i], expr1);
    if(ass1 == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_ASSIGN, (void*)ass1);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    ++j;
    i += 3;

    for1 = token_for_create(tokens_id.for_inc, vals[i], vals[i + 1], vals[i + 2]);
    if(for1 ==  NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_FOR, (void*)for1);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    ++j;
    i += 3;

    io1 = token_io_create(tokens_id.read, vals[i]);
    if(io1 == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_IO, (void*)io1);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    ++i;
    ++j;

    for2 = token_for_create(tokens_id.for_dec, vals[i], vals[i + 1], vals[i + 2]);
    if(for2 ==  NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_FOR, (void*)for2);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    i += 3;
    ++j;

    expr2 = token_expr_create(tokens_id.sub, vals[i + 1], vals[i + 2]);
    if(expr2 == NULL)
        return FAILED;

    ass2 = token_assign_create(vals[i], expr2);
    if(ass2 == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_ASSIGN, (void*)ass2);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    ++j;
    i += 3;

    end1 = token_guard_create(tokens_id.end_for);
    if(end1 == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_GUARD, (void*)end1);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    ++j;

    end2 = token_guard_create(tokens_id.end_for);
    if(end2 == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_GUARD, (void*)end2);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    ++j;

    io2 = token_io_create(tokens_id.write, vals[i]);
    if(io2 == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_IO, (void*)io2);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    board_init();

    /*
        REGS
        ADDR | - | - | - | -

    */

    /*
        a = b / c, we use a b and c

        CUR REGS:
        ADDR | - | - | - | -

        EXPECTED REGS:
        ADDR | a | b | c | -
    */

    /* get register for a, expected 1 */
    reg = get_register(list, 1, vals[0]);
    if(reg != 1)
        return FAILED;

    REG_SET_IN_USE(cpu->registers[1]);

    /* get register for b, expected 2 */
    reg = get_register(list, 1, vals[1]);
    if(reg != 2)
        return FAILED;

    REG_SET_IN_USE(cpu->registers[2]);

    /* get register for c, expected 3 */
    reg = get_register(list, 1, vals[2]);
    if(reg != 3)
        return FAILED;

    REG_SET_IN_USE(cpu->registers[3]);

    /*
        FOR i FROM b TO c DO, we use i and c and IC

        CUR REGS:
        ADDR | a | b | c | -

        EXPECTED REGS:
        ADDR | IC | b | c | i
    */

    SET_VAR_IN_REG(1, 0);
    SET_VAR_IN_REG(2, 1);
    SET_VAR_IN_REG(3, 2);

    REG_SET_IN_USE(cpu->registers[3]);

    /* get register for i, expected 4 */
    reg = get_register(list, 2, vals[3]);
    if(reg != 4)
        return FAILED;

    SET_VAR_IN_REG(4, 3);
    SET_VAR_IN_REG(3, 5);
    REG_SET_IN_USE(cpu->registers[4]);

    /* get register for c, expected 3 */
    reg = get_register(list, 2, vals[5]);
    if(reg != 3)
        return FAILED;

    /* get register for IC, expected 1 */
    reg = get_register(list, 2, vals[14]);
    if(reg != 1)
        return FAILED;

    SET_VAR_IN_REG(1, 14);

    /*
        READ d, we use d

        CUR REGS:
        ADDR | IC | b | c | i

        EXPECTED REGS:
        ADDR | d | b | c | i
    */

    /* get register for d, expected 1 */
    reg = get_register(list, 3, vals[6]);
    if(reg != 1)
        return FAILED;

    SET_VAR_IN_REG(1, 6);

    /*
        FOR j FROM d TO i DO, we use j and i and JI

        CUR REGS:
        ADDR | d | b | c | i

        EXPECTED REGS:
        ADDR | JI | b | j | i
    */

    SET_VAR_IN_REG(4, 9);
    REG_SET_IN_USE(cpu->registers[4]);

    /* get register for j, expected 3 */
    reg = get_register(list, 4, vals[7]);
    if(reg != 3)
        return FAILED;

    SET_VAR_IN_REG(3, 7);
    REG_SET_IN_USE(cpu->registers[3]);

    /* get register for i, expected 4 */
    reg = get_register(list, 4, vals[9]);
    if(reg != 4)
        return FAILED;

    REG_SET_IN_USE(cpu->registers[1]);
    SET_VAR_IN_REG(1, 6);

    /* get register for JI, expected 3 */
    reg = get_register(list, 4, vals[15]);
    if(reg != 1)
        return FAILED;

    /*
        f = b - e

        CUR REGS:
        ADDR | JI | b | j | i

        EXPECTED REGS:
        ADDR | JI | b | j | i
    */
    SET_VAR_IN_REG(2, 11);
    REG_SET_IN_USE(cpu->registers[2]);

    /* get register for b, expected 2 */
    reg = get_register(list, 5, vals[11]);
    if(reg != 2)
        return FAILED;

    /* end for JI */
    REG_SET_FREE(cpu->registers[1]);
    REG_SET_FREE(cpu->registers[3]);

    /* END for IC */
    REG_SET_FREE(cpu->registers[4]);

    /*
        WRITE d

        CUR REGS:
        ADDR | - | b | - | -

        EXPECTED REGS:
        ADDR | d | b | - | -
    */

    /* get register for b, expected 2 */
    reg = get_register(list, 7, vals[13]);
    if(reg != 1)
        return FAILED;


    /* clean up */
    for(i = 0; i < NUM_TOKENS; ++i)
        token_destroy(tokens[i]);

    value_destroy(vals[14]);
    value_destroy(vals[15]);

    arraylist_destroy(list);
    board_destroy();

    return PASSED;

#undef NUM_TOKENS
#undef NUM_VARS
}

static int test_regs_managment3(void)
{
#define NUM_TOKENS  10
#define NUM_VARS    13

    /*
        SCENARIO:

        REGS:
        ADDR | - | - | - | -

        cur token:
        a := b * c

        next tokens:
        IF a != d THEN
            WHILE a < e DO
                READ f
                c := d - e
            ENDWHILE
        ELSE
            WRITE a
        ENDIF
    */

    Token *tokens[NUM_TOKENS];
    Arraylist *list;

    token_assign    *ass1;
    token_assign    *ass2;

    token_expr      *expr1;
    token_expr      *expr2;

    token_io        *io1;
    token_io        *io2;
    token_io        *io3;

    token_cond      *cond1;
    token_cond      *cond2;

    token_while     *while1;
    token_if        *if1;

    token_guard     *endwhile;
    token_guard     *else1;
    token_guard     *endif;

    char *names[] = {"a", "b", "c", "a", "d", "a", "e", "f", "c", "d", "e", "e", "a"};

    var_normal *vn[NUM_VARS];
    Variable *vars[NUM_VARS];
    Value *vals[NUM_VARS];
    int i;
    int j = 0;
    int reg;

    /* Create variables */
    for(i = 0; i < ARRAY_SIZE(names); ++i)
        CREATE_VAR(i);

    i = 0;

    /* create tokens */
    list = arraylist_create(sizeof(Token*));
    if(list == NULL)
        return FAILED;

    expr1 = token_expr_create(tokens_id.mult, vals[i + 1], vals[i + 2]);
    if(expr1 == NULL)
        return FAILED;

    ass1 = token_assign_create(vals[i], expr1);
    if(ass1 == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_ASSIGN, (void*)ass1);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    ++j;
    i += 3;

    cond1 = token_cond_create(tokens_id.ne, vals[i], vals[i + 1]);
    if(cond1 == NULL)
        return FAILED;

    if1 = token_if_create(cond1);
    if(if1 == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_IF, (void*)if1);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    ++j;
    i += 2;

    cond2 = token_cond_create(tokens_id.lt, vals[i], vals[i + 1]);
    if(cond2 == NULL)
        return FAILED;

    while1 = token_while_create(cond2);
    if(while1 == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_WHILE, (void*)while1);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    ++j;
    i += 2;

    io1 = token_io_create(tokens_id.read, vals[i]);
    if(io1 == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_IO, (void*)io1);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    ++i;
    ++j;

    expr2 = token_expr_create(tokens_id.sub, vals[i + 1], vals[i + 2]);
    if(expr2 == NULL)
        return FAILED;

    ass2 = token_assign_create(vals[i], expr2);
    if(ass2 == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_ASSIGN, (void*)ass2);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    ++j;
    i += 3;

    endwhile = token_guard_create(tokens_id.end_while);
    if(endwhile == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_GUARD, (void*)endwhile);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    ++j;

    io2 = token_io_create(tokens_id.write, vals[i]);
    if(io2 == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_IO, (void*)io2);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    ++i;
    ++j;

    else1 = token_guard_create(tokens_id.else_cond);
    if(else1 == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_GUARD, (void*)else1);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    ++j;

    io3 = token_io_create(tokens_id.write, vals[i]);
    if(io3 == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_IO, (void*)io3);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    ++i;
    ++j;

    endif = token_guard_create(tokens_id.end_if);
    if(endif == NULL)
        return FAILED;

    tokens[j] = token_create(TOKEN_GUARD, (void*)endif);
    if(tokens[j] == NULL)
        return FAILED;

    if(arraylist_insert_last(list, (void*)&tokens[j]))
        return FAILED;

    ++j;

    board_init();

    /*
        REGS
        ADDR | - | - | - | -

    */

    /*
        a = b * c, we use a b and c

        CUR REGS:
        ADDR | - | - | - | -

        EXPECTED REGS:
        ADDR | a | b | c | -
    */

    /* get register for a, expected 1 */
    reg = get_register(list, 1, vals[0]);
    if(reg != 1)
        return FAILED;

    REG_SET_IN_USE(cpu->registers[1]);

    /* get register for b, expected 2 */
    reg = get_register(list, 1, vals[1]);
    if(reg != 2)
        return FAILED;

    REG_SET_IN_USE(cpu->registers[2]);

    /* get register for c, expected 3 */
    reg = get_register(list, 1, vals[2]);
    if(reg != 3)
        return FAILED;

    REG_SET_IN_USE(cpu->registers[3]);

    /*
        if a != d, we use a d

        CUR REGS:
        ADDR | a | b | c | -

        EXPECTED REGS:
        ADDR | a | b | c | d
    */

    SET_VAR_IN_REG(2, 1);
    SET_VAR_IN_REG(3, 2);
    SET_VAR_IN_REG(1, 3);

    REG_SET_IN_USE(cpu->registers[1]);

    /* get register for a, expected 1 */
    reg = get_register(list, 2, vals[3]);
    if(reg != 1)
        return FAILED;

    /* get register for d, expected 4 */
    reg = get_register(list, 2, vals[4]);
    if(reg != 4)
        return FAILED;

    SET_VAR_IN_REG(4, 4);

    /*
        while a < e, we use a e

        CUR REGS:
        ADDR | a | b | c | d

        EXPECTED REGS:
        ADDR | a | e | c | d
    */
    SET_VAR_IN_REG(1, 5);
    REG_SET_IN_USE(cpu->registers[1]);

    /* get register for a, expected 1 */
    reg = get_register(list, 3, vals[5]);
    if(reg != 1)
        return FAILED;

    REG_SET_IN_USE(cpu->registers[1]);

    /* get register for e, expected 2 */
    reg = get_register(list, 3, vals[6]);
    if(reg != 2)
        return FAILED;

    SET_VAR_IN_REG(3, 5);
    SET_VAR_IN_REG(2, 6);

    /*
        READ f, we use f

        CUR REGS:
        ADDR | a | e | c | d

        EXPECTED REGS:
        ADDR | a | f | c | d
    */

    /* get register for f, expected 2 */
    reg = get_register(list, 4, vals[7]);
    if(reg != 2)
        return FAILED;

    SET_VAR_IN_REG(2, 7);

    /*
        c = d - e, we use d

        CUR REGS:
        ADDR | a | f | c | d

        EXPECTED REGS:
        ADDR | a | f | c | d
    */

    SET_VAR_IN_REG(4, 9);

    /* get register for d, expected 4 */
    reg = get_register(list, 5, vals[9]);
    if(reg != 4)
        return FAILED;

    /*
        WRITE e we use e

        CUR REGS:
        ADDR | a | f | c | d

        EXPECTED REGS:
        ADDR | e | f | c | d
    */
    SET_VAR_IN_REG(1, 5);

    /* get register for e, expected 1 */
    reg = get_register(list, 7, vals[11]);
    if(reg != 1)
        return FAILED;

    /*
        WRITE a we use a

        CUR REGS:
        ADDR | a | b | c | d

        EXPECTED REGS:
        ADDR | a | b | c | d
    */

    /* state before IF */
    SET_VAR_IN_REG(1, 12);
    SET_VAR_IN_REG(2, 1);
    SET_VAR_IN_REG(3, 2);
    SET_VAR_IN_REG(4, 4);

    /* get register for a, expected 1 */
    reg = get_register(list, 9, vals[12]);
    if(reg != 1)
        return FAILED;

    /* clean up */
    for(i = 0; i < NUM_TOKENS; ++i)
        token_destroy(tokens[i]);

    arraylist_destroy(list);
    board_destroy();

    return PASSED;

#undef NUM_TOKENS
#undef NUM_VARS
}

#undef CREATE_VAR
#undef SET_VAR_IN_REG

static int test_gramma_errors(void)
{
    int err = 0;

    /*
        err1

        ERROR!  syntax error    =
        LINE: 19
        a = c / d;
    */
    fprintf(stderr,"\rEX1");
    err += !!system(  COMP_EXEC " --input ./tests/gramma_errors/err1 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\ssyntax\\serror\\s=\\s"
                    "LINE:\\s19\\sa = c / d;\"");
    /*
        err2

        ERROR!  syntax error    BEGIN
        LINE: 7
        BEGIN
    */
    fprintf(stderr,"\rEX2");
    err += !!system(  COMP_EXEC " --input ./tests/gramma_errors/err2 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\ssyntax\\serror\\sBEGIN\\s"
                    "LINE:\\s7\\sBEGIN\"");

    /*
        err3

        ERROR!  syntax error    END
        LINE: 17
        END
    */
    fprintf(stderr,"\rEX3");
    err += !!system(  COMP_EXEC " --input ./tests/gramma_errors/err3 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\ssyntax\\serror\\sEND\\s"
                    "LINE:\\s17\\sEND\"");

    /*
        err4

        ERROR!  syntax error    END
        LINE: 11
        END
    */
    fprintf(stderr,"\rEX4");
    err += !!system(  COMP_EXEC " --input ./tests/gramma_errors/err4 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\ssyntax\\serror\\sEND\\s"
                    "LINE:\\s11\\sEND\"");

    /*
        err5

        ERROR!  syntax error    +
        LINE: 17
        a := ++b;
    */
    fprintf(stderr,"\rEX5");
    err += !!system(  COMP_EXEC " --input ./tests/gramma_errors/err5 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\ssyntax\\serror\\s\\+\\s"
                    "LINE:\\s17\\sa := \\+\\+b;\"");
    /*
        err6

        ERROR!  syntax error    ma
        LINE: 20
        kukos ma Art of Programming
    */
    fprintf(stderr,"\rEX6");
    err += !!system(  COMP_EXEC " --input ./tests/gramma_errors/err6 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\ssyntax\\serror\\sma\\s"
                    "LINE:\\s20\\skukos ma Art of Programming\"");

    /*
        err7

        ERROR!  syntax error    10
        LINE: 17
        READ 10
    */
    fprintf(stderr,"\rEX7");
    err += !!system(  COMP_EXEC " --input ./tests/gramma_errors/err7 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\ssyntax\\serror\\s10\\s"
                    "LINE:\\s17\\sREAD 10\"");

    /*
        err8
        ERROR!  syntax error    10
        LINE: 21
        FOR 10 FROM 10 TO 20 DO
    */
    fprintf(stderr,"\rEX8");
    err += !!system(  COMP_EXEC " --input ./tests/gramma_errors/err8 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\ssyntax\\serror\\s10\\s"
                    "LINE:\\s21\\sFOR 10 FROM 10 TO 20 DO\"");

    /*
        err9

        ERROR!  syntax error    *
        LINE: 18
        FOR i FROM a TO b * c DO
    */
    fprintf(stderr,"\rEX9");
    err += !!system(  COMP_EXEC " --input ./tests/gramma_errors/err9 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\ssyntax\\serror\\s\\*\\s"
                    "LINE:\\s18\\sFOR i FROM a TO b \\* c DO\"");

    /*
        err10

        ERROR!  syntax error    DO
        LINE: 15
        FOR a FROM 10 TO DO
    */
    fprintf(stderr,"\rEX10");
    err += !!system(  COMP_EXEC " --input ./tests/gramma_errors/err10 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\ssyntax\\serror\\sDO\\s"
                    "LINE:\\s15\\sFOR a FROM 10 TO DO\"");

    /*
        err11

        ERROR!  syntax error    +
        LINE: 17
        IF a < b + c THEN
    */
    fprintf(stderr,"\rEX11");
    err += !!system(  COMP_EXEC " --input ./tests/gramma_errors/err11 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\ssyntax\\serror\\s\\+\\s"
                    "LINE:\\s17\\sIF a < b \\+ c THEN\"");

    /*
        err12

        ERROR!  syntax error    :=
        LINE: 16
        IF a := b THEN
    */
    fprintf(stderr,"\rEX12");
    err += !!system(  COMP_EXEC " --input ./tests/gramma_errors/err12 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\ssyntax\\serror\\s:=\\s"
                    "LINE:\\s16\\sIF a := b THEN\"");
    /*
        err13

        ERROR!  syntax error    ELSE
        LINE: 39
        ELSE
    */
    fprintf(stderr,"\rEX13");
    err += !!system(  COMP_EXEC " --input ./tests/gramma_errors/err13 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\ssyntax\\serror\\sELSE\\s"
                    "LINE:\\s39\\s\\s\\s\\s\\sELSE\"");

    /*
        err14

        ERROR!  syntax error    /
        LINE: 27
        WHILE a < b % d DO
    */
    fprintf(stderr,"\rEX14");
    err += !!system(  COMP_EXEC " --input ./tests/gramma_errors/err14 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\ssyntax\\serror\\s\\/\\s"
                    "LINE:\\s27\\sWHILE a < b / d DO\"");

    /*
        err15

        ERROR!  syntax error    A
        LINE: 7
        VARABEGINEND
    */
    fprintf(stderr,"\rEX15");
    err += !!system(  COMP_EXEC " --input ./tests/gramma_errors/err15 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\ssyntax\\serror\\sA\\s"
                    "LINE:\\s7\\sVARABEGINEND\"");

    err += !!system("rm -f ./tests/fake");

    fprintf(stderr,"\r");
    return err ? FAILED : PASSED;
}

static int test_semantic_errors(void)
{
    int err = 0;

    /*
        err1

        ERROR!
        redeclaration:   b
        LINE: 13        art_of_programming b
    */
    fprintf(stderr,"\rEX1");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err1 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\sredeclaration:\\sb\\s"
                    "LINE:\\s13\\s\\sart_of_programming b\"");

    /*
        err2

        ERROR!
        variable undeclared:  d
        LINE: 13                a := d + 10000;
    */
    fprintf(stderr,"\rEX2");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err2 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\svariable undeclared:\\sd\\s"
                    "LINE:\\s13\\s\\sa := d \\+ 10000;\"");

    /*
        err3

        ERROR!
        uninitialized variable: a
        LINE: 13                IF a < b THEN
    */
    fprintf(stderr,"\rEX3");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err3 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\suninitialized variable:\\sa\\s"
                    "LINE:\\s13\\s\\sIF a < b THEN\"");

    /*
        err4

        ERROR!
        uninitialized variable: a
        LINE: 13                WHILE a <> b DO
    */
    fprintf(stderr,"\rEX4");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err4 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\suninitialized variable:\\sa\\s"
                    "LINE:\\s13\\s\\sWHILE a <> b DO\"");

    /*
        err5

        ERROR!
        uninitialized variable: a
        LINE: 13                FOR i FROM a TO b DO
    */
    fprintf(stderr,"\rEX5");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err5 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\suninitialized variable:\\sa\\s"
                    "LINE:\\s13\\s\\sFOR i FROM a TO b DO\"");

    /*
        err6

        ERROR!
        uninitialized variable: a
        LINE: 14                    WRITE a;
    */
    fprintf(stderr,"\rEX6");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err6 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\suninitialized variable:\\sa\\s"
                    "LINE:\\s14\\s\\s\\s\\s\\s\\sWRITE a\"");

    /*
        err7

        ERROR!
        wrong use, var is an array:     c
        LINE: 13         c := a + b;
    */
    fprintf(stderr,"\rEX7");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err7 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\swrong use, var is an array:\\sc\\s"
                    "LINE:\\s13\\s\\sc := a \\+ b;\"");

    /*
        err8

        ERROR!
        wrong use, var is an array:     c
        LINE: 13         a := 10 + c;
    */
    fprintf(stderr,"\rEX8");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err8 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\swrong use, var is an array:\\sc\\s"
                    "LINE:\\s13\\s\\sa := 10 \\+ c;\"");

    /*
        err9

        ERROR!
        wrong use, var is an array:     a
        LINE: 13         READ a;
    */
    fprintf(stderr,"\rEX9");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err9 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\swrong use, var is an array:\\sa\\s"
                    "LINE:\\s13\\s\\sREAD a;\"");

    /*
        err10

        ERROR!
        wrong use, var is an array:     a
        LINE: 13         WRITE a;
    */
    fprintf(stderr,"\rEX10");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err10 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\swrong use, var is an array:\\sa\\s"
                    "LINE:\\s13\\s\\sWRITE a;\"");

    /*
        err11

        ERROR!
        wrong use, var is an array:     a
        LINE: 13         IF a < 10 THEN
    */
    fprintf(stderr,"\rEX11");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err11 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\swrong use, var is an array:\\sa\\s"
                    "LINE:\\s13\\s\\sIF a < 10 THEN\"");

    /*
        err12

        ERROR!
        wrong use, var is an array:     a
        LINE: 13         WHILE a < 10 DO
    */
    fprintf(stderr,"\rEX12");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err12 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\swrong use, var is an array:\\sa\\s"
                    "LINE:\\s13\\s\\sWHILE a < 10 DO\"");

    /*
        err13

        ERROR!
        wrong use, var is an array:     a
        LINE: 13         FOR i FROM a TO a[1] DO
    */
    fprintf(stderr,"\rEX13");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err13 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\swrong use, var is an array:\\sa\\s"
                    "LINE:\\s13\\s\\sFOR i FROM a TO a\\[1\\] DO\"");

    /*
        err14

        ERROR!
        wrong use, var is not an array: c
        LINE: 13                c[1] := a + b;
    */
    fprintf(stderr,"\rEX14");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err14 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\swrong use, var is not an array:\\sc\\s"
                    "LINE:\\s13\\s\\sc\\[1\\]\\s:= a \\+ b;\"");

    /*
        err15

        ERROR!
        wrong use, var is not an array: c
        LINE: 13                a := 10 + c[1000];
    */
    fprintf(stderr,"\rEX15");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err15 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\swrong use, var is not an array:\\sc\\s"
                    "LINE:\\s13\\s\\sa := 10 \\+ c\\[1000\\];\"");

    /*
        err16

        ERROR!
        wrong use, var is not an array: c
        LINE: 13                READ c[2];
    */
    fprintf(stderr,"\rEX16");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err16 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\swrong use, var is not an array:\\sc\\s"
                    "LINE:\\s13\\s\\sREAD c\\[2\\];\"");

    /*
        err17

        ERROR!
        wrong use, var is not an array: c
        LINE: 13                WRITE c[2];
    */
    fprintf(stderr,"\rEX17");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err17 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\swrong use, var is not an array:\\sc\\s"
                    "LINE:\\s13\\s\\sWRITE c\\[2\\];\"");

    /*
        err18

        ERROR!
        wrong use, var is not an array: c
        LINE: 13                IF c[2] < 10 THEN
    */
    fprintf(stderr,"\rEX18");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err18 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\swrong use, var is not an array:\\sc\\s"
                    "LINE:\\s13\\s\\sIF c\\[2\\] < 10 THEN\"");

    /*
        err19

        ERROR!
        wrong use, var is not an array: c
        LINE: 13                WHILE c[2] < 10 DO
    */
    fprintf(stderr,"\rEX19");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err19 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\swrong use, var is not an array:\\sc\\s"
                    "LINE:\\s13\\s\\sWHILE c\\[2\\] < 10 DO\"");

    /*
        err20

        ERROR!
        wrong use, var is not an array: c
        LINE: 15                FOR i FROM a TO c[2] DO
    */
    fprintf(stderr,"\rEX20");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err20 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\swrong use, var is not an array:\\sc\\s"
                    "LINE:\\s15\\s\\sFOR i FROM a TO c\\[2\\] DO\"");

    /*
        err21

        ERROR!
        array member out of array range:       t[ 10 ]
        LINE: 20                t[10] := 100;
    */
    fprintf(stderr,"\rEX21");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err21 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\sarray member out of array range:\\st\\[ 10 \\]\\s"
                    "LINE:\\s20\\s\\st\\[10\\] := 100;\"");

    /*
        err22

        ERROR!
        array member out of array range:       t[ 11 ]
        LINE: 14                WRITE t[11];
    */
    fprintf(stderr,"\rEX22");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err22 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\sarray member out of array range:\\st\\[ 11 \\]\\s"
                    "LINE:\\s14\\s\\sWRITE t\\[11\\];\"");

    /*
        err23

        ERROR!
        array member out of array range:       t[ 10 ]
        LINE: 13                IF t[0] < t[10] THEN
    */
    fprintf(stderr,"\rEX23");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err23 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\sarray member out of array range:\\st\\[ 10 \\]\\s"
                    "LINE:\\s13\\s\\sIF t\\[0\\] < t\\[10\\] THEN\"");

    /*
        err24

        ERROR!
        array member out of array range:       t[ 10 ]
        LINE: 13                FOR i FROM 1000 DOWNTO t[10] DO
    */
    fprintf(stderr,"\rEX24");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err24 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\sarray member out of array range:\\st\\[ 10 \\]\\s"
                    "LINE:\\s13\\s\\sFOR i FROM 1000 DOWNTO t\\[10\\] DO\"");

    /*
        err25

        ERROR!
        variable undeclared:    i
        LINE: 15                i := i + 1;
    */
    fprintf(stderr,"\rEX25");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err25 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\svariable undeclared:\\si\\s"
                    "LINE:\\s15\\s\\si := i \\+ 1\"");

    /*
        err26

        ERROR!
        iterator is redeclared  i
        LINE: 13         FOR i FROM 1 TO 10 DO
    */
    fprintf(stderr,"\rEX26");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err26 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\siterator is redeclared:\\si\\s"
                    "LINE:\\s13\\s\\sFOR i FROM 1 TO 10 DO\"");

    /*
        err27

        ERROR!
        iterator is redeclared: i
        LINE: 20                       FOR i FROM j TO 10000 DO
    */
    fprintf(stderr,"\rEX27");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err27 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\siterator is redeclared:\\si\\s"
                    "LINE:\\s20\\s\\s\\s\\s\\s\\s\\s\\s\\s\\sFOR i FROM j TO 10000 DO\"");

    /*
        err28

        WARNING!        unused variable:        a
    */
    fprintf(stderr,"\rEX28");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err28 --output ./tests/fake --Wall 2>&1"
                    "| grep -Pzoq \"WARNING\\!\\sunused variable:\\sa\"");

    /*
        err29

        ERROR!        unused variable:        a
    */
    fprintf(stderr,"\rEX29");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err29 --output ./tests/fake --Wall -Werr 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\sunused variable:\\sa\"");

    /*
        err30

        ERROR!
        iterator overwrite:     j
        LINE: 19                        j := j + 1;
    */
    fprintf(stderr,"\rEX30");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err30 --output ./tests/fake --Wall -Werr 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\siterator overwrite:\\sj\\s"
                    "LINE:\\s19\\s\\s\\s\\s\\s\\s\\s\\s\\s\\sj := j \\+ 1;\"");

    /*
        err31

        ERROR!
        iterator overwrite:     i
        LINE: 20                                READ i;
    */
    fprintf(stderr,"\rEX31");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err31 --output ./tests/fake --Wall -Werr 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\siterator overwrite:\\si\\s"
                    "LINE:\\s20\\s\\s\\s\\s\\s\\s\\s\\s\\s\\s\\s\\s\\s\\s\\s\\s\\s\\sREAD i;\"");


    /*
        err32

        ERROR!
        array with size 0:      a
        LINE: 9         a[0]
    */
    fprintf(stderr,"\rEX32");
    err += !!system(  COMP_EXEC " --input ./tests/semantic_errors/err32 --output ./tests/fake 2>&1"
                    "| grep -Pzoq \"ERROR\\!\\sarray with size 0:\\sa\\s"
                    "LINE:\\s9\\s\\sa\\[0\\]\"");

    err += !!system("rm -f ./tests/fake");
    fprintf(stderr,"\r");
    return err ? FAILED : PASSED;
}

static int test_correct_semantic(void)
{
    int err = 0;

    fprintf(stderr,"\rEX1");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex1 --output ./tests/fake >/dev/null 2>&1");

    fprintf(stderr,"\rEX2");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex2 --output ./tests/fake >/dev/null 2>&1");

    fprintf(stderr,"\rEX3");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex3 --output ./tests/fake >/dev/null 2>&1");

    fprintf(stderr,"\rEX4");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex4 --output ./tests/fake >/dev/null 2>&1");

    fprintf(stderr,"\rEX5");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex5 --output ./tests/fake >/dev/null 2>&1");

    fprintf(stderr,"\rEX6");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex6 --output ./tests/fake >/dev/null 2>&1");

    fprintf(stderr,"\rEX7");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex7 --output ./tests/fake >/dev/null 2>&1");

    fprintf(stderr,"\rEX8");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex8 --output ./tests/fake >/dev/null 2>&1");

    fprintf(stderr,"\rEX9");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex9 --output ./tests/fake --Wall --Werror >/dev/null 2>&1");

    fprintf(stderr,"\rEX10");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex10 --output ./tests/fake --Wall --Werror >/dev/null 2>&1");

    fprintf(stderr,"\rEX11");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex11 --output ./tests/fake --Wall --Werror >/dev/null 2>&1");

    fprintf(stderr,"\rEX12");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex12 --output ./tests/fake --Wall --Werror >/dev/null 2>&1");

    fprintf(stderr,"\rEX13");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex13 --output ./tests/fake --Wall --Werror >/dev/null 2>&1");

    fprintf(stderr,"\rEX14");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex14 --output ./tests/fake --Wall --Werror >/dev/null 2>&1");

    fprintf(stderr,"\rEX15");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex15 --output ./tests/fake >/dev/null 2>&1");

    fprintf(stderr,"\rEX16");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex16 --output ./tests/fake --Wall --Werror >/dev/null 2>&1");

    fprintf(stderr,"\rEX17");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex17 --output ./tests/fake --Wall --Werror >/dev/null 2>&1");

    fprintf(stderr,"\rEX18");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex18 --output ./tests/fake --Wall --Werror >/dev/null 2>&1");

    fprintf(stderr,"\rEX19");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex19 --output ./tests/fake --Wall --Werror >/dev/null 2>&1");

    fprintf(stderr,"\rEX20");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex20 --output ./tests/fake --Wall --Werror >/dev/null 2>&1");

    err += !!system("rm -f ./tests/fake");
    fprintf(stderr,"\r");
    return err ? FAILED : PASSED;
}

static int test_correct_tokens(void)
{
    int err = 0;

    fprintf(stderr,"\rEX1");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex1 --output ./tests/correct/tokens/ex1_tokens --tokens >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex1 ./tests/correct/tokens/ex1_tokens >/dev/null");

    fprintf(stderr,"\rEX2");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex2 --output ./tests/correct/tokens/ex2_tokens --tokens >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex2 ./tests/correct/tokens/ex2_tokens >/dev/null");

    fprintf(stderr,"\rEX3");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex3 --output ./tests/correct/tokens/ex3_tokens --tokens >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex3 ./tests/correct/tokens/ex3_tokens >/dev/null");

    fprintf(stderr,"\rEX4");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex4 --output ./tests/correct/tokens/ex4_tokens --tokens >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex4 ./tests/correct/tokens/ex4_tokens >/dev/null");

    fprintf(stderr,"\rEX5");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex5 --output ./tests/correct/tokens/ex5_tokens --tokens >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex5 ./tests/correct/tokens/ex5_tokens >/dev/null");

    fprintf(stderr,"\rEX6");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex6 --output ./tests/correct/tokens/ex6_tokens --tokens >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex6 ./tests/correct/tokens/ex6_tokens >/dev/null");

    fprintf(stderr,"\rEX7");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex7 --output ./tests/correct/tokens/ex7_tokens --tokens >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex7 ./tests/correct/tokens/ex7_tokens >/dev/null");

    fprintf(stderr,"\rEX8");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex8 --output ./tests/correct/tokens/ex8_tokens --tokens --Wall --Werror >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex8 ./tests/correct/tokens/ex8_tokens >/dev/null");

    fprintf(stderr,"\rEX9");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex9 --output ./tests/correct/tokens/ex9_tokens --tokens --Wall --Werror >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex9 ./tests/correct/tokens/ex9_tokens >/dev/null");

    fprintf(stderr,"\rEX10");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex10 --output ./tests/correct/tokens/ex10_tokens --tokens --Wall --Werror >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex10 ./tests/correct/tokens/ex10_tokens >/dev/null");

    fprintf(stderr,"\rEX11");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex11 --output ./tests/correct/tokens/ex11_tokens --tokens --Wall --Werror >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex11 ./tests/correct/tokens/ex11_tokens >/dev/null");

    fprintf(stderr,"\rEX12");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex12 --output ./tests/correct/tokens/ex12_tokens --tokens --Wall --Werror >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex12 ./tests/correct/tokens/ex12_tokens >/dev/null");

    fprintf(stderr,"\rEX13");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex13 --output ./tests/correct/tokens/ex13_tokens --tokens --Wall --Werror >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex13 ./tests/correct/tokens/ex13_tokens >/dev/null");

    fprintf(stderr,"\rEX14");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex14 --output ./tests/correct/tokens/ex14_tokens --tokens --Wall --Werror >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex14 ./tests/correct/tokens/ex14_tokens >/dev/null");

    fprintf(stderr,"\rEX15");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex15 --output ./tests/correct/tokens/ex15_tokens --tokens --Wall --Werror >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex15 ./tests/correct/tokens/ex15_tokens >/dev/null");

    fprintf(stderr,"\rEX16");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex16 --output ./tests/correct/tokens/ex16_tokens --tokens --Wall --Werror >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex16 ./tests/correct/tokens/ex16_tokens >/dev/null");

    fprintf(stderr,"\rEX17");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex17 --output ./tests/correct/tokens/ex17_tokens --tokens --Wall --Werror >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex17 ./tests/correct/tokens/ex17_tokens >/dev/null");

    fprintf(stderr,"\rEX18");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex18 --output ./tests/correct/tokens/ex18_tokens --tokens --Wall --Werror >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex18 ./tests/correct/tokens/ex18_tokens >/dev/null");

    fprintf(stderr,"\rEX19");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex19 --output ./tests/correct/tokens/ex19_tokens --tokens --Wall >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex19 ./tests/correct/tokens/ex19_tokens >/dev/null");

    fprintf(stderr,"\rEX20");
    err += !!system(COMP_EXEC " --input ./tests/correct/ex20 --output ./tests/correct/tokens/ex20_tokens --tokens --Wall >/dev/null 2>&1");
    err += !!system("diff ./tests/correct/tokens/ex20 ./tests/correct/tokens/ex20_tokens >/dev/null");

    err += !!system("rm -rf ./tests/correct/tokens/*_tokens");

    fprintf(stderr,"\r");
    return err ? FAILED : PASSED;
}

static int test_compiled_code(void)
{
    int err = 0;

    /* EX1 */
    fprintf(stderr,"\rEX1");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex1 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in1  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out1 >/dev/null");

    /* EX2 */
    fprintf(stderr,"\rEX2");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex2 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in2  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out2 >/dev/null");

    /* EX3 */
    fprintf(stderr,"\rEX3");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex3 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in3  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out3 >/dev/null");

    /* EX4 */
    fprintf(stderr,"\rEX4");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex4 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in4  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out4 >/dev/null");

    /* EX5 */
    fprintf(stderr,"\rEX5");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex5 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in5  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out5 >/dev/null");

    /* EX6 */
    fprintf(stderr,"\rEX6");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex6 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in6  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out6 >/dev/null");

    /* EX7 */
    fprintf(stderr,"\rEX7");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex7 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in7  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out7 >/dev/null");

    /* EX8 */
    fprintf(stderr,"\rEX8");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex8 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in8  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out8 >/dev/null");

    /* EX9 */
    fprintf(stderr,"\rEX9");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex9 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in9  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out9 >/dev/null");

    /* EX10 */
    fprintf(stderr,"\rEX10");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex10 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in10  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out10 >/dev/null");

    /* EX11 */
    fprintf(stderr,"\rEX11");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex11 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in11  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out11 >/dev/null");

    /* EX12 */
    fprintf(stderr,"\rEX12");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex12 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in12  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out12 >/dev/null");

    /* EX13 */
    fprintf(stderr,"\rEX13");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex13 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in13  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out13 >/dev/null");

    /* EX14 */
    fprintf(stderr,"\rEX14");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex14 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in14  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out14 >/dev/null");

    /* EX15 */
    fprintf(stderr,"\rEX15");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex15 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in15  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out15 >/dev/null");

    /* EX16 */
    fprintf(stderr,"\rEX16");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex16 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in16  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out16 >/dev/null");

    /* EX17 */
    fprintf(stderr,"\rEX17");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex17 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in17  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out17 >/dev/null");

    /* EX18 */
    fprintf(stderr,"\rEX18");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex18 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in18  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out18 >/dev/null");

    /* EX19 */
    fprintf(stderr,"\rEX19");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex19 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in19  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out19 >/dev/null");

    /* EX20 */
    fprintf(stderr,"\rEX20");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex20 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in20  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out20 >/dev/null");

    /* EX21 */
    fprintf(stderr,"\rEX21");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex21 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in21  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out21 >/dev/null");

    /* EX22 */
    fprintf(stderr,"\rEX22");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex22 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in22  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out22 >/dev/null");

    /* EX23 */
    fprintf(stderr,"\rEX23");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex23 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in23  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out23 >/dev/null");

    /* EX24 */
    fprintf(stderr,"\rEX24");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex24 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in24  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out24 >/dev/null");

    /* EX25 */
    fprintf(stderr,"\rEX25");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex25 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in25  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out25 >/dev/null");

    /* EX26 */
    fprintf(stderr,"\rEX26");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex26 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in26  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out26 >/dev/null");

    /* EX27 */
    fprintf(stderr,"\rEX27");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex27 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in27  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out27 >/dev/null");

    /* EX28 */
    fprintf(stderr,"\rEX28");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex28 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in28  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out28 >/dev/null");

    /* EX29 */
    fprintf(stderr,"\rEX29");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex29 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in29  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out29 >/dev/null");

    /* EX30 */
    fprintf(stderr,"\rEX30");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex30 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in30  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out30 >/dev/null");

    /* EX31 */
    fprintf(stderr,"\rEX31");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex31 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in31  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out31 >/dev/null");

    /* EX32 */
    fprintf(stderr,"\rEX32");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex32 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in32  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out32 >/dev/null");

    /* EX33 */
    fprintf(stderr,"\rEX33");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex33 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in33  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out33 >/dev/null");

    /* EX34 */
    fprintf(stderr,"\rEX34");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex34 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in34  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out34 >/dev/null");

    /* EX35 */
    fprintf(stderr,"\rEX35");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex35 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in35  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out35 >/dev/null");

    /* EX36 */
    fprintf(stderr,"\rEX36");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex36 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in36  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out36 >/dev/null");

    /* EX37 */
    fprintf(stderr,"\rEX37");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex37 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in37  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out37 >/dev/null");

    /* EX38 */
    fprintf(stderr,"\rEX38");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex38 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in38  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out38 >/dev/null");

    /* EX39 */
    fprintf(stderr,"\rEX39");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex39 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in39  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out39 >/dev/null");

    /* EX40 */
    fprintf(stderr,"\rEX40");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex40 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in40  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out40 >/dev/null");

    /* EX41 */
    fprintf(stderr,"\rEX41");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex41 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in41  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out41 >/dev/null");

    /* EX42 */
    fprintf(stderr,"\rEX42");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex42 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in42  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out42 >/dev/null");

    /* EX43 */
    fprintf(stderr,"\rEX43");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex43 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in43  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out43 >/dev/null");

    /* EX44 */
    fprintf(stderr,"\rEX44");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex44 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in44  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out44 >/dev/null");

    /* EX45 */
    fprintf(stderr,"\rEX45");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex45 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in45  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out45 >/dev/null");

    /* EX46 */
    fprintf(stderr,"\rEX46");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex46 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in46  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out46 >/dev/null");

    /* EX47 */
    fprintf(stderr,"\rEX47");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex47 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in47  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out47 >/dev/null");

    /* EX48 */
    fprintf(stderr,"\rEX48");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex48 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in48  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out48 >/dev/null");

    /* EX49 */
    fprintf(stderr,"\rEX49");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex49 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in49  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out49 >/dev/null");

    /* EX50 */
    fprintf(stderr,"\rEX50");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex50 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in50  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out50 >/dev/null");

    /* EX51 */
    fprintf(stderr,"\rEX51");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex51 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in51  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out51 >/dev/null");

    /* EX52 */
    fprintf(stderr,"\rEX52");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex52 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in52  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out52 >/dev/null");

    /* EX53 */
    fprintf(stderr,"\rEX53");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex53 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in53  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out53 >/dev/null");

    /* EX54 */
    fprintf(stderr,"\rEX54");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex54 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in54  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out54 >/dev/null");

    /* EX55 */
    fprintf(stderr,"\rEX55");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex55 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in55  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out55 >/dev/null");

    /* EX56 */
    fprintf(stderr,"\rEX56");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex56 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in56  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out56 >/dev/null");

    /* EX57 */
    fprintf(stderr,"\rEX57");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex57 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in57  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out57 >/dev/null");

    /* EX58 */
    fprintf(stderr,"\rEX58");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex58 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in58  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out58 >/dev/null");

    /* EX59 */
    fprintf(stderr,"\rEX59");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex59 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in59  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out59 >/dev/null");

    /* EX60 */
    fprintf(stderr,"\rEX60");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex60 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in60  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out60 >/dev/null");

    /* EX61 */
    fprintf(stderr,"\rEX61");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex61 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in61  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out61 >/dev/null");

    /* EX62 */
    fprintf(stderr,"\rEX62");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex62 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in62  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out62 >/dev/null");

    /* EX63 */
    fprintf(stderr,"\rEX63");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex63 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in63  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out63 >/dev/null");

    /* EX64 */
    fprintf(stderr,"\rEX64");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex64 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in64  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out64 >/dev/null");

    /* EX65 */
    fprintf(stderr,"\rEX65");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex65 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in65  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out65 >/dev/null");

    /* EX66 */
    fprintf(stderr,"\rEX66");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex66 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in66  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out66 >/dev/null");

    /* EX67 */
    fprintf(stderr,"\rEX67");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex67 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in67  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out67 >/dev/null");

    /* EX68 */
    fprintf(stderr,"\rEX68");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex68 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in68  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out68 >/dev/null");

    /* EX69 */
    fprintf(stderr,"\rEX69");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex69 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in69  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out69 >/dev/null");

    /* EX70 */
    fprintf(stderr,"\rEX70");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex70 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in70  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out70 >/dev/null");

    /* EX71 */
    fprintf(stderr,"\rEX71");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex71 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in71  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out71 >/dev/null");

    /* EX72 */
    fprintf(stderr,"\rEX72");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex72 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in72  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out72 >/dev/null");

    /* EX73 */
    fprintf(stderr,"\rEX73");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex73 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in73  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out73 >/dev/null");

    /* EX74 */
    fprintf(stderr,"\rEX74");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex74 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in74  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out74 >/dev/null");

    /* EX75 */
    fprintf(stderr,"\rEX75");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex75 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in75  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out75 >/dev/null");

    /* EX76 */
    fprintf(stderr,"\rEX76");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex76 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in76  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out76 >/dev/null");

    /* EX77 */
    fprintf(stderr,"\rEX77");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex77 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in77  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out77 >/dev/null");

    /* EX78 */
    fprintf(stderr,"\rEX78");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex78 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in78  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out78 >/dev/null");

    /* EX79 */
    fprintf(stderr,"\rEX79");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex79 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in79  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out79 >/dev/null");

    /* EX80 */
    fprintf(stderr,"\rEX80");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex80 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in80  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out80 >/dev/null");

    /* EX81 */
    fprintf(stderr,"\rEX81");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex81 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in81  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out81 >/dev/null");

    /* EX82 */
    fprintf(stderr,"\rEX82");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex82 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in82  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out82 >/dev/null");

    /* EX83 */
    fprintf(stderr,"\rEX83");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex83 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in83  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out83 >/dev/null");

    /* EX84 */
    fprintf(stderr,"\rEX84");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex84 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in84  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out84 >/dev/null");

    /* EX85 */
    fprintf(stderr,"\rEX85");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex85 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in85  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out85 >/dev/null");

    /* EX86 */
    fprintf(stderr,"\rEX86");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex86 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in86  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out86 >/dev/null");

    /* EX87 */
    fprintf(stderr,"\rEX87");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex87 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in87  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out87 >/dev/null");

    /* EX88 */
    fprintf(stderr,"\rEX88");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex88 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in88  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out88 >/dev/null");

    /* EX89 */
    fprintf(stderr,"\rEX89");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex89 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in89  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out89 >/dev/null");

    /* EX90 */
    fprintf(stderr,"\rEX90");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex90 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in90  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out90 >/dev/null");

    /* EX91 */
    fprintf(stderr,"\rEX91");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex91 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in91  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out91 >/dev/null");

    /* EX92 */
    fprintf(stderr,"\rEX92");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex92 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in92  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out92 >/dev/null");

    /* EX93 */
    fprintf(stderr,"\rEX93");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex93 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in93  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out93 >/dev/null");

    /* EX94 */
    fprintf(stderr,"\rEX94");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex94 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in94  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out94 >/dev/null");

    /* EX95 */
    fprintf(stderr,"\rEX95");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex95 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in95  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out95 >/dev/null");

    /* EX96 */
    fprintf(stderr,"\rEX96");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex96 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in96  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out96 >/dev/null");

    /* EX97 */
    fprintf(stderr,"\rEX97");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex97 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in97  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out97 >/dev/null");

    /* EX98 */
    fprintf(stderr,"\rEX98");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex98 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in98  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out98 >/dev/null");

    /* EX99 */
    fprintf(stderr,"\rEX99");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex99 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in99  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out99 >/dev/null");

    /* EX100 */
    fprintf(stderr,"\rEX100");
    err += !!system(COMP_EXEC " --input ./tests/asm_correct/ex100 --output ./tests/asm_correct/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/asm_correct/asm < ./tests/asm_correct/in100  > ./tests/asm_correct/result"
                    "&& ./tests/edit_file.sh ./tests/asm_correct/result");
    err += !!system("diff ./tests/asm_correct/result ./tests/asm_correct/out100 >/dev/null");

    err += !!system("rm -f ./tests/asm_correct/result ./tests/asm_correct/asm");

    fprintf(stderr,"\r");
    return err ? FAILED : PASSED;
}

static int test_gebala_code(void)
{
    int err = 0;

    /* EX1 */
    fprintf(stderr,"\rEX1");
    err += !!system(COMP_EXEC " --input ./tests/gebala/ex1 --output ./tests/gebala/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gebala/asm < ./tests/gebala/in1  > ./tests/gebala/result"
                    "&& ./tests/edit_file.sh ./tests/gebala/result");
    err += !!system("diff ./tests/gebala/result ./tests/gebala/out1 >/dev/null");

    /* EX2 */
    fprintf(stderr,"\rEX2");
    err += !!system(COMP_EXEC " --input ./tests/gebala/ex2 --output ./tests/gebala/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gebala/asm < ./tests/gebala/in2  > ./tests/gebala/result"
                    "&& ./tests/edit_file.sh ./tests/gebala/result");
    err += !!system("diff ./tests/gebala/result ./tests/gebala/out2 >/dev/null");

    /* EX3 */
    fprintf(stderr,"\rEX3");
    err += !!system(COMP_EXEC " --input ./tests/gebala/ex3 --output ./tests/gebala/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gebala/asm < ./tests/gebala/in3  > ./tests/gebala/result"
                    "&& ./tests/edit_file.sh ./tests/gebala/result");
    err += !!system("diff ./tests/gebala/result ./tests/gebala/out3 >/dev/null");

    /* EX4 */
    fprintf(stderr,"\rEX4");
    err += !!system(COMP_EXEC " --input ./tests/gebala/ex4 --output ./tests/gebala/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gebala/asm < ./tests/gebala/in4  > ./tests/gebala/result"
                    "&& ./tests/edit_file.sh ./tests/gebala/result");
    err += !!system("diff ./tests/gebala/result ./tests/gebala/out4 >/dev/null");

    /* EX5 */
    fprintf(stderr,"\rEX5");
    err += !!system(COMP_EXEC " --input ./tests/gebala/ex5 --output ./tests/gebala/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gebala/asm < ./tests/gebala/in5  > ./tests/gebala/result"
                    "&& ./tests/edit_file.sh ./tests/gebala/result");
    err += !!system("diff ./tests/gebala/result ./tests/gebala/out5 >/dev/null");

    /* EX6 */
    fprintf(stderr,"\rEX6");
    err += !!system(COMP_EXEC " --input ./tests/gebala/ex6 --output ./tests/gebala/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gebala/asm < ./tests/gebala/in6  > ./tests/gebala/result"
                    "&& ./tests/edit_file.sh ./tests/gebala/result");
    err += !!system("diff ./tests/gebala/result ./tests/gebala/out6 >/dev/null");

    /* EX7 */
    fprintf(stderr,"\rEX7");
    err += !!system(COMP_EXEC " --input ./tests/gebala/ex7 --output ./tests/gebala/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gebala/asm < ./tests/gebala/in7  > ./tests/gebala/result"
                    "&& ./tests/edit_file.sh ./tests/gebala/result");
    err += !!system("diff ./tests/gebala/result ./tests/gebala/out7 >/dev/null");

    /* EX8 */
    fprintf(stderr,"\rEX8");
    err += !!system(COMP_EXEC " --input ./tests/gebala/ex8 --output ./tests/gebala/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gebala/asm < ./tests/gebala/in8  > ./tests/gebala/result"
                    "&& ./tests/edit_file.sh ./tests/gebala/result");
    err += !!system("diff ./tests/gebala/result ./tests/gebala/out8 >/dev/null");

    err += !!system("rm -f ./tests/gebala/result ./tests/gebala/asm");

    fprintf(stderr,"\r");
    return err ? FAILED : PASSED;
}

static int test_gotfryd_code(void)
{
    int err = 0;

    /* EX1 */
    fprintf(stderr,"\rEX1");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd/ex1 --output ./tests/gotfryd/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd/asm < ./tests/gotfryd/in1  > ./tests/gotfryd/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd/result");
    err += !!system("diff ./tests/gotfryd/result ./tests/gotfryd/out1 >/dev/null");

    /* EX2 */
    fprintf(stderr,"\rEX2");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd/ex2 --output ./tests/gotfryd/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd/asm < ./tests/gotfryd/in2  > ./tests/gotfryd/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd/result");
    err += !!system("diff ./tests/gotfryd/result ./tests/gotfryd/out2 >/dev/null");

    /* EX3 */
    fprintf(stderr,"\rEX3");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd/ex3 --output ./tests/gotfryd/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd/asm < ./tests/gotfryd/in3  > ./tests/gotfryd/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd/result");
    err += !!system("diff ./tests/gotfryd/result ./tests/gotfryd/out3 >/dev/null");

    /* EX4 */
    fprintf(stderr,"\rEX4");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd/ex4 --output ./tests/gotfryd/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd/asm < ./tests/gotfryd/in4  > ./tests/gotfryd/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd/result");
    err += !!system("diff ./tests/gotfryd/result ./tests/gotfryd/out4 >/dev/null");

    /* EX5 */
    fprintf(stderr,"\rEX5");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd/ex5 --output ./tests/gotfryd/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd/asm < ./tests/gotfryd/in5  > ./tests/gotfryd/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd/result");
    err += !!system("diff ./tests/gotfryd/result ./tests/gotfryd/out5 >/dev/null");

    /* EX6 */
    fprintf(stderr,"\rEX6");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd/ex6 --output ./tests/gotfryd/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd/asm < ./tests/gotfryd/in6  > ./tests/gotfryd/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd/result");
    err += !!system("diff ./tests/gotfryd/result ./tests/gotfryd/out6 >/dev/null");

    /* EX7 */
    fprintf(stderr,"\rEX7");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd/ex7 --output ./tests/gotfryd/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd/asm < ./tests/gotfryd/in7  > ./tests/gotfryd/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd/result");
    err += !!system("diff ./tests/gotfryd/result ./tests/gotfryd/out7 >/dev/null");

    /* EX8 */
    fprintf(stderr,"\rEX8");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd/ex8 --output ./tests/gotfryd/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd/asm < ./tests/gotfryd/in8  > ./tests/gotfryd/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd/result");
    err += !!system("diff ./tests/gotfryd/result ./tests/gotfryd/out8 >/dev/null");

    /* EX9 */
    fprintf(stderr,"\rEX9");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd/ex9 --output ./tests/gotfryd/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd/asm < ./tests/gotfryd/in9  > ./tests/gotfryd/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd/result");
    err += !!system("diff ./tests/gotfryd/result ./tests/gotfryd/out9 >/dev/null");

    /* EX10 */
    fprintf(stderr,"\rEX10");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd/ex10 --output ./tests/gotfryd/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd/asm < ./tests/gotfryd/in10  > ./tests/gotfryd/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd/result");
    err += !!system("diff ./tests/gotfryd/result ./tests/gotfryd/out10 >/dev/null");

    /* EX11 */
    fprintf(stderr,"\rEX11");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd/ex11 --output ./tests/gotfryd/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd/asm < ./tests/gotfryd/in11  > ./tests/gotfryd/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd/result");
    err += !!system("diff ./tests/gotfryd/result ./tests/gotfryd/out11 >/dev/null");

    /* EX12 */
    fprintf(stderr,"\rEX12");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd/ex12 --output ./tests/gotfryd/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd/asm < ./tests/gotfryd/in12  > ./tests/gotfryd/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd/result");
    err += !!system("diff ./tests/gotfryd/result ./tests/gotfryd/out12 >/dev/null");

    err += !!system("rm -f ./tests/gotfryd/result ./tests/gotfryd/asm");

    fprintf(stderr,"\r");
    return err ? FAILED : PASSED;
}

static int test_gotfryd_code2(void)
{
    int err = 0;

    /* EX1 */
    fprintf(stderr,"\rEX1");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex1 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in1.1  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out1.1 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex1 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in1.2  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out1.2 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex1 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in1.3  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out1.3 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex1 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in1.4  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out1.4 >/dev/null");


    /* EX2 */
    fprintf(stderr,"\rEX2");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex2 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in2  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out2 >/dev/null");


    /* EX3 */
    fprintf(stderr,"\rEX3");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex3 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in3.1  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out3.1 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex3 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in3.2  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out3.2 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex3 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in3.3  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out3.3 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex3 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in3.4  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out3.4 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex3 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in3.5  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out3.5 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex3 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in3.6  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out3.6 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex3 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in3.7  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out3.7 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex3 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in3.8  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out3.8 >/dev/null");

    /* EX4 */
    fprintf(stderr,"\rEX4");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex4 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in4.1  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out4.1 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex4 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in4.2  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out4.2 >/dev/null");

    /* EX5 */
    fprintf(stderr,"\rEX5");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex5 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in5.1  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out5.1 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex5 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in5.2  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out5.2 >/dev/null");

    /* EX6 */
    fprintf(stderr,"\rEX6");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex6 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in6.1  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out6.1 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex6 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in6.2  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out6.2 >/dev/null");

    /* EX7 */
    fprintf(stderr,"\rEX7");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex7 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in7  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out7 >/dev/null");

    /* EX8 */
    fprintf(stderr,"\rEX8");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex8 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in8  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out8 >/dev/null");

    /* EX9 */
    fprintf(stderr,"\rEX9");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex9 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in9.1  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out9.1 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex9 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in9.2  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out9.2 >/dev/null");

    /* EX10 */
    fprintf(stderr,"\rEX10");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex10 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in10.1  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out10.1 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex10 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in10.2  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out10.2 >/dev/null");

    /* EX11 */
    fprintf(stderr,"\rEX11");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex11 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in11.1  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out11.1 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex11 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in11.2  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out11.2 >/dev/null");

    /* EX12 */
    fprintf(stderr,"\rEX12");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex12 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in12  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out12 >/dev/null");

    /* EX13 */
    fprintf(stderr,"\rEX13");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex13 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in13  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out13 >/dev/null");

    /* EX14 */
    fprintf(stderr,"\rEX14");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex14 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in14  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out14 >/dev/null");

    /* EX15 */
    fprintf(stderr,"\rEX15");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex15 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in15  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out15 >/dev/null");

    /* EX16 */
    fprintf(stderr,"\rEX16");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex16 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in16  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out16 >/dev/null");

    /* EX17 */
    fprintf(stderr,"\rEX17");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex17 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in17  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out17 >/dev/null");

    /* EX18 */
    fprintf(stderr,"\rEX18");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex18 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in18  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out18 >/dev/null");

    /* EX19 */
    fprintf(stderr,"\rEX19");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex19 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in19.1  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out19.1 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex19 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in19.2  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out19.2 >/dev/null");

    /* EX20 */
    fprintf(stderr,"\rEX20");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex20 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in20.1  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out20.1 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex20 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in20.2  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out20.2 >/dev/null");

    /* EX21 */
    fprintf(stderr,"\rEX21");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex21 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in21.1  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out21.1 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex21 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in21.2  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out21.2 >/dev/null");

    /* EX22 */
    fprintf(stderr,"\rEX22");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex22 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in22.1  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out22.1 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex22 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in22.2  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out22.2 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex22 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in22.3  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out22.3 >/dev/null");

    /* EX23 */
    fprintf(stderr,"\rEX23");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex23 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in23.1  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out23.1 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex23 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in23.2  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out23.2 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex23 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in23.3  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out23.3 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex23 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in23.4  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out23.4 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex23 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in23.5  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out23.5 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex23 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in23.6  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out23.6 >/dev/null");

    /* EX24 */
    fprintf(stderr,"\rEX24");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex24 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in24  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out24 >/dev/null");

    /* EX25 */
    fprintf(stderr,"\rEX25");
    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex25 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in25.1  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out25.1 >/dev/null");

    err += !!system(COMP_EXEC " --input ./tests/gotfryd2/ex25 --output ./tests/gotfryd2/asm >/dev/null 2>&1");
    err += !!system( INT_EXEC " ./tests/gotfryd2/asm < ./tests/gotfryd2/in25.2  > ./tests/gotfryd2/result"
                    "&& ./tests/edit_file.sh ./tests/gotfryd2/result");
    err += !!system("diff ./tests/gotfryd2/result ./tests/gotfryd2/out25.2 >/dev/null");

    err += !!system("rm -f ./tests/gotfryd2/result ./tests/gotfryd2/asm");

    fprintf(stderr,"\r");
    return err ? FAILED : PASSED;
}

void run(void)
{
    TEST(test_create_variables());
    TEST(test_big_array_create());
    TEST(test_cmp_variables());

    TEST(test_create_tokens());
    TEST(test_tokens_str());

    TEST(test_pump_algo());
    TEST(test_mult_algo());

    TEST(test_memory_managment());
    TEST(test_regs_managment1());
    TEST(test_regs_managment2());
    TEST(test_regs_managment3());

    TEST(test_gramma_errors());
    TEST(test_semantic_errors());

    TEST(test_correct_semantic());

    TEST(test_correct_tokens());

    TEST(test_compiled_code());
    TEST(test_gebala_code());
    TEST(test_gotfryd_code());
    TEST(test_gotfryd_code2());
}


int main(void)
{
    run();
    return 0;
}
