#include <compiler.h>
#include <parser_helper.h>
#include <optimizer.h>
#include <filebuffer.h>
#include <asm.h>
#include <arch.h>

/* Buffer for file */
static file_buffer *fb;

/* extern from compierl.h */
/* compiled asm code */
Arraylist *asmcode;
/* avl of cvars */
Avl* compiler_variables;
/* stack with labels */
Stack *labels;

/* stack with loop begining lines */
static Stack *looplines;

/* stack with Cfor structures */
static Stack *forloops;

/* loop counters */
static uint64_t while_c = 0;
static uint64_t for_c = 0;

/* if counter */
static uint64_t if_c = 0;

/* compiler option */
Option option =
{
    .wall           =   0,
    .werr           =   0,
    .optimal        =   0,
    .tokens         =   0,
    .padding        =   0,
    .input_file     =   NULL,
    .output_file    =   NULL
};

/*
    Wrirte tokens to output file

    PARAMS
    @IN tokens - tokens list

    RETURN
    0 iff success
    Non-zero value iff failure
*/
static int write_tokens(Arraylist *tokens) __nonull__(1);

/*
    Compile token list to asm code

    PARAMS
    @IN tokens - tokens list

    RETURN
    0 iff success
    Non-zero value iff failure
*/
static int compile(Arraylist *tokens) __nonull__(1);

/*
    Alloc all variables and arrays in memory

    PARAMS
    @IN vars - avl with pvars

    RETURN
    0 iff success
    Non-zero value iff failure
*/
static int alloc_variables(Avl *vars) __nonull__(1);

/*
    compile all tokens to asm code

    PARAMS
    @IN tokens - tokens list

    RETURN
    0 iff success
    Non-zero value iff failure
*/
static int compiler_helper(Arraylist *tokens) __nonull__(1);

/*
    If we have loop we don't know how to trace value
    so set all variables used in loop as symbolic,
    but iff we have
        a := b + c;
        a := 0
        READ a;
    in loop we overwrite symbolic flag,
    so do it on every token

    PARAMS
    @IN token - pointer to token

    RETURN
    0 iff success
    Non-zero value iff failure

*/
static int set_symbolic_in_loop(Token *token);

/*
    Synchronize all register with memory,
    FREE registers
    set REG_PTR to symbolic

    PARAMS
    NO PARAMS

    RETURN
    0 iff success
    Non-zero value iff failure
*/
static int sync_all(void);

/*
    Compile assign token

    PARAMS
    @IN token - poiner to token

    RETURN
    0 iff success
    Non-zero value iff failure
*/
static int compile_token_assign(token_assign *token) __nonull__(1);

/*
    Compile for token

    PARAMS
    @IN token - poiner to token

    RETURN
    0 iff success
    Non-zero value iff failure
*/
static int compile_token_for(token_for *token) __nonull__(1);

/*
    Compile guard token

    PARAMS
    @IN token - poiner to token

    RETURN
    0 iff success
    Non-zero value iff failure
*/
static int compile_token_guard(token_guard *token) __nonull__(1);

/*
    Compile if token

    PARAMS
    @IN token - poiner to token

    RETURN
    0 iff success
    Non-zero value iff failure
*/
static int compile_token_if(token_if *token) __nonull__(1);

/*
    Compile io token

    PARAMS
    @IN token - poiner to token

    RETURN
    0 iff success
    Non-zero value iff failure
*/
static int compile_token_io(token_io *token) __nonull__(1);

/*
    Compile while token

    PARAMS
    @IN token - poiner to token

    RETURN
    0 iff success
    Non-zero value iff failure
*/
static int compile_token_while(token_while *token) __nonull__(1);

/*
    ADD HELPER

    PARAMS
    @IN token - poiner to token

    RETURN
    0 iff success
    Non-zero value iff failure
*/
static int __add(token_assign *token) __nonull__(1);

/*
    SUB HELPER

    PARAMS
    @IN token - poiner to token

    RETURN
    0 iff success
    Non-zero value iff failure
*/
static int __sub(token_assign *token) __nonull__(1);

/*
    MULT HELPER

    PARAMS
    @IN token - poiner to token

    RETURN
    0 iff success
    Non-zero value iff failure
*/
static int __mult(token_assign *token) __nonull__(1);

/*
    DIV HELPER

    PARAMS
    @IN token - poiner to token

    RETURN
    0 iff success
    Non-zero value iff failure
*/
static int __div(token_assign *token) __nonull__(1);

/*
    MOD HELPER

    PARAMS
    @IN token - poiner to token

    RETURN
    0 iff success
    Non-zero value iff failure
*/
static int __mod(token_assign *token) __nonull__(1);

/*
    ASSIGN HELPER

    PARAMS
    @IN token - poiner to token

    RETURN
    0 iff success
    Non-zero value iff failure
*/
static int __assign(token_assign *token) __nonull__(1);

/*
    COND HELPER

    PARAMS
    @IN token - poiner to token

    RETURN
    0 iff success
    Non-zero value iff failure
*/
static int __cond(token_cond *token) __nonull__(1);

static __inline__ int cond_fake(void)
{
    LOG("COND FAKE\n", "");

    if( label_fake() )
        ERROR("label_fake error\n", 1, "");

    if( label_fake() )
        ERROR("label_fake error\n", 1, "");

    return 0;
}

static __inline__ BOOL cvar_need_malloc(const char *name)
{
    if(! strcmp(name, TEMP_ADDR_NAME) || ! strcmp(name, TEMP_DIV_HELPER)  )
        return FALSE;

    return TRUE;
}

static __inline__ BOOL need_jump(Arraylist *tokens, uint64_t pos)
{
    Arraylist_iterator it;
    Token *token;

    uint64_t counter = 0;

    /* skip obsolete tokens */
    for(  arraylist_iterator_init(tokens, &it, ITI_BEGIN);
        ! arraylist_iterator_end(&it) && counter < pos;
          arraylist_iterator_next(&it))
        {
            ++counter;
        }

    /* for each token on list DO: */
    for( ;
        ! arraylist_iterator_end(&it);
          arraylist_iterator_next(&it))
        {
            arraylist_iterator_get_data(&it, (void*)&token);

            if(token->type == TOKEN_GUARD &&
                token->body.guard->type == tokens_id.end_if)
                    return FALSE;

            if(token->type != TOKEN_GUARD ||
                (token->type == TOKEN_GUARD && token->body.guard->type != tokens_id.skip) )
                    return TRUE;
        }

    return TRUE;
}

static int write_tokens(Arraylist *tokens)
{
    char *str;
    Arraylist_iterator it;
    Token *token;

    TRACE("");

    if(tokens == NULL)
        ERROR("tokens == NULL\n", 1, "");

    for(  arraylist_iterator_init(tokens, &it, ITI_BEGIN);
        ! arraylist_iterator_end(&it);
          arraylist_iterator_next(&it))
        {
            arraylist_iterator_get_data(&it, (void*)&token);

            str = token_str(token);

            file_buffer_append(fb, str);
            file_buffer_append(fb, "\n");

            FREE(str);
        }

    return 0;
}

static int alloc_variables(Avl *vars)
{
    Pvar *var;

    var_normal *vn;
    Variable *variable;
    Value *val;

    Array *arr;

    Avl_iterator it;

    Cvar *cvar;

    TRACE("");

    if(vars == NULL)
        ERROR("vars == NULL\n", 1, "");

    for(  avl_iterator_init(vars, &it, ITI_BEGIN);
        ! avl_iterator_end(&it);
          avl_iterator_next(&it))
        {
            avl_iterator_get_data(&it, (void*)&var);

            /* normal variable */
            if(var->type == PTOKEN_VAR)
            {
                LOG("Alloc variable %s\n", var->name);

                /* alloc variable in virtual memory */
                vn = var_normal_create(var->name);
                if(vn == NULL)
                    ERROR("var_normal_create error\n", 1, "");

                variable = variable_create(VAR_NORMAL, (void*)vn);
                if(variable == NULL)
                    ERROR("variable_create error\n", 1, "");

                val = value_create(VARIABLE, (void*)variable);
                if(val == NULL)
                    ERROR("value_create error\n", 1, "");

                if(cvar_need_malloc(var->name))
                    my_malloc(memory, VALUE, (void*)val);

                /* Add Value to compiler variables */
                cvar = cvar_create(VALUE, (void*)val);
                if(cvar == NULL)
                    ERROR("cvar_create error\n", 1, "");

                if(avl_insert(compiler_variables, (void*)&cvar))
                    ERROR("avl_insert error\n", 1, "");
            }
            else
            {
                LOG("Alloc array %s with len %ju\n", var->name, var->array_len);

                arr = array_create(var->name, var->array_len);
                if(arr == NULL)
                    ERROR("array_create error\n", 1, "");

                if(var->array_len <= ARRAY_MAX_LEN)
                    my_malloc(memory, ARRAY, (void*)arr);
                else
                    my_malloc(memory, BIG_ARRAY, (void*)arr);

                /* Add Array to compiler variables */
                cvar = cvar_create(ARRAY, (void*)arr);
                if(cvar == NULL)
                    ERROR("cvar_create error\n", 1, "");

                if(avl_insert(compiler_variables, (void*)&cvar))
                    ERROR("avl_insert error\n", 1, "");
            }
        }

    return 0;
}

static int set_symbolic_in_loop(Token *token)
{
    Cvar *res;
    Cvar *left;
    Cvar *right;

    TRACE("");

    /* set symbol iff we have loop */
    if(while_c == 0 && for_c == 0 && if_c == 0)
        return 0;

    LOG("while_c = %ju for_c = %ju if_c = %ju\n", while_c, for_c, if_c);

    switch(token->type)
    {
        case TOKEN_ASSIGN:
        {
            if(value_can_trace(token->body.assign->res))
            {
                res = cvar_get_by_value(token->body.assign->res);
                LOG("SET symbolic flag on %s\n", res->name);
                value_set_symbolic_flag(res->body.val);
            }
            else
            {
                if(token->body.assign->res->type == VARIABLE)
                    if(token->body.assign->res->body.var->type == VAR_ARR)
                        if(token->body.assign->res->body.var->body.arr->var_offset != NULL)
                        {
                            res = cvar_get_by_name(token->body.assign->res->body.var->body.arr->var_offset->name);
                            LOG("SET symbolic flag on %s\n", res->name);
                            value_set_symbolic_flag(res->body.val);
                        }
            }

            if(value_can_trace(token->body.assign->expr->left))
            {
                left = cvar_get_by_value(token->body.assign->expr->left);
                LOG("SET symbolic flag on %s\n", left->name);
                value_set_symbolic_flag(left->body.val);
            }
            else
            {
                if(token->body.assign->expr->left->type == VARIABLE)
                    if(token->body.assign->expr->left->body.var->type == VAR_ARR)
                        if(token->body.assign->expr->left->body.var->body.arr->var_offset != NULL)
                        {
                            left = cvar_get_by_name(token->body.assign->expr->left->body.var->body.arr->var_offset->name);
                            LOG("SET symbolic flag on %s\n", left->name);
                            value_set_symbolic_flag(left->body.val);
                        }
            }

            if(token->body.assign->expr->op != tokens_id.undefined)
            {
                if(value_can_trace(token->body.assign->expr->right))
                {
                    right = cvar_get_by_value(token->body.assign->expr->right);
                    LOG("SET symbolic flag on %s\n", right->name);
                    value_set_symbolic_flag(right->body.val);
                }
                else
                {
                    if(token->body.assign->expr->right->type == VARIABLE)
                        if(token->body.assign->expr->right->body.var->type == VAR_ARR)
                            if(token->body.assign->expr->right->body.var->body.arr->var_offset != NULL)
                            {
                                right = cvar_get_by_name(token->body.assign->expr->right->body.var->body.arr->var_offset->name);
                                LOG("SET symbolic flag on %s\n", right->name);
                                value_set_symbolic_flag(right->body.val);
                            }
                }
            }

            break;
        }
        case TOKEN_FOR:
        {
            if(value_can_trace(token->body.for_loop->begin_value))
            {
                left = cvar_get_by_value(token->body.for_loop->begin_value);
                LOG("SET symbolic flag on %s\n", left->name);
                value_set_symbolic_flag(left->body.val);
            }
            else
            {
                if(token->body.for_loop->begin_value->type == VARIABLE)
                    if(token->body.for_loop->begin_value->body.var->type == VAR_ARR)
                        if(token->body.for_loop->begin_value->body.var->body.arr->var_offset != NULL)
                        {
                            left = cvar_get_by_name(token->body.for_loop->begin_value->body.var->body.arr->var_offset->name);
                            LOG("SET symbolic flag on %s\n", left->name);
                            value_set_symbolic_flag(left->body.val);
                        }
            }

            if(value_can_trace(token->body.for_loop->end_value))
            {
                right = cvar_get_by_value(token->body.for_loop->end_value);
                LOG("SET symbolic flag on %s\n", right->name);
                value_set_symbolic_flag(right->body.val);
            }
            else
            {
                if(token->body.for_loop->end_value->type == VARIABLE)
                    if(token->body.for_loop->end_value->body.var->type == VAR_ARR)
                        if(token->body.for_loop->end_value->body.var->body.arr->var_offset != NULL)
                        {
                            right = cvar_get_by_name(token->body.for_loop->end_value->body.var->body.arr->var_offset->name);
                            LOG("SET symbolic flag on %s\n", right->name);
                            value_set_symbolic_flag(right->body.val);
                        }
            }

            break;
        }
        case TOKEN_IF:
        {
            if(value_can_trace(token->body.if_cond->cond->left))
            {
                left = cvar_get_by_value(token->body.if_cond->cond->left);
                LOG("SET symbolic flag on %s\n", left->name);
                value_set_symbolic_flag(left->body.val);
            }
            else
            {
                if(token->body.if_cond->cond->left->type == VARIABLE)
                    if(token->body.if_cond->cond->left->body.var->type == VAR_ARR)
                        if(token->body.if_cond->cond->left->body.var->body.arr->var_offset != NULL)
                        {
                            left = cvar_get_by_name(token->body.if_cond->cond->left->body.var->body.arr->var_offset->name);
                            LOG("SET symbolic flag on %s\n", left->name);
                            value_set_symbolic_flag(left->body.val);
                        }
            }

            if(value_can_trace(token->body.if_cond->cond->right))
            {
                right = cvar_get_by_value(token->body.if_cond->cond->right);
                LOG("SET symbolic flag on %s\n", right->name);
                value_set_symbolic_flag(right->body.val);
            }
            else
            {
                if(token->body.if_cond->cond->right->type == VARIABLE)
                    if(token->body.if_cond->cond->right->body.var->type == VAR_ARR)
                        if(token->body.if_cond->cond->right->body.var->body.arr->var_offset != NULL)
                        {
                            right = cvar_get_by_name(token->body.if_cond->cond->right->body.var->body.arr->var_offset->name);
                            LOG("SET symbolic flag on %s\n", right->name);
                            value_set_symbolic_flag(right->body.val);
                        }
            }

            break;
        }
        case TOKEN_IO:
        {
            if(value_can_trace(token->body.io->res))
            {
                res = cvar_get_by_value(token->body.io->res);
                LOG("SET symbolic flag on %s\n", res->name);
                value_set_symbolic_flag(res->body.val);
            }
            else
            {
                if(token->body.io->res->type == VARIABLE)
                    if(token->body.io->res->body.var->type == VAR_ARR)
                        if(token->body.io->res->body.var->body.arr->var_offset != NULL)
                        {
                            res = cvar_get_by_name(token->body.io->res->body.var->body.arr->var_offset->name);
                            LOG("SET symbolic flag on %s\n", res->name);
                            value_set_symbolic_flag(res->body.val);
                        }
            }

            break;
        }
        case TOKEN_WHILE:
        {
            if(value_can_trace(token->body.while_loop->cond->left))
            {
                left = cvar_get_by_value(token->body.while_loop->cond->left);
                LOG("SET symbolic flag on %s\n", left->name);
                value_set_symbolic_flag(left->body.val);
            }
            else
            {
                if(token->body.while_loop->cond->left->type == VARIABLE)
                    if(token->body.while_loop->cond->left->body.var->type == VAR_ARR)
                        if(token->body.while_loop->cond->left->body.var->body.arr->var_offset != NULL)
                        {
                            left = cvar_get_by_name(token->body.while_loop->cond->left->body.var->body.arr->var_offset->name);
                            LOG("SET symbolic flag on %s\n", left->name);
                            value_set_symbolic_flag(left->body.val);
                        }
            }

            if(value_can_trace(token->body.while_loop->cond->right))
            {
                right = cvar_get_by_value(token->body.while_loop->cond->right);
                LOG("SET symbolic flag on %s\n", right->name);
                value_set_symbolic_flag(right->body.val);
            }
            else
            {
                if(token->body.while_loop->cond->right->type == VARIABLE)
                    if(token->body.while_loop->cond->right->body.var->type == VAR_ARR)
                        if(token->body.while_loop->cond->right->body.var->body.arr->var_offset != NULL)
                        {
                            right = cvar_get_by_name(token->body.while_loop->cond->right->body.var->body.arr->var_offset->name);
                            LOG("SET symbolic flag on %s\n", right->name);
                            value_set_symbolic_flag(right->body.val);
                        }
            }

            break;
        }
        case TOKEN_GUARD:
        {
            break;
        }
        default:
        {
            ERROR("unrecognized token type\n", 1, "");
        }
    }

    return 0;
}

static int sync_all(void)
{
    int i;
    Cvar *cvar;

    TRACE("");

    LOG("SYNC REGISTERS\n", "");

    for(i = REG_PTR + 1; i < REGS_NUMBER; ++i)
        if( ! IS_REG_FREE(cpu->registers[i]) && ! IS_REG_IN_USE(cpu->registers[i]) )
        {
            LOG("SYNC REG %ju\n", cpu->registers[i]->num);

            if( do_synchronize(cpu->registers[i]) )
                ERROR("do_synchronize error\n", 1, "");

            REG_SET_FREE(cpu->registers[i]);
        }

    LOG("SET SYMBOLIC FLAG TO PTR\n", "");
    cvar = cvar_get_by_name(PTR_NAME);
    value_set_symbolic_flag(cvar->body.val);

    return 0;
}

static int compiler_helper(Arraylist *tokens)
{
    Arraylist_iterator it;
    Token *token;

    int i;

    TRACE("");

    token_list = tokens;
    token_list_pos = 1;

    /* for each token do compile */
    for(   arraylist_iterator_init(tokens, &it, ITI_BEGIN);
         ! arraylist_iterator_end(&it);
           arraylist_iterator_next(&it) )
        {
            arraylist_iterator_get_data(&it, (void*)&token);

            set_symbolic_in_loop(token);
            switch(token->type)
            {
                case TOKEN_ASSIGN:
                {
                    if ( compile_token_assign(token->body.assign) )
                        ERROR("compile_token_assign error\n", 1, "");

                    break;
                }
                case TOKEN_FOR:
                {
                    if( compile_token_for(token->body.for_loop) )
                        ERROR("compile_token_for error\n", 1, "");

                    break;
                }
                case TOKEN_GUARD:
                {
                    if ( compile_token_guard(token->body.guard) )
                        ERROR("compile_token_guard error\n", 1, "");

                    break;
                }
                case TOKEN_IF:
                {
                    if( compile_token_if(token->body.if_cond) )
                        ERROR("compile_token_if error\n", 1, "");

                    break;
                }
                case TOKEN_IO:
                {
                    if( compile_token_io(token->body.io) )
                        ERROR("compile_token_io error\n", 1, "");

                    break;
                }
                case TOKEN_WHILE:
                {
                    if( compile_token_while(token->body.while_loop) )
                        ERROR("compile_token_while error\n", 1, "");

                    break;
                }
                default:
                {
                    ERROR("unrecognized token type\n", 1, "");
                }
            }

            ++token_list_pos;

            /* assert NO REG IS IN USE */
            for(i = 0; i < REGS_NUMBER; ++i)
                if(IS_REG_IN_USE(cpu->registers[i]))
                    ERROR("REGISTER IN USE AFTER TOKEN\n", 1, "");
        }

        /* at the end we need halt */
        if(do_halt())
            ERROR("do_halt error\n", 1, "");

    return 0;
}

