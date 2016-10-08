#include <compiler_algo.h>

/* extern from compiler.h */
Arraylist *token_list;
Stack *labels;
uint64_t token_list_pos;

/*
    strcmp

    PARAMS
    @IN a - 1st string
    @IN b - 2nd string

    RETURN:
    -1 iff a < b
    0 iff a == b
    1 iff a > b
*/

static int my_strcmp(void *a , void *b) __nonull__(1, 2);

/*
    Analize token list from @from to end list and get the best register

    PARAMS
    @IN tokens - tokens list
    @IN from - number of token from we start analyze

    RETURN
    -1 iff failure
    Reg number iff success
*/
static int get_register_num(Arraylist *tokens, uint64_t from) __nonull__(1);

int my_strcmp(void *a , void *b)
{
    char *_a = *(char**)a;
    char *_b = *(char**)b;

    int ret = strcmp(_a, _b);

    if(ret < 0)
        return -1;

    if(ret > 0)
        return 1;

    return 0;
}

static int get_register_num(Arraylist *tokens, uint64_t from)
{
#define GET_REG_BY_NAME_PTR(REG, NAME) \
    do{ \
        for(i = 0; i < reg_to_trace; ++i) \
            if(names[i] == NAME) /* ADDR(names[i]) == ADDR(NAME) */ \
                REG = i + 1; \
    }while(0)

#define CHECK_WINNER(REG) \
    do{\
        if(regs->num_entries == 1) \
        { \
            GET_REG_BY_NAME_PTR(REG, (char*)(*((char**)regs->array))); \
            goto clean; \
        } \
    }while(0)

#define DELETE_VAL_BY_STR(STR) \
    do{ \
        pos = darray_search_first(regs, (void*)&STR); \
        if(pos != -1 ) \
            darray_delete_pos(regs, pos); \
    }while(0)

#define DELETE_VAL(VAL) \
    do{ \
        val_name = value_str(VAL); \
        if(val_name == NULL) \
            ERROR("val_name == NULL\n", -1 ,""); \
        \
        DELETE_VAL_BY_STR(val_name); \
        \
        FREE(val_name); \
        CHECK_WINNER(reg); \
    }while(0)

#define DELETE_IT(FOR) \
    do{ \
        val_name = get_iterator_name(FOR->iterator); \
        if(val_name == NULL) \
            ERROR("val_name == NULL\n", -1 ,""); \
        \
        DELETE_VAL_BY_STR(val_name); \
        \
        FREE(val_name); \
        CHECK_WINNER(reg); \
    }while(0)

#define DELETE_HIT(FOR) \
 do{ \
        val_name = get_local_iterator_name(FOR->iterator); \
        if(val_name == NULL) \
            ERROR("val_name == NULL\n", -1 ,""); \
        \
        DELETE_VAL_BY_STR(val_name); \
        \
        FREE(val_name); \
        CHECK_WINNER(reg); \
    }while(0)

    int reg;

    int i;
    int pos;

    int while_counter = 0;
    int for_counter = 0;
    int if_counter = 0;

    Token *token;
    Arraylist_iterator it;

    Token *back_token;
    Arraylist_iterator back;

    uint64_t counter = 0;
    int reg_to_trace = 0;

    Darray *regs;
    Darray_iterator dit;

    char *names[REGS_NUMBER - 1];

    char *val_name;

    TRACE("");

    if(tokens == NULL)
        ERROR("tokens == NULL\n", -1, "");

    /* we have free reg so use it */
    reg = get_free_reg(cpu);
    if(reg != -1)
        return reg;

    memset(names, 0, sizeof(char*) * ARRAY_SIZE(names));

    regs = darray_create(UNSORTED, 0, sizeof(char*), my_strcmp);
    if(regs == NULL)
        ERROR("regs == NULL\n", -1, "");

    /* create sorted array with value name in regs */
    for(i = REG_PTR + 1; i < REGS_NUMBER; ++i)
    {
        if(cpu->registers[i]->val != NULL)
        {
            names[reg_to_trace] = value_str(cpu->registers[i]->val);
            if(names[reg_to_trace] == NULL)
                ERROR("val_name == NULL\n", -1, "");

            /* can't use regster in use */
            if( ! IS_REG_IN_USE(cpu->registers[i]))
            {
                if(darray_insert(regs, (void*)&names[reg_to_trace]))
                    ERROR("darray_insert error\n", -1, "");
            }

            ++reg_to_trace;
        }
    }

    CHECK_WINNER(reg);

    /**** analyze tokens and choose the best reg *****/

    /* skip obsolete tokens */
    for(  arraylist_iterator_init(tokens, &it, ITI_BEGIN);
        ! arraylist_iterator_end(&it) && counter < from;
          arraylist_iterator_next(&it))
        {
            ++counter;
        }

    /* save checkpoint for going back */
    if( memcpy((void*)&back, (void*)&it, sizeof(Arraylist_iterator)) == NULL )
        ERROR("mempcy error\n", -1, "");

    /* for each token on list DO: */
    for( ;
        ! arraylist_iterator_end(&it);
          arraylist_iterator_next(&it))
        {
            arraylist_iterator_get_data(&it, (void*)&token);

            /* analyze used values in this token */
            if(token->type == TOKEN_IO)
            {
                if(token->body.io->res->type == VARIABLE)
                    DELETE_VAL(token->body.io->res);
            }
            else if(token->type == TOKEN_IF)
            {
                if(token->body.if_cond->cond->left->type == VARIABLE)
                    DELETE_VAL(token->body.if_cond->cond->left);

                if(token->body.if_cond->cond->right->type == VARIABLE)
                    DELETE_VAL(token->body.if_cond->cond->right);

                ++if_counter;
            }
            else if(token->type == TOKEN_WHILE)
            {
                if(token->body.while_loop->cond->left->type == VARIABLE)
                    DELETE_VAL(token->body.while_loop->cond->left);

                if(token->body.while_loop->cond->right->type == VARIABLE)
                    DELETE_VAL(token->body.while_loop->cond->right);

                ++while_counter;
            }
            else if(token->type == TOKEN_ASSIGN)
            {
                if(token->body.assign->expr->op == tokens_id.add)
                {
                    if( token->body.assign->expr->left->type == VARIABLE)
                        DELETE_VAL(token->body.assign->expr->left);

                    if( token->body.assign->expr->right->type == VARIABLE )
                        DELETE_VAL(token->body.assign->expr->right);
                }
                else if(token->body.assign->expr->op == tokens_id.sub)
                {
                    if( token->body.assign->expr->left->type == VARIABLE)
                        DELETE_VAL(token->body.assign->expr->left);
                }
                else if(token->body.assign->expr->op == tokens_id.mult ||
                        token->body.assign->expr->op == tokens_id.div ||
                        token->body.assign->expr->op == tokens_id.mod )
                {
                    if( token->body.assign->expr->left->type == VARIABLE ||
                        token->body.assign->expr->right->type == VARIABLE)
                        DELETE_VAL(token->body.assign->res);

                    if( token->body.assign->expr->left->type == VARIABLE)
                        DELETE_VAL(token->body.assign->expr->left);

                    if( token->body.assign->expr->right->type == VARIABLE)
                        DELETE_VAL(token->body.assign->expr->right);
                }
            }
            else if(token->type == TOKEN_FOR)
            {

                DELETE_HIT(token->body.for_loop);
                DELETE_IT(token->body.for_loop);

                ++for_counter;
            }
            else if(token->type == TOKEN_GUARD)
            {
                if(token->body.guard->type == tokens_id.end_while)
                {
                    if(while_counter == 0)
                    {
                        /* GO BACK to first while */
                        for(  arraylist_iterator_prev(&back);
                            ! arraylist_iterator_end(&back);
                              arraylist_iterator_prev(&back))
                        {
                            arraylist_iterator_get_data(&back, (void*)&back_token);

                            if(back_token->type == TOKEN_WHILE)
                                break;
                        }

                        if(back_token->body.while_loop->cond->left->type == VARIABLE)
                            DELETE_VAL(back_token->body.while_loop->cond->left);

                        if(back_token->body.while_loop->cond->right->type == VARIABLE)
                            DELETE_VAL(back_token->body.while_loop->cond->right);
                    }
                    else
                        --while_counter;
                }
                else if(token->body.guard->type == tokens_id.end_for)
                {
                    if(for_counter == 0)
                    {
                        /* GO BACK to first FOR */
                        for(  arraylist_iterator_prev(&back);
                            ! arraylist_iterator_end(&back);
                              arraylist_iterator_prev(&back))
                        {
                            arraylist_iterator_get_data(&back, (void*)&back_token);

                            if(back_token->type == TOKEN_FOR)
                                break;
                        }

                        DELETE_HIT(back_token->body.for_loop);
                        DELETE_IT(back_token->body.for_loop);
                    }
                    else
                        --for_counter;
                }
                else if(token->body.guard->type == tokens_id.end_if)
                {
                    if(if_counter)
                        --if_counter;
                }
                else if(token->body.guard->type == tokens_id.else_cond)
                {
                    if(if_counter == 0)
                    {
                        /* we are in if else statment but we start from if statement, so else code is death code */
                        for(;
                            ! arraylist_iterator_end(&it);
                              arraylist_iterator_next(&it))
                            {
                                arraylist_iterator_get_data(&it, (void*)&token);

                                if(token->type == TOKEN_GUARD && token->body.guard->type == tokens_id.end_if)
                                    break;
                            }
                    }
                }
            }
        }

        /* here we analyze all tokens but not delete all values so get 1st NOT IN USE REGS */
        for( darray_iterator_init(regs, &dit, ITI_BEGIN);
             ! darray_iterator_end(&dit);
            darray_iterator_next(&dit))
        {
            darray_iterator_get_data(&dit, (void*)&val_name);
            GET_REG_BY_NAME_PTR(reg, val_name);
            if(! IS_REG_IN_USE(cpu->registers[reg]))
                break;
        }


clean:
    for(i = 0; i < reg_to_trace; ++i)
        FREE(names[i]);

    darray_destroy(regs);

    return reg;

#undef DELETE_VAL_BY_STR
#undef DELETE_IT
#undef DELETE_VAL
#undef GET_REG_BY_NAME_PTR
#undef CHECK_WINNER
}

