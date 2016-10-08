#include <asm.h>
#include <arch.h>
#include <common.h>

const Mnemonics mnemonics =
{
    .get    =   "GET",
    .put    =   "PUT",
    .load   =   "LOAD",
    .store  =   "STORE",
    .add    =   "ADD",
    .sub    =   "SUB",
    .copy   =   "COPY",
    .shr    =   "SHR",
    .shl    =   "SHL",
    .inc    =   "INC",
    .dec    =   "DEC",
    .zero   =   "ZERO",
    .jump   =   "JUMP",
    .jzero  =   "JZERO",
    .jodd   =   "JODD",
    .halt   =   "HALT"

};

const Opcodes opcodes =
{
    .get    =   1,
    .put    =   2,
    .load   =   3,
    .store  =   4,
    .add    =   5,
    .sub    =   6,
    .copy   =   7,
    .shr    =   8,
    .shl    =   9,
    .inc    =   10,
    .dec    =   11,
    .zero   =   12,
    .jump   =   13,
    .jzero  =   14,
    .jodd   =   15,
    .halt   =   16

};

const_value *const_value_create(uint64_t val)
{
    const_value *cv;

    TRACE("");

    cv = (const_value*)malloc(sizeof(const_value));
    if(cv == NULL)
        ERROR("malloc errro\n", NULL, "");

    cv->value = val;
    cv->type = CONST_VAL;

    return cv;
}

const_value *const_bigvalue_create(mpz_t val)
{
    const_value *cv;

    TRACE("");

    cv = (const_value*)malloc(sizeof(const_value));
    if(cv == NULL)
        ERROR("malloc errro\n", NULL, "");

    cv->value = 0;
    cv->type = BIG_CONST;

    mpz_init(cv->big_value);
    mpz_set(cv->big_value, val);

    return cv;
}

void const_value_destroy(const_value *cv)
{
    TRACE("");

    if(cv == NULL)
    {
        LOG("cv == NULL\n", "");
        return;
    }

    if(cv->type == BIG_CONST)
        mpz_clear(cv->big_value);

    FREE(cv);
}

void const_value_print(const_value *cv)
{
    TRACE("");

    if(cv == NULL)
    {
        LOG("cv == NULL\n", "");

        return;
    }

    printf(" %ju ", cv->value);
}

var_normal *var_normal_create(const char *name)
{
    var_normal *var;

    TRACE("");

    if(name == NULL)
        ERROR("name == NULL\n", NULL, "");

    var = (var_normal*)malloc(sizeof(var_normal));

    if(asprintf(&var->name, "%s", name) == -1)
        ERROR("asprintf error\n", NULL, "");

    mpz_init(var->value);

    var->symbolic_value = NULL;
    var->is_symbolic = 0;

    return var;
}

void var_normal_destroy(var_normal *var)
{
    TRACE("");

    if(var == NULL)
    {
        LOG("var == NULL\n", "");

        return;
    }

    mpz_clear(var->value);

    FREE(var->name);
    FREE(var->symbolic_value);
    FREE(var);
}

int var_normal_cmp(void *var1, void *var2)
{
    var_normal *_var1 = *(var_normal**)var1;
    var_normal *_var2 = *(var_normal**)var2;

    int ret = strcmp(_var1->name, _var2->name);

    if(ret < 0)
        return -1;

    if(ret > 0)
        return 1;

    return 0;
}

void var_normal_print(var_normal *var)
{
    TRACE("");

    if(var == NULL)
    {
        LOG("var == NULL\n", "");

        return;
    }

    if(mpz_cmp_ui(var->value, 0))
        gmp_printf (" %s --> %Zd ", var->name, var->value);
    else
        printf(" %s ", var->name);
}

var_arr *var_arr_create(var_normal *var, Array *arr, uint64_t offset)
{
    var_arr *va;

    TRACE("");

    if(var == NULL)
        ERROR("var == NULL\n", NULL, "");

    va = (var_arr*)malloc(sizeof(var_arr));
    if(va == NULL)
        ERROR("malloc error\n", NULL, "");

    va->var = var;
    va->arr = arr;
    va->offset = offset;
    va->var_offset = NULL;

    return va;
}