static int __cond(token_cond *token)
{
    int res;
    int reg;

    Cvar *cvar_left;
    Cvar *cvar_right;

    uint64_t tempval1;
    uint64_t tempval2;

    mpz_t val;
    mpz_t val2;

    TRACE("");

    cvar_left = cvar_get_by_value(token->left);
    cvar_right = cvar_get_by_value(token->right);

    /* N r M */
    if(token->left->type == CONST_VAL && token->right->type == CONST_VAL)
    {
        LOG("Both are const\n", "");

        tempval1 = token->left->body.cv->value;
        tempval2 = token->right->body.cv->value;

        /* if TRUE DO NOTHIG IF FALSE JUMP LABEL FALSE */
        /* I use NOT N r M to make this more visible */
        if(token->r == tokens_id.lt)
        {
            if( ! (tempval1 < tempval2) )
            {
                if(do_jump_label(LABEL_FALSE))
                    ERROR("do_jump_label error\n", 1, "");

                label_fake();
            }
            else
                cond_fake();
        }
        else if( token->r == tokens_id.le)
        {
            if( ! (tempval1 <= tempval2) )
            {
                if(do_jump_label(LABEL_FALSE))
                    ERROR("do_jump_label error\n", 1, "");

                label_fake();
            }
            else
                cond_fake();
        }
        else if(token->r == tokens_id.gt)
        {
            if( ! (tempval1 > tempval2) )
            {
                if(do_jump_label(LABEL_FALSE))
                    ERROR("do_jump_label error\n", 1, "");

                label_fake();
            }
            else
                cond_fake();
        }
        else if(token->r == tokens_id.ge)
        {
            if( ! (tempval1 >= tempval2) )
            {
                if(do_jump_label(LABEL_FALSE))
                    ERROR("do_jump_label error\n", 1, "");

                label_fake();
            }
            else
                cond_fake();
        }
        else if(token->r == tokens_id.eq)
        {
            if( ! (tempval1 == tempval2) )
            {
                if(do_jump_label(LABEL_FALSE))
                    ERROR("do_jump_label error\n", 1, "");

                label_fake();
            }
            else
                cond_fake();
        }
        else if(token->r == tokens_id.ne)
        {
            if( ! (tempval1 != tempval2) )
            {
                if(do_jump_label(LABEL_FALSE))
                    ERROR("do_jump_label error\n", 1, "");

                label_fake();
            }
            else
                cond_fake();
        }
    }
    /* only left is const */
    else if(token->left->type == CONST_VAL)
    {
        LOG("Only left is const\n", "");

        /* right is not symbolic so check cond now */
        if(value_can_trace(token->right) && ! value_is_symbolic(cvar_right->body.val))
        {
            LOG("Right is not symbolic\n", "");

            tempval1 = token->left->body.cv->value;

            value_get_val(cvar_right->body.val, val);
            tempval2 = mpz2ull(val);

            mpz_clear(val);

            /* if TRUE DO NOTHIG IF FALSE JUMP LABEL FALSE */
            /* I use NOT N r M to make this more visible */
            if(token->r == tokens_id.lt)
            {
                if( ! (tempval1 < tempval2) )
                {
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    label_fake();
                }
                else
                    cond_fake();
            }
            else if( token->r == tokens_id.le)
            {
                if( ! (tempval1 <= tempval2) )
                {
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    label_fake();
                }
                else
                    cond_fake();
            }
            else if(token->r == tokens_id.gt)
            {
                if( ! (tempval1 > tempval2) )
                {
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    label_fake();
                }
                else
                    cond_fake();
            }
            else if(token->r == tokens_id.ge)
            {
                if( ! (tempval1 >= tempval2) )
                {
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    label_fake();
                }
                else
                    cond_fake();
            }
            else if(token->r == tokens_id.eq)
            {
                if( ! (tempval1 == tempval2) )
                {
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    label_fake();
                }
                else
                    cond_fake();
            }
            else if(token->r == tokens_id.ne)
            {
                if( ! (tempval1 != tempval2) )
                {
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    label_fake();
                }
                else
                    cond_fake();
            }
        }
        else
        {
            LOG("Right is symbolic\n", "");

            if(token->left->body.cv->value == 0ull)
            {
                LOG("CONST = %ju, use jzero instruction\n", token->left->body.cv->value);

                /* get register for RIGHT, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->right, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                if(! value_can_trace(token->right)
                    || cpu->registers[reg]->val != cvar_right->body.val)
                    if(do_load(cpu->registers[reg], token->right))
                        ERROR("do_load error\n", 1, "");

                if(token->r == tokens_id.lt)
                {
                    /* only 0 is not < a  */
                    if(do_jzero_label(cpu->registers[reg], LABEL_FALSE))
                        ERROR("do_jzero_label error\n", 1, "");

                    /* allign labels */
                    if(label_fake())
                        ERROR("label_fake error\n", 1, "");
                }
                else if( token->r == tokens_id.le)
                {
                    /* always 0 <= a */
                    if( cond_fake() )
                        ERROR("cond_fake error\n", 1, "");
                }
                else if(token->r == tokens_id.gt)
                {
                    /* always false */
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    /* allign labels */
                    if(label_fake())
                        ERROR("label_fake error\n", 1, "");
                }
                else if(token->r == tokens_id.ge)
                {
                    /* only for 0 it is true */
                    if(do_jzero(cpu->registers[reg], asmcode->length + 2))
                        ERROR("do_jzero error\n", 1, "");

                    /* always false */
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    /* allign labels */
                    if(label_fake())
                        ERROR("label_fake error\n", 1, "");

                }
                else if(token->r == tokens_id.eq)
                {
                    /* only for 0 it is true */
                    if(do_jzero(cpu->registers[reg], asmcode->length + 2))
                        ERROR("do_jzero error\n", 1, "");

                    /* always false */
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    /* allign labels */
                    if(label_fake())
                        ERROR("label_fake error\n", 1, "");
                }
                else if(token->r == tokens_id.ne)
                {
                    if(do_jzero_label(cpu->registers[reg], LABEL_FALSE))
                        ERROR("do_jzero_label error\n", 1, "");

                    /* allign labels */
                    if(label_fake())
                        ERROR("label_fake error\n", 1, "");
                }

                REG_SET_FREE(cpu->registers[reg]);
            }
            else
            {
                /* get register for LEFT, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->left, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* set always token->left, this value has set const val ( needed for restore value ) */
                reg_set_val(cpu->registers[reg], token->left);

                /* sync value we need right in memory */
                if(cvar_right->up_to_date == 0)
                    if(do_synchronize(cvar_right->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                /* pump to left const value */
                if(do_pump(cpu->registers[reg], token->left->body.cv->value, FALSE))
                    ERROR("do_pump error\n", 1, "");

                /* use version with left reg right memory */
                if(token->r == tokens_id.lt)
                {
                    if(do_lt1(cpu->registers[reg], token->right))
                        ERROR("do_lt1 error\n", 1, "");
                }
                else if( token->r == tokens_id.le)
                {
                    if(do_le1(cpu->registers[reg], token->right))
                        ERROR("do_le1 error\n", 1, "");
                }
                else if(token->r == tokens_id.gt)
                {
                    if(do_gt1(cpu->registers[reg], token->right))
                        ERROR("do_gt1 error\n", 1, "");
                }
                else if(token->r == tokens_id.ge)
                {
                    if(do_ge1(cpu->registers[reg], token->right))
                        ERROR("do_ge1 error\n", 1, "");
                }
                else if(token->r == tokens_id.eq)
                {
                    if(do_eq1(cpu->registers[reg], token->right))
                        ERROR("do_lt1 error\n", 1, "");
                }
                else if(token->r == tokens_id.ne)
                {
                    if(do_ne1(cpu->registers[reg], token->right))
                        ERROR("do_lt1 error\n", 1, "");
                }

                REG_SET_FREE(cpu->registers[reg]);
            }
        }
    }
    /* only right is const */
    else if(token->right->type == CONST_VAL)
    {
        LOG("Only right is const\n", "");

        /* left is not symbolic so check cond now */
        if(value_can_trace(token->left) && ! value_is_symbolic(cvar_left->body.val))
        {
            LOG("Left is not symbolic\n", "");

            tempval2 = token->right->body.cv->value;

            value_get_val(cvar_left->body.val, val);
            tempval1 = mpz2ull(val);

            mpz_clear(val);

            /* if TRUE DO NOTHIG IF FALSE JUMP LABEL FALSE */
            /* I use NOT N r M to make this more visible */
            if(token->r == tokens_id.lt)
            {
                if( ! (tempval1 < tempval2) )
                {
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    label_fake();
                }
                else
                    cond_fake();
            }
            else if( token->r == tokens_id.le)
            {
                if( ! (tempval1 <= tempval2) )
                {
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    label_fake();
                }
                else
                    cond_fake();
            }
            else if(token->r == tokens_id.gt)
            {
                if( ! (tempval1 > tempval2) )
                {
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    label_fake();
                }
                else
                    cond_fake();
            }
            else if(token->r == tokens_id.ge)
            {
                if( ! (tempval1 >= tempval2) )
                {
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    label_fake();
                }
                else
                    cond_fake();
            }
            else if(token->r == tokens_id.eq)
            {
                if( ! (tempval1 == tempval2) )
                {
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    label_fake();
                }
                else
                    cond_fake();
            }
            else if(token->r == tokens_id.ne)
            {
                if( ! (tempval1 != tempval2) )
                {
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    label_fake();
                }
                else
                    cond_fake();
            }
        }
        else
        {
            LOG("Left is symbolic\n", "");

            if(token->right->body.cv->value == 0ull)
            {
                LOG("CONST = %ju, use jzero instruction\n", token->right->body.cv->value);

                /* get register for LEFT, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->left, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                if(! value_can_trace(token->left)
                    || cpu->registers[reg]->val != cvar_left->body.val)
                    if(do_load(cpu->registers[reg], token->left))
                        ERROR("do_load error\n", 1, "");

                if(token->r == tokens_id.lt)
                {
                    /* always false */
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    /* allign labels */
                    if(label_fake())
                        ERROR("label_fake error\n", 1, "");
                }
                else if( token->r == tokens_id.le)
                {
                    /* only for 0 it is true */
                    if(do_jzero(cpu->registers[reg], asmcode->length + 2))
                        ERROR("do_jzero error\n", 1, "");

                    /* always false */
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    /* allign labels */
                    if(label_fake())
                        ERROR("label_fake error\n", 1, "");
                }
                else if(token->r == tokens_id.gt)
                {
                    /* only 0 is not < a  */
                    if(do_jzero_label(cpu->registers[reg], LABEL_FALSE))
                        ERROR("do_jzero_label error\n", 1, "");

                    /* allign labels */
                    if(label_fake())
                        ERROR("label_fake error\n", 1, "");
                }
                else if(token->r == tokens_id.ge)
                {
                    /* always 0 <= a */
                    if( cond_fake() )
                        ERROR("cond_fake error\n", 1, "");
                }
                else if(token->r == tokens_id.eq)
                {
                    /* only for 0 it is true */
                    if(do_jzero(cpu->registers[reg], asmcode->length + 2))
                        ERROR("do_jzero error\n", 1, "");

                    /* always false */
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    /* allign labels */
                    if(label_fake())
                        ERROR("label_fake error\n", 1, "");
                }
                else if(token->r == tokens_id.ne)
                {
                    /* only 0 is not < a  */
                    if(do_jzero_label(cpu->registers[reg], LABEL_FALSE))
                        ERROR("do_jzero_label error\n", 1, "");

                    /* allign labels */
                    if(label_fake())
                        ERROR("label_fake error\n", 1, "");
                }

                REG_SET_FREE(cpu->registers[reg]);
            }
            else
            {
                /* get register for RIGHT, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->right, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* always set token->right, this value has set const value, needed for restore value */
                reg_set_val(cpu->registers[reg], token->right);

                /* sync value we need left in memory */
                if(cvar_left->up_to_date == 0)
                    if(do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                /* pump to right const value */
                if(do_pump(cpu->registers[reg], token->right->body.cv->value, FALSE))
                    ERROR("do_pump error\n", 1, "");

                /* use version2 with right reg left memory */
                if(token->r == tokens_id.lt)
                {
                    if(do_lt2(cpu->registers[reg], token->left))
                        ERROR("do_lt2 error\n", 1, "");
                }
                else if( token->r == tokens_id.le)
                {
                    if(do_le2(cpu->registers[reg], token->left))
                        ERROR("do_le2 error\n", 1, "");
                }
                else if(token->r == tokens_id.gt)
                {
                    if(do_gt2(cpu->registers[reg], token->left))
                        ERROR("do_gt2 error\n", 1, "");
                }
                else if(token->r == tokens_id.ge)
                {
                    if(do_ge2(cpu->registers[reg], token->left))
                        ERROR("do_ge2 error\n", 1, "");
                }
                else if(token->r == tokens_id.eq)
                {
                    if(do_eq2(cpu->registers[reg], token->left))
                        ERROR("do_lt2 error\n", 1, "");
                }
                else if(token->r == tokens_id.ne)
                {
                    if(do_ne2(cpu->registers[reg], token->left))
                        ERROR("do_lt2 error\n", 1, "");
                }

                REG_SET_FREE(cpu->registers[reg]);
            }
        }
    }
    /* both are variables */
    else
    {
        LOG("Both are vars\n", "");

        /* both are not symbolic so pump value */
        if(    value_can_trace(token->left) && ! value_is_symbolic(cvar_left->body.val)
            && value_can_trace(token->right) && ! value_is_symbolic(cvar_right->body.val) )
        {
            LOG("Both are not symbolic\n", "");

            value_get_val(cvar_left->body.val, val);
            tempval1 = mpz2ull(val);

            mpz_clear(val);

            value_get_val(cvar_right->body.val, val2);
            tempval2 = mpz2ull(val2);

            mpz_clear(val2);

            /* if TRUE DO NOTHIG IF FALSE JUMP LABEL FALSE */
            /* I use NOT N r M to make this more visible */
            if(token->r == tokens_id.lt)
            {
                if( ! (tempval1 < tempval2) )
                {
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    label_fake();
                }
                else
                    cond_fake();
            }
            else if( token->r == tokens_id.le)
            {
                if( ! (tempval1 <= tempval2) )
                {
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    label_fake();
                }
                else
                    cond_fake();
            }
            else if(token->r == tokens_id.gt)
            {
                if( ! (tempval1 > tempval2) )
                {
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    label_fake();
                }
                else
                    cond_fake();
            }
            else if(token->r == tokens_id.ge)
            {
                if( ! (tempval1 >= tempval2) )
                {
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    label_fake();
                }
                else
                    cond_fake();
            }
            else if(token->r == tokens_id.eq)
            {
                if( ! (tempval1 == tempval2) )
                {
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    label_fake();
                }
                else
                    cond_fake();
            }
            else if(token->r == tokens_id.ne)
            {
                if( ! (tempval1 != tempval2) )
                {
                    if(do_jump_label(LABEL_FALSE))
                        ERROR("do_jump_label error\n", 1, "");

                    label_fake();
                }
                else
                    cond_fake();
            }
        }
        else
        {
            LOG("At least one is symbolic\n", "");

            res = better_reg(token_list, token_list_pos, token->left, token->right);

            /* left to reg right to memory */
            if(res <= 0)
            {
                LOG("Load left, Right to memory\n", "");

                /* get register for LEFT, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->left, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* sync left we destroy value */
                if(cvar_left->up_to_date == 0)
                    if( do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                if(! value_can_trace(token->left)
                    || cpu->registers[reg]->val != cvar_left->body.val)
                    if(do_load(cpu->registers[reg], token->left))
                        ERROR("do_load error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* set always token->left, this value has set const value, needed for restore value */
                reg_set_val(cpu->registers[reg], token->left);

                /* we need right in mem */
                if(cvar_right->up_to_date == 0)
                    if( do_synchronize(cvar_right->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                /* use version with left reg right memory */
                if(token->r == tokens_id.lt)
                {
                    if(do_lt1(cpu->registers[reg], token->right))
                        ERROR("do_lt1 error\n", 1, "");
                }
                else if( token->r == tokens_id.le)
                {
                    if(do_le1(cpu->registers[reg], token->right))
                        ERROR("do_le1 error\n", 1, "");
                }
                else if(token->r == tokens_id.gt)
                {
                    if(do_gt1(cpu->registers[reg], token->right))
                        ERROR("do_gt1 error\n", 1, "");
                }
                else if(token->r == tokens_id.ge)
                {
                    if(do_ge1(cpu->registers[reg], token->right))
                        ERROR("do_ge1 error\n", 1, "");
                }
                else if(token->r == tokens_id.eq)
                {
                    if(do_eq1(cpu->registers[reg], token->right))
                        ERROR("do_eq1 error\n", 1, "");
                }
                else if(token->r == tokens_id.ne)
                {
                    if(do_ne1(cpu->registers[reg], token->right))
                        ERROR("do_ne1 error\n", 1, "");
                }

                REG_SET_FREE(cpu->registers[reg]);
            }
            else /* load right to reg left to memory */
            {
                LOG("Load right, left to memory\n", "");

                /* get register for RIGHT, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->right, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* sync right we destroy value */
                if(cvar_right->up_to_date == 0)
                    if( do_synchronize(cvar_right->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                if(! value_can_trace(token->right)
                    || cpu->registers[reg]->val != cvar_right->body.val)
                    if(do_load(cpu->registers[reg], token->right))
                        ERROR("do_load error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* set always token->right, this value has set const value, needed for restore value */
                reg_set_val(cpu->registers[reg], token->right);

                /* we need left in mem */
                if(cvar_left->up_to_date == 0)
                    if( do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                /* use version2 with right reg left memory */
                if(token->r == tokens_id.lt)
                {
                    if(do_lt2(cpu->registers[reg], token->left))
                        ERROR("do_lt2 error\n", 1, "");
                }
                else if( token->r == tokens_id.le)
                {
                    if(do_le2(cpu->registers[reg], token->left))
                        ERROR("do_le2 error\n", 1, "");
                }
                else if(token->r == tokens_id.gt)
                {
                    if(do_gt2(cpu->registers[reg], token->left))
                        ERROR("do_gt2 error\n", 1, "");
                }
                else if(token->r == tokens_id.ge)
                {
                    if(do_ge2(cpu->registers[reg], token->left))
                        ERROR("do_ge2 error\n", 1, "");
                }
                else if(token->r == tokens_id.eq)
                {
                    if(do_eq2(cpu->registers[reg], token->left))
                        ERROR("do_eq2 error\n", 1, "");
                }
                else if(token->r == tokens_id.ne)
                {
                    if(do_ne2(cpu->registers[reg], token->left))
                        ERROR("do_ne2 error\n", 1, "");
                }

                REG_SET_FREE(cpu->registers[reg]);
            }
        }
    }

    return 0;
}

static int __assign(token_assign *token)
{
    int reg;

    Cvar *cvar_res;
    Cvar *cvar_left;

    mpz_t val;
    mpz_t val2;

    TRACE("");

    cvar_res = cvar_get_by_value(token->res);
    cvar_left = cvar_get_by_value(token->expr->left);

    if(token->expr->left->type == CONST_VAL)
    {
        LOG("Left is const\n", "");

        /* get register for operation, synchronize iff needed  */
        reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
        if(reg == -1)
            ERROR("do_get_register error\n", 1, "");

        /* lock reg */
        REG_SET_IN_USE(cpu->registers[reg]);

        /* set value  */
        if( ! value_can_trace(token->res))
            reg_set_val(cpu->registers[reg], token->res);
        else
            reg_set_val(cpu->registers[reg], cvar_res->body.val);

        /* pump value to reg */
        if( do_pump(cpu->registers[reg], token->expr->left->body.cv->value, TRUE) )
            ERROR("do_pump error\n", 1, "");

        /* we can't trace value so imediatly store it */
        if(! value_can_trace(token->res) )
        {
            if(do_store(cpu->registers[reg]))
                ERROR("do_store error\n", 1, "");

            /* free reg */
            REG_SET_FREE(cpu->registers[reg]);
        }
        else
        {
            REG_SET_BUSY(cpu->registers[reg]);

            cvar_res->up_to_date = 0;
        }
    }
    else /* variable */
    {
        /* normal var */
        if(token->expr->left->body.var->type == VAR_NORMAL)
        {
            LOG("left is var_normal\n", "");

            /* we can pump value */
            if( ! value_is_symbolic(cvar_left->body.val))
            {
                LOG("lef is not symbolic\n", "");

                value_get_val(cvar_left->body.val, val2);

                LOG("COST %d vs %d\n", BPUMP_COST(val2), op_cost.load + op_cost.store );
                if(BPUMP_COST(val2) <= op_cost.load + op_cost.store)
                {
                    LOG("BETTER PUMP\n", "");

                    /* get register for operation, synchronize iff needed  */
                    reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                    if(reg == -1)
                        ERROR("do_get_register error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg]);

                    /* set value  */
                    if( ! value_can_trace(token->res))
                        reg_set_val(cpu->registers[reg], token->res);
                    else
                        reg_set_val(cpu->registers[reg], cvar_res->body.val);

                    /* get left value */
                    value_get_val(cvar_left->body.val, val);

                    /* pump value to reg */
                    if( do_pump_bigvalue(cpu->registers[reg], val, TRUE) )
                        ERROR("do_pump error\n", 1, "");

                    mpz_clear(val);

                    /* we can't trace value so immediatly store it */
                    if(! value_can_trace(token->res) )
                    {
                        if(do_store(cpu->registers[reg]))
                            ERROR("do_store error\n", 1, "");

                        /* free reg */
                        REG_SET_FREE(cpu->registers[reg]);
                    }
                    else
                    {
                        REG_SET_BUSY(cpu->registers[reg]);
                        cvar_res->up_to_date = 0;
                    }
                }
                else
                {
                    LOG("BETTER LOAD\n", "");

                    /* get register for operation, synchronize iff needed  */
                    reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                    if(reg == -1)
                        ERROR("do_get_register error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg]);

                    /* we want to copy from mem to reg, so synchronize with memory */
                    if(cvar_left->up_to_date == 0)
                        if(do_synchronize(cvar_left->body.val->reg))
                            ERROR("do_synchronize error\n", 1, "");

                    if(! value_can_trace(token->expr->left)
                        || cpu->registers[reg]->val != cvar_left->body.val)
                            if(do_load(cpu->registers[reg], token->expr->left))
                                ERROR("do_load error\n", 1, "");

                    /* get left value */
                    value_get_val(cvar_left->body.val, val);

                    /* set value  */
                    if( ! value_can_trace(token->res))
                        reg_set_val(cpu->registers[reg], token->res);
                    else
                    {
                        value_set_val(cvar_res->body.val, val);
                        reg_set_val(cpu->registers[reg], cvar_res->body.val);
                    }

                    mpz_clear(val);

                    /* we can't trace value so immediatly store it */
                    if(! value_can_trace(token->res) )
                    {
                        if(do_store(cpu->registers[reg]))
                            ERROR("do_store error\n", 1, "");

                        /* free reg */
                        REG_SET_FREE(cpu->registers[reg]);
                    }
                    else
                    {
                        REG_SET_BUSY(cpu->registers[reg]);
                        cvar_res->up_to_date = 0;
                    }
                }

                mpz_clear(val2);
            }
            else /* value is symbolic copy val from memory */
            {
                LOG("Left is symbolic\n", "");

                /* get register for operation, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* we want to copy from mem to reg, so synchronize with memory */
                if(cvar_left->up_to_date == 0)
                    if(do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                if(! value_can_trace(token->expr->left)
                    || cpu->registers[reg]->val != cvar_left->body.val)
                        if(do_load(cpu->registers[reg], token->expr->left))
                            ERROR("do_load error\n", 1, "");

                /* set value  */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);

                /* we can't trace value so immediatly store it */
                if(! value_can_trace(token->res) )
                {
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
        }
        else /* array */
        {
            LOG("Left is array\n", "");

            /* get register for operation, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            /* lock reg */
            REG_SET_IN_USE(cpu->registers[reg]);

            if(do_load(cpu->registers[reg], token->expr->left))
                ERROR("do_load error\n", 1, "");

            /* set value  */
            if( ! value_can_trace(token->res))
                reg_set_val(cpu->registers[reg], token->res);
            else
                reg_set_val(cpu->registers[reg], cvar_res->body.val);

            /* we can't trace value so immediatly store it */
            if(! value_can_trace(token->res) )
            {
                if(do_store(cpu->registers[reg]))
                    ERROR("do_store error\n", 1, "");

                /* free reg */
                REG_SET_FREE(cpu->registers[reg]);
            }
            else
            {
                REG_SET_BUSY(cpu->registers[reg]);
                cvar_res->up_to_date = 0;
            }
        }
    }

    return 0;
}

static int __add(token_assign *token)
{
    Cvar *cvar_res;
    Cvar *cvar_left;
    Cvar *cvar_right;

    int res;
    int reg;
    int i;

    mpz_t val;

    mpz_t temp1;
    mpz_t temp2;

    TRACE("");

    cvar_res = cvar_get_by_value(token->res);
    cvar_left = cvar_get_by_value(token->expr->left);
    cvar_right = cvar_get_by_value(token->expr->right);

    /* a = N + M, pump to a N + M */
    if(token->expr->left->type == CONST_VAL && token->expr->right->type == CONST_VAL)
    {
        LOG("Both are const\n", "");

        mpz_init(val);
        mpz_init(temp1);
        mpz_init(temp2);

        ull2mpz(temp1, token->expr->left->body.cv->value);
        ull2mpz(temp2, token->expr->right->body.cv->value);

        mpz_add(val, temp1, temp2);

        /* get register for operation, synchronize iff needed  */
        reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
        if(reg == -1)
            ERROR("do_get_register error\n", 1, "");

        /* lock reg */
        REG_SET_IN_USE(cpu->registers[reg]);

        /* set value  */
        if( ! value_can_trace(token->res) )
            reg_set_val(cpu->registers[reg], token->res);
        else
            reg_set_val(cpu->registers[reg], cvar_res->body.val);

        if( do_pump_bigvalue(cpu->registers[reg], val, TRUE) )
            ERROR("do_pump_bigvalue error\n", 1, "");

        /* we can't trace value so immediatly store it */
        if( ! value_can_trace(token->res) )
        {
            if( do_store(cpu->registers[reg]) )
                ERROR("do_store error\n", 1, "");

            /* free reg */
            REG_SET_FREE(cpu->registers[reg]);
        }
        else
        {
            REG_SET_BUSY(cpu->registers[reg]);
            cvar_res->up_to_date = 0;
        }

        mpz_clear(val);
        mpz_clear(temp1);
        mpz_clear(temp2);
    }
    /* only left is const */
    else if(token->expr->left->type == CONST_VAL)
    {
        LOG("Only left is const\n", "");

        /* right is not symbolic so can pump value */
        if(value_can_trace(token->expr->right) && ! value_is_symbolic(cvar_right->body.val))
        {
            LOG("Right is not symbolic\n", "");

            /* calc value to pump */
            mpz_init(val);
            mpz_init(temp1);

            ull2mpz(temp1, token->expr->left->body.cv->value);
            value_get_val(cvar_right->body.val, temp2);

            mpz_add(val, temp1, temp2);

            /* get register for operation, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, token->expr->right, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            if(cpu->registers[reg] == cvar_right->body.val->reg &&
                (cvar_right->up_to_date == 1 || ( value_can_trace(token->res) && cvar_right->body.val == cvar_res->body.val )) &&
                    token->expr->left->body.cv->value * op_cost.inc <
                        ((op_cost.add + BPUMP_COST(val))) )
            {
                LOG("BETTER INC\n", "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                /* can we trace new value ? */
                if(value_can_trace(token->res) && value_can_trace(token->expr->right))
                {
                    mpz_clear(val);
                    value_get_val(cvar_right->body.val, val);

                    value_set_val(cvar_res->body.val, val);

                }

                i = token->expr->left->body.cv->value;

                while(i)
                {
                    /* temporary INC */
                    if( do_inc(cpu->registers[reg], TRUE) )
                        ERROR("do_inc error\n", 1, "");

                    --i;
                }

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else
            {
                LOG("BETTER PUMP\n", "");

                /* get register for operation, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* set value */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);

                if(do_pump_bigvalue(cpu->registers[reg], val, TRUE))
                    ERROR("do_pump_bigvalue error\n", 1, "");

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    cvar_res->up_to_date = 0;
                    REG_SET_BUSY(cpu->registers[reg]);
                }
            }

            mpz_clear(val);
            mpz_clear(temp1);
            mpz_clear(temp2);
        }
        else
        {
            LOG("Right is symbolic\n", "");
            LOG("COST: %ju vs %d\n", token->expr->left->body.cv->value * op_cost.inc,
                ((op_cost.add + PUMP_COST(token->expr->left->body.cv->value))) );

            /* It't better to INC few times */
            if( token->expr->left->body.cv->value * op_cost.inc <
                    ((op_cost.add + PUMP_COST(token->expr->left->body.cv->value))) )
            {
                LOG("BETTER INC\n", "");

                /* get register for operation, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->expr->right, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                if(cvar_right->up_to_date == 0 &&
                    ( value_can_trace(token->expr->right) && cvar_res->body.val != cvar_right->body.val ))
                    if(do_synchronize(cvar_right->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                /* we have to load  */
                if(! value_can_trace(token->expr->right)
                    || cpu->registers[reg]->val != cvar_right->body.val)
                    if(do_load(cpu->registers[reg], token->expr->right))
                        ERROR("do_load error\n", 1, "");

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                /* can we trace new value ? */
                if(value_can_trace(token->res) && value_can_trace(token->expr->right))
                {
                    value_get_val(cvar_right->body.val, val);

                    value_set_val(cvar_res->body.val, val);

                    mpz_clear(val);
                }

                i = token->expr->left->body.cv->value;

                while(i)
                {
                    /* temporary INC */
                    if( do_inc(cpu->registers[reg], TRUE) )
                        ERROR("do_inc error\n", 1, "");

                    --i;
                }

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else /* it's better to add */
            {
                LOG("BETTER ADD\n", "");

                /* get register for TEMP, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->expr->left, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                if(cvar_right->up_to_date == 0)
                    if(do_synchronize(cvar_right->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                if(do_pump(cpu->registers[reg], token->expr->left->body.cv->value, FALSE))
                    ERROR("do_pump error\n", 1, "");

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                /* R0 = right.addr, so RN = N, we can ADD RN = RN + Right */
                if(do_set_val_addr(cpu->registers[REG_PTR], token->expr->right, TRUE))
                    ERROR("do_set_val_addr error\n", 1, "");

                /* do add */
                if(do_add(cpu->registers[reg], FALSE))
                    ERROR("do_add error\n", 1, "");

                /* can we trace new value ? */
                if(value_can_trace(token->res) && value_can_trace(token->expr->right))
                {
                    value_get_val(cvar_right->body.val, val);

                    mpz_add_ui(val, val, token->expr->left->body.cv->value);

                    value_set_val(cvar_res->body.val, val);

                    mpz_clear(val);
                }

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
        }
    }
    /* only right is const */
    else if(token->expr->right->type == CONST_VAL)
    {
        LOG("Only right is const\n", "");

        /* left is not symbolic so can pump value */
        if(value_can_trace(token->expr->left) && ! value_is_symbolic(cvar_left->body.val))
        {
            LOG("Left is not symbolic\n", "");

            mpz_init(val);
            mpz_init(temp1);

            ull2mpz(temp1, token->expr->right->body.cv->value);
            value_get_val(cvar_left->body.val, temp2);

            mpz_add(val, temp1, temp2);

            /* get register for operation, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, token->expr->left, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");


            if(cpu->registers[reg] == cvar_left->body.val->reg &&
                (cvar_left->up_to_date == 1 || (value_can_trace(token->res) && cvar_res->body.val == cvar_left->body.val )) &&
                token->expr->right->body.cv->value * op_cost.inc <
                        ((op_cost.add + BPUMP_COST(val)) ))
            {
                LOG("BETTER INC\n", "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* set val or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                /* can we trace new value ? */
                if(value_can_trace(token->res) && value_can_trace(token->expr->left) )
                {
                    mpz_clear(val);

                    value_get_val(cvar_left->body.val, val);

                    value_set_val(cvar_res->body.val, val);
                }

                i = token->expr->right->body.cv->value;

                while(i)
                {
                    /* temporary INC */
                    if( do_inc(cpu->registers[reg], TRUE) )
                        ERROR("do_inc error\n", 1, "");

                    --i;
                }

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else
            {
                LOG("BETTER PUMP\n", "");

                /* get register for operation, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* set value  */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);

                if(do_pump_bigvalue(cpu->registers[reg], val, TRUE))
                    ERROR("do_pump_bigvalue error\n", 1, "");

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    cvar_res->up_to_date = 0;
                    REG_SET_BUSY(cpu->registers[reg]);
                }
            }

            mpz_clear(val);
            mpz_clear(temp1);
            mpz_clear(temp2);
        }
        else
        {
            LOG("Left is symbolic\n", "");
            LOG("COST: %ju vs %d\n", token->expr->right->body.cv->value * op_cost.inc,
                    ((op_cost.add + PUMP_COST(token->expr->right->body.cv->value))) );
            /* It't better to INC few times */
            if( token->expr->right->body.cv->value * op_cost.inc <
                    ((op_cost.add + PUMP_COST(token->expr->right->body.cv->value))) )
            {
                LOG("BETTER INC\n", "");

                /* get register for operation, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->expr->left, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                if(cvar_left->up_to_date == 0 &&
                    ( value_can_trace(token->expr->left) && cvar_res->body.val != cvar_left->body.val ))
                    if( do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                /* we have to load  */
                if(! value_can_trace(token->expr->left)
                    || cpu->registers[reg]->val != cvar_left->body.val)
                    if(do_load(cpu->registers[reg], token->expr->left))
                        ERROR("do_load error\n", 1, "");

                /* set val or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                /* can we trace new value ? */
                if(value_can_trace(token->res) && value_can_trace(token->expr->left) )
                {
                    value_get_val(cvar_left->body.val, val);

                    value_set_val(cvar_res->body.val, val);

                    mpz_clear(val);
                }

                i = token->expr->right->body.cv->value;

                while(i)
                {
                    /* temporary INC */
                    if( do_inc(cpu->registers[reg], TRUE) )
                        ERROR("do_inc error\n", 1, "");

                    --i;
                }

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    cvar_res->up_to_date = 0;
                    REG_SET_BUSY(cpu->registers[reg]);
                }

            }
            else /* it's better to add */
            {
                LOG("BETTER ADD\n", "");

                /* get register for TEMP, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->expr->right, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                if(cvar_left->up_to_date == 0)
                    if( do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                /* do not trace it, it is temporary value */
                if(do_pump(cpu->registers[reg], token->expr->right->body.cv->value, FALSE))
                    ERROR("do_pump error\n", 1, "");

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                /* R0 = left.addr, so RN = N, we can ADD RN = RN + Left */
                if(do_set_val_addr(cpu->registers[REG_PTR], token->expr->left, TRUE))
                    ERROR("do_set_val_addr error\n", 1, "");

                /* do add */
                if(do_add(cpu->registers[reg], FALSE))
                    ERROR("do_add error\n", 1, "");


                /* can we trace new value ? */
                if(value_can_trace(token->res) && value_can_trace(token->expr->left))
                {
                    value_get_val(cvar_left->body.val, val);

                    mpz_add_ui(val, val, token->expr->right->body.cv->value);

                    value_set_val(cvar_res->body.val, val);

                    mpz_clear(val);
                }

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
        }
    }
    /* both are variables */
    else
    {
        LOG("Both are vars\n", "");

        /* both are not symbolic so pump value */
        if(    value_can_trace(token->expr->left) && ! value_is_symbolic(cvar_left->body.val)
            && value_can_trace(token->expr->right) && ! value_is_symbolic(cvar_right->body.val) )
        {

            LOG("Both are not symbolic\n", "");

            mpz_init(val);

            /* calc value to pump */
            value_get_val(cvar_left->body.val, temp1);
            value_get_val(cvar_right->body.val, temp2);

            mpz_add(val, temp1, temp2);

            /* get register for operation, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            /* lock reg */
            REG_SET_IN_USE(cpu->registers[reg]);

            /* set value  */
            if( ! value_can_trace(token->res) )
                reg_set_val(cpu->registers[reg], token->res);
            else
                reg_set_val(cpu->registers[reg], cvar_res->body.val);

            if( do_pump_bigvalue(cpu->registers[reg], val, TRUE) )
                ERROR("do_pump_bigvalue error\n", 1, "");

            /* we can't trace value so immediatly store it */
            if( ! value_can_trace(token->res) )
            {
                if( do_store(cpu->registers[reg]) )
                    ERROR("do_store error\n", 1, "");

                /* free reg */
                REG_SET_FREE(cpu->registers[reg]);
            }
            else
            {
                REG_SET_BUSY(cpu->registers[reg]);
                cvar_res->up_to_date = 0;
            }

            mpz_clear(val);
            mpz_clear(temp1);
            mpz_clear(temp2);
        }
        else
        {
            LOG("At least one is symbolic\n", "");

            res = better_reg(token_list, token_list_pos, token->expr->left, token->expr->right);

            /* left to reg */
            if(res <= 0)
            {
                LOG("Load left, Right to memory\n", "");

                /* get register for TEMP, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->expr->left, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                if(cvar_right->up_to_date == 0)
                    if(do_synchronize(cvar_right->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                if(cvar_left->up_to_date == 0 &&
                ( value_can_trace(token->expr->left) && cvar_res->body.val != cvar_left->body.val ))
                    if(do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                if(! value_can_trace(token->expr->left)
                    || cpu->registers[reg]->val != cvar_left->body.val)
                    if(do_load(cpu->registers[reg], token->expr->left))
                        ERROR("do_load error\n", 1, "");

                /* set value  */
                if( ! value_can_trace(token->res) )
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);

                /* both vars are the same so a + a ==> a << 1 */
                if( variable_cmp((void*)&token->expr->left->body.var,
                                (void*)&token->expr->right->body.var) == 0)
                {
                    if(do_shl(cpu->registers[reg], FALSE) )
                        ERROR("do_shl error\n", 1, "");
                }
                else
                {
                    /* R0 = right.addr, Rn = load left */
                    if(do_set_val_addr(cpu->registers[REG_PTR], token->expr->right, TRUE))
                        ERROR("do_set_val_addr error\n", 1, "");

                    /* do add */
                    if(do_add(cpu->registers[reg], FALSE))
                        ERROR("do_add error\n", 1, "");
                }
            }
            else /* load right */
            {
                LOG("Load right, left to memory\n", "");

                /* get register for TEMP, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->expr->right, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                if(cvar_left->up_to_date == 0)
                    if(do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                if(cvar_right->up_to_date == 0 &&
                ( value_can_trace(token->expr->right) && cvar_res->body.val != cvar_right->body.val ))
                    if(do_synchronize(cvar_right->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                if(! value_can_trace(token->expr->right)
                    || cpu->registers[reg]->val != cvar_right->body.val)
                    if(do_load(cpu->registers[reg], token->expr->right))
                        ERROR("do_load error\n", 1, "");

                /* set value  */
                if( ! value_can_trace(token->res) )
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);

                /* both vars are the same so a + a ==> a << 1 */
                if( variable_cmp((void*)&token->expr->left->body.var,
                                (void*)&token->expr->right->body.var) == 0)
                {
                    if(do_shl(cpu->registers[reg], FALSE) )
                        ERROR("do_shl error\n", 1, "");
                }
                else
                {
                    /* R0 = left.addr, Rn = right left */
                    if(do_set_val_addr(cpu->registers[REG_PTR], token->expr->left, TRUE))
                        ERROR("do_set_val_addr error\n", 1, "");

                    /* do add */
                    if(do_add(cpu->registers[reg], FALSE))
                        ERROR("do_add error\n", 1, "");
                }
            }

            if(!value_can_trace(token->res))
            {
                /* STORE val in res */
                if(do_store(cpu->registers[reg]))
                    ERROR("do_store error\n", 1, "");

                /* free reg */
                REG_SET_FREE(cpu->registers[reg]);
            }
            else
            {
                REG_SET_BUSY(cpu->registers[reg]);
                cvar_res->up_to_date = 0;
            }
        }
    }
    return 0;
}

static int __sub(token_assign *token)
{
    Cvar *cvar_res;
    Cvar *cvar_left;
    Cvar *cvar_right;
    Cvar *cvar_temp;

    int reg;
    int reg2;

    int i;

    mpz_t val;

    mpz_t temp1;
    mpz_t temp2;

    uint64_t subres;

    TRACE("");

    cvar_res = cvar_get_by_value(token->res);
    cvar_left = cvar_get_by_value(token->expr->left);
    cvar_right = cvar_get_by_value(token->expr->right);

    /* a = N - M, pump to a MAX(0, N - M )*/
    if(token->expr->left->type == CONST_VAL && token->expr->right->type == CONST_VAL)
    {
        LOG("Both are const\n", "");

        if(token->expr->left->body.cv->value < token->expr->right->body.cv->value)
            subres = 0;
        else
            subres = token->expr->left->body.cv->value - token->expr->right->body.cv->value;

        /* get register for operation, synchronize iff needed  */
        reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
        if(reg == -1)
            ERROR("do_get_register error\n", 1, "");

        /* lock reg */
        REG_SET_IN_USE(cpu->registers[reg]);

        /* set value  */
        if( ! value_can_trace(token->res) )
            reg_set_val(cpu->registers[reg], token->res);
        else
            reg_set_val(cpu->registers[reg], cvar_res->body.val);

        if( do_pump(cpu->registers[reg], subres, TRUE) )
            ERROR("do_pump_bigvalue error\n", 1, "");

        /* we can't trace value so immediatly store it */
        if( ! value_can_trace(token->res) )
        {
            if( do_store(cpu->registers[reg]) )
                ERROR("do_store error\n", 1, "");

            /* free reg */
            REG_SET_FREE(cpu->registers[reg]);
        }
        else
        {
            REG_SET_BUSY(cpu->registers[reg]);
            cvar_res->up_to_date = 0;
        }
    }
    /* only left is const */
    else if(token->expr->left->type == CONST_VAL)
    {
        LOG("Only left is const\n", "");

        /* right is not symbolic so can pump value */
        if(value_can_trace(token->expr->right) && ! value_is_symbolic(cvar_right->body.val))
        {
            LOG("Right is not symbolic\n", "");

            /* calc value to pump */
            mpz_init(val);
            mpz_init(temp1);

            ull2mpz(temp1, token->expr->left->body.cv->value);
            value_get_val(cvar_right->body.val, temp2);

            if(mpz_cmp(temp1, temp2) < 0)
                mpz_set_ui(val, 0);
            else
                mpz_sub(val, temp1, temp2);

            /* get register for operation, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            /* lock reg */
            REG_SET_IN_USE(cpu->registers[reg]);

            /* set value  */
            if( ! value_can_trace(token->res))
                reg_set_val(cpu->registers[reg], token->res);
            else
                reg_set_val(cpu->registers[reg], cvar_res->body.val);

            if(do_pump_bigvalue(cpu->registers[reg], val, TRUE))
                ERROR("do_pump_bigvalue error\n", 1, "");

            /* we can't trace value so immediatly store it */
            if(! value_can_trace(token->res) )
            {
                if(do_store(cpu->registers[reg]))
                    ERROR("do_store error\n", 1, "");

                /* free reg */
                REG_SET_FREE(cpu->registers[reg]);
            }
            else
            {
                REG_SET_BUSY(cpu->registers[reg]);
                cvar_res->up_to_date = 0;
            }

            mpz_clear(val);
            mpz_clear(temp1);
            mpz_clear(temp2);
        }
        else
        {
            LOG("Right is symbolic\n", "");

            /* get register for TEMP, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, token->expr->left, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            if(cvar_right->up_to_date == 0)
                if(do_synchronize(cvar_right->body.val->reg))
                    ERROR("do_synchronize error\n", 1, "");

            /* lock reg */
            REG_SET_IN_USE(cpu->registers[reg]);

            if(do_pump(cpu->registers[reg], token->expr->left->body.cv->value, FALSE))
                ERROR("do_pump error\n", 1, "");

            /* can we trace or not ? */
            if(value_can_trace(token->res))
                reg_set_val(cpu->registers[reg], cvar_res->body.val);
            else
                reg_set_val(cpu->registers[reg], token->res);

            /* R0 = right.addr, so RN = N, we can SUB RN = RN - Right */
            if(do_set_val_addr(cpu->registers[REG_PTR], token->expr->right, TRUE))
                ERROR("do_set_val_addr error\n", 1, "");

            /* do sub */
            if(do_sub(cpu->registers[reg], FALSE))
                ERROR("do_sub error\n", 1, "");

            /* can we trace new value ? */
            if(value_can_trace(token->res) && value_can_trace(token->expr->right))
            {
                value_get_val(cvar_right->body.val, val);

                mpz_sub_ui(val, val, token->expr->left->body.cv->value);
                if(mpz_cmp_ui(val, 0) < 0)
                    mpz_set_ui(val, 0);

                value_set_val(cvar_res->body.val, val);

                mpz_clear(val);
            }

            if(!value_can_trace(token->res))
            {
                /* STORE val in res */
                if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                /* free reg */
                REG_SET_FREE(cpu->registers[reg]);
            }
            else
            {
                REG_SET_BUSY(cpu->registers[reg]);
                cvar_res->up_to_date = 0;
            }
        }
    }
    /* only right is const */
    else if(token->expr->right->type == CONST_VAL)
    {
        LOG("Only right is const\n", "");

        /* left is not symbolic so can pump value */
        if(value_can_trace(token->expr->left) && ! value_is_symbolic(cvar_left->body.val))
        {
            LOG("Left is not symbolic\n", "");

            mpz_init(val);
            mpz_init(temp2);

            ull2mpz(temp2, token->expr->right->body.cv->value);
            value_get_val(cvar_left->body.val, temp1);

            if(mpz_cmp(temp1, temp2) < 0)
                mpz_set_ui(val, 0);
            else
                mpz_sub(val, temp1, temp2);

            /* get register for operation, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, token->expr->left, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            if(cpu->registers[reg] == cvar_left->body.val->reg &&
                (cvar_left->up_to_date == 1 || (value_can_trace(token->res) && cvar_res->body.val == cvar_left->body.val )) &&
                token->expr->right->body.cv->value * op_cost.dec <
                        (PUMP_COST(token->expr->right->body.cv->value)) )
            {
                LOG("BETTER DEC\n", "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* set val or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                /* can we trace new value ? */
                if(value_can_trace(token->res) && value_can_trace(token->expr->left) )
                {
                    mpz_clear(val);

                    value_get_val(cvar_left->body.val, val);

                    value_set_val(cvar_res->body.val, val);
                }

                i = token->expr->right->body.cv->value;

                while(i)
                {
                    /* temporary DEC */
                    if( do_dec(cpu->registers[reg], TRUE) )
                        ERROR("do_inc error\n", 1, "");

                    --i;
                }

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else
            {
                LOG("BETTER PUMP\n", "");

                /* get register for operation, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* set value  */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);

                if(do_pump_bigvalue(cpu->registers[reg], val, TRUE))
                    ERROR("do_pump_bigvalue error\n", 1, "");

                /* we can't trace value so immediatly store it */
                if(! value_can_trace(token->res) )
                {
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }

            mpz_clear(val);
            mpz_clear(temp1);
            mpz_clear(temp2);
        }
        else
        {
            LOG("Left is symbolic\n", "");

            LOG("COST: %ju vs %d\n", token->expr->right->body.cv->value * op_cost.dec,
                    ((op_cost.sub + PUMP_COST(token->expr->right->body.cv->value) + (op_cost.store << 1))) );

            /* It't better to DEC few times */
            if( token->expr->right->body.cv->value * op_cost.dec <
                    ((op_cost.sub + PUMP_COST(token->expr->right->body.cv->value) + (op_cost.store << 1))) )
            {
                LOG("BETTER DEC\n", "");

                /* get register for operation, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->expr->left, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                if(cvar_left->up_to_date == 0 &&
                ( value_can_trace(token->expr->left) && cvar_res->body.val != cvar_left->body.val ))
                    if( do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                /* we have to load  */
                if(! value_can_trace(token->expr->left)
                    || cpu->registers[reg]->val != cvar_left->body.val)
                    if(do_load(cpu->registers[reg], token->expr->left))
                        ERROR("do_load error\n", 1, "");

                /* set val or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                /* can we trace new value ? */
                if(value_can_trace(token->res) && value_can_trace(token->expr->left) )
                {
                    value_get_val(cvar_left->body.val, val);

                    value_set_val(cvar_res->body.val, val);

                    mpz_clear(val);
                }

                i = token->expr->right->body.cv->value;

                while(i)
                {
                    /* temporary DEC */
                    if( do_dec(cpu->registers[reg], TRUE) )
                        ERROR("do_inc error\n", 1, "");

                    --i;
                }

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    cvar_res->up_to_date = 0;
                    REG_SET_BUSY(cpu->registers[reg]);
                }
            }
            else
            {
                /* get register for TEMP, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->expr->right, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                REG_SET_IN_USE(cpu->registers[reg]);

                cvar_temp = cvar_get_by_name(TEMP1_NAME);
                reg_set_val(cpu->registers[reg], cvar_temp->body.val);

                /* get register for LEFT, synchronize iff needed  */
                reg2 = do_get_register(token_list, token_list_pos, token->expr->left, FALSE);
                if(reg2 == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg2]);

                if(cvar_left->up_to_date == 0 &&
                ( value_can_trace(token->expr->left) && cvar_res->body.val != cvar_left->body.val ))
                    if( do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                if(! value_can_trace(token->expr->left)
                    || cpu->registers[reg2]->val != cvar_left->body.val)
                    if(do_load(cpu->registers[reg2], token->expr->left))
                        ERROR("do_load error\n", 1, "");

                /* do not trace it, it is temporary value */
                if(do_pump(cpu->registers[reg], token->expr->right->body.cv->value, FALSE))
                    ERROR("do_pump error\n", 1, "");

                if(do_store(cpu->registers[reg]))
                    ERROR("do_store error\n", 1, "");

                REG_SET_FREE(cpu->registers[reg]);

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg2], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg2], token->res);

                /* do sub */
                if(do_sub(cpu->registers[reg2], FALSE))
                    ERROR("do_sub error\n", 1, "");

                /* can we trace new value ? */
                if(value_can_trace(token->res) && value_can_trace(token->expr->left))
                {
                    value_get_val(cvar_left->body.val, val);

                    mpz_sub_ui(val, val, token->expr->right->body.cv->value);
                    if(mpz_cmp_ui(val, 0) < 0)
                        mpz_set_ui(val, 0);

                    value_set_val(cvar_res->body.val, val);

                    mpz_clear(val);
                }

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg2]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg2]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg2]);
                    cvar_res->up_to_date = 0;
                }
            }
        }
    }
    else
    {
        LOG("At least one is symbolic\n", "");

        /* both are not symbolic so pump value */
        if(    value_can_trace(token->expr->left) && ! value_is_symbolic(cvar_left->body.val)
            && value_can_trace(token->expr->right) && ! value_is_symbolic(cvar_right->body.val) )
        {

            LOG("Both are not symbolic\n", "");

            mpz_init(val);

            /* calc value to pump */
            value_get_val(cvar_left->body.val, temp1);
            value_get_val(cvar_right->body.val, temp2);

            if(mpz_cmp(temp1, temp2) < 0)
                mpz_set_ui(val, 0);
            else
                mpz_sub(val, temp1, temp2);

            /* get register for operation, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            /* lock reg */
            REG_SET_IN_USE(cpu->registers[reg]);

            /* set value  */
            if( ! value_can_trace(token->res) )
                reg_set_val(cpu->registers[reg], token->res);
            else
                reg_set_val(cpu->registers[reg], cvar_res->body.val);

            if( do_pump_bigvalue(cpu->registers[reg], val, TRUE) )
                ERROR("do_pump_bigvalue error\n", 1, "");

            /* we can't trace value so immediatly store it */
            if( ! value_can_trace(token->res) )
            {
                if( do_store(cpu->registers[reg]) )
                    ERROR("do_store error\n", 1, "");

                /* free reg */
                REG_SET_FREE(cpu->registers[reg]);
            }
            else
            {
                REG_SET_BUSY(cpu->registers[reg]);
                cvar_res->up_to_date = 0;
            }

            mpz_clear(val);
            mpz_clear(temp1);
            mpz_clear(temp2);
        }
        else
        {
            /* get register for TEMP, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, token->expr->left, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            if(cvar_right->up_to_date == 0)
                if(do_synchronize(cvar_right->body.val->reg))
                    ERROR("do_synchronize error\n", 1, "");

            if(cvar_left->up_to_date == 0 &&
            ( value_can_trace(token->expr->left) && cvar_res->body.val != cvar_left->body.val ))
                if(do_synchronize(cvar_left->body.val->reg))
                    ERROR("do_synchronize error\n", 1, "");

            /* lock reg */
            REG_SET_IN_USE(cpu->registers[reg]);

            if(! value_can_trace(token->expr->left)
                || cpu->registers[reg]->val != cvar_left->body.val)
                if(do_load(cpu->registers[reg], token->expr->left))
                    ERROR("do_load error\n", 1, "");

            /* set value  */
            if( ! value_can_trace(token->res) )
                reg_set_val(cpu->registers[reg], token->res);
            else
                reg_set_val(cpu->registers[reg], cvar_res->body.val);

            if(variable_cmp((void*)&token->expr->left->body.var,
                (void*)&token->expr->right->body.var) == 0)
            {
                if(do_zero(cpu->registers[reg], FALSE))
                    ERROR("do_zero error\n", 1, "");
            }
            else
            {
                /* R0 = right.addr, Rn = load left */
                if(do_set_val_addr(cpu->registers[REG_PTR], token->expr->right, TRUE))
                    ERROR("do_set_val_addr error\n", 1, "");

                /* do sub */
                if(do_sub(cpu->registers[reg], FALSE))
                    ERROR("do_sub error\n", 1, "");
            }

            if(!value_can_trace(token->res))
            {
                /* STORE val in res */
                if(do_store(cpu->registers[reg]))
                    ERROR("do_store error\n", 1, "");

                /* free reg */
                REG_SET_FREE(cpu->registers[reg]);
            }
            else
            {
                REG_SET_BUSY(cpu->registers[reg]);
                cvar_res->up_to_date = 0;
            }
        }
    }

    return 0;
}