/* private static inline funtion */
static BOOL __inline__ is_needed (Value *val, Value *it)
{
    TRACE("");

    if(val == NULL || it == NULL)
        ERROR("val == NULL || it == NULL\n", FALSE, "");

    if(val->type == CONST_VAL)
        return FALSE;

    if(val->body.var->type == VAR_NORMAL)
        return variable_cmp((void*)&val->body.var, (void*)&it->body.var) == 0;
    else if( val->body.var->body.arr->var_offset != NULL )
        return var_normal_cmp((void*)&val->body.var->body.arr->var_offset,
            (void*)&it->body.var->body.var) == 0;

    return FALSE;
}

BOOL is_iterator_needed(Arraylist *tokens, uint64_t pos, Value *it)
{
    Token *token;
    Arraylist_iterator ait;

    uint64_t counter = 0;
    uint64_t for_c = 1;

    TRACE("");

    /* skip obsolete tokens */
    for(  arraylist_iterator_init(tokens, &ait, ITI_BEGIN);
        ! arraylist_iterator_end(&ait) && counter < pos;
          arraylist_iterator_next(&ait))
        {
            ++counter;
        }

    /* for each token on list DO: */
    for( ;
        ! arraylist_iterator_end(&ait) && for_c;
          arraylist_iterator_next(&ait))
        {
            arraylist_iterator_get_data(&ait, (void*)&token);

            /* analyze used values in this token */
            if(token->type == TOKEN_IO)
            {
                if(is_needed(token->body.io->res, it))
                    return TRUE;
            }
            else if(token->type == TOKEN_IF)
            {
                if( is_needed(token->body.if_cond->cond->left, it) )
                    return TRUE;

                if( is_needed(token->body.if_cond->cond->right, it) )
                    return TRUE;
            }
            else if(token->type == TOKEN_WHILE)
            {
                if( is_needed(token->body.while_loop->cond->left, it) )
                    return TRUE;

                if( is_needed(token->body.while_loop->cond->right, it) )
                    return TRUE;
            }
            else if(token->type == TOKEN_ASSIGN)
            {
                if(is_needed(token->body.assign->res, it))
                    return TRUE;

                if(is_needed(token->body.assign->expr->left, it))
                    return TRUE;

                if(token->body.assign->expr->op != tokens_id.undefined)
                    if(is_needed(token->body.assign->expr->right, it))
                        return TRUE;

            }
            else if(token->type == TOKEN_FOR)
            {
                ++for_c;

                if(is_needed(token->body.for_loop->begin_value, it))
                    return TRUE;

                if(is_needed(token->body.for_loop->end_value, it))
                    return TRUE;
            }
            else if(token->type == TOKEN_GUARD)
            {
                if(token->body.guard->type == tokens_id.end_for)
                    --for_c;
            }
        }

    return FALSE;
}

int do_synchronize(Register *reg)
{
    /*
        we assume that we synchronize only variables
        we can't trace arrays so we need store always after use
    */
    Cvar *cvar;

    TRACE("");

    if(reg == NULL)
        ERROR("reg == NULL\n", 1, "");

    /* synchronize with memory */
    if( ! IS_REG_FREE(reg) && reg->val != NULL)
    {
        /* check up_to_date */
        cvar = cvar_get_by_value(reg->val);

        LOG("synchronize %s\n", cvar->name);

        if(cvar->up_to_date == 0)
            if( do_store(reg) )
                ERROR("do_store error\n", 1, "");

        cvar->up_to_date = 1;

        LOG("Now %s is up to date\n", cvar->name);
    }

    return 0;
}

int get_register(Arraylist *tokens, uint64_t pos, Value *val)
{
    /* here val is NULL or val is common value from cvars */
    int reg;

#ifdef DEBUG_MODE
    char *str;
#endif

    TRACE("");

    if(tokens == NULL)
        ERROR("tokens == NULL\n", -1, "");

    /* val == NULL iff we can't trace it */
    if(val != NULL && val->reg != NULL )
    {
#ifdef DEBUG_MODE
            str = value_str(val);
            LOG("REG %d has value %s used it again\n", val->reg->num, str);
            FREE(str);
#endif
        reg = val->reg->num;
    }
    else
    {
        LOG("Get NEW reg\n", "");
        reg = get_register_num(tokens, pos);
    }

    return reg;
}

int do_get_register(Arraylist *tokens, uint64_t pos, Value *val, BOOL force_sync)
{
    Cvar *cvar;
    Value *common_val;
    int reg;
    char *str = NULL;

    TRACE("");

    if(tokens == NULL)
        ERROR("tokens == NULL\n", -1, "");
#ifdef DEBUG_MODE
    str = value_str(val);
#endif

    /*
        check value if can't trace val set val to NULL
        else get common value from cvars
    */
    if(! value_can_trace(val))
    {
        LOG("Can't trace %s, set common_val to NULL\n", str);
        common_val = NULL;
    }
    else
    {
        LOG("Can trace %s set common val\n", str);
        cvar = cvar_get_by_value(val);
        common_val = cvar->body.val;
    }

#ifdef DEBUG_MODE
    FREE(str);
#endif

    reg = get_register(tokens, pos, common_val);
    if(reg == -1)
        ERROR("get_register error\n", -1, "");

    /* change common value in register */
    if(cpu->registers[reg]->val != NULL && (cpu->registers[reg]->val != common_val || force_sync))
    {
        if(do_synchronize(cpu->registers[reg]))
            ERROR("do_synchronize error\n", -1, "");
    }

    return reg;
}

int better_reg(Arraylist *tokens, uint64_t pos, Value *val1, Value *val2)
{
    Cvar *cvar1;
    Cvar *cvar2;

    int reg;

    char *str1 = NULL;
    char *str2 = NULL;

    Register regs[REGS_NUMBER];

    int i;

    TRACE("");

#ifdef DEBUG_MODE
    str1 = value_str(val1);
    str2 = value_str(val2);
#endif

    /* if can't trace better choose another one */
    if( !value_can_trace(val1) && ! value_can_trace(val2))
    {
        LOG("both %s and %s can't trace so draw\n", str1, str2);

#ifdef DEBUG_MODE
        FREE(str1);
        FREE(str2);
#endif

        return 0;
    }
    else if( ! value_can_trace(val1))
    {
        LOG("%s can't trace so %s wins\n", str1, str2);

#ifdef DEBUG_MODE
        FREE(str1);
        FREE(str2);
#endif
        return 1;
    }
    else if( ! value_can_trace(val2))
    {
        LOG("%s can't trace so %s wins\n", str2, str1);

#ifdef DEBUG_MODE
        FREE(str1);
        FREE(str2);
#endif
        return -1;
    }

#ifdef DEBUG_MODE
    FREE(str1);
    FREE(str2);
#endif

    /* both can trace */
    cvar1 = cvar_get_by_value(val1);
    cvar2 = cvar_get_by_value(val2);

    /* iff we have to get another register for value get another one */
    if(cvar1->body.val->reg == NULL && cvar2->body.val->reg == NULL)
    {
        LOG("both %s and %s havn't registers so draw\n", cvar1->name, cvar2->name);
        return 0;
    }
    else if(cvar1->body.val->reg == NULL)
    {
        LOG("%s hasn't reg so %s wins\n", cvar1->name, cvar2->name);
        return 1;
    }
    else if( cvar2->body.val->reg == NULL)
    {
        LOG("%s hasn't reg so %s wins\n", cvar2->name, cvar1->name);
        return -1;
    }

    if(IS_REG_IN_USE(cvar1->body.val->reg) && IS_REG_IN_USE(cvar2->body.val->reg))
        return -2;
    else if (IS_REG_IN_USE(cvar1->body.val->reg))
        return 1;
    else if (IS_REG_IN_USE(cvar2->body.val->reg))
        return -1;

    /* both are in regs */
    LOG("prepare regs for cheating\n", "");
    /* copy regs ( save status ) */
    for(i = 0; i < REGS_NUMBER; ++i)
        if( memcpy(&regs[i], cpu->registers[i], sizeof(Register)) == NULL )
            ERROR("memcpy error\n", -2, "");

    /*
        cheat that all regs except cvar1 and cvar2 are in use
        so algo choose between only cvar1  and cvar2
    */
    for(i = 0; i < REGS_NUMBER; ++i)
        REG_SET_IN_USE(cpu->registers[i]);

    REG_SET_BUSY(cvar1->body.val->reg);
    REG_SET_BUSY(cvar2->body.val->reg);

    reg = get_register_num(tokens, pos);

    /* restore reg status */
    for(i = 0; i < REGS_NUMBER; ++i)
        if( memcpy(cpu->registers[i], &regs[i] , sizeof(Register)) == NULL )
            ERROR("memcpy error\n", -2, "");

    if(cpu->registers[reg] == cvar1->body.val->reg)
        return 1;
    else
        return -1;
}