void var_arr_destroy(var_arr *va)
{
    TRACE("");

    if(va == NULL)
    {
        LOG("va == NULL\n", "");

        return;
    }

    var_normal_destroy(va->var);

    if(va->var_offset != NULL)
        var_normal_destroy(va->var_offset);

    FREE(va);
}

int var_arr_cmp(void *va1, void *va2)
{
    int res;

    var_arr *_va1 = *(var_arr**)va1;
    var_arr *_va2 = *(var_arr**)va2;

    res = var_normal_cmp((void*)&_va1->var, (void*)&_va2->var);
    if(res)
        return res;

    if(_va1->var_offset == NULL && _va2->var_offset == NULL)
    {
        if( _va1->offset < _va2->offset )
            return -1;
        else if ( _va1->offset > _va2->offset )
            return 1;

        return 0;
    }

    else if( _va1->var_offset == NULL)
        return -1;
    else if( _va2->var_offset == NULL)
        return 1;

    return var_normal_cmp((void*)&_va1->var_offset, (void*)&_va2->var_offset);
}

void var_arr_print(var_arr *va)
{
    TRACE("");

    if(va == NULL)
    {
        LOG("va == NULL\n", "");

        return;
    }

    if(va->var_offset == NULL)
    {
        if( mpz_cmp_si(va->var->value, 0))
            gmp_printf (" %s[ %ju ] --> %Zd ", va->var->name, va->offset, va->var->value);
        else
            printf(" %s[ %ju ] ", va->var->name, va->offset);
    }

    else
    {
        if( mpz_cmp_si(va->var->value, 0))
            gmp_printf (" %s[ %ju ] --> %Zd ", va->var->name, va->offset, va->var->value);
        else
        {
            if( mpz_cmp_si(va->var_offset->value, 0))
                gmp_printf (" %s[ %s ---> %Zd ] ", va->var->name, va->var_offset->name, va->var_offset->value);
            else
                printf(" %s[ %s ] ", va->var->name, va->var_offset->name);
        }
    }
}

Variable *variable_create(VAR_TYPE type, void *str)
{
    Variable *var;

    TRACE("");

    if( (type != VAR_NORMAL && type != VAR_ARR ) || str == NULL)
        ERROR("(type != VAR_NORMAL && type != VAR_ARR ) || str == NULL\n", NULL, "");

    var = (Variable*)malloc(sizeof(Variable));
    if(var == NULL)
        ERROR("malloc error\n", NULL, "");

    var->type = type;

    switch (type)
    {
        case VAR_NORMAL:
        {
            var->body.var = str;
            break;
        }
        case VAR_ARR:
        {
            var->body.arr = str;
            break;
        }
        default:
        {
            FREE(var);

            /* This is obsolete but to avoid warn with no default i declare it */
            ERROR("unrecognized type\n", NULL, "");
        }
    }

    return var;
}

void variable_destroy(Variable *var)
{
    TRACE("");

    if(var == NULL)
    {
        LOG("var == NULL\n", "");

        return;
    }

    switch (var->type)
    {
        case VAR_NORMAL:
        {
            var_normal_destroy(var->body.var);
            break;
        }
        case VAR_ARR:
        {
            var_arr_destroy(var->body.arr);
            break;
        }
        default:
        {
            LOG("unrecognized type\n", "");
            return;
        }
    }

    FREE(var);
}

int variable_cmp(void *var1, void *var2)
{
    Variable *_var1 = *(Variable**)var1;
    Variable *_var2 = *(Variable**)var2;

    if(_var1->type == _var2->type)
    {
        switch(_var1->type)
        {
            case VAR_NORMAL:
            {
                LOG("VAR_NORMAL CNP\n", "");
                return var_normal_cmp((void*)&_var1->body.var, (void*)&_var2->body.var);
            }
            case VAR_ARR:
            {
                LOG("VAR_ARR_CMP\n", "");
                return var_arr_cmp((void*)&_var1->body.arr, (void*)&_var2->body.arr);
            }
            default:
            {
                ERROR("unrecognized type\n", -2, "");
            }
        }
    }
    else
    {
        return 2;
    }
}