static int __mult(token_assign *token)
{
    Cvar *cvar_res;
    Cvar *cvar_left;
    Cvar *cvar_right;
    Cvar *cvar_temp;
    Cvar *cvar_temp2;

    int reg;
    int reg2;
    int reg3;

    int i;
    uint64_t value;

    mpz_t val;

    mpz_t temp1;
    mpz_t temp2;

    TRACE("");

    cvar_res = cvar_get_by_value(token->res);
    cvar_left = cvar_get_by_value(token->expr->left);
    cvar_right = cvar_get_by_value(token->expr->right);
    cvar_temp = cvar_get_by_name(TEMP1_NAME);
    cvar_temp2 = cvar_get_by_name(TEMP2_NAME);

    /* a = N * M, pump to a N * M */
    if(token->expr->left->type == CONST_VAL && token->expr->right->type == CONST_VAL)
    {
        LOG("Both are const\n", "");

        mpz_init(val);
        mpz_init(temp1);
        mpz_init(temp2);

        ull2mpz(temp1, token->expr->left->body.cv->value);
        ull2mpz(temp2, token->expr->right->body.cv->value);

        mpz_mul(val, temp1, temp2);

        /* get register for operation, synchronize iff needed  */
        reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
        if(reg == -1)
            ERROR("do_get_register error\n", 1, "");

        /* lock reg */
        REG_SET_IN_USE(cpu->registers[reg]);

        /* set value  */
        if( ! value_can_trace(token->res) )
            reg_set_val(cpu->registers[reg], token->res);
        else
            reg_set_val(cpu->registers[reg], cvar_res->body.val);

        if( do_pump_bigvalue(cpu->registers[reg], val, TRUE) )
            ERROR("do_pump_bigvalue error\n", 1, "");

        /* we can't trace value so immediatly store it */
        if( ! value_can_trace(token->res) )
        {
            if( do_store(cpu->registers[reg]) )
                ERROR("do_store error\n", 1, "");

            /* free reg */
            REG_SET_FREE(cpu->registers[reg]);
        }
        else
        {
            REG_SET_BUSY(cpu->registers[reg]);
            cvar_res->up_to_date = 0;
        }

        mpz_clear(val);
        mpz_clear(temp1);
        mpz_clear(temp2);
    }
    /* only left is const */
    else if(token->expr->left->type == CONST_VAL)
    {
        LOG("Only left is const\n", "");

        /* right is not symbolic so can pump value */
        if(value_can_trace(token->expr->right) && ! value_is_symbolic(cvar_right->body.val))
        {
            LOG("Right is not symbolic\n", "");

            /* calc valur to pump */
            mpz_init(val);
            mpz_init(temp1);

            ull2mpz(temp1, token->expr->left->body.cv->value);
            value_get_val(cvar_right->body.val, temp2);

            mpz_mul(val, temp1, temp2);

            /* get register for operation, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            /* lock reg */
            REG_SET_IN_USE(cpu->registers[reg]);

            /* set value  */
            if( ! value_can_trace(token->res))
                reg_set_val(cpu->registers[reg], token->res);
            else
                reg_set_val(cpu->registers[reg], cvar_res->body.val);

            if(do_pump_bigvalue(cpu->registers[reg], val, TRUE))
                ERROR("do_pump_bigvalue error\n", 1, "");

            /* we can't trace value so immediatly store it */
            if(! value_can_trace(token->res) )
            {
                if(do_store(cpu->registers[reg]))
                    ERROR("do_store error\n", 1, "");

                /* free reg */
                REG_SET_FREE(cpu->registers[reg]);
            }
            else
            {
                REG_SET_BUSY(cpu->registers[reg]);
                cvar_res->up_to_date = 0;
            }

            mpz_clear(val);
            mpz_clear(temp1);
            mpz_clear(temp2);
        }
        else
        {
            LOG("Right is symbolic\n", "");

            if( token->expr->left->body.cv->value == 0ull)
            {
                LOG("CONST = %ju\n",token->expr->left->body.cv->value );

                /* get register for res, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                REG_SET_IN_USE(cpu->registers[reg]);

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                if(do_zero(cpu->registers[reg], TRUE))
                    ERROR("do_zero error\n", 1, "");

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else if( token->expr->left->body.cv->value == 1ull)
            {
                LOG("CONST = %ju\n",token->expr->left->body.cv->value );

                /* get register for operation, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* we want to copy from mem to reg, so synchronize with memory */
                if(cvar_right->up_to_date == 0)
                    if(do_synchronize(cvar_right->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                if(! value_can_trace(token->expr->right)
                    || cpu->registers[reg]->val != cvar_right->body.val)
                        if(do_load(cpu->registers[reg], token->expr->right))
                            ERROR("do_load error\n", 1, "");

                /* set value  */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);

                /* we can't trace value so immediatly store it */
                if(! value_can_trace(token->res) )
                {
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else
            {
                if(HAMM_WEIGHT(token->expr->left->body.cv->value, 0ull) == 1)
                {
                    LOG("Left = %ju so use SHL\n", token->expr->left->body.cv->value);

                    /* get register for right, synchronize iff needed  */
                    reg = do_get_register(token_list, token_list_pos, token->expr->right, FALSE);
                    if(reg == -1)
                        ERROR("do_get_register error\n", 1, "");

                    if(cvar_right->up_to_date == 0 &&
                        ( value_can_trace(token->expr->right) && cvar_res->body.val != cvar_right->body.val ))
                        if(do_synchronize(cvar_right->body.val->reg))
                            ERROR("do_synchronize error\n", 1, "");

                    if(! value_can_trace(token->expr->right)
                        || cpu->registers[reg]->val != cvar_right->body.val)
                        if(do_load(cpu->registers[reg], token->expr->right))
                            ERROR("do_load error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg]);

                    /* can we trace or not ? */
                    if(value_can_trace(token->res))
                        reg_set_val(cpu->registers[reg], cvar_res->body.val);
                    else
                        reg_set_val(cpu->registers[reg], token->res);

                    /* can we trace new value ? */
                    if(value_can_trace(token->res) && value_can_trace(token->expr->right))
                    {
                        value_get_val(cvar_right->body.val, val);

                        value_set_val(cvar_res->body.val, val);

                        mpz_clear(val);
                    }

                    value = token->expr->left->body.cv->value;
                    i = 0;
                    while( ! GET_BIT(value, i) )
                    {
                        if( do_shl(cpu->registers[reg], TRUE) )
                            ERROR("do_shl error\n", 1 ,"");

                        ++i;
                    }

                    if(!value_can_trace(token->res))
                    {
                        /* STORE val in res */
                        if(do_store(cpu->registers[reg]))
                            ERROR("do_store error\n", 1, "");

                        /* free reg */
                        REG_SET_FREE(cpu->registers[reg]);
                    }
                    else
                    {
                        REG_SET_BUSY(cpu->registers[reg]);
                        cvar_res->up_to_date = 0;
                    }
                }
                else
                {
                    LOG("Normal Case\n", "");

                    /* get register for res, synchronize iff needed  */
                    reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                    if(reg == -1)
                        ERROR("do_get_register error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg]);

                    /* set value  */
                    if( ! value_can_trace(token->res))
                        reg_set_val(cpu->registers[reg], token->res);
                    else
                        reg_set_val(cpu->registers[reg], cvar_res->body.val);

                    /* get register for right, synchronize iff needed  */
                    reg3 = do_get_register(token_list, token_list_pos, cvar_temp2->body.val, FALSE);
                    if(reg3 == -1)
                        ERROR("do_get_register error\n", 1, "");

                    if(cvar_right->up_to_date == 0)
                        if(do_synchronize(cvar_right->body.val->reg))
                            ERROR("do_synchronize error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg3]);

                    /* store right in temp memory */
                    if(! value_can_trace(token->expr->right)
                        || cpu->registers[reg3]->val != cvar_right->body.val)
                        if(do_load(cpu->registers[reg3], token->expr->right))
                            ERROR("do_load error\n", 1, "");

                    /* temp memory for mult */
                    reg_set_val(cpu->registers[reg3], cvar_temp2->body.val);

                    if(do_store(cpu->registers[reg3]))
                        ERROR("do_store error\n", 1, "");

                    /* get register for left, synchronize iff needed  */
                    reg2 = do_get_register(token_list, token_list_pos, cvar_temp->body.val, FALSE);
                    if(reg2 == -1)
                        ERROR("do_get_register error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg2]);

                    /* temp memory for reg */
                    reg_set_val(cpu->registers[reg2], cvar_temp->body.val);

                    if(do_pump(cpu->registers[reg2], token->expr->left->body.cv->value, FALSE))
                        ERROR("do_pump error\n", 1, "");

                    if(do_store(cpu->registers[reg2]))
                        ERROR("do_store error\n", 1, "");

                    /* do mult */
                    if(do_mult(cpu->registers[reg], cpu->registers[reg2], cpu->registers[reg3], FALSE))
                        ERROR("do_mult error\n", 1, "");

                    if(!value_can_trace(token->res))
                    {
                        /* STORE val in res */
                        if(do_store(cpu->registers[reg]))
                            ERROR("do_store error\n", 1, "");

                        /* free reg */
                        REG_SET_FREE(cpu->registers[reg]);
                    }
                    else
                    {
                        REG_SET_BUSY(cpu->registers[reg]);
                        cvar_res->up_to_date = 0;
                    }

                    REG_SET_FREE(cpu->registers[reg2]);
                    REG_SET_FREE(cpu->registers[reg3]);
                }
            }
        }
    }
    /* only right is const */
    else if(token->expr->right->type == CONST_VAL)
    {
        LOG("Only right is const\n", "");

        /* left is not symbolic so can pump value */
        if(value_can_trace(token->expr->left) && ! value_is_symbolic(cvar_left->body.val))
        {
            LOG("Left is not symbolic\n", "");

            /* calc valur to pump */
            mpz_init(val);
            mpz_init(temp1);

            ull2mpz(temp1, token->expr->right->body.cv->value);
            value_get_val(cvar_left->body.val, temp2);

            mpz_mul(val, temp1, temp2);

            /* get register for operation, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            /* lock reg */
            REG_SET_IN_USE(cpu->registers[reg]);

            /* set value  */
            if( ! value_can_trace(token->res))
                reg_set_val(cpu->registers[reg], token->res);
            else
                reg_set_val(cpu->registers[reg], cvar_res->body.val);

            if(do_pump_bigvalue(cpu->registers[reg], val, TRUE))
                ERROR("do_pump_bigvalue error\n", 1, "");

            /* we can't trace value so immediatly store it */
            if(! value_can_trace(token->res) )
            {
                if(do_store(cpu->registers[reg]))
                    ERROR("do_store error\n", 1, "");

                /* free reg */
                REG_SET_FREE(cpu->registers[reg]);
            }
            else
            {
                REG_SET_BUSY(cpu->registers[reg]);
                cvar_res->up_to_date = 0;
            }

            mpz_clear(val);
            mpz_clear(temp1);
            mpz_clear(temp2);
        }
        else
        {
            LOG("Left is symbolic\n", "");
            if( token->expr->right->body.cv->value == 0ull)
            {
                LOG("CONST = %ju\n", token->expr->right->body.cv->value);

                /* get register for res, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                REG_SET_IN_USE(cpu->registers[reg]);

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                if(do_zero(cpu->registers[reg], TRUE))
                    ERROR("do_zero error\n", 1, "");

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else if( token->expr->right->body.cv->value == 1ull)
            {
                LOG("CONST = %ju\n", token->expr->right->body.cv->value);

                /* get register for operation, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* we want to copy from mem to reg, so synchronize with memory */
                if(cvar_left->up_to_date == 0)
                    if(do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                if(! value_can_trace(token->expr->left)
                    || cpu->registers[reg]->val != cvar_left->body.val)
                        if(do_load(cpu->registers[reg], token->expr->left))
                            ERROR("do_load error\n", 1, "");

                /* set value  */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);

                /* we can't trace value so immediatly store it */
                if(! value_can_trace(token->res) )
                {
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else
            {
                if(HAMM_WEIGHT(token->expr->right->body.cv->value, 0ull) == 1)
                {
                    LOG("Right = %ju so use SHL\n", token->expr->right->body.cv->value);

                    /* get register for left, synchronize iff needed  */
                    reg = do_get_register(token_list, token_list_pos, token->expr->left, FALSE);
                    if(reg == -1)
                        ERROR("do_get_register error\n", 1, "");

                    if(cvar_left->up_to_date == 0 &&
                    ( value_can_trace(token->expr->left) && cvar_res->body.val != cvar_left->body.val ))
                        if(do_synchronize(cvar_left->body.val->reg))
                            ERROR("do_synchronize error\n", 1, "");

                    if(! value_can_trace(token->expr->left)
                        || cpu->registers[reg]->val != cvar_left->body.val)
                        if(do_load(cpu->registers[reg], token->expr->left))
                            ERROR("do_load error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg]);

                    /* can we trace or not ? */
                    if(value_can_trace(token->res))
                        reg_set_val(cpu->registers[reg], cvar_res->body.val);
                    else
                        reg_set_val(cpu->registers[reg], token->res);

                    /* can we trace new value ? */
                    if(value_can_trace(token->res) && value_can_trace(token->expr->left))
                    {
                        value_get_val(cvar_left->body.val, val);

                        value_set_val(cvar_res->body.val, val);

                        mpz_clear(val);
                    }

                    value = token->expr->right->body.cv->value;
                    i = 0;
                    while( ! GET_BIT(value, i) )
                    {
                        if( do_shl(cpu->registers[reg], TRUE) )
                            ERROR("do_shl error\n", 1 ,"");

                        ++i;
                    }

                    if(!value_can_trace(token->res))
                    {
                        /* STORE val in res */
                        if(do_store(cpu->registers[reg]))
                            ERROR("do_store error\n", 1, "");

                        /* free reg */
                        REG_SET_FREE(cpu->registers[reg]);
                    }
                    else
                    {
                        REG_SET_BUSY(cpu->registers[reg]);
                        cvar_res->up_to_date = 0;
                    }
                }
                else
                {
                    LOG("Normal Case\n", "");

                    /* get register for res, synchronize iff needed  */
                    reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                    if(reg == -1)
                        ERROR("do_get_register error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg]);

                    /* set value  */
                    if( ! value_can_trace(token->res))
                        reg_set_val(cpu->registers[reg], token->res);
                    else
                        reg_set_val(cpu->registers[reg], cvar_res->body.val);

                    /* get register for right, synchronize iff needed  */
                    reg2 = do_get_register(token_list, token_list_pos, cvar_temp->body.val, FALSE);
                    if(reg2 == -1)
                        ERROR("do_get_register error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg2]);

                    /* temp memory for mult */
                    reg_set_val(cpu->registers[reg2], cvar_temp->body.val);

                    if(do_pump(cpu->registers[reg2], token->expr->right->body.cv->value, FALSE))
                        ERROR("do_pump error\n", 1, "");

                    if(do_store(cpu->registers[reg2]))
                        ERROR("do_store error\n", 1, "");

                    /* get register for left, synchronize iff needed  */
                    reg3 = do_get_register(token_list, token_list_pos, cvar_temp2->body.val, FALSE);
                    if(reg3 == -1)
                        ERROR("do_get_register error\n", 1, "");

                    if(cvar_left->up_to_date == 0)
                        if(do_synchronize(cvar_left->body.val->reg))
                            ERROR("do_synchronize error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg3]);

                    /* temp memory for mult */
                    reg_set_val(cpu->registers[reg3], cvar_temp2->body.val);

                    /* store left in temp memory */
                    if(! value_can_trace(token->expr->left)
                        || cpu->registers[reg3]->val != cvar_left->body.val)
                        if(do_load(cpu->registers[reg3], token->expr->left))
                            ERROR("do_load error\n", 1, "");

                    if(do_store(cpu->registers[reg3]))
                        ERROR("do_store error\n", 1, "");

                    /* do mult */
                    if(do_mult(cpu->registers[reg], cpu->registers[reg3], cpu->registers[reg2], FALSE))
                        ERROR("do_mult error\n", 1, "");

                    if(!value_can_trace(token->res))
                    {
                        /* STORE val in res */
                        if(do_store(cpu->registers[reg]))
                            ERROR("do_store error\n", 1, "");

                        /* free reg */
                        REG_SET_FREE(cpu->registers[reg]);
                    }
                    else
                    {
                        REG_SET_BUSY(cpu->registers[reg]);
                        cvar_res->up_to_date = 0;
                    }

                    REG_SET_FREE(cpu->registers[reg2]);
                    REG_SET_FREE(cpu->registers[reg3]);
                }
            }
        }
    }
    /* both are variables */
    else
    {
        LOG("Both are vars\n", "");

        /* both are not symbolic so pump value */
        if(    value_can_trace(token->expr->left) && ! value_is_symbolic(cvar_left->body.val)
            && value_can_trace(token->expr->right) && ! value_is_symbolic(cvar_right->body.val) )
        {

            LOG("Both are not symbolic\n", "");

            mpz_init(val);

            /* calc value to pump */
            value_get_val(cvar_left->body.val, temp1);
            value_get_val(cvar_right->body.val, temp2);

            mpz_mul(val, temp1, temp2);

            /* get register for operation, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            /* lock reg */
            REG_SET_IN_USE(cpu->registers[reg]);

            /* set value  */
            if( ! value_can_trace(token->res) )
                reg_set_val(cpu->registers[reg], token->res);
            else
                reg_set_val(cpu->registers[reg], cvar_res->body.val);

            if( do_pump_bigvalue(cpu->registers[reg], val, TRUE) )
                ERROR("do_pump_bigvalue error\n", 1, "");

            /* we can't trace value so immediatly store it */
            if( ! value_can_trace(token->res) )
            {
                if( do_store(cpu->registers[reg]) )
                    ERROR("do_store error\n", 1, "");

                /* free reg */
                REG_SET_FREE(cpu->registers[reg]);
            }
            else
            {
                REG_SET_BUSY(cpu->registers[reg]);
                cvar_res->up_to_date = 0;
            }

            mpz_clear(val);
            mpz_clear(temp1);
            mpz_clear(temp2);
        }
        else
        {
            LOG("At least one is symbolic\n", "");

            if(  value_can_trace(token->expr->left) && ! value_is_symbolic(cvar_left->body.val) &&
                mpz_cmp_si(cvar_left->body.val->body.var->body.var->value, 0) == 0)
            {
                /* get register for res, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                REG_SET_IN_USE(cpu->registers[reg]);

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                if(do_zero(cpu->registers[reg], TRUE))
                    ERROR("do_zero error\n", 1, "");

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else if (value_can_trace(token->expr->left) && ! value_is_symbolic(cvar_left->body.val) &&
                mpz_cmp_si(cvar_left->body.val->body.var->body.var->value, 1) == 0)
            {
                /* get register for operation, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* we want to copy from mem to reg, so synchronize with memory */
                if(cvar_right->up_to_date == 0)
                    if(do_synchronize(cvar_right->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                if(! value_can_trace(token->expr->right)
                    || cpu->registers[reg]->val != cvar_right->body.val)
                        if(do_load(cpu->registers[reg], token->expr->right))
                            ERROR("do_load error\n", 1, "");

                /* set value  */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);

                /* we can't trace value so immediatly store it */
                if(! value_can_trace(token->res) )
                {
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else if (value_can_trace(token->expr->left) && ! value_is_symbolic(cvar_left->body.val) &&
                BHAMM_WEIGHT(cvar_left->body.val->body.var->body.var->value, 0ull) == 1)
            {
                /* get register for right, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->expr->right, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                if(cvar_right->up_to_date == 0 &&
                    ( value_can_trace(token->expr->right) && cvar_res->body.val != cvar_right->body.val ))
                    if(do_synchronize(cvar_right->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                if(! value_can_trace(token->expr->right)
                    || cpu->registers[reg]->val != cvar_right->body.val)
                    if(do_load(cpu->registers[reg], token->expr->right))
                        ERROR("do_load error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                /* can we trace new value ? */
                if(value_can_trace(token->res) && value_can_trace(token->expr->right))
                {
                    value_get_val(cvar_right->body.val, val);

                    value_set_val(cvar_res->body.val, val);

                    mpz_clear(val);
                }

                value_get_val(cvar_left->body.val, val);
                i = 0;
                while( ! BGET_BIT(val, i) )
                {
                    if( do_shl(cpu->registers[reg], TRUE) )
                        ERROR("do_shl error\n", 1 ,"");

                    ++i;
                }

                mpz_clear(val);

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else if (value_can_trace(token->expr->right) && ! value_is_symbolic(cvar_right->body.val) &&
                    mpz_cmp_si(cvar_right->body.val->body.var->body.var->value, 0) == 0)
            {
                /* get register for res, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                REG_SET_IN_USE(cpu->registers[reg]);

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                if(do_zero(cpu->registers[reg], TRUE))
                    ERROR("do_zero error\n", 1, "");

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else if (value_can_trace(token->expr->right) && ! value_is_symbolic(cvar_right->body.val) &&
                    mpz_cmp_si(cvar_right->body.val->body.var->body.var->value, 1) == 0)
            {
                /* get register for operation, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* we want to copy from mem to reg, so synchronize with memory */
                if(cvar_left->up_to_date == 0)
                    if(do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                if(! value_can_trace(token->expr->left)
                    || cpu->registers[reg]->val != cvar_left->body.val)
                        if(do_load(cpu->registers[reg], token->expr->left))
                            ERROR("do_load error\n", 1, "");

                /* set value  */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);

                /* we can't trace value so immediatly store it */
                if(! value_can_trace(token->res) )
                {
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else if (value_can_trace(token->expr->right) && ! value_is_symbolic(cvar_right->body.val) &&
                BHAMM_WEIGHT(cvar_right->body.val->body.var->body.var->value, 0ull) == 1)
            {
                /* get register for right, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->expr->left, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                if(cvar_left->up_to_date == 0 &&
                    ( value_can_trace(token->expr->left) && cvar_res->body.val != cvar_left->body.val ))
                    if(do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                if(! value_can_trace(token->expr->left)
                    || cpu->registers[reg]->val != cvar_left->body.val)
                    if(do_load(cpu->registers[reg], token->expr->left))
                        ERROR("do_load error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                /* can we trace new value ? */
                if(value_can_trace(token->res) && value_can_trace(token->expr->left))
                {
                    value_get_val(cvar_left->body.val, val);

                    value_set_val(cvar_res->body.val, val);

                    mpz_clear(val);
                }

                value_get_val(cvar_right->body.val, val);
                i = 0;
                while( ! BGET_BIT(val, i) )
                {
                    if( do_shl(cpu->registers[reg], TRUE) )
                        ERROR("do_shl error\n", 1 ,"");

                    ++i;
                }

                mpz_clear(val);

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else
            {
                /* get register for res, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* set value  */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);

                /* get register for right, synchronize iff needed  */
                reg3 = do_get_register(token_list, token_list_pos, cvar_temp->body.val, FALSE);
                if(reg3 == -1)
                    ERROR("do_get_register error\n", 1, "");

                if(cvar_right->up_to_date == 0)
                    if(do_synchronize(cvar_right->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg3]);

                /* store right in temp memory */
                if(! value_can_trace(token->expr->right)
                    || cpu->registers[reg3]->val != cvar_right->body.val)
                    if(do_load(cpu->registers[reg3], token->expr->right))
                        ERROR("do_load error\n", 1, "");

                reg_set_val(cpu->registers[reg3], cvar_temp->body.val);

                if(do_store(cpu->registers[reg3]))
                    ERROR("do_store error\n", 1, "");

                /* get register for left, synchronize iff needed  */
                reg2 = do_get_register(token_list, token_list_pos, cvar_temp2->body.val, FALSE);
                if(reg2 == -1)
                    ERROR("do_get_register error\n", 1, "");

                if(cvar_left->up_to_date == 0)
                    if(do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg2]);

                reg_set_val(cpu->registers[reg2], cvar_temp2->body.val);

                /* store left in temp memory */
                if(! value_can_trace(token->expr->left)
                    || cpu->registers[reg2]->val != cvar_left->body.val)
                    if(do_load(cpu->registers[reg2], token->expr->left))
                        ERROR("do_load error\n", 1, "");

                if(do_store(cpu->registers[reg2]))
                    ERROR("do_store error\n", 1, "");

                /* do mult */
                if(do_mult(cpu->registers[reg], cpu->registers[reg2], cpu->registers[reg3], FALSE))
                    ERROR("do_mult error\n", 1, "");

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }

                REG_SET_FREE(cpu->registers[reg2]);
                REG_SET_FREE(cpu->registers[reg3]);
            }
        }
    }

    return 0;
}

static int __div(token_assign *token)
{
    /*
        0 / 0 = 0
        a / 0 = 0
        0 / b = 0
        a / 1 = a
        a % a = (a == 0 ? 0 : 1)
    */
    Cvar *cvar_res;
    Cvar *cvar_left;
    Cvar *cvar_right;
    Cvar *cvar_temp;
    Cvar *cvar_temp2;

    int reg;
    int reg2;
    int reg3;

    int i;
    uint64_t value;

    mpz_t val;

    mpz_t temp1;
    mpz_t temp2;

    TRACE("");

    cvar_res = cvar_get_by_value(token->res);
    cvar_left = cvar_get_by_value(token->expr->left);
    cvar_right = cvar_get_by_value(token->expr->right);
    cvar_temp = cvar_get_by_name(TEMP1_NAME);
    cvar_temp2 = cvar_get_by_name(TEMP2_NAME);

    /* a = N / M, pump to a N / M */
    if(token->expr->left->type == CONST_VAL && token->expr->right->type == CONST_VAL)
    {
        LOG("Both are const\n", "");

        mpz_init(val);
        mpz_init(temp1);
        mpz_init(temp2);

        ull2mpz(temp1, token->expr->left->body.cv->value);
        ull2mpz(temp2, token->expr->right->body.cv->value);

        if(mpz_cmp_ui(temp2, 0) == 0)
            mpz_set_ui(val, 0);
        else
            mpz_fdiv_q(val, temp1, temp2);

        /* get register for operation, synchronize iff needed  */
        reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
        if(reg == -1)
            ERROR("do_get_register error\n", 1, "");

        /* lock reg */
        REG_SET_IN_USE(cpu->registers[reg]);

        /* set value  */
        if( ! value_can_trace(token->res) )
            reg_set_val(cpu->registers[reg], token->res);
        else
            reg_set_val(cpu->registers[reg], cvar_res->body.val);

        if( do_pump_bigvalue(cpu->registers[reg], val, TRUE) )
            ERROR("do_pump_bigvalue error\n", 1, "");

        /* we can't trace value so immediatly store it */
        if( ! value_can_trace(token->res) )
        {
            if( do_store(cpu->registers[reg]) )
                ERROR("do_store error\n", 1, "");

            /* free reg */
            REG_SET_FREE(cpu->registers[reg]);
        }
        else
        {
            REG_SET_BUSY(cpu->registers[reg]);
            cvar_res->up_to_date = 0;
        }

        mpz_clear(val);
        mpz_clear(temp1);
        mpz_clear(temp2);
    }
    /* only left is const */
    else if(token->expr->left->type == CONST_VAL)
    {
        LOG("Only left is const\n", "");

        /* right is not symbolic so can pump value */
        if(value_can_trace(token->expr->right) && ! value_is_symbolic(cvar_right->body.val))
        {
            LOG("Right is not symbolic\n", "");

            /* calc valur to pump */
            mpz_init(val);
            mpz_init(temp1);

            ull2mpz(temp1, token->expr->left->body.cv->value);
            value_get_val(cvar_right->body.val, temp2);

            if(mpz_cmp_ui(temp2, 0) == 0)
                mpz_set_ui(val, 0);
            else
                mpz_fdiv_q(val, temp1, temp2);

            /* get register for operation, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            /* lock reg */
            REG_SET_IN_USE(cpu->registers[reg]);

            /* set value  */
            if( ! value_can_trace(token->res))
                reg_set_val(cpu->registers[reg], token->res);
            else
                reg_set_val(cpu->registers[reg], cvar_res->body.val);

            if(do_pump_bigvalue(cpu->registers[reg], val, TRUE))
                ERROR("do_pump_bigvalue error\n", 1, "");

            /* we can't trace value so immediatly store it */
            if(! value_can_trace(token->res) )
            {
                if(do_store(cpu->registers[reg]))
                    ERROR("do_store error\n", 1, "");

                /* free reg */
                REG_SET_FREE(cpu->registers[reg]);
            }
            else
            {
                REG_SET_BUSY(cpu->registers[reg]);
                cvar_res->up_to_date = 0;
            }

            mpz_clear(val);
            mpz_clear(temp1);
            mpz_clear(temp2);
        }
        else
        {
            LOG("Right is symbolic\n", "");

            if( token->expr->left->body.cv->value == 0ull)
            {
                LOG("CONST = %ju\n", token->expr->left->body.cv->value);

                /* get register for res, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                REG_SET_IN_USE(cpu->registers[reg]);

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                if(do_zero(cpu->registers[reg], TRUE))
                    ERROR("do_zero error\n", 1, "");

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else
            {
                /* get register for res, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* set value  */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);

                /* get register for right, synchronize iff needed  */
                reg3 = do_get_register(token_list, token_list_pos, cvar_temp2->body.val, FALSE);
                if(reg3 == -1)
                    ERROR("do_get_register error\n", 1, "");

                if(cvar_right->up_to_date == 0)
                    if(do_synchronize(cvar_right->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg3]);

                /* temp memory for div */
                reg_set_val(cpu->registers[reg3], cvar_temp2->body.val);

                /* store right in temp memory */
                if(! value_can_trace(token->expr->right)
                    || cpu->registers[reg3]->val != cvar_right->body.val)
                    if(do_load(cpu->registers[reg3], token->expr->right))
                        ERROR("do_load error\n", 1, "");

                if(do_store(cpu->registers[reg3]))
                    ERROR("do_store error\n", 1, "");

                /* get register for left, synchronize iff needed  */
                reg2 = do_get_register(token_list, token_list_pos, cvar_temp->body.val, FALSE);
                if(reg2 == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg2]);

                /* temp memory for div */
                reg_set_val(cpu->registers[reg2], cvar_temp->body.val);

                if(do_pump(cpu->registers[reg2], token->expr->left->body.cv->value, FALSE))
                    ERROR("do_pump error\n", 1, "");

                if(do_store(cpu->registers[reg2]))
                    ERROR("do_store error\n", 1, "");

                /* do div */
                if(do_div(cpu->registers[reg], cpu->registers[reg2], cpu->registers[reg3], FALSE))
                    ERROR("do_mult error\n", 1, "");

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }

                REG_SET_FREE(cpu->registers[reg2]);
                REG_SET_FREE(cpu->registers[reg3]);
            }
        }
    }
    /* only right is const */
    else if(token->expr->right->type == CONST_VAL)
    {
        LOG("Only right is const\n", "");

        /* left is not symbolic so can pump value */
        if(value_can_trace(token->expr->left) && ! value_is_symbolic(cvar_left->body.val))
        {
            LOG("Left is not symbolic\n", "");

            /* calc value to pump */
            mpz_init(val);
            mpz_init(temp1);

            ull2mpz(temp1, token->expr->right->body.cv->value);
            value_get_val(cvar_left->body.val, temp2);

            if(mpz_cmp_ui(temp1, 0) == 0)
                mpz_set_ui(val, 0);
            else
                mpz_fdiv_q(val, temp2, temp1);

            /* get register for operation, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            /* lock reg */
            REG_SET_IN_USE(cpu->registers[reg]);

            /* set value  */
            if( ! value_can_trace(token->res))
                reg_set_val(cpu->registers[reg], token->res);
            else
                reg_set_val(cpu->registers[reg], cvar_res->body.val);

            if(do_pump_bigvalue(cpu->registers[reg], val, TRUE))
                ERROR("do_pump_bigvalue error\n", 1, "");

            /* we can't trace value so immediatly store it */
            if(! value_can_trace(token->res) )
            {
                if(do_store(cpu->registers[reg]))
                    ERROR("do_store error\n", 1, "");

                /* free reg */
                REG_SET_FREE(cpu->registers[reg]);
            }
            else
            {
                REG_SET_BUSY(cpu->registers[reg]);
                cvar_res->up_to_date = 0;
            }

            mpz_clear(val);
            mpz_clear(temp1);
            mpz_clear(temp2);
        }
        else
        {
            LOG("Left is symbolic\n", "");
            if( token->expr->right->body.cv->value == 0ull)
            {
                LOG("CONST = %ju\n", token->expr->right->body.cv->value);

                /* get register for res, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                REG_SET_IN_USE(cpu->registers[reg]);

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                if(do_zero(cpu->registers[reg], TRUE))
                    ERROR("do_zero error\n", 1, "");

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else if( token->expr->right->body.cv->value == 1ull)
            {
                LOG("CONST = %ju\n", token->expr->right->body.cv->value);

                /* get register for operation, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* we want to copy from mem to reg, so synchronize with memory */
                if(cvar_left->up_to_date == 0)
                    if(do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                if(! value_can_trace(token->expr->left)
                    || cpu->registers[reg]->val != cvar_left->body.val)
                        if(do_load(cpu->registers[reg], token->expr->left))
                            ERROR("do_load error\n", 1, "");

                /* set value  */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);

                /* we can't trace value so immediatly store it */
                if(! value_can_trace(token->res) )
                {
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else
            {
                if(HAMM_WEIGHT(token->expr->right->body.cv->value, 0ull) == 1)
                {
                    LOG("Right = %ju so use SHR\n", token->expr->right->body.cv->value);

                    /* get register for left, synchronize iff needed  */
                    reg = do_get_register(token_list, token_list_pos, token->expr->left, FALSE);
                    if(reg == -1)
                        ERROR("do_get_register error\n", 1, "");

                    if(cvar_left->up_to_date == 0 &&
                    ( value_can_trace(token->expr->left) && cvar_res->body.val != cvar_left->body.val ))
                        if(do_synchronize(cvar_left->body.val->reg))
                            ERROR("do_synchronize error\n", 1, "");

                    if(! value_can_trace(token->expr->left)
                        || cpu->registers[reg]->val != cvar_left->body.val)
                        if(do_load(cpu->registers[reg], token->expr->left))
                            ERROR("do_load error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg]);

                    /* can we trace or not ? */
                    if(value_can_trace(token->res))
                        reg_set_val(cpu->registers[reg], cvar_res->body.val);
                    else
                        reg_set_val(cpu->registers[reg], token->res);

                    /* can we trace new value ? */
                    if(value_can_trace(token->res) && value_can_trace(token->expr->left))
                    {
                        value_get_val(cvar_left->body.val, val);

                        value_set_val(cvar_res->body.val, val);

                        mpz_clear(val);
                    }

                    value = token->expr->right->body.cv->value;
                    i = 0;
                    while( ! GET_BIT(value, i) )
                    {
                        if( do_shr(cpu->registers[reg], TRUE) )
                            ERROR("do_shl error\n", 1 ,"");

                        ++i;
                    }

                    if(!value_can_trace(token->res))
                    {
                        /* STORE val in res */
                        if(do_store(cpu->registers[reg]))
                            ERROR("do_store error\n", 1, "");

                        /* free reg */
                        REG_SET_FREE(cpu->registers[reg]);
                    }
                    else
                    {
                        REG_SET_BUSY(cpu->registers[reg]);
                        cvar_res->up_to_date = 0;
                    }
                }
                else
                {
                    /* get register for res, synchronize iff needed  */
                    reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                    if(reg == -1)
                        ERROR("do_get_register error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg]);

                    /* set value  */
                    if( ! value_can_trace(token->res))
                        reg_set_val(cpu->registers[reg], token->res);
                    else
                        reg_set_val(cpu->registers[reg], cvar_res->body.val);

                    /* get register for right, synchronize iff needed  */
                    reg2 = do_get_register(token_list, token_list_pos, cvar_temp->body.val, FALSE);
                    if(reg2 == -1)
                        ERROR("do_get_register error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg2]);

                    /*  temp memory for div */
                    reg_set_val(cpu->registers[reg2], cvar_temp->body.val);

                    if(do_pump(cpu->registers[reg2], token->expr->right->body.cv->value, FALSE))
                        ERROR("do_pump error\n", 1, "");

                    if(do_store(cpu->registers[reg2]))
                        ERROR("do_store error\n", 1, "");

                    /* get register for left, synchronize iff needed  */
                    reg3 = do_get_register(token_list, token_list_pos, cvar_temp2->body.val, FALSE);
                    if(reg3 == -1)
                        ERROR("do_get_register error\n", 1, "");

                    if(cvar_left->up_to_date == 0)
                        if(do_synchronize(cvar_left->body.val->reg))
                            ERROR("do_synchronize error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg3]);

                    /* store left in temp memory */
                    if(! value_can_trace(token->expr->left)
                        || cpu->registers[reg3]->val != cvar_left->body.val)
                        if(do_load(cpu->registers[reg3], token->expr->left))
                            ERROR("do_load error\n", 1, "");

                    reg_set_val(cpu->registers[reg3], cvar_temp2->body.val);

                    if(do_store(cpu->registers[reg3]))
                        ERROR("do_store error\n", 1, "");

                    /* do div */
                    if(do_div(cpu->registers[reg], cpu->registers[reg3], cpu->registers[reg2], FALSE))
                        ERROR("do_mult error\n", 1, "");

                    if(!value_can_trace(token->res))
                    {
                        /* STORE val in res */
                        if(do_store(cpu->registers[reg]))
                            ERROR("do_store error\n", 1, "");

                        /* free reg */
                        REG_SET_FREE(cpu->registers[reg]);
                    }
                    else
                    {
                        REG_SET_BUSY(cpu->registers[reg]);
                        cvar_res->up_to_date = 0;
                    }

                    REG_SET_FREE(cpu->registers[reg2]);
                    REG_SET_FREE(cpu->registers[reg3]);
                }
            }
        }
    }
    /* both are variables */
    else
    {
        LOG("Both are vars\n", "");

        /* both are not symbolic so pump value */
        if(    value_can_trace(token->expr->left) && ! value_is_symbolic(cvar_left->body.val)
            && value_can_trace(token->expr->right) && ! value_is_symbolic(cvar_right->body.val) )
        {
            LOG("Both are not symbolic\n", "");

            mpz_init(val);

            /* calc value to pump */
            value_get_val(cvar_left->body.val, temp1);
            value_get_val(cvar_right->body.val, temp2);

            if(mpz_cmp_ui(temp2, 0) == 0)
                mpz_set_ui(val, 0);
            else
                mpz_fdiv_q(val, temp1, temp2);

            /* get register for operation, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            /* lock reg */
            REG_SET_IN_USE(cpu->registers[reg]);

            /* set value  */
            if( ! value_can_trace(token->res) )
                reg_set_val(cpu->registers[reg], token->res);
            else
                reg_set_val(cpu->registers[reg], cvar_res->body.val);

            if( do_pump_bigvalue(cpu->registers[reg], val, TRUE) )
                ERROR("do_pump_bigvalue error\n", 1, "");

            /* we can't trace value so immediatly store it */
            if( ! value_can_trace(token->res) )
            {
                if( do_store(cpu->registers[reg]) )
                    ERROR("do_store error\n", 1, "");

                /* free reg */
                REG_SET_FREE(cpu->registers[reg]);
            }
            else
            {
                REG_SET_BUSY(cpu->registers[reg]);
                cvar_res->up_to_date = 0;
            }

            mpz_clear(val);
            mpz_clear(temp1);
            mpz_clear(temp2);
        }
        else
        {
            LOG("At least one is symbolic\n", "");

            if(  value_can_trace(token->expr->left) && ! value_is_symbolic(cvar_left->body.val) &&
                mpz_cmp_si(cvar_left->body.val->body.var->body.var->value, 0) == 0)
            {
                /* get register for res, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                REG_SET_IN_USE(cpu->registers[reg]);

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                if(do_zero(cpu->registers[reg], TRUE))
                    ERROR("do_zero error\n", 1, "");

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else if (value_can_trace(token->expr->right) && ! value_is_symbolic(cvar_right->body.val) &&
                    mpz_cmp_si(cvar_right->body.val->body.var->body.var->value, 0) == 0)
            {
                /* get register for res, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                REG_SET_IN_USE(cpu->registers[reg]);

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                if(do_zero(cpu->registers[reg], TRUE))
                    ERROR("do_zero error\n", 1, "");

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else if(value_can_trace(token->expr->right) && ! value_is_symbolic(cvar_right->body.val) &&
                    mpz_cmp_si(cvar_right->body.val->body.var->body.var->value, 1) == 0)
            {
                /* get register for operation, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* we want to copy from mem to reg, so synchronize with memory */
                if(cvar_left->up_to_date == 0)
                    if(do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                if(! value_can_trace(token->expr->left)
                    || cpu->registers[reg]->val != cvar_left->body.val)
                        if(do_load(cpu->registers[reg], token->expr->left))
                            ERROR("do_load error\n", 1, "");

                /* set value  */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);

                /* we can't trace value so immediatly store it */
                if(! value_can_trace(token->res) )
                {
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else if(value_can_trace(token->expr->right) && ! value_is_symbolic(cvar_right->body.val) &&
                    BHAMM_WEIGHT(cvar_right->body.val->body.var->body.var->value, 0ull) == 1)
            {
                /* get register for left, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->expr->left, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                if(cvar_left->up_to_date == 0 &&
                ( value_can_trace(token->expr->left) && cvar_res->body.val != cvar_left->body.val ))
                    if(do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                if(! value_can_trace(token->expr->left)
                    || cpu->registers[reg]->val != cvar_left->body.val)
                    if(do_load(cpu->registers[reg], token->expr->left))
                        ERROR("do_load error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                /* can we trace new value ? */
                if(value_can_trace(token->res) && value_can_trace(token->expr->left))
                {
                    value_get_val(cvar_left->body.val, val);

                    value_set_val(cvar_res->body.val, val);

                    mpz_clear(val);
                }

                value_get_val(cvar_right->body.val, val);
                i = 0;
                while( ! BGET_BIT(val, i) )
                {
                    if( do_shr(cpu->registers[reg], TRUE) )
                        ERROR("do_shl error\n", 1 ,"");

                    ++i;
                }

                mpz_clear(val);

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else
            {
                if(variable_cmp((void*)&token->expr->left->body.var,
                    (void*)&token->expr->right->body.var) == 0)
                {
                    /* get register for res, synchronize iff needed  */
                    reg = do_get_register(token_list, token_list_pos, token->expr->left, FALSE);
                    if(reg == -1)
                        ERROR("do_get_register error\n", 1, "");

                    if(cvar_left->up_to_date == 0)
                        if(do_synchronize(cvar_left->body.val->reg))
                            ERROR("do_synchronize error\n", 1, "");

                    if(! value_can_trace(token->expr->left)
                        || cpu->registers[reg]->val != cvar_left->body.val)
                        if(do_load(cpu->registers[reg], token->expr->left))
                            ERROR("do_load error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg]);

                    /* set value  */
                    if( ! value_can_trace(token->res))
                        reg_set_val(cpu->registers[reg], token->res);
                    else
                        reg_set_val(cpu->registers[reg], cvar_res->body.val);

                    if(do_jzero(cpu->registers[reg], asmcode->length + 4))
                        ERROR("do_jzero error\n", 1, "");

                    if(do_zero(cpu->registers[reg], FALSE))
                        ERROR("do_zero error\n", 1, "");

                    if(do_inc(cpu->registers[reg], FALSE))
                        ERROR("do_inc error\n", 1, "");

                    if(do_jump(asmcode->length + 2))
                        ERROR("do_jump error\n", 1, "");

                    if(do_zero(cpu->registers[reg], FALSE))
                        ERROR("do_zero error\n", 1, "");

                    /* we have branch here so set to symbolic */
                    if(value_can_trace(token->res))
                        value_set_symbolic_flag(cvar_res->body.val);
                }
                else
                {
                    /* get register for res, synchronize iff needed  */
                    reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                    if(reg == -1)
                        ERROR("do_get_register error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg]);

                    /* set value  */
                    if( ! value_can_trace(token->res))
                        reg_set_val(cpu->registers[reg], token->res);
                    else
                        reg_set_val(cpu->registers[reg], cvar_res->body.val);

                    /* get register for right, synchronize iff needed  */
                    reg3 = do_get_register(token_list, token_list_pos, cvar_temp->body.val, FALSE);
                    if(reg3 == -1)
                        ERROR("do_get_register error\n", 1, "");

                    if(cvar_right->up_to_date == 0)
                        if(do_synchronize(cvar_right->body.val->reg))
                            ERROR("do_synchronize error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg3]);

                    /* store right in temp memory */
                    if(! value_can_trace(token->expr->right)
                        || cpu->registers[reg3]->val != cvar_right->body.val)
                        if(do_load(cpu->registers[reg3], token->expr->right))
                            ERROR("do_load error\n", 1, "");

                    /* temp memory for div */
                    reg_set_val(cpu->registers[reg3], cvar_temp->body.val);

                    if(do_store(cpu->registers[reg3]))
                        ERROR("do_store error\n", 1, "");

                    /* get register for left, synchronize iff needed  */
                    reg2 = do_get_register(token_list, token_list_pos, cvar_temp2->body.val, FALSE);
                    if(reg2 == -1)
                        ERROR("do_get_register error\n", 1, "");

                    if(cvar_left->up_to_date == 0)
                        if(do_synchronize(cvar_left->body.val->reg))
                            ERROR("do_synchronize error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg2]);

                    /* store left in temp memory */
                    if(! value_can_trace(token->expr->left)
                        || cpu->registers[reg2]->val != cvar_left->body.val)
                        if(do_load(cpu->registers[reg2], token->expr->left))
                            ERROR("do_load error\n", 1, "");

                    /* temp memory for div */
                    reg_set_val(cpu->registers[reg2], cvar_temp2->body.val);

                    if(do_store(cpu->registers[reg2]))
                        ERROR("do_store error\n", 1, "");

                    /* do div */
                    if(do_div(cpu->registers[reg], cpu->registers[reg2], cpu->registers[reg3], FALSE))
                        ERROR("do_mult error\n", 1, "");


                    REG_SET_FREE(cpu->registers[reg2]);
                    REG_SET_FREE(cpu->registers[reg3]);
                }

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
        }
    }

    return 0;
}

static int __mod(token_assign *token)
{
    /*
        0 % 0 = 0
        a % 0 = 0
        0 % b = 0
        a % 1 = 0
        a % a = 0
        a % 2 = return JODD
    */
    Cvar *cvar_res;
    Cvar *cvar_left;
    Cvar *cvar_right;
    Cvar *cvar_temp;
    Cvar *cvar_temp2;

    int reg;
    int reg2;
    int reg3;

    mpz_t val;

    mpz_t temp1;
    mpz_t temp2;

    TRACE("");

    cvar_res = cvar_get_by_value(token->res);
    cvar_left = cvar_get_by_value(token->expr->left);
    cvar_right = cvar_get_by_value(token->expr->right);
    cvar_temp = cvar_get_by_name(TEMP1_NAME);
    cvar_temp2 = cvar_get_by_name(TEMP2_NAME);

    /* a = N % M, pump to a N % M */
    if(token->expr->left->type == CONST_VAL && token->expr->right->type == CONST_VAL)
    {
        LOG("Both are const\n", "");

        mpz_init(val);
        mpz_init(temp1);
        mpz_init(temp2);

        ull2mpz(temp1, token->expr->left->body.cv->value);
        ull2mpz(temp2, token->expr->right->body.cv->value);

        if(mpz_cmp_ui(temp2, 0) == 0)
            mpz_set_ui(val, 0);
        else
            mpz_mod(val, temp1, temp2);

        /* get register for operation, synchronize iff needed  */
        reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
        if(reg == -1)
            ERROR("do_get_register error\n", 1, "");

        /* lock reg */
        REG_SET_IN_USE(cpu->registers[reg]);

        /* set value  */
        if( ! value_can_trace(token->res) )
            reg_set_val(cpu->registers[reg], token->res);
        else
            reg_set_val(cpu->registers[reg], cvar_res->body.val);

        if( do_pump_bigvalue(cpu->registers[reg], val, TRUE) )
            ERROR("do_pump_bigvalue error\n", 1, "");

        /* we can't trace value so immediatly store it */
        if( ! value_can_trace(token->res) )
        {
            if( do_store(cpu->registers[reg]) )
                ERROR("do_store error\n", 1, "");

            /* free reg */
            REG_SET_FREE(cpu->registers[reg]);
        }
        else
        {
            REG_SET_BUSY(cpu->registers[reg]);
            cvar_res->up_to_date = 0;
        }

        mpz_clear(val);
        mpz_clear(temp1);
        mpz_clear(temp2);
    }
    /* only left is const */
    else if(token->expr->left->type == CONST_VAL)
    {
        LOG("Only left is const\n", "");

        /* right is not symbolic so can pump value */
        if(value_can_trace(token->expr->right) && ! value_is_symbolic(cvar_right->body.val))
        {
            LOG("Right is not symbolic\n", "");

            /* calc valur to pump */
            mpz_init(val);
            mpz_init(temp1);

            ull2mpz(temp1, token->expr->left->body.cv->value);
            value_get_val(cvar_right->body.val, temp2);

            if(mpz_cmp_ui(temp2, 0) == 0)
                mpz_set_ui(val, 0);
            else
                mpz_mod(val, temp1, temp2);

            /* get register for operation, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            /* lock reg */
            REG_SET_IN_USE(cpu->registers[reg]);

            /* set value  */
            if( ! value_can_trace(token->res))
                reg_set_val(cpu->registers[reg], token->res);
            else
                reg_set_val(cpu->registers[reg], cvar_res->body.val);

            if(do_pump_bigvalue(cpu->registers[reg], val, TRUE))
                ERROR("do_pump_bigvalue error\n", 1, "");

            /* we can't trace value so immediatly store it */
            if(! value_can_trace(token->res) )
            {
                if(do_store(cpu->registers[reg]))
                    ERROR("do_store error\n", 1, "");

                /* free reg */
                REG_SET_FREE(cpu->registers[reg]);
            }
            else
            {
                REG_SET_BUSY(cpu->registers[reg]);
                cvar_res->up_to_date = 0;
            }

            mpz_clear(val);
            mpz_clear(temp1);
            mpz_clear(temp2);
        }
        else
        {
            LOG("Right is symbolic\n", "");

            if( token->expr->left->body.cv->value == 0ull)
            {
                LOG("CONST = %ju\n", token->expr->left->body.cv->value );

                /* get register for res, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                REG_SET_IN_USE(cpu->registers[reg]);

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                if(do_zero(cpu->registers[reg], TRUE))
                    ERROR("do_zero error\n", 1, "");

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else
            {
                /* get register for res, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* set value  */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);

                /* get register for right, synchronize iff needed  */
                reg3 = do_get_register(token_list, token_list_pos, cvar_temp2->body.val, FALSE);
                if(reg3 == -1)
                    ERROR("do_get_register error\n", 1, "");

                if(cvar_right->up_to_date == 0)
                    if(do_synchronize(cvar_right->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg3]);

                /* store right in temp memory */
                if(! value_can_trace(token->expr->right)
                    || cpu->registers[reg3]->val != cvar_right->body.val)
                    if(do_load(cpu->registers[reg3], token->expr->right))
                        ERROR("do_load error\n", 1, "");

                /* temp memory for mod */
                reg_set_val(cpu->registers[reg3], cvar_temp2->body.val);

                if(do_store(cpu->registers[reg3]))
                    ERROR("do_store error\n", 1, "");

                /* get register for left, synchronize iff needed  */
                reg2 = do_get_register(token_list, token_list_pos, cvar_temp->body.val, FALSE);
                if(reg2 == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg2]);

                /* temp memory for mod */
                reg_set_val(cpu->registers[reg2], cvar_temp->body.val);

                if(do_pump(cpu->registers[reg2], token->expr->left->body.cv->value, FALSE))
                    ERROR("do_pump error\n", 1, "");

                if(do_store(cpu->registers[reg2]))
                    ERROR("do_store error\n", 1, "");

                /* do mod */
                if(do_mod(cpu->registers[reg], cpu->registers[reg2], cpu->registers[reg3], FALSE))
                    ERROR("do_mult error\n", 1, "");

                /* value is in reg2 */
                REG_SET_FREE(cpu->registers[reg]);
                REG_SET_FREE(cpu->registers[reg3]);

                /* set value  */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg2], token->res);
                else
                    reg_set_val(cpu->registers[reg2], cvar_res->body.val);

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg2]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg2]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg2]);
                    cvar_res->up_to_date = 0;
                }
            }
        }
    }
    /* only right is const */
    else if(token->expr->right->type == CONST_VAL)
    {
        LOG("Only right is const\n", "");

        /* left is not symbolic so can pump value */
        if(value_can_trace(token->expr->left) && ! value_is_symbolic(cvar_left->body.val))
        {
            LOG("Left is not symbolic\n", "");

            /* calc valur to pump */
            mpz_init(val);
            mpz_init(temp1);

            ull2mpz(temp1, token->expr->right->body.cv->value);
            value_get_val(cvar_left->body.val, temp2);

            if(mpz_cmp_ui(temp1, 0) == 0)
                mpz_set_ui(val, 0);
            else
                mpz_mod(val, temp2, temp1);

            /* get register for operation, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            /* lock reg */
            REG_SET_IN_USE(cpu->registers[reg]);

            /* set value  */
            if( ! value_can_trace(token->res))
                reg_set_val(cpu->registers[reg], token->res);
            else
                reg_set_val(cpu->registers[reg], cvar_res->body.val);

            if(do_pump_bigvalue(cpu->registers[reg], val, TRUE))
                ERROR("do_pump_bigvalue error\n", 1, "");

            /* we can't trace value so immediatly store it */
            if(! value_can_trace(token->res) )
            {
                if(do_store(cpu->registers[reg]))
                    ERROR("do_store error\n", 1, "");

                /* free reg */
                REG_SET_FREE(cpu->registers[reg]);
            }
            else
            {
                REG_SET_BUSY(cpu->registers[reg]);
                cvar_res->up_to_date = 0;
            }

            mpz_clear(val);
            mpz_clear(temp1);
            mpz_clear(temp2);
        }
        else
        {
            LOG("Left is symbolic\n", "");
            if( token->expr->right->body.cv->value == 0ull ||
                token->expr->right->body.cv->value == 1ull )
            {
                LOG("CONST = %ju\n",token->expr->right->body.cv->value);
                /* get register for res, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                REG_SET_IN_USE(cpu->registers[reg]);

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                if(do_zero(cpu->registers[reg], TRUE))
                    ERROR("do_zero error\n", 1, "");

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else if( token->expr->right->body.cv->value == 2ull)
            {
                LOG("CONST = %ju\n",token->expr->right->body.cv->value);

                /* get register for operation, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->expr->left, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                if(cvar_left->up_to_date == 0)
                    if(do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                /* load left */
                if(! value_can_trace(token->expr->left)
                    || cpu->registers[reg]->val != cvar_left->body.val)
                    if(do_load(cpu->registers[reg], token->expr->left))
                        ERROR("do_load error\n", 1, "");

                /* set value  */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);

                /* res = left & 1 */
                if(do_jodd(cpu->registers[reg], asmcode->length + 3))
                    ERROR("do_jodd error\n", 1, "");

                if(do_zero(cpu->registers[reg], FALSE))
                    ERROR("do_zero error\n", 1, "");

                if(do_jump(asmcode->length + 3))
                    ERROR("do_jump error\n", 1, "");

                if(do_zero(cpu->registers[reg], FALSE))
                    ERROR("do_zero error\n", 1, "");

                if(do_inc(cpu->registers[reg], FALSE))
                    ERROR("do_inc error\n", 1, "");

                /* we have branch here, so we can't trace value */
                if( value_can_trace(token->res))
                    value_set_symbolic_flag(cvar_res->body.val);

                /* we can't trace value so immediatly store it */
                if(! value_can_trace(token->res) )
                {
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else
            {
                /* get register for res, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* set value  */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* get register for right, synchronize iff needed  */
                reg2 = do_get_register(token_list, token_list_pos, cvar_temp->body.val, FALSE);
                if(reg2 == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg2]);

                /* temp memory for mod */
                reg_set_val(cpu->registers[reg2], cvar_temp->body.val);

                if(do_pump(cpu->registers[reg2], token->expr->right->body.cv->value, FALSE))
                    ERROR("do_pump error\n", 1, "");

                if(do_store(cpu->registers[reg2]))
                    ERROR("do_store error\n", 1, "");

                /* get register for left, synchronize iff needed  */
                reg3 = do_get_register(token_list, token_list_pos, cvar_temp2->body.val, FALSE);
                if(reg3 == -1)
                    ERROR("do_get_register error\n", 1, "");

                if(cvar_left->up_to_date == 0)
                    if(do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg3]);

                /* store left in temp memory */
                if(! value_can_trace(token->expr->left)
                    || cpu->registers[reg3]->val != cvar_left->body.val)
                    if(do_load(cpu->registers[reg3], token->expr->left))
                        ERROR("do_load error\n", 1, "");

                /* temp memory for mod */
                reg_set_val(cpu->registers[reg3], cvar_temp2->body.val);

                if(do_store(cpu->registers[reg3]))
                    ERROR("do_store error\n", 1, "");

                /* do mod */
                if(do_mod(cpu->registers[reg], cpu->registers[reg3], cpu->registers[reg2], FALSE))
                    ERROR("do_mult error\n", 1, "");


                REG_SET_FREE(cpu->registers[reg]);
                REG_SET_FREE(cpu->registers[reg2]);

                /* set value  */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg3], token->res);
                else
                    reg_set_val(cpu->registers[reg3], cvar_res->body.val);

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg3]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg3]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg3]);
                    cvar_res->up_to_date = 0;
                }
            }
        }
    }
    /* both are variables */
    else
    {
        LOG("Both are vars\n", "");

        /* both are not symbolic so pump value */
        if(    value_can_trace(token->expr->left) && ! value_is_symbolic(cvar_left->body.val)
            && value_can_trace(token->expr->right) && ! value_is_symbolic(cvar_right->body.val) )
        {
            LOG("Both are not symbolic\n", "");

            mpz_init(val);

            /* calc value to pump */
            value_get_val(cvar_left->body.val, temp1);
            value_get_val(cvar_right->body.val, temp2);

            if(mpz_cmp_ui(temp2, 0) == 0)
                mpz_set_ui(val, 0);
            else
                mpz_mod(val, temp1, temp2);

            /* get register for operation, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            /* lock reg */
            REG_SET_IN_USE(cpu->registers[reg]);

            /* set value  */
            if( ! value_can_trace(token->res) )
                reg_set_val(cpu->registers[reg], token->res);
            else
                reg_set_val(cpu->registers[reg], cvar_res->body.val);

            if( do_pump_bigvalue(cpu->registers[reg], val, TRUE) )
                ERROR("do_pump_bigvalue error\n", 1, "");

            /* we can't trace value so immediatly store it */
            if( ! value_can_trace(token->res) )
            {
                if( do_store(cpu->registers[reg]) )
                    ERROR("do_store error\n", 1, "");

                /* free reg */
                REG_SET_FREE(cpu->registers[reg]);
            }
            else
            {
                REG_SET_BUSY(cpu->registers[reg]);
                cvar_res->up_to_date = 0;
            }

            mpz_clear(val);
            mpz_clear(temp1);
            mpz_clear(temp2);
        }
        else
        {
            LOG("At least one is symbolic\n", "");

            if(  value_can_trace(token->expr->left) && ! value_is_symbolic(cvar_left->body.val) &&
                mpz_cmp_si(cvar_left->body.val->body.var->body.var->value, 0) == 0)
            {
                /* get register for res, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                REG_SET_IN_USE(cpu->registers[reg]);

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                if(do_zero(cpu->registers[reg], TRUE))
                    ERROR("do_zero error\n", 1, "");

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else if (value_can_trace(token->expr->right) && ! value_is_symbolic(cvar_right->body.val) &&(
                    ( mpz_cmp_si(cvar_right->body.val->body.var->body.var->value, 0) == 0) ||
                    ( mpz_cmp_si(cvar_right->body.val->body.var->body.var->value, 1) == 0) ))
            {
                /* get register for res, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                REG_SET_IN_USE(cpu->registers[reg]);

                /* can we trace or not ? */
                if(value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);
                else
                    reg_set_val(cpu->registers[reg], token->res);

                if(do_zero(cpu->registers[reg], TRUE))
                    ERROR("do_zero error\n", 1, "");

                if(!value_can_trace(token->res))
                {
                    /* STORE val in res */
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else if(value_can_trace(token->expr->right) && ! value_is_symbolic(cvar_right->body.val) &&
                    mpz_cmp_si(cvar_right->body.val->body.var->body.var->value, 2) == 0)
            {
                /* get register for operation, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->expr->left, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* set value  */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);

                if(cvar_left->up_to_date == 0)
                    if(do_synchronize(cvar_left->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                /* load left */
                if(! value_can_trace(token->expr->left)
                    || cpu->registers[reg]->val != cvar_left->body.val)
                    if(do_load(cpu->registers[reg], token->expr->left))
                        ERROR("do_load error\n", 1, "");

                /* res = res & 1 */
                if(do_jodd(cpu->registers[reg], asmcode->length + 3))
                    ERROR("do_jodd error\n", 1, "");

                if(do_zero(cpu->registers[reg], FALSE))
                    ERROR("do_zero error\n", 1, "");

                if(do_jump(asmcode->length + 3))
                    ERROR("do_jump error\n", 1, "");

                if(do_zero(cpu->registers[reg], FALSE))
                    ERROR("do_zero error\n", 1, "");

                if(do_inc(cpu->registers[reg], FALSE))
                    ERROR("do_inc error\n", 1, "");

                /* we have branch here, so we can't trace value */
                if(value_can_trace((token->res)))
                    value_set_symbolic_flag(cvar_res->body.val);

                /* we can't trace value so immediatly store it */
                if(! value_can_trace(token->res) )
                {
                    if(do_store(cpu->registers[reg]))
                        ERROR("do_store error\n", 1, "");

                    /* free reg */
                    REG_SET_FREE(cpu->registers[reg]);
                }
                else
                {
                    REG_SET_BUSY(cpu->registers[reg]);
                    cvar_res->up_to_date = 0;
                }
            }
            else
            {
                /* get register for res, synchronize iff needed  */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* set value  */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar_res->body.val);

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                if(variable_cmp((void*)&token->expr->left->body.var,
                    (void*)&token->expr->right->body.var) == 0)
                {
                    if(do_zero(cpu->registers[reg], TRUE))
                        ERROR("do_zero error\n", 1, "");

                    if(!value_can_trace(token->res))
                    {
                        /* STORE val in res */
                        if(do_store(cpu->registers[reg]))
                            ERROR("do_store error\n", 1, "");

                        /* free reg */
                        REG_SET_FREE(cpu->registers[reg]);
                    }
                    else
                    {
                        REG_SET_BUSY(cpu->registers[reg]);
                        cvar_res->up_to_date = 0;
                    }
                }
                else
                {
                    /* get register for right, synchronize iff needed  */
                    reg3 = do_get_register(token_list, token_list_pos, cvar_temp->body.val, FALSE);
                    if(reg3 == -1)
                        ERROR("do_get_register error\n", 1, "");

                    if(cvar_right->up_to_date == 0)
                        if(do_synchronize(cvar_right->body.val->reg))
                            ERROR("do_synchronize error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg3]);

                    /* store right in temp memory */
                    if(! value_can_trace(token->expr->right)
                        || cpu->registers[reg3]->val != cvar_right->body.val)
                        if(do_load(cpu->registers[reg3], token->expr->right))
                            ERROR("do_load error\n", 1, "");

                    /* temp memory for mod */
                    reg_set_val(cpu->registers[reg3], cvar_temp->body.val);

                    if(do_store(cpu->registers[reg3]))
                        ERROR("do_store error\n", 1, "");

                    /* get register for left, synchronize iff needed  */
                    reg2 = do_get_register(token_list, token_list_pos, cvar_temp2->body.val, FALSE);
                    if(reg2 == -1)
                        ERROR("do_get_register error\n", 1, "");

                    if(cvar_left->up_to_date == 0)
                        if(do_synchronize(cvar_left->body.val->reg))
                            ERROR("do_synchronize error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[reg2]);

                    /* store left in temp memory */
                    if(! value_can_trace(token->expr->left)
                        || cpu->registers[reg2]->val != cvar_left->body.val)
                        if(do_load(cpu->registers[reg2], token->expr->left))
                            ERROR("do_load error\n", 1, "");

                    /* temp memory for mod */
                    reg_set_val(cpu->registers[reg2], cvar_temp2->body.val);

                    if(do_store(cpu->registers[reg2]))
                        ERROR("do_store error\n", 1, "");

                    /* do mod */
                    if(do_mod(cpu->registers[reg], cpu->registers[reg2], cpu->registers[reg3], FALSE))
                        ERROR("do_mult error\n", 1, "");

                    REG_SET_FREE(cpu->registers[reg]);
                    REG_SET_FREE(cpu->registers[reg3]);

                    /* set value  */
                    if( ! value_can_trace(token->res))
                        reg_set_val(cpu->registers[reg2], token->res);
                    else
                        reg_set_val(cpu->registers[reg2], cvar_res->body.val);

                    if(!value_can_trace(token->res))
                    {
                        /* STORE val in res */
                        if(do_store(cpu->registers[reg2]))
                            ERROR("do_store error\n", 1, "");

                        /* free reg */
                        REG_SET_FREE(cpu->registers[reg2]);
                    }
                    else
                    {
                        REG_SET_BUSY(cpu->registers[reg2]);
                        cvar_res->up_to_date = 0;
                    }
                }
            }
        }
    }

    return 0;
}

static int compile_token_assign(token_assign *token)
{
    Cvar *cvar_res;
    Cvar *cvar_left;
    Cvar *cvar_right;

#ifdef DEBUG_MODE
    char *str;
#endif

    TRACE("");

#ifdef DEBUG_MODE
    str = token_assign_str(token);

    LOG("TOKEN: %s\n", str);

    FREE(str);
#endif

    set_array_to_vararr(token->res);
    set_array_to_vararr(token->expr->left);

    cvar_res = cvar_get_by_value(token->res);
    cvar_left = cvar_get_by_value(token->expr->left);

    if(token->expr->op != tokens_id.undefined)
    {
        set_array_to_vararr(token->expr->right);

        cvar_right = cvar_get_by_value(token->expr->right);
    }

    /*  res = left */
    if(token->expr->op == tokens_id.undefined )
    {
        if(__assign(token))
            ERROR("__assign error\n", 1, "");

        if(value_can_trace(token->res))
        {
            if(value_can_trace(token->expr->left))
            {
                if(value_is_symbolic(cvar_left->body.val))
                    value_set_symbolic_flag(cvar_res->body.val);
            }
            else if(token->expr->left->type != CONST_VAL)
                value_set_symbolic_flag(cvar_res->body.val);
        }

    }
    /* res = left OP right */
    else
    {
        /* res = left + right */
        if( token->expr->op == tokens_id.add)
        {
            if(__add(token))
                ERROR("__add error\n", 1, "");
        }
        /* res = left - right */
        else if( token->expr->op == tokens_id.sub)
        {
            if(__sub(token))
                ERROR("__sub error\n", 1, "");
        }
        /* res = left * right */
        else if(token->expr->op == tokens_id.mult)
        {
            if(__mult(token))
                ERROR("__mult error\n", 1, "");
        }
        /* res = left / right */
        else if(token->expr->op == tokens_id.div)
        {
            if(__div(token))
                ERROR("__div error\n", 1, "");
        }
        /* res = left % right  */
        else if(token->expr->op == tokens_id.mod)
        {
            if(__mod(token))
                ERROR("__mod error\n", 1, "");
        }

        if(value_can_trace(token->res))
        {
            if(value_can_trace(token->expr->left))
            {
                if(value_is_symbolic(cvar_left->body.val))
                    value_set_symbolic_flag(cvar_res->body.val);
            }
            else if(token->expr->left->type != CONST_VAL)
                value_set_symbolic_flag(cvar_res->body.val);

            if(value_can_trace(token->expr->right))
            {
                if(value_is_symbolic(cvar_right->body.val))
                    value_set_symbolic_flag(cvar_res->body.val);
            }
            else if(token->expr->right->type != CONST_VAL)
                value_set_symbolic_flag(cvar_res->body.val);
        }
    }
    return 0;
}

static int compile_token_for(token_for *token)
{
#define N   3
#define M   3
    /* to alloc iterators */
    char *it_name = NULL;
    char *hit_name;

    var_normal *hit_vn;
    Variable *hit_var;
    Value *hit_val;

    Value *it_val;

    Cvar *it;
    Cvar *hit;

    /* to create tokens */
    const_value *one;
    Value       *one_val;
    Value       *thit_val[N];
    Value       *begin_value;
    Value       *begin_value2;
    Value       *end_value;
    Value       *iterator;

    token_assign    *ass[M];
    token_expr      *expr[M];

    int i;
    BOOL is_alloc = FALSE;

    mpz_t val1;
    mpz_t val2;

    int reg;

    uint64_t line;

    Cfor *cfor;

    /* another loop */
    ++for_c;

    it = NULL;

    /* always we use helper iterator */
    hit_name = get_local_iterator_name(token->iterator);

    hit_vn = var_normal_create(hit_name);
    if(hit_vn == NULL)
        ERROR("var_normal_create error\n", 1, "");

    hit_var = variable_create(VAR_NORMAL, (void*)hit_vn);
    if(hit_var == NULL)
        ERROR("variable_create error\n", 1, "");

    hit_val = value_create(VARIABLE, (void*)hit_var);
    if(hit_val == NULL)
        ERROR("value_create error\n", 1, "");

    if(! cvar_is_declared_by_name(hit_name))
    {
        LOG("Alloc %s iterator helper\n", hit_name);

        is_alloc = TRUE;

        my_malloc(memory, LOOP_VAR, (void*)hit_val);

        /* Add Value to compiler variables */
        hit = cvar_create(VALUE, (void*)hit_val);
        if(hit == NULL)
            ERROR("cvar_create error\n", 1, "");

        value_set_symbolic_flag(hit->body.val);

        if(avl_insert(compiler_variables, (void*)&hit))
            ERROR("avl_insert error\n", 1, "");
    }

    /* we need iterator */
    if(is_iterator_needed(token_list, token_list_pos, token->iterator))
    {
        it_name = get_iterator_name(token->iterator);
        LOG("We need iterator %s\n", it_name);

        if(! cvar_is_declared_by_name(it_name))
        {
            if( value_copy(&it_val, token->iterator) )
                ERROR("value_copy error\n", 1, "");

            LOG("Alloc iterator %s\n", it_name);

            my_malloc(memory, LOOP_VAR, (void*)it_val);

            /* Add Value to compiler variables */
            it = cvar_create(VALUE, (void*)it_val);
            if(it == NULL)
                ERROR("cvar_create error\n", 1, "");

            value_set_symbolic_flag(it->body.val);

            if(avl_insert(compiler_variables, (void*)&it))
                ERROR("avl_insert error\n", 1, "");
        }
        else
        {
            LOG("Iterator %s is in memory\n", it_name);
            it = cvar_get_by_name(it_name);
        }
    }

    if(token->begin_value->type == CONST_VAL && token->end_value->type == CONST_VAL)
    {
        LOG("FOR LOOP has both const values range\n", "");

        mpz_init(val1);
        mpz_init(val2);

        ull2mpz(val1, token->begin_value->body.cv->value);
        ull2mpz(val2, token->end_value->body.cv->value);

        hit = cvar_get_by_name(hit_name);

        if(token->type == tokens_id.for_inc)
        {
            mpz_add_ui(val2, val2, 1);

            if(mpz_cmp(val2, val1) < 0)
                mpz_set_si(val2, 0);
            else
                mpz_sub(val2, val2, val1);

            /* get register for operation, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, hit->body.val, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            /* lock reg */
            REG_SET_IN_USE(cpu->registers[reg]);

            reg_set_val(cpu->registers[reg], hit->body.val);

            if( do_pump_bigvalue(cpu->registers[reg], val2, TRUE) )
                ERROR("do_pump_bigvalue error\n", 1, "");

            REG_SET_BUSY(cpu->registers[reg]);
            hit->up_to_date = 0;

        }
        else
        {
            mpz_add_ui(val1, val1, 1);

            if(mpz_cmp(val1, val2) < 0)
                mpz_set_si(val1, 0);
            else
                mpz_sub(val1, val1, val2);

            /* get register for operation, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, hit->body.val, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            /* lock reg */
            REG_SET_IN_USE(cpu->registers[reg]);

            reg_set_val(cpu->registers[reg], hit->body.val);

            if( do_pump_bigvalue(cpu->registers[reg], val1, TRUE) )
                ERROR("do_pump_bigvalue error\n", 1, "");

            REG_SET_BUSY(cpu->registers[reg]);
            hit->up_to_date = 0;
        }

        mpz_clear(val1);
        mpz_clear(val2);
    }
    else
    {
        LOG("We have to build we tokens for FOR LOOP\n", "");

        LOG("Compile tokens for iterator helper %s\n", hit_name);

        /* build HI values for tokens */
        for(i = 0; i < N; ++i)
            if (value_copy(&thit_val[i], hit_val) )
                ERROR("value_copy error\n", 1, "");

        if( value_copy(&begin_value, token->begin_value) )
            ERROR("value_copy error\n", 1, "");

        if( value_copy(&end_value, token->end_value) )
            ERROR("value_copy error\n", 1, "");

        one = const_value_create(1ull);
        if(one == NULL)
            ERROR("const_value_create error\n", 1, "");

        one_val = value_create(CONST_VAL, (void*)one);
        if(one_val == NULL)
            ERROR("value_create error\n", 1, "");

        if(token->type == tokens_id.for_inc)
        {
            /* Hi = end + 1 */
            expr[0] = token_expr_create(tokens_id.add, end_value, one_val);
            if(expr[0] == NULL)
                ERROR("token_expr_create error\n", 1, "");

            ass[0] = token_assign_create(thit_val[0], expr[0]);
            if(ass[0] == NULL)
                ERROR("token_assign_create error\n", 1, "");

            /* create Hi = Hi - begin */
            expr[1] = token_expr_create(tokens_id.sub, thit_val[1], begin_value);
            if(expr[1] == NULL)
                ERROR("token_expr_create error\n", 1, "");

            ass[1] = token_assign_create(thit_val[2], expr[1]);
            if(ass[1] == NULL)
                ERROR("token_assign_create error\n", 1, "");
        }
        else
        {
            /* Hi = begin + 1 */
            expr[0] = token_expr_create(tokens_id.add, begin_value, one_val);
            if(expr[0] == NULL)
                ERROR("token_expr_create error\n", 1, "");

            ass[0] = token_assign_create(thit_val[0], expr[0]);
            if(ass[0] == NULL)
                ERROR("token_assign_create error\n", 1, "");

            /* create Hi = Hi - end */
            expr[1] = token_expr_create(tokens_id.sub, thit_val[1], end_value);
            if(expr[1] == NULL)
                ERROR("token_expr_create error\n", 1, "");

            ass[1] = token_assign_create(thit_val[2], expr[1]);
            if(ass[1] == NULL)
                ERROR("token_assign_create error\n", 1, "");
        }

        compile_token_assign(ass[0]);
        compile_token_assign(ass[1]);

        token_assign_destroy(ass[0]);
        token_assign_destroy(ass[1]);
    }

    /* to avoid overwrite HIT register by IT reg set in use */
    REG_SET_IN_USE(hit->body.val->reg);

    if(it != NULL)
    {
        LOG("Compile tokens for iterator %s\n", it_name);

        if(value_copy(&iterator, token->iterator))
            ERROR("value_copy error\n", 1, "");

        if(value_copy(&begin_value2, token->begin_value))
            ERROR("value_copy error\n", 1, "");

        /*  IT = begin */
        expr[2] = token_expr_create(tokens_id.undefined, begin_value2, NULL);
        if(expr[2] == NULL)
            ERROR("token_expr_create error\n", 1, "");

        ass[2] = token_assign_create(iterator, expr[2]);
        if(ass[2] == NULL)
            ERROR("token_assign_create error\n", 1, "");

        compile_token_assign(ass[2]);
        token_assign_destroy(ass[2]);

        /* set reg in use */
        REG_SET_IN_USE(it->body.val->reg);
    }

    cfor = cfor_create(it, hit, it == NULL ? NULL : it->body.val->reg,
            hit->body.val->reg, token->type);
    if(cfor == NULL)
        ERROR("cfor_create error\n", 1, "");

    if(stack_push(forloops, (void*)&cfor))
        ERROR("stack_push error\n", 1, "");

    if(! is_alloc)
        value_destroy(hit_val);

    /* sync all but not it and hit registers */
    if( sync_all() )
        ERROR("sync_all error\n", 1, "");

    line = asmcode->length;

    /* create cond for loop */
    if(do_jzero_label(hit->body.val->reg, LABEL_END))
        ERROR("do_jzero_label error\n", 1, "");

    /* for begin line */
    if(stack_push(looplines, (void*)&line))
        ERROR("stack_push error\n", 1, "");

    REG_SET_BUSY(hit->body.val->reg);

    if(it != NULL)
        REG_SET_BUSY(it->body.val->reg);

    FREE(hit_name);
    if(it_name != NULL)
        FREE(it_name);

    return 0;
#undef N
#undef M
}

static int compile_token_guard(token_guard *token)
{
    uint64_t line;
    Cfor *cfor;

    Cvar *ptr;

    if(token->type == tokens_id.end_while)
    {
        LOG("ENDWHILE\n", "");

        LOG("SYNC ALL\n", "");
        sync_all();

        if(stack_pop(looplines, (void*)&line))
            ERROR("do_stack error\n", 1, "");

        LOG("Compile JUMP to begining\n", "");

        if(do_jump(line))
            ERROR("do_jump error\n", 1, "");

        label_to_line(asmcode->length);
        label_to_line(asmcode->length);

        --while_c;
    }
    else if(token->type == tokens_id.end_for)
    {
        LOG("ENDFOR\n", "");

        /* get for declaration */
        if(stack_pop(forloops, (void*)&cfor))
            ERROR("stack_pop error\n", 1, "");

        /* we have good reg now, no change is needed */
        if(cfor->hit->body.val->reg == cfor->hit_reg)
        {
            LOG("Iterator helper %s HAS GOOD REG\n", cfor->hit->name);
            REG_SET_IN_USE(cfor->hit_reg);
        }
        else
            LOG("Iterator helper %s HAS INCORRECT REG\n", cfor->hit->name);


        /* we have good reg now, no change is needed */
        if(cfor->it_needed == 1)
        {
            if(cfor->it->body.val->reg == cfor->it_reg)
            {
                LOG("Iterator %s HAS GOOD REG\n", cfor->it->name);
                REG_SET_IN_USE(cfor->it_reg);
            }
            else
                LOG("Iterator %s HAS INCORRECT REG\n", cfor->it->name);
        }

        LOG("SYNC ALL\n", "");

        /* sync only no-for registers */
        if(sync_all())
            ERROR("sync_all error\n", 1, "");

        /* need load */
        if( ! IS_REG_IN_USE(cfor->hit_reg))
        {
            LOG("LOAD REG FOR Iterator helper %s\n", cfor->hit->name);

            if(do_load(cfor->hit_reg, cfor->hit->body.val))
                ERROR("do_load error\n", 1, "");

            REG_SET_IN_USE(cfor->hit_reg);
            reg_set_val(cfor->hit_reg, cfor->hit->body.val);
        }

        LOG("DEC Iterator Helper %s\n", cfor->hit->name);

        /* dec counter */
        if(do_dec(cfor->hit_reg, TRUE))
            ERROR("do_dec error\n", 1, "");

        cfor->hit->up_to_date = 0;

        REG_SET_FREE(cfor->hit_reg);

        if(cfor->it_needed == 1)
        {
            /* need load */
            if(  ! IS_REG_IN_USE(cfor->it_reg))
            {
                LOG("LOAD REG FOR Iterator %s\n", cfor->it->name);

                if(do_load(cfor->it_reg, cfor->it->body.val))
                    ERROR("do_load error\n", 1, "");

                REG_SET_IN_USE(cfor->it_reg);
                reg_set_val(cfor->it_reg, cfor->it->body.val);
            }

            if(cfor->for_type == FOR_DEC)
            {
                LOG("DEC Iterator %s\n", cfor->it->name);

                if(do_dec(cfor->it->body.val->reg, TRUE))
                    ERROR("do_dec error\n", 1, "");
            }
            else
            {
                LOG("INC Iterator %s\n", cfor->it->name);

                if(do_inc(cfor->it->body.val->reg, TRUE))
                    ERROR("do_dec error\n", 1, "");
            }

            cfor->it->up_to_date = 0;

            REG_SET_FREE(cfor->it->body.val->reg);
        }

        LOG("FREE Iterator helper %s\n", cfor->hit->name);

        my_free(memory, cfor->hit->body.val->chunk->addr);

        if(cfor->it_needed == 1)
        {
            LOG("FREE Iterator %s\n", cfor->it->name);

            my_free(memory, cfor->it->body.val->chunk->addr);
        }

        /* create jump to begining */
        if(stack_pop(looplines, (void*)&line))
            ERROR("do_stack error\n", 1, "");

        LOG("Compile JUMP TO begining\n", "");

        if(do_jump(line))
            ERROR("do_jump error\n", 1, "");

        /* here we have end of loop */
        label_to_line(asmcode->length);
        --for_c;

        LOG("Set PTR REG to SYMBOLIC\n", "");

        ptr = cvar_get_by_name(PTR_NAME);
        value_set_symbolic_flag(ptr->body.val);

        if(avl_delete(compiler_variables, (void*)&cfor->hit))
            ERROR("avl_delete error\n", 1 ,"");

        if(cfor->it_needed == 1)
            if(avl_delete(compiler_variables, (void*)&cfor->it))
                ERROR("avl_delete error\n", 1, "");

        cfor_destroy(cfor);
    }
    else if(token->type == tokens_id.else_cond)
    {
        if(sync_all())
            ERROR("sync_all error\n", 1, "");

        /* is the last if line */
        if(need_jump(token_list, token_list_pos))
        {
            /* change if cond labels to line */
            if(label_to_line(asmcode->length + 1))
                ERROR("label_to_line error\n", 1, "");

            if(label_to_line(asmcode->length + 1))
                ERROR("label_to_line error\n", 1, "");

            LOG("NEED JUMP BEFORE ELSE\n", "");
            if(do_jump_label(LABEL_END))
                ERROR("do_jump_label error\n", 1, "");
        }
        else
        {
            /* change if cond labels to line */
            if(label_to_line(asmcode->length))
                ERROR("label_to_line error\n", 1, "");

            if(label_to_line(asmcode->length))
                ERROR("label_to_line error\n", 1, "");

            LOG("JUMP BEFORE ELSE NO NEEDED\n", "");

            if(label_fake())
                ERROR("label_fake error\n", 1, "");
        }

    }
    else if(token->type == tokens_id.end_if)
    {
        if(sync_all())
            ERROR("sync_all error\n", 1, "");

        if(label_to_line(asmcode->length))
            ERROR("label_to_line error\n", 1, "");

        --if_c;
    }

    return 0;
}

static int compile_token_if(token_if *token)
{
    TRACE("");

    /* sync all */
    if(sync_all())
        ERROR("sync_all error\n", 1, "");

    /* compile token if */
    if(__cond(token->cond))
        ERROR("__cond error\n", 1, "");

    ++if_c;

    return 0;
}

static int compile_token_io(token_io *token)
{
    int reg;
    Cvar *cvar;

#ifdef DEBUG_MODE
    char *str;
#endif

    TRACE("");

#ifdef DEBUG_MODE
    str = token_io_str(token);

    LOG("TOKEN: %s\n", str);

    FREE(str);
#endif

    cvar = cvar_get_by_value(token->res);

    /* READ */
    if(token->op == tokens_id.read)
    {
        LOG("READ\n", "");

        set_array_to_vararr(token->res);

        /* get register for operation, synchronize iff needed  */
        reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
        if(reg == -1)
            ERROR("do_get_register error\n", 1, "");

        /* lock reg */
        REG_SET_IN_USE(cpu->registers[reg]);

        /* set value  */
        if( ! value_can_trace(token->res))
            reg_set_val(cpu->registers[reg], token->res);
        else
            reg_set_val(cpu->registers[reg], cvar->body.val);

        /* get value from stdin */
        if(do_get(cpu->registers[reg], TRUE) )
            ERROR("do_get_error\n", 1, "");

        /* we can't trace value so imediatly store it */
        if(! value_can_trace(token->res) )
        {
            if(do_store(cpu->registers[reg]))
                ERROR("do_store error\n", 1, "");

            /* free reg */
            REG_SET_FREE(cpu->registers[reg]);
        }
        else
            REG_SET_BUSY(cpu->registers[reg]);
    }
    else /* WRITE */
    {
        LOG("WRITE\n", "");

        /* write const val */
        if(token->res->type == CONST_VAL)
        {
            LOG("res is const\n", "");

            /* get register for operation, synchronize iff needed  */
            reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
            if(reg == -1)
                ERROR("do_get_register error\n", 1, "");

            /* lock reg */
            REG_SET_IN_USE(cpu->registers[reg]);

            /* pump value to reg, write it and free register */
            if( do_pump(cpu->registers[reg], token->res->body.cv->value, FALSE) )
                ERROR("do_pump error\n", 1, "");

            if(do_put(cpu->registers[reg]))
                ERROR("do_put error\n", 1, "");

            REG_SET_FREE(cpu->registers[reg]);
        }
        else /* write variable */
        {
            LOG("res is var\n", "");

            if(token->res->body.var->type == VAR_NORMAL)
            {
                LOG("res is var_normal\n", "");

                /* get register for operation */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* we have variable in reg so put it on stdout */
                if(cpu->registers[reg]->val == cvar->body.val)
                {
                    /* PUT To stdin */
                    if(do_put(cpu->registers[reg]))
                        ERROR("do_put error\n", 1, "");

                    REG_SET_BUSY(cpu->registers[reg]);
                }
                else /* we need load var to reg */
                {
                    /* if var is const pump up value and print it  */
                    if(! value_is_symbolic(cvar->body.val)
                        && BPUMP_COST(cvar->body.val->body.var->body.var->value) < op_cost.load )
                    {
                        LOG("res is not symbolic\n", "");

                        if(do_pump_bigvalue(cpu->registers[reg], cvar->body.val->body.var->body.var->value, TRUE) )
                            ERROR("do_pump error\n", 1, "");

                        /* PUT To stdin */
                        if(do_put(cpu->registers[reg]))
                            ERROR("do_put error\n", 1, "");
                    }
                    else /* we have to load value from memmory and put it on stdout */
                    {
                        LOG("res is symbolic\n", "");

                        /* LOAD VALUE TO REG */
                        if(do_load(cpu->registers[reg], token->res) )
                            ERROR("do_load error\n", 1, "");

                        /* PUT To stdin */
                        if(do_put(cpu->registers[reg]))
                            ERROR("do_put error\n", 1, "");
                    }

                    /* set value  */
                    if( ! value_can_trace(token->res))
                    {
                        reg_set_val(cpu->registers[reg], token->res);

                        REG_SET_FREE(cpu->registers[reg]);
                    }
                    else
                    {
                        reg_set_val(cpu->registers[reg], cvar->body.val);

                        REG_SET_BUSY(cpu->registers[reg]);
                    }
                }
            }
            else
            {
                LOG("res is array\n", "");

                set_array_to_vararr(token->res);

                /* get register for operation */
                reg = do_get_register(token_list, token_list_pos, token->res, FALSE);
                if(reg == -1)
                    ERROR("do_get_register error\n", 1, "");

                /* lock reg */
                REG_SET_IN_USE(cpu->registers[reg]);

                /* set value  */
                if( ! value_can_trace(token->res))
                    reg_set_val(cpu->registers[reg], token->res);
                else
                    reg_set_val(cpu->registers[reg], cvar->body.val);

                /* LOAD VALUE TO REG */
                if(do_load(cpu->registers[reg], token->res) )
                    ERROR("do_load error\n", 1, "");

                /* PUT To stdin */
                if(do_put(cpu->registers[reg]))
                    ERROR("do_put error\n", 1, "");

                /* we can't trace array so free reg */
                REG_SET_FREE(cpu->registers[reg]);
            }
        }
    }
    return 0;
}

static int compile_token_while(token_while *token)
{
    Cvar *left;
    Cvar *right;

    uint64_t line;

    /* SYNC ALL */
    sync_all();

    /* we want to jump here to recheck cond, so save this line on stack */
    line = asmcode->length;


    if(value_can_trace(token->cond->left))
    {
        left = cvar_get_by_value(token->cond->left);
        value_set_symbolic_flag(left->body.val);
    }

    if(value_can_trace(token->cond->right))
    {
        right = cvar_get_by_value(token->cond->right);
        value_set_symbolic_flag(right->body.val);
    }

    if(stack_push(looplines, (void*)&line))
        ERROR("stack_push error\n", 1, "");

    /* COMPILE COND next instruction have to be 1st instruction from loop */
    if(__cond(token->cond))
        ERROR("__cond error\n", 1, "");

    /* go in loop, trigger symbolic setting */
    ++while_c;

    return 0;
}

static int compile(Arraylist *tokens)
{
    Avl_iterator it;
    Cvar *cvar;
    Pvar *pvar;

    Arraylist_iterator ait;
    char *line;

#ifdef DEBUG_MODE
    char *str = NULL;
#endif

    TRACE("");

    if(tokens == NULL)
        ERROR("tokens == NULL\n", 1, "");

    /* PREPARE BOARD */
    if( board_init() )
        ERROR("board_init error\n", 1, "");

    /* PREPARE COMPILER VARIABLES AND MEMORY MANAGMENT */
    compiler_variables = avl_create(sizeof(Cvar*), cvar_cmp);
    if(compiler_variables == NULL)
        ERROR("avl_create error\n", 1, "");

    /* ADD special Variable PTR for pointer from PTR */
    pvar = pvar_create(PTR_NAME, PTOKEN_VAR, 0);
    if(pvar == NULL)
        ERROR("pvar_create error\n", 1, "");

    if(avl_insert(variables, (void*)&pvar))
        ERROR("avl_insert error\n", 1, "");

    /* ADD temp VARIABLE */
    pvar = pvar_create(TEMP1_NAME, PTOKEN_VAR, 0);
    if(pvar == NULL)
        ERROR("pvar_create error\n", 1, "");

    if(avl_insert(variables, (void*)&pvar))
        ERROR("avl_insert error\n", 1, "");

    /* ADD temp VARIABLE */
    pvar = pvar_create(TEMP2_NAME, PTOKEN_VAR, 0);
    if(pvar == NULL)
        ERROR("pvar_create error\n", 1, "");

    if(avl_insert(variables, (void*)&pvar))
        ERROR("avl_insert error\n", 1, "");

    if( prepare_mem_sections(variables, tokens) )
        ERROR("prepare_mem_sections error\n", 1, "");

    /* VARS NO NEED MEMORY */
    /* ADD temp VARIABLE */
    pvar = pvar_create(TEMP_ADDR_NAME, PTOKEN_VAR, 0);
    if(pvar == NULL)
        ERROR("pvar_create error\n", 1, "");

    if(avl_insert(variables, (void*)&pvar))
        ERROR("avl_insert error\n", 1, "");

    /* ADD temp VARIABLE */
    pvar = pvar_create(TEMP_DIV_HELPER, PTOKEN_VAR, 0);
    if(pvar == NULL)
        ERROR("pvar_create error\n", 1, "");

    if(avl_insert(variables, (void*)&pvar))
        ERROR("avl_insert error\n", 1, "");

    if(alloc_variables(variables))
        ERROR("alloc variables error\n", 1, "");


#ifdef DEBUG_MODE
    str = mpz_get_str(str, 10, memory->big_arrays_allocated);

    LOG("MEMORY ALLOCATED\n", "");
    LOG("ALLOCATED VARS: %ju\n", memory->var_allocated);
    LOG("ALLOCATED LOOP: %ju\n", memory->loop_var_allocated);
    LOG("ALLOCATED ARRS: %ju\n", memory->arrays_allocated);
    LOG("ALLOCATED BIGA: %s\n", str);

    FREE(str);
#endif

    /* Prepare arraylist with asm code lines */
    asmcode = arraylist_create(sizeof(char*));
    if(asmcode == NULL)
        ERROR("arraylist_create error\n", 1, "");

    cvar = cvar_get_by_name(PTR_NAME);
    reg_set_val(cpu->registers[REG_PTR], cvar->body.val);
    REG_SET_BUSY(cpu->registers[REG_PTR]);

    /* Prepare stack with labels */
    labels = stack_create(sizeof(Label*));
    if(labels == NULL)
        ERROR("stack_create error\n", 1, "");

    /* Prepare stack with loop lines */
    looplines = stack_create(sizeof(uint64_t));
    if(looplines == NULL)
        ERROR("stack_create error\n", 1, "");

    /* Prepare stack with Cfor */
    forloops = stack_create(sizeof(Cfor*));
    if(forloops == NULL)
        ERROR("stack_create error\n", 1, "");

    if(compiler_helper(tokens))
        ERROR("compile error\n", 1, "");

    /* write lines to file */
    for(  arraylist_iterator_init(asmcode, &ait, ITI_BEGIN);
        ! arraylist_iterator_end(&ait);
          arraylist_iterator_next(&ait))
        {
            arraylist_iterator_get_data(&ait, (void*)&line);
            file_buffer_append(fb, line);
            FREE(line);
        }

    file_buffer_synch(fb);

    /* CLEANUP */
    cvar = cvar_get_by_name(TEMP_ADDR_NAME);
    value_destroy(cvar->body.val);

    cvar = cvar_get_by_name(TEMP_DIV_HELPER);
    value_destroy(cvar->body.val);

    for(  avl_iterator_init(compiler_variables, &it, ITI_BEGIN);
        ! avl_iterator_end(&it);
        avl_iterator_next(&it))
        {
            avl_iterator_get_data(&it, (void*)&cvar);

            cvar_destroy(cvar);
        }

    avl_destroy(compiler_variables);
    arraylist_destroy(asmcode);
    stack_destroy(labels);
    stack_destroy(looplines);
    stack_destroy(forloops);
    board_destroy();

    return 0;
}

uint64_t calc_loop_mem_section(Arraylist *tokens)
{
    uint64_t max = 0;
    uint64_t cur = 0;

    Arraylist_iterator it;
    Token *token;

    TRACE("");

    if(tokens == NULL)
        ERROR("tokens == NULL\n", 0, "");

    for(  arraylist_iterator_init(tokens, &it, ITI_BEGIN);
        ! arraylist_iterator_end(&it);
          arraylist_iterator_next(&it))
        {
            arraylist_iterator_get_data(&it, (void*)&token);
            if(token->type == TOKEN_FOR)
                cur += 2; /* iterator and end value */

            if(token->type == TOKEN_GUARD && token->body.guard->type == tokens_id.end_for)
                cur -= 2; /* iterator and end value */

            max = MAX(max, cur);
        }

    return max;
}

int prepare_mem_sections(Avl *vars, Arraylist *tokens)
{
    Pvar *var;

    uint64_t mem_vars;
    uint64_t mem_arrays;
    uint64_t mem_loop_vars;

    Avl_iterator it;

#ifdef DEBUG_MODE
    char *str = NULL;
#endif

    TRACE("");

    if(vars == NULL)
        ERROR("vars == NULL\n", 1, "");

    mem_vars = 0;
    mem_arrays = 0;

    for(  avl_iterator_init(vars, &it, ITI_BEGIN);
        ! avl_iterator_end(&it);
          avl_iterator_next(&it))
        {
                avl_iterator_get_data(&it, (void*)&var);

                if(var->type == PTOKEN_VAR)
                    ++mem_vars;
                else
                {
                    if(var->array_len <= ARRAY_MAX_LEN)
                        mem_arrays += var->array_len;
                }
        }

    mem_loop_vars = calc_loop_mem_section(tokens);

    memory->loop_var_first_addr = 0;
    memory->loop_var_last_addr = memory->loop_var_first_addr + mem_loop_vars - 1;

    memory->var_first_addr = memory->loop_var_last_addr + 1;
    memory->var_last_addr = memory->var_first_addr + mem_vars - 1;

    memory->arrays_first_addr = memory->var_last_addr + 1;
    memory->arrays_last_addr = memory->arrays_first_addr + mem_arrays - 1;

    ull2mpz(memory->big_arrays_first_addr, memory->arrays_last_addr + 1);

    /*
        OLD CONFIG
        memory->var_first_addr = 0;
        memory->var_last_addr = memory->var_first_addr + mem_vars - 1;

        memory->loop_var_first_addr = memory->var_last_addr + 1;
        memory->loop_var_last_addr = memory->loop_var_first_addr + mem_loop_vars - 1;

        memory->arrays_first_addr = memory->loop_var_last_addr + 1;
        memory->arrays_last_addr = memory->arrays_first_addr + mem_arrays - 1;

        ull2mpz(memory->big_arrays_first_addr, memory->arrays_last_addr + 1);
    */

#ifdef DEBUG_MODE
    str = mpz_get_str(str, 10, memory->big_arrays_first_addr);

    LOG("MEMORY\n", "");
    LOG("VARS: %ju --- %ju\n", memory->var_first_addr, memory->var_last_addr);
    LOG("LOOP: %ju --- %ju\n", memory->loop_var_first_addr, memory->loop_var_last_addr);
    LOG("ARRS: %ju --- %ju\n", memory->arrays_first_addr, memory->arrays_last_addr);
    LOG("BIGA: %s  --- INF\n", str);

    FREE(str);
#endif

    return 0;
}

Cvar *cvar_create(VAR_TYPE type, void* str)
{
    Cvar *var;

    TRACE("");

    if(str == NULL)
        ERROR("str == NULL\n", NULL, "");

    var = (Cvar*)malloc(sizeof(Cvar));
    if(var == NULL)
        ERROR("malloc error\n", NULL, "");

    /* No value here and in memory so we assume that is up to date */
    var->up_to_date = 1;
    var->padding = 0;

    var->type = type;
    if(type == VALUE)
    {
        if(((Value*)str)->body.var->type == VAR_NORMAL)
        {
            if(asprintf(&var->name, "%s", ((Value*)str)->body.var->body.var->name) == -1)
                ERROR("asprintf error\n", NULL, "");
        }
        else
        {
            if(asprintf(&var->name, "%s", ((Value*)str)->body.var->body.arr->var->name) == -1)
                ERROR("asprintf error\n", NULL, "");
        }

        var->body.val = (Value*)str;
    }
    else
    {
        if(asprintf(&var->name, "%s", ((Array*)str)->name) == -1)
            ERROR("asprintf error\n", NULL, "");

        var->body.arr = (Array*)str;
    }

    return var;
}

void cvar_destroy(Cvar *cvar)
{
    TRACE("");

    if(cvar == NULL)
    {
        LOG("cvar == NULL\n", "");

        return;
    }

    FREE(cvar->name);

    if(cvar->type == ARRAY || cvar->type == BIG_ARRAY)
    {
        if(cvar->type == ARRAY)
            darray_destroy(cvar->body.arr->arr);

        mpz_clear(cvar->body.arr->addr);

        FREE(cvar->body.arr->name);
        FREE(cvar->body.arr);
    }

    FREE(cvar);
}

int cvar_cmp(void *a, void *b)
{
    Cvar *_cvar1 = *(Cvar**)a;
    Cvar *_cvar2 = *(Cvar**)b;

    int ret = strcmp(_cvar1->name, _cvar2->name);

    if(ret < 0)
        return -1;

    if(ret > 0)
        return 1;

    return 0;
}

Cvar *cvar_get_by_value(Value *val)
{
    char *name;

    TRACE("");

    if(val->type == CONST_VAL)
        return NULL;

    if(val->body.var->type == VAR_NORMAL)
        name = val->body.var->body.var->name;
    else
        name = val->body.var->body.arr->var->name;

    return cvar_get_by_name(name);
}

Cvar *cvar_get_by_name(const char *name)
{
    Cvar *in;
    Cvar *out;

    var_normal *vn;
    Variable *var;
    Value *val;

    TRACE("");

    vn = var_normal_create(name);
    if(vn == NULL)
        ERROR("var_normal_create error\n", NULL, "");

    var = variable_create(VAR_NORMAL, (void*)vn);
    if(var == NULL)
        ERROR("variable_create error\n", NULL, "");

    val = value_create(VARIABLE, (void*)var);
    if(val == NULL)
        ERROR("value_create error\n", NULL, "");

    in = cvar_create(VALUE, (void*)val);
    if(in == NULL)
        ERROR("cvar_create error\n", NULL, "");

    if( avl_search(compiler_variables, (void*)&in, (void*)&out) )
        ERROR("cvar %s doesn't exists\n", NULL, name);

    cvar_destroy(in);
    value_destroy(val);

    return out;
}

BOOL cvar_is_declared_by_name(const char *name)
{
    Cvar *in;

    var_normal *vn;
    Variable *var;
    Value *val;

    BOOL res;

    TRACE("");

    vn = var_normal_create(name);
    if(vn == NULL)
        ERROR("var_normal_create error\n", FALSE, "");

    var = variable_create(VAR_NORMAL, (void*)vn);
    if(var == NULL)
        ERROR("variable_create error\n", FALSE, "");

    val = value_create(VARIABLE, (void*)var);
    if(val == NULL)
        ERROR("value_create error\n", FALSE, "");

    in = cvar_create(VALUE, (void*)val);
    if(in == NULL)
        ERROR("cvar_create error\n", FALSE, "");

    res =  avl_key_exist(compiler_variables, (void*)&in);

    cvar_destroy(in);
    value_destroy(val);

    return res;
}

BOOL cvar_is_declared_by_value(Value *val)
{
    char *name;

    TRACE("");

    if(val->type == CONST_VAL)
        return FALSE;

    if(val->body.var->type == VAR_NORMAL)
        name = val->body.var->body.var->name;
    else
        name = val->body.var->body.arr->var->name;

    return cvar_is_declared_by_name(name);
}

Label *label_create(Arraylist_node *ptr, int8_t type)
{
    Label *label;

    TRACE("");

    if(ptr == NULL && type != LABEL_FAKE)
        ERROR("ptr == NULL\n", NULL, "");

    label = (Label*)malloc(sizeof(Label));
    if(label == NULL)
        ERROR("malloc error\n", NULL , "");

    label->ptr = ptr;
    label->type = type;

    return label;
}

void label_destroy(Label *label)
{
    TRACE("");

    if(label == NULL)
    {
        LOG("label == NULL\n", "");

        return;
    }

    FREE(label);
}

Cfor *cfor_create(Cvar *it, Cvar *hit, Register *it_reg, Register *hit_reg, uint8_t for_type)
{
    Cfor *cfor;

    TRACE("");

    if(hit == NULL || hit_reg == NULL)
        ERROR("hit == NULL || hit_reg == NULL\n", NULL, "");

    cfor = (Cfor*)malloc(sizeof(Cfor));
    if(cfor == NULL)
        ERROR("malloc error\n", NULL, "");

    cfor->hit = hit;
    cfor->it = it;
    cfor->it_reg = it_reg;
    cfor->hit_reg = hit_reg;

    cfor->for_type = for_type == tokens_id.for_dec ? FOR_DEC : FOR_INC;
    cfor->it_needed = it != NULL;
    cfor->padding = 0;

    return cfor;
}

void cfor_destroy(Cfor *cfor)
{
    TRACE("");

    if(cfor == NULL)
    {
        LOG("cfor == NULL\n", "");

        return;
    }

    cvar_destroy(cfor->hit);

    if(cfor->it_needed == 1)
        cvar_destroy(cfor->it);

    FREE(cfor);
}

int main_compile(void)
{
    int ret;
    Arraylist *tokens = NULL;
    Arraylist_iterator it;
    Token *token;

    Avl_iterator avlit;
    Pvar *pvar;

    int fd;

    TRACE("");

    if(option.input_file == NULL || option.output_file == NULL)
        ERROR("input file == NULL || output file == NULL\n", 1, "");

    /* parsing source code to token list */
    ret = parse(option.input_file, &tokens);
    if(ret)
        ERROR("parsing error\n", 1, "");

    /* if we want optimalization do it to the same list */
    if(option.optimal)
    {
        ret = main_optimizing(tokens, &tokens);
        if(ret)
            ERROR("optimalization error\n", 1, "");
    }

    /* buffer output file */
    fd = open(option.output_file, O_RDWR | O_LARGEFILE | O_TRUNC | O_CREAT, 0644);
    if(fd == -1)
        ERROR("cannot open %s file\n", 1, option.output_file);

    /* create File buffer for this file */
    fb = file_buffer_create(fd, PROT_READ | PROT_WRITE | MAP_SHARED);
    if(fb == NULL)
        ERROR("file_buffer_create error\n", 1, "");

    if(option.tokens)
    {
        /* write token list to file */
        ret = write_tokens(tokens);
        if(ret)
            ERROR("write tokens error\n", 1, "");
    }
    else
    {
        /* compile tokens to asm code */
        ret = compile(tokens);
        if(ret)
            ERROR("compiling error\n", 1, "");
    }

    /* destroy tokens */
    for(  arraylist_iterator_init(tokens, &it, ITI_BEGIN);
        ! arraylist_iterator_end(&it);
          arraylist_iterator_next(&it))
        {
            arraylist_iterator_get_data(&it, (void*)&token);

            token_destroy(token);
        }

    arraylist_destroy(tokens);

    /* destroy variables */
    for(  avl_iterator_init(variables, &avlit, ITI_BEGIN);
        ! avl_iterator_end(&avlit);
          avl_iterator_next(&avlit))
        {
            avl_iterator_get_data(&avlit, (void*)&pvar);

            pvar_destroy(pvar);
        }

    avl_destroy(variables);

    /* destroy buffer */
    file_buffer_destroy(fb);
    close(fd);

    return 0;
}