int do_zero(Register *reg, BOOL trace)
{
    char *codeline;
    mpz_t val;
    Cvar *cvar;

#ifdef DEBUG_MODE
    char *str = NULL;
#endif

    TRACE("");

    if(reg == NULL)
        ERROR("reg == NULL\n", 1, "");

    if(IS_REG_FREE(reg))
        ERROR("reg is free\n", 1, "");

    /* generate asm code */
    if(asprintf(&codeline, "%s %ju\n", mnemonics.zero, reg->num) == -1)
        ERROR("asprintf error\n", 1, "");

    if(arraylist_insert_last(asmcode, (void*)&codeline))
        ERROR("arraylist_insert_last error\n", 1, "");



    /* we don't want to trace this value */
    if(! trace)
        return 0;

    /* do not trace value */
    if(reg->val == NULL)
        return 0;

    /* trace value iff we can do it */
    if(reg->val->body.var->type == VAR_ARR)
        return 0;

    /* we change value */
    cvar = cvar_get_by_value(reg->val);

    LOG("%s isn't now up to date\n",cvar->name);

    cvar->up_to_date = 0;

    /* set value in register */
    mpz_init(val);
    mpz_set_si(val, 0);

    value_set_val(cvar->body.val, val);

    mpz_clear(val);

#ifdef DEBUG_MODE
    value_get_val(cvar->body.val, val);
    str = mpz_get_str(str, 10, val);
    LOG("Now %s = %s\n", cvar->name, str);

    FREE(str);
    mpz_clear(val);
#endif

    value_reset_symbolic(cvar->body.val);

    return 0;
}

int do_inc(Register *reg, BOOL trace)
{
    char *codeline;
    mpz_t val;
    Cvar *cvar;

#ifdef DEBUG_MODE
    char *str = NULL;
#endif

    TRACE("");

    if(reg == NULL)
        ERROR("reg == NULL\n", 1, "");

    if(IS_REG_FREE(reg))
        ERROR("reg is free\n", 1, "");

    /* generate asm code */
    if(asprintf(&codeline, "%s %ju\n", mnemonics.inc, reg->num) == -1)
        ERROR("asprintf error\n", 1, "");

    if(arraylist_insert_last(asmcode, (void*)&codeline))
        ERROR("arraylist_insert_last error\n", 1, "");



    /* we don't want to trace this value */
    if(! trace)
        return 0;

    /* if we can't trace don't do it */
    if(! value_can_trace(reg->val))
        return 0;

    /* we change value */
    cvar = cvar_get_by_value(reg->val);
    cvar->up_to_date = 0;

    LOG("%s isn't now up to date\n",cvar->name);

    if(value_is_symbolic(cvar->body.val))
        return 0;

    /* set value in register */
    value_get_val(cvar->body.val, val);
    mpz_add_ui(val ,val , 1);

    value_set_val(cvar->body.val, val);

    mpz_clear(val);

#ifdef DEBUG_MODE
    value_get_val(cvar->body.val, val);
    str = mpz_get_str(str, 10, val);
    LOG("Now %s = %s\n", cvar->name, str);

    FREE(str);
    mpz_clear(val);
#endif

    return 0;
}

int do_dec(Register *reg, BOOL trace)
{
    char *codeline;
    mpz_t val;
    Cvar *cvar;

#ifdef DEBUG_MODE
    char *str = NULL;
#endif

    TRACE("");

    if(reg == NULL)
        ERROR("reg == NULL\n", 1, "");

    if(IS_REG_FREE(reg))
        ERROR("reg is free\n", 1, "");

    /* generate asm code */
    if(asprintf(&codeline, "%s %ju\n", mnemonics.dec, reg->num) == -1)
        ERROR("asprintf error\n", 1, "");

    if(arraylist_insert_last(asmcode, (void*)&codeline))
        ERROR("arraylist_insert_last error\n", 1, "");



    /* we don't want to trace this value */
    if(! trace)
        return 0;

    /* if we can't trace don't do it */
    if(! value_can_trace(reg->val))
        return 0;

    /* we change value */
    cvar = cvar_get_by_value(reg->val);
    cvar->up_to_date = 0;

    LOG("%s isn't now up to date\n",cvar->name);

    if(value_is_symbolic(cvar->body.val))
        return 0;

    /* set value in register */
    value_get_val(cvar->body.val, val);
    mpz_sub_ui(val ,val , 1);

    value_set_val(cvar->body.val, val);

    mpz_clear(val);

#ifdef DEBUG_MODE
    value_get_val(cvar->body.val, val);
    str = mpz_get_str(str, 10, val);
    LOG("Now %s = %s\n", cvar->name, str);

    FREE(str);
    mpz_clear(val);
#endif

    return 0;
}

int do_shl(Register *reg, BOOL trace)
{
    char *codeline;
    mpz_t val;
    Cvar *cvar;

    TRACE("");

#ifdef DEBUG_MODE
    char *str = NULL;
#endif

    if(reg == NULL)
        ERROR("reg == NULL\n", 1, "");

    if(IS_REG_FREE(reg))
        ERROR("reg is free\n", 1, "");

    /* generate asm code */
    if(asprintf(&codeline, "%s %ju\n", mnemonics.shl, reg->num) == -1)
        ERROR("asprintf error\n", 1, "");

    if(arraylist_insert_last(asmcode, (void*)&codeline))
        ERROR("arraylist_insert_last error\n", 1, "");



    /* we don't want to trace this value */
    if(! trace)
        return 0;

    /* if we can't trace don't do it */
    if(! value_can_trace(reg->val))
        return 0;

    /* we change value */
    cvar = cvar_get_by_value(reg->val);
    cvar->up_to_date = 0;

    LOG("%s isn't now up to date\n",cvar->name);

    if(value_is_symbolic(cvar->body.val))
        return 0;

    /* set value in register */
    value_get_val(cvar->body.val, val);
    mpz_mul_2exp(val, val, 1);

    value_set_val(cvar->body.val, val);

    mpz_clear(val);

#ifdef DEBUG_MODE
    value_get_val(cvar->body.val, val);
    str = mpz_get_str(str, 10, val);
    LOG("Now %s = %s\n", cvar->name, str);

    FREE(str);
    mpz_clear(val);
#endif

    return 0;
}

int do_shr(Register *reg, BOOL trace)
{
    char *codeline;
    mpz_t val;
    Cvar *cvar;

    TRACE("");

#ifdef DEBUG_MODE
    char *str = NULL;
#endif

    if(reg == NULL)
        ERROR("reg == NULL\n", 1, "");

    if(IS_REG_FREE(reg))
        ERROR("reg is free\n", 1, "");

    /* generate asm code */
    if(asprintf(&codeline, "%s %ju\n", mnemonics.shr, reg->num) == -1)
        ERROR("asprintf error\n", 1, "");

    if(arraylist_insert_last(asmcode, (void*)&codeline))
        ERROR("arraylist_insert_last error\n", 1, "");



    /* we don't want to trace this value */
    if(! trace)
        return 0;

    /* if we can't trace don't do it */
    if(! value_can_trace(reg->val))
        return 0;

    /* we change value */
    cvar = cvar_get_by_value(reg->val);
    cvar->up_to_date = 0;

    LOG("%s isn't now up to date\n",cvar->name);

    if(value_is_symbolic(cvar->body.val))
        return 0;

    value_get_val(cvar->body.val, val);
    mpz_tdiv_q_2exp(val, val, 1);

    value_set_val(cvar->body.val, val);

    mpz_clear(val);

#ifdef DEBUG_MODE
    value_get_val(cvar->body.val, val);
    str = mpz_get_str(str, 10, val);
    LOG("Now %s = %s\n", cvar->name, str);

    FREE(str);
    mpz_clear(val);
#endif

    return 0;
}