char *variable_get_name(Variable *var)
{
    char *name;

    TRACE("");

    if(var == NULL)
        ERROR("var == NULL\n", NULL, "");

    switch(var->type)
    {
        case VAR_NORMAL:
        {
            name = var->body.var->name;
            break;
        }
        case VAR_ARR:
        {
            name = var->body.arr->var->name;
            break;
        }
        /* obsolete but declared to avoid warining */
        default:
        {
            name = NULL;
            break;
        }
    }

    return name;
}

void variable_print(Variable *var)
{
    TRACE("");

    if(var == NULL)
    {
        LOG("var == NULL\n", "");
        return;
    }

    switch(var->type)
    {
        case VAR_NORMAL:
        {
            var_normal_print(var->body.var);

            break;
        }
        case VAR_ARR:
        {
            var_arr_print(var->body.arr);

            break;
        }
        default:
        {
            LOG("unrecognized type\n", "");
            break;
        }
    }
}

Value *value_create(VAR_TYPE type, void *str)
{
    Value *val;

    TRACE("");

    if( (type != CONST_VAL && type != VARIABLE) || str == NULL )
        ERROR("(type != CONST_VAL && type != VARIABLE) || str == NULL\n", NULL, "");

    val = (Value*)malloc(sizeof(Value));
    if(val == NULL)
        ERROR("malloc error\n", NULL, "");

    val->type = type;

    switch (type)
    {
        case CONST_VAL:
        {
            val->body.cv = str;
            break;
        }
        case VARIABLE:
        {
            val->body.var = str;
            break;
        }
        default:
        {
            FREE(val);

            /* This is obsolete but to avoid warn with no default i declare it */
            ERROR("unrecognized type\n", NULL, "");
        }
    }

    val->chunk = NULL;
    val->reg = NULL;

    return val;
}

void value_destroy(Value *val)
{
    TRACE("");

    if(val == NULL)
    {
        LOG("val == NULL\n", "");

        return;
    }

    switch (val->type)
    {
        case CONST_VAL:
        {
            const_value_destroy(val->body.cv);
            break;
        }
        case VARIABLE:
        {
            variable_destroy(val->body.var);
            break;
        }
        default:
        {
            LOG("unrecognized type\n", "");
            return;
        }
    }

    FREE(val);
}

void value_print(Value *value)
{
    TRACE("");

    if(value == NULL)
    {
        LOG("value == NULL\n", "");

        return;
    }

    switch(value->type)
    {
        case CONST_VAL:
        {
            const_value_print(value->body.cv);

            break;
        }
        case VARIABLE:
        {
            variable_print(value->body.var);

            break;
        }
        default:
        {
            LOG("unrecognized type\n", "");

            break;
        }
    }
}

char *value_str(Value *val)
{
    char *str;

    TRACE("");

    if(val == NULL)
        ERROR("val == NULL\n", NULL, "");

    switch(val->type)
    {
        case CONST_VAL:
        {
            if(asprintf(&str, " %ju ", val->body.cv->value) == -1)
                ERROR("asprintf error\n", NULL, "");

            break;
        }
        case VARIABLE:
        {
            if(val->body.var->type == VAR_NORMAL)
            {
                if(asprintf(&str, " %s ", variable_get_name(val->body.var)) == -1)
                    ERROR("asprintf error\n", NULL, "");
            }
            else
            {
                if(val->body.var->body.arr->var_offset == NULL)
                {
                    if(asprintf(&str, " %s[ %ju ] ", variable_get_name(val->body.var),
                        val->body.var->body.arr->offset) == -1)
                        ERROR("asprintf error\n", NULL, "");
                }
                else
                {
                    if(asprintf(&str, " %s[ %s ] ", variable_get_name(val->body.var),
                        val->body.var->body.arr->var_offset->name) == -1)
                            ERROR("asprintf error\n", NULL, "");
                }
            }

            break;
        }
        default:
        {
            LOG("unrecognized type\n", "");
            str = NULL;
            break;
        }
    }

    return str;
}

int value_get_val(Value *value, mpz_t val)
{
    TRACE("");

    mpz_init(val);

    if(value == NULL)
        ERROR("value == NULL\n", 1, "");

    if(value->type == VARIABLE)
    {
        if(value->body.var->type == VAR_NORMAL)
            mpz_set(val, value->body.var->body.var->value);
        else
            mpz_set(val, value->body.var->body.arr->var->value);
    }
    else
    {
        if(value->body.cv->type == CONST_VAL)
            ull2mpz(val, value->body.cv->value);
        else
            mpz_set(val, value->body.cv->big_value);
    }

    return 0;
}

void value_set_val(Value *value, mpz_t val)
{
    TRACE("");

    if(value == NULL)
    {
        LOG("value == NULL\n", 1, "");

        return;
    }

    if(value->type == VARIABLE)
    {
        if(value->body.var->type == VAR_NORMAL)
            mpz_set(value->body.var->body.var->value, val);
        else
            mpz_set(value->body.var->body.arr->var->value, val);
    }
    else
    {
        if(value->body.cv->type == CONST_VAL)
            value->body.cv->value = mpz2ull(val);
        else
            mpz_set(value->body.cv->big_value, val);
    }
}

int value_reset_symbolic_value(Value *val)
{
    TRACE("");

    if(val == NULL)
        ERROR("val == NULL\n", 1, "");

    if(val->type == VARIABLE)
    {
        if(val->body.var->type == VAR_NORMAL)
        {
            if(val->body.var->body.var->symbolic_value != NULL)
                FREE(val->body.var->body.var->symbolic_value);
        }
        else
        {
            if(val->body.var->body.arr->var->symbolic_value)
                FREE(val->body.var->body.arr->var->symbolic_value);
        }
    }

    return 0;
}

int value_add_symbolic(Value *val, char *str)
{
    char *old;

    TRACE("");

    if(val == NULL || str == NULL)
        ERROR("val == NULL || str == NULL\n", 1, "");

    if(val->type == CONST_VAL)
        ERROR("val is const\n", 1, "");

    if(val->body.var->type == VAR_NORMAL)
        old = val->body.var->body.var->symbolic_value;
    else
        old = val->body.var->body.arr->var->symbolic_value;

    if(old == NULL)
    {
        if(val->body.var->type == VAR_NORMAL)
        {
            if(asprintf(&val->body.var->body.var->symbolic_value, "%s", str) == -1)
                ERROR("asprintf error\n", 1, "");
        }
        else
        {
            if(asprintf(&val->body.var->body.arr->var->symbolic_value, "%s", str) == -1)
                ERROR("asprintf error\n", 1, "");
        }
    }
    else
    {
        if(val->body.var->type == VAR_NORMAL)
        {
            if(asprintf(&val->body.var->body.var->symbolic_value, "%s%s", old, str) == -1)
                ERROR("asprintf error\n", 1, "");
        }
        else
        {
            if(asprintf(&val->body.var->body.arr->var->symbolic_value, "%s%s", old, str) == -1)
                ERROR("asprintf error\n", 1, "");
        }

        FREE(old);
    }

    return 0;
}

int value_copy(Value **dst, Value *src)
{
    const_value     *cv;
    var_normal      *vn;
    var_normal      *vn2;
    var_arr         *va;
    Variable        *var;
    Value           *val;

    TRACE("");

    if(dst == NULL || src == NULL)
        ERROR("dst == NULL || src == NULL\n", 1, "");

    if(src->type == CONST_VAL)
    {
        cv = const_value_create(0ull);
        if(cv == NULL)
            ERROR("const_value_create error\n", 1, "");

        cv->type = src->body.cv->type;
        cv->value = src->body.cv->value;

        if(cv->type == BIG_CONST)
            mpz_set(cv->big_value, src->body.cv->big_value);

        val = value_create(CONST_VAL, (void*)cv);
        if(val == NULL)
            ERROR("value_create error\n", 1, "");

        val->chunk = src->chunk;
        val->reg = src->reg;

        *dst = val;
    }
    else /* VARIABLE */
    {
        if(src->body.var->type == VAR_NORMAL)
        {
            vn = var_normal_create(src->body.var->body.var->name);
            if(vn == NULL)
                ERROR("var_normal_create error\n", 1, "");

            vn->is_symbolic = src->body.var->body.var->is_symbolic;
            vn->symbolic_value = src->body.var->body.var->symbolic_value;
            mpz_set(vn->value, src->body.var->body.var->value);

            var = variable_create(VAR_NORMAL, (void*)vn);
            if(var == NULL)
                ERROR("variable_create error\n", 1, "");

            val = value_create(VARIABLE, (void*)var);
            if(val == NULL)
                ERROR("value_create error\n", 1, "");

            val->chunk = src->chunk;
            val->reg = src->reg;

            *dst = val;
        }
        else /* VAR ARR */
        {
            vn = var_normal_create(src->body.var->body.arr->var->name);
            if(vn == NULL)
                ERROR("var_normal_create error\n", 1, "");

            vn->is_symbolic = src->body.var->body.arr->var->is_symbolic;
            vn->symbolic_value = src->body.var->body.arr->var->symbolic_value;
            mpz_set(vn->value, src->body.var->body.arr->var->value);

            va = var_arr_create(vn, src->body.var->body.arr->arr,
                    src->body.var->body.arr->offset);
            if(va == NULL)
                ERROR("var_arr_create error\n", 1, "");

            if(src->body.var->body.arr->var_offset == NULL)
                va->var_offset = NULL;
            else
            {
                vn2 = var_normal_create(src->body.var->body.arr->var_offset->name);
                if(vn2 == NULL)
                    ERROR("var_normal_create error\n", 1, "");

                va->var_offset = vn2;
            }

            var = variable_create(VAR_ARR, (void*)va);
            if(var == NULL)
                ERROR("variable_create error\n", 1, "");

            val = value_create(VARIABLE, (void*)var);
            if(val == NULL)
                ERROR("value_create error\n", 1, "");

            val->chunk = src->chunk;
            val->reg = src->reg;

            *dst = val;
        }
    }

    return 0;
}