int do_load(Register *reg, Value *val)
{
    char *codeline;

#ifdef DEBUG_MODE
    char *str = NULL;
#endif

    TRACE("");

    if(reg == NULL || val == NULL)
        ERROR("reg == NULL || val == NULL\n", 1, "");

    /* RO = addr(val) val is not common val so t[a] is ok here */
    if(do_set_val_addr(cpu->registers[REG_PTR], val, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

#ifdef DEBUG_MODE
        str = value_str(val);

        LOG("LOAD %s\n", str);

        FREE(str);
#endif

    /* generate asm code */
    if(asprintf(&codeline, "%s %ju\n", mnemonics.load, reg->num) == -1)
        ERROR("asprintf error\n", 1, "");

    if(arraylist_insert_last(asmcode, (void*)&codeline))
        ERROR("arraylist_insert_last error\n", 1, "");

    return 0;
}

int do_store(Register *reg)
{
    char *codeline;

#ifdef DEBUG_MODE
    char *str;
#endif

    TRACE("");

    if(reg == NULL)
        ERROR("reg == NULL\n", 1, "");

    /* RO = addr(val) val is not common val so t[a] is ok here */
    if(do_set_val_addr(cpu->registers[REG_PTR], reg->val, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    /* generate asm code */
    if(asprintf(&codeline, "%s %ju\n", mnemonics.store, reg->num) == -1)
        ERROR("asprintf error\n", 1, "");

    if(arraylist_insert_last(asmcode, (void*)&codeline))
        ERROR("arraylist_insert_last error\n", 1, "");



#ifdef DEBUG_MODE
    str = value_str(reg->val);

    LOG("STORE %s\n", str);

    FREE(str);
#endif

    return 0;
}

int do_halt()
{
    char *codeline;

    TRACE("");

    /* generate asm code */
    if(asprintf(&codeline, "%s\n", mnemonics.halt) == -1)
        ERROR("asprintf error\n", 1, "");

    if(arraylist_insert_last(asmcode, (void*)&codeline))
        ERROR("arraylist_insert_last error\n", 1, "");



    return 0;
}

int do_pump(Register *reg, uint64_t val, BOOL trace)
{
    int i;

    TRACE("");

    if(reg == NULL)
        ERROR("reg == NULL\n", 1, "");

    i = (sizeof(uint64_t) << 3) - 1;

    while( ! GET_BIT(val, i) && i)
        --i;

    if( do_zero(reg, trace) )
        ERROR("do_zero error\n", 1, "");

    for(; i > 0; --i)
        if(GET_BIT(val, i))
        {
            if( do_inc(reg, trace) )
                ERROR("do_inc error\n", 1, "");

            if( do_shl(reg, trace) )
                ERROR("do_shl error\n", 1, "");
        }
        else
        {
            if( do_shl(reg, trace) )
                ERROR("do_shl error\n", 1, "");
        }

    if( GET_BIT(val, i))
    {
        if( do_inc(reg, trace) )
            ERROR("do_inc error\n", 1, "");
    }

    return 0;
}

int do_pump_bigvalue(Register *reg, mpz_t val, BOOL trace)
{
    uint64_t i;

    TRACE("");

    if(reg == NULL)
        ERROR("reg == NULL\n", 1, "");

    i = mpz_sizeinbase(val, 2);

    if( do_zero(reg, trace) )
        ERROR("do_zero error\n", 1, "");

    for(--i; i > 0; --i)
        if(BGET_BIT(val, i))
        {
            if( do_inc(reg, trace) )
                ERROR("do_inc error\n", 1, "");

            if( do_shl(reg, trace) )
                ERROR("do_shl error\n", 1, "");
        }
        else
        {
            if( do_shl(reg, trace) )
                ERROR("do_shl error\n", 1, "");
        }

    if( BGET_BIT(val, i))
    {
        if( do_inc(reg, trace) )
            ERROR("do_inc error\n", 1, "");
    }

    return 0;
}

int do_get(Register *reg, BOOL trace)
{
    Cvar *cvar;
    char *codeline;

    TRACE("");

    /* generate asm code */
    if(asprintf(&codeline, "%s %ju\n", mnemonics.get, reg->num) == -1)
        ERROR("asprintf error\n", 1, "");

    if(arraylist_insert_last(asmcode, (void*)&codeline))
        ERROR("arraylist_insert_last error\n", 1, "");



    /* we don't want to trace this value */
    if(! trace)
        return 0;

    if(value_can_trace(reg->val))
    {
        /* load new value */
        cvar = cvar_get_by_value(reg->val);
        cvar->up_to_date = 0;

        LOG("%s isn't now up to date\n",cvar->name);

        value_set_symbolic_flag(cvar->body.val);
    }

    return 0;
}

int do_put(Register *reg)
{
    char *codeline;

    TRACE("");

    /* generate asm code */
    if(asprintf(&codeline, "%s %ju\n", mnemonics.put, reg->num) == -1)
        ERROR("asprintf error\n", 1, "");

    if(arraylist_insert_last(asmcode, (void*)&codeline))
        ERROR("arraylist_insert_last error\n", 1, "");



    return 0;
}

int do_add(Register *reg, BOOL trace)
{
    Cvar *cvar;
    char *codeline;

    mpz_t val;
    mpz_t val2;
    mpz_t addr;

    mem_chunk *chunk;

#ifdef DEBUG_MODE
    char *str = NULL;
#endif

    TRACE("");

    /* generate asm code */
    if(asprintf(&codeline, "%s %ju\n", mnemonics.add, reg->num) == -1)
        ERROR("asprintf error\n", 1, "");

    if(arraylist_insert_last(asmcode, (void*)&codeline))
        ERROR("arraylist_insert_last error\n", 1, "");



    /* we don't want to trace this value */
    if(! trace)
        return 0;

    /* if we can't trace don't do it */
    if(! value_can_trace(reg->val))
        return 0;

    /* we change value */
    cvar = cvar_get_by_value(reg->val);
    cvar->up_to_date = 0;

    LOG("%s isn't now up to date\n",cvar->name);

    if(value_is_symbolic(cvar->body.val))
        return 0;

    value_get_val(cvar->body.val, val);

    /* get value from memory */
    value_get_val(cpu->registers[REG_PTR]->val, addr);

    chunk = memory_get_chunk(mpz2ull(addr));
    if(chunk == NULL)
        ERROR("memory_get_chunk error\n", 1, "");

    value_get_val(chunk->val, val2);

    /* update value */
    mpz_add(val, val, val2);

    value_set_val(cvar->body.val, val);

    mpz_clear(val);
    mpz_clear(val2);
    mpz_clear(addr);

#ifdef DEBUG_MODE
    value_get_val(cvar->body.val, val);
    str = mpz_get_str(str, 10, val);
    LOG("Now %s = %s\n", cvar->name, str);

    FREE(str);
    mpz_clear(val);
#endif

    return 0;
}

int do_sub(Register *reg, BOOL trace)
{
    Cvar *cvar;
    char *codeline;

    mpz_t val;
    mpz_t val2;
    mpz_t addr;

    mem_chunk *chunk;

#ifdef DEBUG_MODE
    char *str = NULL;
#endif

    TRACE("");

    /* generate asm code */
    if(asprintf(&codeline, "%s %ju\n", mnemonics.sub, reg->num) == -1)
        ERROR("asprintf error\n", 1, "");

    if(arraylist_insert_last(asmcode, (void*)&codeline))
        ERROR("arraylist_insert_last error\n", 1, "");



    /* we don't want to trace this value */
    if(! trace)
        return 0;

    /* if we can't trace don't do it */
    if(! value_can_trace(reg->val))
        return 0;

    /* we change value */
    cvar = cvar_get_by_value(reg->val);
    cvar->up_to_date = 0;

    LOG("%s isn't now up to date\n",cvar->name);

    if(value_is_symbolic(cvar->body.val))
        return 0;

    value_get_val(cvar->body.val, val);

    /* get value from memory */
    value_get_val(cpu->registers[REG_PTR]->val, addr);

    chunk = memory_get_chunk(mpz2ull(addr));
    if(chunk == NULL)
        ERROR("memory_get_chunk error\n", 1, "");

    value_get_val(chunk->val, val2);

    /* update value */
    mpz_sub(val, val, val2);

    if(mpz_cmp_ui(val, 0) < 0)
        mpz_set_ui(val, 0);

    value_set_val(cvar->body.val, val);

    mpz_clear(val);
    mpz_clear(val2);
    mpz_clear(addr);

#ifdef DEBUG_MODE
    value_get_val(cvar->body.val, val);
    str = mpz_get_str(str, 10, val);
    LOG("Now %s = %s\n", cvar->name, str);

    FREE(str);
    mpz_clear(val);
#endif

    return 0;
}

int do_mult(Register *res,Register *left, Register *right, BOOL trace)
{
    Cvar *cvar_res;
    Cvar *cvar_left;
    Cvar *cvar_right;

    uint64_t k;
    uint64_t mult2;

    mpz_t val;
    mpz_t val2;
    mpz_t val3;

    mpz_t old_ptrval;
    Cvar *cvar_ptr;

#ifdef DEBUG_MODE
    char *str = NULL;
#endif


#define LABEL_MULT2_LABEL LABEL_TRUE


    TRACE("");

    cvar_ptr = cvar_get_by_name(PTR_NAME);

    /***** compile mult *****/

    /* R0 = left.addr */
    if(do_set_val_addr(cpu->registers[REG_PTR], left->val, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    /* RES = 0 */
    if(do_zero(res, FALSE))
        ERROR("do_zero error\n", 1, "");

    /* IF left == 0 END */
    if(do_jzero_label(left, LABEL_END))
        ERROR("do_jzero error\n", 1, "");

    /* IF right == 0 END */
    if(do_jzero_label(right, LABEL_END))
        ERROR("do_jzero error\n", 1, "");

    /* RIGHT = RIGHT - LEFT */
    if(do_sub(right, FALSE))
        ERROR("do_sub error\n", 1, "");

    /* RIGHT <= LEFT GO TO RES = LEFT * RIGHT */
    if(do_jzero_label(right, LABEL_MULT2_LABEL))
        ERROR("do_jzero error\n", 1, "");

    /* save ptr value */
    value_get_val(cvar_ptr->body.val, old_ptrval);

    /* LOAD Right, so R0 = right.addr */
    if(do_load(right, right->val))
        ERROR("do_load error\n", 1, "");

    /*** mult1 res = right * left ***/
/* COND1: */
    k = asmcode->length;
    if(do_jzero_label(left, LABEL_END))
        ERROR("do_jzero error\n", 1, "");

    if(do_jodd(left, k + 6))
        ERROR("do_jodd error\n", 1, "");

    if(do_shl(right, FALSE))
        ERROR("do_shl error\n", 1, "");

    if(do_store(right))
        ERROR("do_store error\n", 1, "");

    if(do_shr(left, FALSE))
        ERROR("do_shr error\n", 1, "");

    if(do_jump(k))
        ERROR("do_jump error\n", 1, "");

/* ODD1: */
    if(do_add(res, FALSE))
        ERROR("do_add error\n", 1, "");

    if(do_shl(right, FALSE))
        ERROR("do_shl error\n", 1, "");

    if(do_store(right))
        ERROR("do_store error\n", 1, "");

    if(do_shr(left, FALSE))
        ERROR("do_shr error\n", 1, "");

    if(do_jump(k))
        ERROR("do_jump error\n", 1, "");

    /*** mult2 res = left * right ***/
/*  LABEL_MULT2: */

    /* Another branch so restore ptr value */
    value_set_val(cvar_ptr->body.val, old_ptrval);

    mpz_clear(old_ptrval);

    mult2 = asmcode->length;
    /* R0 = right.addr */
    if(do_load(right, right->val))
        ERROR("do_load error\n", 1, "");

    if(do_set_val_addr(cpu->registers[REG_PTR], left->val, FALSE))
        ERROR("do_set_val_addr error\n", 1, "");

/* COND2: */
    k = asmcode->length;
    if(do_jzero_label(right, LABEL_END))
        ERROR("do_jzero error\n", 1, "");

    if(do_jodd(right, k + 6))
        ERROR("do_jodd error\n", 1, "");

    if(do_shl(left, FALSE))
        ERROR("do_shl error\n", 1, "");

    if(do_store(left))
        ERROR("do_store error\n", 1, "");

    if(do_shr(right, FALSE))
        ERROR("do_shr error\n", 1, "");

    if(do_jump(k))
        ERROR("do_jump error\n", 1, "");

/* ODD2: */
    if(do_add(res, FALSE))
        ERROR("do_add error\n", 1, "");

    if(do_shl(left, FALSE))
        ERROR("do_shl error\n", 1, "");

    if(do_store(left))
        ERROR("do_store error\n", 1, "");

    if(do_shr(right, FALSE))
        ERROR("do_shr error\n", 1, "");

    if(do_jump(k))
        ERROR("do_jump error\n", 1, "");
/* END: */
    if( label_to_line(asmcode->length) )
        ERROR("label_to_line 1 error\n", 1 ,"");

    if( label_to_line(asmcode->length) )
        ERROR("label_to_line 2 error\n", 1 ,"");

    if( label_to_line(mult2) )
        ERROR("label_to_line 3 error\n", 1 ,"");

    if( label_to_line(asmcode->length) )
        ERROR("label_to_line 4 error\n", 1 ,"");

    if( label_to_line(asmcode->length) )
        ERROR("label_to_line 5 error\n", 1 ,"");

    value_set_symbolic_flag(cvar_ptr->body.val);

    /* we don't want to trace this value */
    if(! trace)
        return 0;

    /* if we can't trace don't do it */
    if(! value_can_trace(res->val))
        return 0;

    /* we change value */
    cvar_res = cvar_get_by_value(res->val);
    cvar_res->up_to_date = 0;

    LOG("%s isn't now up to date\n",cvar_res->name);

    if(value_is_symbolic(cvar_res->body.val))
        return 0;

    cvar_left = cvar_get_by_value(left->val);
    cvar_right = cvar_get_by_value(right->val);

    value_get_val(cvar_res->body.val, val);
    value_get_val(cvar_left->body.val, val2);
    value_get_val(cvar_right->body.val, val3);

    /* update value */
    mpz_mul(val, val2, val3);

    value_set_val(cvar_res->body.val, val);

    mpz_clear(val);
    mpz_clear(val2);
    mpz_clear(val3);

#ifdef DEBUG_MODE
    value_get_val(cvar_res->body.val, val);
    str = mpz_get_str(str, 10, val);
    LOG("Now %s = %s\n", cvar_res->name, str);

    FREE(str);
    mpz_clear(val);
#endif

    return 0;

#undef LABEL_MULT2_LABEL
}

int do_div(Register *res,Register *left, Register *right, BOOL trace)
{
#define LABEL_GT    LABEL_FALSE

    Register *helper;
    int reg;
    Cvar *div_helper;
    Cvar *ptr;

    int k;
    int l;
    int m;

    TRACE("");

    div_helper = cvar_get_by_name(TEMP_DIV_HELPER);

    ptr = cvar_get_by_name(PTR_NAME);

    reg = do_get_register(token_list, token_list_pos, div_helper->body.val, FALSE);
    if(reg == -1)
        ERROR("do_get_register error\n", 1, "");

    helper = cpu->registers[reg];
    reg_set_val(helper, div_helper->body.val);
    REG_SET_IN_USE(helper);

    /* SET res to 0 */
    if(do_zero(res, FALSE))
        ERROR("do_zero error\n", 1, "");

    if(do_jzero_label(left, LABEL_END))
        ERROR("do_jzero_label error\n", 1, "");

    if(do_jzero_label(right, LABEL_END))
        ERROR("do_jzero_label error\n", 1, "");

    if(do_set_val_addr(cpu->registers[REG_PTR], right->val, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_zero(helper, FALSE))
        ERROR("do_zero error\n", 1, "");

    if(do_inc(helper, FALSE))
        ERROR("do_inc error\n", 1, "");

    k = asmcode->length;
    if(do_jzero(right, k + 4))
        ERROR("do_jzero error\n", 1, "");

    if(do_shr(right, FALSE))
        ERROR("do_shr error\n", 1, "");

    if(do_shr(left, FALSE))
        ERROR("do_shr error\n", 1 ,"");

    if(do_jump(k))
        ERROR("do_jump error\n", 1, "");

    if(do_load(right, right->val))
        ERROR("do_load error\n", 1, "");

    l = asmcode->length;

    if(do_jzero(left, l + 5))
        ERROR("do_jzero error\n", 1, "");

    if(do_shr(left, FALSE))
        ERROR("do_shr error\n", 1, "");

    if(do_shl(right, FALSE))
        ERROR("do_shl error\n", 1, "");

    if(do_inc(helper, FALSE))
        ERROR("do_inc error\n", 1, "");

    if(do_jump(l))
        ERROR("do_jump error\n", 1, "");

    if(do_store(right))
        ERROR("do_store error\n", 1, "");

    if(do_load(left, left->val))
        ERROR("do_load error\n", 1, "");

    if(do_set_val_addr(cpu->registers[REG_PTR], right->val, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    m = asmcode->length;

    if(do_jzero_label(helper, LABEL_END))
        ERROR("do_jzero_label error\n", 1, "");

    if(do_inc(left, FALSE))
        ERROR("do_inc error\n", 1, "");

    if(do_sub(left, FALSE))
        ERROR("do_sub error\n", 1, "");

    if(do_jzero_label(left, LABEL_GT))
        ERROR("do_jzero_label error\n", 1, "");

    if(do_dec(left, FALSE))
        ERROR("do_dec error\n", 1, "");

    if(do_dec(helper, FALSE))
        ERROR("do_dec error\n", 1, "");

    if(do_shl(res, FALSE))
        ERROR("do_shl error\n", 1, "");

    if(do_inc(res, FALSE))
        ERROR("do_inc error\n", 1, "");

    value_set_symbolic_flag(ptr->body.val);

    if(do_store(left))
        ERROR("do_store error\n", 1, "");

    if(do_set_val_addr(cpu->registers[REG_PTR], right->val, FALSE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_shr(right, FALSE))
        ERROR("do_shr error\n", 1, "");

    if(do_store(right))
        ERROR("do_store error\n", 1, "");

    value_set_symbolic_flag(ptr->body.val);

    if(do_jump(m))
        ERROR("do_jump error\n", 1, "");

    label_to_line(asmcode->length);

    value_set_symbolic_flag(ptr->body.val);

    if(do_load(left, left->val))
        ERROR("do_load error\n", 1, "");

    if(do_shl(res, FALSE))
        ERROR("do_shl error\n", 1, "");

    if(do_dec(helper, FALSE))
        ERROR("do_dec error\n", 1, "");

    if(do_shr(right, FALSE))
        ERROR("do_shr error\n", 1 ,"");

    if(do_store(right))
        ERROR("do_store error\n", 1, "");

    value_set_symbolic_flag(ptr->body.val);

    if(do_jump(m))
        ERROR("do_jump error\n", 1, "");

    /* end change label to line */
    label_to_line(asmcode->length);
    label_to_line(asmcode->length);
    label_to_line(asmcode->length);

    value_set_symbolic_flag(ptr->body.val);

    REG_SET_FREE(helper);

    return 0;

#undef LABEL_GT
}

int do_mod(Register *res, Register *left, Register *right, BOOL trace)
{
#define LABEL_GT    LABEL_FALSE

    Register *helper;
    int reg;
    Cvar *div_helper;
    Cvar *ptr;

    int k;
    int l;
    int m;

    TRACE("");

    div_helper = cvar_get_by_name(TEMP_DIV_HELPER);

    ptr = cvar_get_by_name(PTR_NAME);

    reg = do_get_register(token_list, token_list_pos, div_helper->body.val, FALSE);
    if(reg == -1)
        ERROR("do_get_register error\n", 1, "");

    helper = cpu->registers[reg];
    reg_set_val(helper, div_helper->body.val);
    REG_SET_IN_USE(helper);

    /* SET res to 0 */
    if(do_zero(res, FALSE))
        ERROR("do_zero error\n", 1, "");

    if(do_jzero(left, asmcode->length + 2))
        ERROR("do_jzero error\n", 1, "");

    if(do_jump(asmcode->length + 3))
        ERROR("do_jump error\n", 1, "");

    if(do_zero(left, FALSE))
        ERROR("do_zero error\n", 1, "");

    if(do_jump_label(LABEL_END))
        ERROR("do_jump_label error\n", 1, "");

    if(do_jzero(right, asmcode->length + 2))
        ERROR("do_jzero error\n", 1, "");

    if(do_jump(asmcode->length + 3))
        ERROR("do_jump error\n", 1, "");

    if(do_zero(left, FALSE))
        ERROR("do_zero error\n", 1, "");

    if(do_jump_label(LABEL_END))
        ERROR("do_jump_label error\n", 1, "");

    if(do_set_val_addr(cpu->registers[REG_PTR], right->val, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_zero(helper, FALSE))
        ERROR("do_zero error\n", 1, "");

    if(do_inc(helper, FALSE))
        ERROR("do_inc error\n", 1, "");

    k = asmcode->length;
    if(do_jzero(right, k + 4))
        ERROR("do_jzero error\n", 1, "");

    if(do_shr(right, FALSE))
        ERROR("do_shr error\n", 1, "");

    if(do_shr(left, FALSE))
        ERROR("do_shr error\n", 1 ,"");

    if(do_jump(k))
        ERROR("do_jump error\n", 1, "");

    if(do_load(right, right->val))
        ERROR("do_load error\n", 1, "");

    l = asmcode->length;

    if(do_jzero(left, l + 5))
        ERROR("do_jzero error\n", 1, "");

    if(do_shr(left, FALSE))
        ERROR("do_shr error\n", 1, "");

    if(do_shl(right, FALSE))
        ERROR("do_shl error\n", 1, "");

    if(do_inc(helper, FALSE))
        ERROR("do_inc error\n", 1, "");

    if(do_jump(l))
        ERROR("do_jump error\n", 1, "");

    if(do_store(right))
        ERROR("do_store error\n", 1, "");

    if(do_load(left, left->val))
        ERROR("do_load error\n", 1, "");

    if(do_set_val_addr(cpu->registers[REG_PTR], right->val, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    m = asmcode->length;

    if(do_jzero_label(helper, LABEL_END))
        ERROR("do_jzero_label error\n", 1, "");

    if(do_inc(left, FALSE))
        ERROR("do_inc error\n", 1, "");

    if(do_sub(left, FALSE))
        ERROR("do_sub error\n", 1, "");

    if(do_jzero_label(left, LABEL_GT))
        ERROR("do_jzero_label error\n", 1, "");

    if(do_dec(left, FALSE))
        ERROR("do_dec error\n", 1, "");

    if(do_dec(helper, FALSE))
        ERROR("do_dec error\n", 1, "");

    if(do_shl(res, FALSE))
        ERROR("do_shl error\n", 1, "");

    if(do_inc(res, FALSE))
        ERROR("do_inc error\n", 1, "");

    value_set_symbolic_flag(ptr->body.val);

    if(do_store(left))
        ERROR("do_store error\n", 1, "");

    if(do_set_val_addr(cpu->registers[REG_PTR], right->val, FALSE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_shr(right, FALSE))
        ERROR("do_shr error\n", 1, "");

    if(do_store(right))
        ERROR("do_store error\n", 1, "");

    value_set_symbolic_flag(ptr->body.val);

    if(do_jump(m))
        ERROR("do_jump error\n", 1, "");

    label_to_line(asmcode->length);

    value_set_symbolic_flag(ptr->body.val);

    if(do_load(left, left->val))
        ERROR("do_load error\n", 1, "");

    if(do_shl(res, FALSE))
        ERROR("do_shl error\n", 1, "");

    if(do_dec(helper, FALSE))
        ERROR("do_dec error\n", 1, "");

    if(do_shr(right, FALSE))
        ERROR("do_shr error\n", 1 ,"");

    if(do_store(right))
        ERROR("do_store error\n", 1, "");

    value_set_symbolic_flag(ptr->body.val);

    if(do_jump(m))
        ERROR("do_jump error\n", 1, "");

    /* end change label to line */
    label_to_line(asmcode->length);
    label_to_line(asmcode->length);
    label_to_line(asmcode->length);

    value_set_symbolic_flag(ptr->body.val);

    REG_SET_FREE(helper);

    return 0;
#undef LABEL_GT
}

int do_jump(uint64_t line)
{
    char *codeline;

    TRACE("");

    /* generate asm code */
    if(asprintf(&codeline, "%s %ju\n", mnemonics.jump, line) == -1)
        ERROR("asprintf error\n", 1, "");

    if(arraylist_insert_last(asmcode, (void*)&codeline))
        ERROR("arraylist_insert_last error\n", 1, "");



    return 0;
}

int do_jzero(Register *reg, uint64_t line)
{
    char *codeline;

    TRACE("");

    /* generate asm code */
    if(asprintf(&codeline, "%s %ju %ju\n", mnemonics.jzero, reg->num, line) == -1)
        ERROR("asprintf error\n", 1, "");

    if(arraylist_insert_last(asmcode, (void*)&codeline))
        ERROR("arraylist_insert_last error\n", 1, "");



    return 0;
}

int do_jodd(Register *reg, uint64_t line)
{
    char *codeline;

    TRACE("");

    /* generate asm code */
    if(asprintf(&codeline, "%s %ju %ju\n", mnemonics.jodd, reg->num, line) == -1)
        ERROR("asprintf error\n", 1, "");

    if(arraylist_insert_last(asmcode, (void*)&codeline))
        ERROR("arraylist_insert_last error\n", 1, "");



    return 0;
}

int do_jump_label(int8_t type)
{
    Label *label;
    char *codeline;

    TRACE("");

    /* generate asm code */
    if(asprintf(&codeline, "%s ", mnemonics.jump) == -1)
        ERROR("asprintf error\n", 1, "");

    if(arraylist_insert_last(asmcode, (void*)&codeline))
        ERROR("arraylist_insert_last error\n", 1, "");



    label = label_create(asmcode->tail, type);
    if(label == NULL)
        ERROR("label_create error\n", 1, "");

    if (stack_push(labels, (void*)&label) )
        ERROR("stack_push error\n", 1, "");

    return 0;
}

int do_jodd_label(Register *reg, int8_t type)
{
    Label *label;
    char *codeline;

    TRACE("");

    /* generate asm code */
    if(asprintf(&codeline, "%s %ju ", mnemonics.jodd, reg->num) == -1)
        ERROR("asprintf error\n", 1, "");

    if(arraylist_insert_last(asmcode, (void*)&codeline))
        ERROR("arraylist_insert_last error\n", 1, "");



    label = label_create(asmcode->tail, type);
    if(label == NULL)
        ERROR("label_create error\n", 1, "");

    if (stack_push(labels, (void*)&label) )
        ERROR("stack_push error\n", 1, "");

    return 0;
}

int do_jzero_label(Register *reg, int8_t type)
{
    Label *label;
    char *codeline;

    TRACE("");

    /* generate asm code */
    if(asprintf(&codeline, "%s %ju ", mnemonics.jzero, reg->num) == -1)
        ERROR("asprintf error\n", 1, "");

    if(arraylist_insert_last(asmcode, (void*)&codeline))
        ERROR("arraylist_insert_last error\n", 1, "");

    label = label_create(asmcode->tail, type);
    if(label == NULL)
        ERROR("label_create error\n", 1, "");

    if (stack_push(labels, (void*)&label) )
        ERROR("stack_push error\n", 1, "");

    return 0;
}

int do_copy(Register *reg, BOOL trace)
{
    Cvar *cvar;
    Cvar *ptr;

    char *codeline;

    mpz_t val;

#ifdef DEBUG_MODE
    char *str = NULL;
#endif

    TRACE("");

    /* generate asm code */
    if(asprintf(&codeline, "%s %ju\n", mnemonics.copy, reg->num) == -1)
        ERROR("asprintf error\n", 1, "");

    if(arraylist_insert_last(asmcode, (void*)&codeline))
        ERROR("arraylist_insert_last error\n", 1, "");

    /* we don't want to trace this value */
    if(! trace)
        return 0;

    /* if we can't trace don't do it */
    if(! value_can_trace(reg->val))
        return 0;

    /* we change value */
    ptr = cvar_get_by_name(PTR_NAME);
    ptr->up_to_date = 0;

    LOG("%s isn't now up to date\n",ptr->name);

    cvar = cvar_get_by_value(reg->val);
    if(! value_is_symbolic(cvar->body.val))
    {
        value_get_val(cvar->body.val, val);
        value_set_val(ptr->body.val, val);
        mpz_clear(val);

#ifdef DEBUG_MODE
        value_get_val(cvar->body.val, val);
        str = mpz_get_str(str, 10, val);
        LOG("Now %s = %s\n", ptr->name, str);

        FREE(str);
        mpz_clear(val);
#endif
    }
    return 0;
}


int do_set_val_addr(Register *reg, Value *val, BOOL trace)
{
    Cvar *ptr = NULL;
    Cvar *cvar;
    Cvar *offset = NULL;
    Cvar *temp1 = NULL;

    mpz_t addr;
    mpz_t temp;

    uint64_t cost;
    uint64_t my_addr;
    uint64_t dst_addr;

    mpz_t bmy_addr;
    mpz_t bdst_addr;

    int regnum = 0;

    TRACE("");

    if(reg == NULL || val == NULL)
        ERROR("reg == NULL || val == NULL\n", 1 ,"");

    cvar = cvar_get_by_value(val);

    LOG("R%ju = ADDR(%s)\n", reg->num, cvar->name);

    /* if reg isn't ptr reg synchronize it */
    if(reg->num != REG_PTR )
    {
        if( do_synchronize(reg) )
            ERROR("do_synchronize error\n", 1, "");
    }
    else
        ptr = cvar_get_by_name(PTR_NAME);

    if(cvar->type == VALUE)
    {
        LOG("is value\n", "");

        if(ptr != NULL && ! value_is_symbolic(ptr->body.val))
        {
            value_get_val(ptr->body.val, temp);
            my_addr = mpz2ull(temp);

            mpz_clear(temp);

            dst_addr = cvar->body.val->chunk->addr;

            if(my_addr < dst_addr)
                cost = (dst_addr - my_addr) * op_cost.inc;
            else
                cost = (my_addr - dst_addr) * op_cost.dec;

            if(cost < PUMP_COST(dst_addr))
            {
                if(my_addr == 0 && cost)
                    if(do_zero(reg, TRUE))
                        ERROR("do_zero ERROR\n", 1, "");

                while(my_addr < dst_addr)
                {
                    if(do_inc(reg, TRUE))
                        ERROR("do_inc error\n", 1, "");

                    ++my_addr;
                }

                while(my_addr > dst_addr)
                {
                    if(do_dec(reg, TRUE))
                        ERROR("do_inc error\n", 1, "");

                    --my_addr;
                }
            }
            else
            {
                if(do_pump(reg, dst_addr, TRUE))
                    ERROR("do_pump error\n", 1, "");
            }
        }
        else
        {
            /* pump to register addr value */
            if(do_pump(reg, cvar->body.val->chunk->addr, trace))
                ERROR("do_pump error\n", 1, "");
        }
    }
    else /* array */
    {
        if(val->body.var->body.arr->var_offset != NULL)
            offset = cvar_get_by_name(val->body.var->body.arr->var_offset->name);

        LOG("Is Array\n", "");

        mpz_init(addr);

        /* if offset is constans */
        if(val->body.var->body.arr->var_offset == NULL)
        {
            LOG("Offset is const\n", "");

            mpz_init(temp);

            /* calc addr + offset */
            ull2mpz(temp, val->body.var->body.arr->offset);
            mpz_add(addr, cvar->body.arr->addr, temp);

            mpz_clear(temp);

            if(ptr != NULL && ! value_is_symbolic(ptr->body.val))
            {
                value_get_val(ptr->body.val, bmy_addr);

                mpz_init(bdst_addr);
                mpz_set(bdst_addr, addr);

                if(mpz_cmp(bmy_addr, bdst_addr) < 0)
                {
                    mpz_init(temp);
                    mpz_sub(temp, bdst_addr, bmy_addr);

                    if(mpz_cmp_ui(temp, 1 << 10) > 0)
                        cost = 1ull << 31;
                    else
                        cost = mpz2ull(temp) * op_cost.inc;

                    mpz_clear(temp);
                }
                else
                {
                    mpz_init(temp);
                    mpz_sub(temp, bmy_addr, bdst_addr);

                    if(mpz_cmp_ui(temp, 1 << 10) > 0)
                        cost = 1ull << 31;
                    else
                        cost = mpz2ull(temp) * op_cost.dec;

                    mpz_clear(temp);
                }

                if(cost < BPUMP_COST(bdst_addr))
                {
                    if(mpz_cmp_ui(bmy_addr, 0) == 0 && cost)
                        if(do_zero(reg, TRUE))
                            ERROR("do_zero ERROR\n", 1, "");

                    while(mpz_cmp(bmy_addr, bdst_addr) < 0)
                    {
                        if(do_inc(reg, TRUE))
                            ERROR("do_inc error\n", 1, "");

                        mpz_add_ui(bmy_addr, bmy_addr, 1);
                    }

                    while(mpz_cmp(bmy_addr, bdst_addr) > 0)
                    {
                        if(do_dec(reg, TRUE))
                            ERROR("do_inc error\n", 1, "");

                        mpz_sub_ui(bmy_addr, bmy_addr, 1);
                    }
                }
                else
                {
                    /* pump addr */
                    if( do_pump_bigvalue(reg, addr, trace) )
                        ERROR("do_pump_bigvalue error\n", 1, "");
                }

                mpz_clear(bmy_addr);
                mpz_clear(bdst_addr);
            }
            else
            {
                /* pump addr */
                if( do_pump_bigvalue(reg, addr, trace) )
                    ERROR("do_pump_bigvalue error\n", 1, "");
            }

        }
        else /* offset is variable */
        {
            LOG("Offset is var\n", "");

            /* if var is not symbolic */
            if( ! value_is_symbolic(offset->body.val) )
            {
                LOG("Offset is not symbolic\n", "");

                value_get_val(offset->body.val, temp);

                /* value is constans so calc addr */
                mpz_add(addr, cvar->body.arr->addr, temp);

                mpz_clear(temp);

                if(ptr != NULL && ! value_is_symbolic(ptr->body.val))
                {
                    value_get_val(ptr->body.val, bmy_addr);

                    mpz_init(bdst_addr);
                    mpz_set(bdst_addr, addr);

                    if(mpz_cmp(bmy_addr, bdst_addr) < 0)
                    {
                        mpz_init(temp);
                        mpz_sub(temp, bdst_addr, bmy_addr);

                        if(mpz_cmp_ui(temp, 1 << 10) > 0)
                            cost = 1ull << 31;
                        else
                            cost = mpz2ull(temp) * op_cost.inc;

                        mpz_clear(temp);
                    }
                    else
                    {
                        mpz_init(temp);
                        mpz_sub(temp, bmy_addr, bdst_addr);

                        if(mpz_cmp_ui(temp, 1 << 10) > 0)
                            cost = 1ull << 31;
                        else
                            cost = mpz2ull(temp) * op_cost.dec;

                        mpz_clear(temp);
                    }

                    if(cost < BPUMP_COST(bdst_addr))
                    {
                        if(mpz_cmp_ui(bmy_addr, 0) == 0)
                            if(do_zero(reg, TRUE))
                                ERROR("do_zero ERROR\n", 1, "");

                        while(mpz_cmp(bmy_addr, bdst_addr) < 0)
                        {
                            if(do_inc(reg, TRUE))
                                ERROR("do_inc error\n", 1, "");

                            mpz_add_ui(bmy_addr, bmy_addr, 1);
                        }

                        while(mpz_cmp(bmy_addr, bdst_addr) > 0)
                        {
                            if(do_dec(reg, TRUE))
                                ERROR("do_inc error\n", 1, "");

                            mpz_sub_ui(bmy_addr, bmy_addr, 1);
                        }
                    }
                    else
                    {
                        /* pump addr */
                        if( do_pump_bigvalue(reg, addr, trace) )
                            ERROR("do_pump_bigvalue error\n", 1, "");
                    }

                    mpz_clear(bmy_addr);
                    mpz_clear(bdst_addr);
                }
                else
                {
                    /* pump addr */
                    if( do_pump_bigvalue(reg, addr, trace) )
                        ERROR("do_pump_bigvalue error\n", 1, "");
                }
            }
            else
            {
                LOG("Offset is symbolic\n", "");

                /* have to add addr */

                temp1 = cvar_get_by_name(TEMP_ADDR_NAME);

                if(offset->up_to_date == 0)
                    if(do_synchronize(offset->body.val->reg))
                        ERROR("do_synchronize error\n", 1, "");

                if(reg == cpu->registers[REG_PTR])
                {
                    regnum = do_get_register(token_list, token_list_pos, temp1->body.val, FALSE);
                    if(regnum == -1)
                        ERROR("do_get_register error\n", 1, "");

                    /* lock reg */
                    REG_SET_IN_USE(cpu->registers[regnum]);

                    reg_set_val(cpu->registers[regnum], temp1->body.val);

                    if(do_pump_bigvalue(cpu->registers[regnum], cvar->body.arr->addr, FALSE)  )
                        ERROR("do_pump_bigvalue error\n", 1, "");

                    /* pump addr */
                    if( do_pump(cpu->registers[REG_PTR], offset->body.val->chunk->addr, trace) )
                        ERROR("do_pump_value error\n", 1, "");

                    if(do_add(cpu->registers[regnum], FALSE) )
                        ERROR("do_add error\n", 1, "");

                    if(do_copy(cpu->registers[regnum], trace))
                        ERROR("do_copy error\n", 1, "");

                    REG_SET_FREE(cpu->registers[regnum]);

                    value_set_symbolic_flag(ptr->body.val);
                }
                else
                {
                    if(do_pump_bigvalue(reg, cvar->body.arr->addr, FALSE)  )
                        ERROR("do_pump_bigvalue error\n", 1, "");

                    /* pump addr */
                    if( do_pump(cpu->registers[REG_PTR], offset->body.val->chunk->addr, trace) )
                        ERROR("do_pump_value error\n", 1, "");

                    if(do_add(reg, FALSE) )
                        ERROR("do_add error\n", 1, "");

                }
            }
        }

        mpz_clear(addr);
    }

    return 0;
}

int do_lt1(Register *left, Value *right)
{
    TRACE("");

    if(left == NULL || right == NULL)
        ERROR("left == NULL || right == NULL\n", 1, "");

    if(do_inc(left, FALSE))
        ERROR("do_inc error\n", 1, "");

    if(do_set_val_addr(cpu->registers[REG_PTR], right, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_sub(left, FALSE))
        ERROR("do_sub error\n", 1, "");

    if(do_jzero(left, asmcode->length + 2))
        ERROR("do_jzero error\n", 1, "");

    if(do_jump_label(LABEL_FALSE))
        ERROR("do_jump_label error\n", 1, "");

    /* we use only 1 lable so fake  */
    if(label_fake())
        ERROR("label_fake error\n", 1, "");

    return 0;
}

int do_lt2(Register *right, Value *left)
{
    TRACE("");

    if(left == NULL || right == NULL)
        ERROR("left == NULL || right == NULL\n", 1, "");

    if(do_set_val_addr(cpu->registers[REG_PTR], left, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_sub(right, FALSE))
        ERROR("do_sub error\n", 1, "");

    if(do_jzero_label(right, LABEL_FALSE))
        ERROR("do_jzero_label", 1, "");

    /* we use only 1 lable so fake  */
    if(label_fake())
        ERROR("label_fake error\n", 1, "");

    return 0;
}

int do_gt1(Register *left, Value *right)
{
    TRACE("");

    if(left == NULL || right == NULL)
        ERROR("left == NULL || right == NULL\n", 1, "");

    if(do_set_val_addr(cpu->registers[REG_PTR], right, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_sub(left, FALSE))
        ERROR("do_sub error\n", 1, "");

    if(do_jzero_label(left, LABEL_FALSE))
        ERROR("do_jzero_label", 1, "");

    /* we use only 1 lable so fake  */
    if(label_fake())
        ERROR("label_fake error\n", 1, "");

    return 0;
}

int do_gt2(Register *right, Value *left)
{
    TRACE("");

    if(left == NULL || right == NULL)
        ERROR("left == NULL || right == NULL\n", 1, "");

    if(do_inc(right, FALSE))
        ERROR("do_inc error\n", 1, "");

    if(do_set_val_addr(cpu->registers[REG_PTR], left, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_sub(right, FALSE))
        ERROR("do_sub error\n", 1, "");

    if(do_jzero(right, asmcode->length + 2))
        ERROR("do_jzero error\n", 1, "");

    if(do_jump_label(LABEL_FALSE))
        ERROR("do_jump_label error\n", 1, "");

    /* we use only 1 lable so fake  */
    if(label_fake())
        ERROR("label_fake error\n", 1, "");

    return 0;
}

int do_le1(Register *left, Value *right)
{
    TRACE("");

    if(left == NULL || right == NULL)
        ERROR("left == NULL || right == NULL\n", 1, "");

    if(do_set_val_addr(cpu->registers[REG_PTR], right, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_sub(left, FALSE))
        ERROR("do_sub error\n", 1, "");

    if(do_jzero(left, asmcode->length + 2))
        ERROR("do_jzero error\n", 1, "");

    if(do_jump_label(LABEL_FALSE))
        ERROR("do_jump_label error\n", 1, "");

    /* we use only 1 lable so fake  */
    if(label_fake())
        ERROR("label_fake error\n", 1, "");

    return 0;
}

int do_le2(Register *right, Value *left)
{
    TRACE("");

    if(left == NULL || right == NULL)
        ERROR("left == NULL || right == NULL\n", 1, "");

    if(do_inc(right, FALSE))
        ERROR("do_inc error\n", 1, "");

    if(do_set_val_addr(cpu->registers[REG_PTR], left, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_sub(right, FALSE))
        ERROR("do_sub error\n", 1, "");

    if(do_jzero_label(right, LABEL_FALSE))
        ERROR("do_jzero_label error\n", 1, "");

    /* we use only 1 lable so fake  */
    if(label_fake())
        ERROR("label_fake error\n", 1, "");

    return 0;
}

int do_ge1(Register *left, Value *right)
{
    TRACE("");

    if(left == NULL || right == NULL)
        ERROR("left == NULL || right == NULL\n", 1, "");

    if(do_inc(left, FALSE))
        ERROR("do_inc error\n", 1, "");

    if(do_set_val_addr(cpu->registers[REG_PTR], right, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_sub(left, FALSE))
        ERROR("do_sub error\n", 1, "");

    if(do_jzero_label(left, LABEL_FALSE))
        ERROR("do_jzero_label error\n", 1, "");

    /* we use only 1 lable so fake  */
    if(label_fake())
        ERROR("label_fake error\n", 1, "");

    return 0;
}

int do_ge2(Register *right, Value *left)
{
    TRACE("");

    if(left == NULL || right == NULL)
        ERROR("left == NULL || right == NULL\n", 1, "");

    if(do_set_val_addr(cpu->registers[REG_PTR], left, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_sub(right, FALSE))
        ERROR("do_sub error\n", 1, "");

    if(do_jzero(right, asmcode->length + 2))
        ERROR("do_jzero error\n", 1, "");

    if(do_jump_label(LABEL_FALSE))
        ERROR("do_jump_label error\n", 1, "");

    /* we use only 1 lable so fake  */
    if(label_fake())
        ERROR("label_fake error\n", 1, "");

    return 0;
}

int do_eq1(Register *left, Value *right)
{
    Cvar *ptr;

    TRACE("");

    if(left == NULL || right == NULL)
        ERROR("left == NULL || right == NULL\n", 1, "");

    ptr = cvar_get_by_name(PTR_NAME);

    if(do_set_val_addr(cpu->registers[REG_PTR], right, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_sub(left, FALSE))
        ERROR("do_sub error\n", 1, "");

    /* JUMP TO HELPER: */
    if(do_jzero(left, asmcode->length + 2))
        ERROR("do_jzero error\n", 1, "");

    if(do_jump_label(LABEL_FALSE))
        ERROR("do_jump_label error\n", 1, "");

/* HELPER: */
    /* destroy left val so restore */
    if(left->val->type == CONST_VAL)
    {
        if(do_pump(left, left->val->body.cv->value, FALSE))
            ERROR("do_pump error\n", 1, "");
    }
    else
    {
        if(do_load(left, left->val))
            ERROR("do_load error\n", 1, "");
    }

    if(do_inc(left, FALSE))
        ERROR("do_inc error\n", 1, "");

    if(do_set_val_addr(cpu->registers[REG_PTR], right, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_sub(left, FALSE))
        ERROR("do_sub error\n", 1, "");

    if(do_jzero_label(left, LABEL_FALSE))
        ERROR("do_jzero_label error\n", 1, "");

    value_set_symbolic_flag(ptr->body.val);

    return 0;
}

int do_eq2(Register *right, Value *left)
{
    Cvar *ptr;

    TRACE("");

    if(left == NULL || right == NULL)
        ERROR("left == NULL || right == NULL\n", 1, "");

    ptr = cvar_get_by_name(PTR_NAME);

    if(do_set_val_addr(cpu->registers[REG_PTR], left, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_sub(right, FALSE))
        ERROR("do_sub error\n", 1, "");

    /* JUMP TO HELPER: */
    if(do_jzero(right, asmcode->length + 2))
        ERROR("do_jzero error\n", 1, "");

    if(do_jump_label(LABEL_FALSE))
        ERROR("do_jump_label error\n", 1, "");

/* HELPER: */
    /* destroy right val so restore */
    if(right->val->type == CONST_VAL)
    {
        if(do_pump(right, right->val->body.cv->value, FALSE))
            ERROR("do_pump error\n", 1, "");
    }
    else
    {
        if(do_load(right, right->val))
            ERROR("do_load error\n", 1, "");
    }

    if(do_inc(right, FALSE))
        ERROR("do_inc error\n", 1, "");

    if(do_set_val_addr(cpu->registers[REG_PTR], left, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_sub(right, FALSE))
        ERROR("do_sub error\n", 1, "");

    if(do_jzero_label(right, LABEL_FALSE))
        ERROR("do_jzero_label error\n", 1, "");

    value_set_symbolic_flag(ptr->body.val);

    return 0;
}

int do_ne1(Register *left, Value *right)
{
    Cvar *ptr;

    TRACE("");

    if(left == NULL || right == NULL)
        ERROR("left == NULL || right == NULL\n", 1, "");

    ptr = cvar_get_by_name(PTR_NAME);

    if(do_set_val_addr(cpu->registers[REG_PTR], right, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_sub(left, FALSE))
        ERROR("do_sub error\n", 1, "");

    /* JUMP TO HELPER: */
    if(do_jzero(left, asmcode->length + 2))
        ERROR("do_jzero error\n", 1, "");

    if(do_jump_label(LABEL_TRUE))
        ERROR("do_jump_label error\n", 1, "");

/* HELPER: */
    /* destroy left val so restore */
    if(left->val->type == CONST_VAL)
    {
        if(do_pump(left, left->val->body.cv->value, FALSE))
            ERROR("do_pump error\n", 1, "");
    }
    else
    {
        if(do_load(left, left->val))
            ERROR("do_load error\n", 1, "");
    }

    if(do_inc(left, FALSE))
        ERROR("do_inc error\n", 1, "");

    if(do_set_val_addr(cpu->registers[REG_PTR], right, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_sub(left, FALSE))
        ERROR("do_sub error\n", 1, "");

    if(do_jzero(left, asmcode->length + 2))
        ERROR("do_jzero_label error\n", 1, "");

    value_set_symbolic_flag(ptr->body.val);

    /* LABEL_TRUE to line */
    if(label_to_line(asmcode->length + 1))
        ERROR("label_to_line error\n", 1 ,"");

    if(do_jump_label(LABEL_FALSE))
        ERROR("do_jump_label error\n", 1, "");

    /* we use only 1 lable so fake  */
    if(label_fake())
        ERROR("label_fake error\n", 1, "");

    return 0;
}

int do_ne2(Register *right, Value *left)
{
    Cvar *ptr;

    TRACE("");

    if(left == NULL || right == NULL)
        ERROR("left == NULL || right == NULL\n", 1, "");

    ptr = cvar_get_by_name(PTR_NAME);

    if(do_set_val_addr(cpu->registers[REG_PTR], left, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_sub(right, FALSE))
        ERROR("do_sub error\n", 1, "");

    /* JUMP TO HELPER: */
    if(do_jzero(right, asmcode->length + 2))
        ERROR("do_jzero error\n", 1, "");

    if(do_jump_label(LABEL_TRUE))
        ERROR("do_jump_label error\n", 1, "");

/* HELPER: */
    /* destroy right val so restore */
    if(right->val->type == CONST_VAL)
    {
        if(do_pump(right, right->val->body.cv->value, FALSE))
            ERROR("do_pump error\n", 1, "");
    }
    else
    {
        if(do_load(right, right->val))
            ERROR("do_load error\n", 1, "");
    }

    if(do_inc(right, FALSE))
        ERROR("do_inc error\n", 1, "");

    if(do_set_val_addr(cpu->registers[REG_PTR], left, TRUE))
        ERROR("do_set_val_addr error\n", 1, "");

    if(do_sub(right, FALSE))
        ERROR("do_sub error\n", 1, "");

    if(do_jzero(right, asmcode->length + 2))
        ERROR("do_jzero_label error\n", 1, "");

    value_set_symbolic_flag(ptr->body.val);

    /* LABEL_TRUE to line */
    if(label_to_line(asmcode->length + 1))
        ERROR("label_to_line error\n", 1 ,"");

    if(do_jump_label(LABEL_FALSE))
        ERROR("do_jump_label error\n", 1, "");

    /* we use only 1 lable so fake  */
    if(label_fake())
        ERROR("label_fake error\n", 1, "");

    return 0;
}