Array *array_create(const char *name, uint64_t len)
{
/*
    Private define
*/
#define CLEANUP(DA, IT, VA) \
    do{ \
        for(  darray_iterator_init(DA, &IT, ITI_BEGIN); \
            ! darray_iterator_end(&IT); \
              darray_iterator_next(&IT)) \
        { \
            darray_iterator_get_data(&IT, (void*)&VA); \
            var_arr_destroy(VA); \
        } \
    }while(0)

    uint64_t i;

    Array *array;
    var_arr *va;
    var_normal *var;

    Darray_iterator it;

    TRACE("");

    if(name == NULL)
        ERROR("name == NULL\n", NULL, "");

    array = (Array*)malloc(sizeof(Array));
    if(array == NULL)
        ERROR("malloc error\n", NULL, "");

    if(asprintf(&array->name, "%s", name) == -1)
        ERROR("asprintf error\n", NULL, "");

    mpz_init(array->addr);
    array->chunk = NULL;

    array->len = len;
    if(len <= ARRAY_MAX_LEN)
    {
        array->type = ARRAY;

        array->arr = darray_create(UNSORTED, 1 << 10, sizeof(var_arr*), var_arr_cmp);
        if(array->arr == NULL)
            ERROR("darray_create error\n", NULL, "");

        for(i = 0; i < len; ++i)
        {
            var = var_normal_create(name);
            if(var == NULL)
            {
                CLEANUP(array->arr, it, va);

                ERROR("var_normal_create error\n", NULL, "");
            }

            va = var_arr_create(var, array, i);
            if(va == NULL)
            {
                var_normal_destroy(var);
                CLEANUP(array->arr, it, va);

                ERROR("var_arr_create error\n", NULL, "");
            }

            if( darray_insert(array->arr, (void*)&va) )
            {
                var_arr_destroy(va);
                CLEANUP(array->arr, it, va);
                ERROR("darray_insert_last error\n", NULL, "");
            }
        }
    }
    else
    {
        array->type = BIG_ARRAY;
        array->arr = NULL;
    }

    return array;

#undef CLEANUP
}

void array_destroy(Array *array)
{
    Darray_iterator it;
    var_arr *va;

    TRACE("");

    if(array == NULL)
    {
        LOG("array == NULL\n", "");
        return;
    }

    if(array->type == ARRAY)
    {
        for( darray_iterator_init(array->arr, &it, ITI_BEGIN); \
             ! darray_iterator_end(&it); \
             darray_iterator_next(&it))
             {
                darray_iterator_get_data(&it,(void*)&va);

                var_arr_destroy(va);
             }

        darray_destroy(array->arr);
    }

    mpz_clear(array->addr);

    FREE(array->name);
    FREE(array);
}
