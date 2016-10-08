#include <common.h>
#include <parser_helper.h>
#include <stdarg.h>

extern Avl *variables;
extern Darray *code_lines;

int strspl(char *str, const char delimeter, Darray *arr)
{
    char *ptr;
    char *prev;
    char *new_str;
    uint64_t bytes;

    TRACE("");

    if(str == NULL || arr == NULL)
        ERROR("str == NULL || arr == NULL\n", 1, "");

    ptr = str;
    prev = str;

    while( *ptr != '\0' )
    {
        /* find delimeter */
        while( *ptr != '\0' && *ptr != delimeter )
            ++ptr;

        /* copy string to new string */
        bytes = (uint64_t)ptr - (uint64_t)prev + 1;
        new_str = (char*)malloc(sizeof(char) * bytes);
        if(new_str == NULL)
            ERROR("malloc error\n", 1, "");

        if( memcpy(new_str, prev, bytes - 1) == NULL )
            ERROR("memcpy error\n", 1, "");

        new_str[bytes - 1] = '\0';

        /* insert pointer to string to arr */
        if(darray_insert(arr, (void*)&new_str))
            ERROR("darray_insert error\n", 1, "");

        ++ptr;
        prev = ptr;
    }

    return 0;
}

void print_err(uint64_t fl, uint64_t ll, const char *msg, ...)
{
    uint64_t i;
    va_list args;

    if(msg == NULL || fl > ll)
    {
        LOG("msg == NULL || ff > ll\n", "");

        return;
    }

    fprintf(stderr, "%sERROR!\n", RED);

    va_start (args,msg);
   	vfprintf(stderr,msg,args);
   	va_end(args);

    for(i = fl; i <= ll; ++i)
        fprintf(stderr, "LINE: %ju\t\t%s\n", i + 1, ((char**)code_lines->array)[i]);

    fprintf(stderr, "%s\n", RESET);
}

void check_cond(Pcond *cond)
{
    Pvalue wrapper1;
    Pvalue wrapper2;

    TRACE("");

    if(cond == NULL)
    {
        LOG("cond == NULL\n", "");

        return;
    }

    /* check correctnes */
    wrapper1.val = cond->cond->left;
    pval_check_use(&wrapper1, cond->line - 1, cond->line - 1);

    if(wrapper1.val->type == VARIABLE)
        pval_check_init(&wrapper1, cond->line - 1, cond->line - 1);

    wrapper2.val = cond->cond->right;
    pval_check_use(&wrapper2, cond->line - 1, cond->line - 1);

    if(wrapper2.val->type == VARIABLE)
        pval_check_init(&wrapper2, cond->line - 1, cond->line - 1);
}

void use_cond(Pcond *cond)
{
    TRACE("");

    if(cond == NULL)
    {
        LOG("cond == NULL\n", "");

        return;
    }

    if(cond->cond->left->type == VARIABLE)
        use(variable_get_name(cond->cond->left->body.var));

    if(cond->cond->right->type == VARIABLE)
        use(variable_get_name(cond->cond->right->body.var));
}

Pvar *pvar_create(char *name, uint8_t type, uint64_t array_len)
{
    Pvar *pvar;

    TRACE("");

    if(name == NULL)
        ERROR("name == NULL\n", NULL, "");

    pvar = (Pvar*)malloc(sizeof(Pvar));
    if(pvar == NULL)
        ERROR("malloc error\n", NULL, "");

    pvar->set = 0;
    pvar->used = 0;
    pvar->type = type;

    if(pvar->type == PTOKEN_ARR)
        pvar->array_len = array_len;
    else
        pvar->array_len = 0;

    pvar->padding = 0;

    if(asprintf(&pvar->name, "%s", name) == -1)
        ERROR("asprintf error\n", NULL, "");

    return pvar;
}

void pvar_destroy(Pvar *pvar)
{
    TRACE("");

    if(pvar == NULL)
    {
        LOG("pvar == NULL\n", "");

        return;
    }

    FREE(pvar->name);
    FREE(pvar);
}

int pvar_cmp(void *pvar1, void *pvar2)
{
    Pvar *_pvar1 = *(Pvar**)pvar1;
    Pvar *_pvar2 = *(Pvar**)pvar2;

    int ret = strcmp(_pvar1->name, _pvar2->name);

    if(ret < 0)
        return -1;

    if(ret > 0)
        return 1;

    return 0;
}

BOOL is_declared(char *name)
{
    Pvar *in;

    BOOL ret;

    TRACE("");

    if(name == NULL)
        ERROR("name == NULL\n", FALSE, "");

    in = pvar_create(name, PTOKEN_VAR, 0);
    if(in == NULL)
        ERROR("pvar_create error\n", FALSE, "");

    ret = avl_key_exist(variables, (void*)&in);

    pvar_destroy(in);

    return ret;
}

char *pval_str(Pvalue *val)
{
    return value_str(val->val);
}

void declare(uint8_t type, parser_token *token, uint64_t array_len)
{
    Pvar *pvar;

    TRACE("");

    pvar = pvar_create(token->str, type, array_len);
    if(pvar == NULL)
    {
        LOG("pvar_create error\n", "");
        return;
    }

    /* double declaration */
    if(avl_insert(variables, (void*)&pvar))
    {
        YY_LOG("[YACC]\tdouble declaration\n", "");

        print_err(token->line - 1, token->line - 1, "redeclaration:\t%s\n", token->str);

        pvar_destroy(pvar);

        exit(1);
    }

    if(type == PTOKEN_ARR && array_len == 0)
    {
        YY_LOG("[YACC]\tarray with size 0\n", "");

        print_err(token->line - 1, token->line - 1, "array with size 0:\t%s\n", token->str);

        pvar_destroy(pvar);

        exit(1);
    }

}

void undeclare(char *name)
{
    Pvar *pvar;
    Pvar *out;

    TRACE("");

    pvar = pvar_create(name, 0, 0);
    if(pvar == NULL)
    {
        LOG("pvar_create error\n", "");
        return;
    }

    avl_search(variables, (void*)&pvar, (void*)&out);

    if(avl_delete(variables, (void*)&out))
    {
        YY_LOG("[YACC]\tvar %s undeclared\n", name);
    }

    pvar_destroy(pvar);
    pvar_destroy(out);

    return;
}

Pvar *pvar_get_by_name(char *name)
{
    Pvar *in;
    Pvar *out;

    TRACE("");

    if(name == NULL)
        ERROR("name == NULL\n", NULL, "");

    in = pvar_create(name, PTOKEN_VAR, 0);
    if(in == NULL)
        ERROR("pvar_create error\n", NULL, "");

    avl_search(variables, (void*)&in, (void*)&out);

    pvar_destroy(in);

    return out;
}

BOOL is_set(char *name)
{
    Pvar *var;
    TRACE("");

    if(name == NULL)
        ERROR("name == NULL\n", FALSE, "");

    var = pvar_get_by_name(name);
    if(var == NULL)
        ERROR("pvar_get_by_name error\n", FALSE, "");

    return IS_VAR_SET(var);
}

void set(char *name)
{
    Pvar *var;

    TRACE("");

    if(name == NULL)
    {
        LOG("name == NULL\n", "");

        return;
    }

    var = pvar_get_by_name(name);
    if(var == NULL)
    {
        LOG("pvar_get_by_name error\n", "");

        return;
    }

    VAR_SET(var);
}

BOOL is_used(char *name)
{
    Pvar *var;
    TRACE("");

    if(name == NULL)
        ERROR("name == NULL\n", FALSE, "");

    var = pvar_get_by_name(name);
    if(var == NULL)
        ERROR("pvar_get_by_name error\n", FALSE, "");

    return IS_VAR_USE(var);
}

void use(char *name)
{
    Pvar *var;

    TRACE("");

    if(name == NULL)
    {
        LOG("name == NULL\n", "");

        return;
    }

    var = pvar_get_by_name(name);
    if(var == NULL)
    {
        LOG("pvar_get_by_name error\n", "");

        return;
    }

    VAR_USE(var);
}

BOOL is_correct_use(char *name, uint8_t use_type)
{
    Pvar *var;
    TRACE("");

    if(name == NULL)
        ERROR("name == NULL\n", FALSE, "");

    var = pvar_get_by_name(name);
    if(var == NULL)
        ERROR("pvar_get_by_name error\n", FALSE, "");

    if(use_type == PTOKEN_VAR)
    {
        if( (var->type == PTOKEN_IT || var->type == PTOKEN_VAR))
            return TRUE;
        else
            return FALSE;
    }
    else
    {
        if(var->type == PTOKEN_ARR)
            return TRUE;
        else
            return FALSE;
    }
}

BOOL is_arr_out_of_range(char *name, uint64_t offset)
{
    Pvar *var;
    TRACE("");

    if(name == NULL)
        ERROR("name == NULL\n", FALSE, "");

    var = pvar_get_by_name(name);
    if(var == NULL)
        ERROR("pvar_get_by_name error\n", FALSE, "");

    return var->array_len <= offset;
}

void pval_check_use(Pvalue *val, uint64_t fl, uint64_t ll)
{
    char *name = NULL;
    uint8_t type;

    TRACE("");

    if(val == NULL || fl > ll)
    {
        LOG("val == NULL || fl > ll\n", "");

        return;
    }

    if(val->val->type == VARIABLE)
    {
        /* get name */
        name = variable_get_name(val->val->body.var);

        /* get type */
        if(val->val->body.var->type == VAR_ARR)
            type = PTOKEN_ARR;
        else
            type = PTOKEN_VAR;

        /* check declaration of val */
        if( ! is_declared(name))
        {
            print_err(fl, ll, "variable undeclared:\t%s\n", name);

            exit(1);
        }

        if( ! is_correct_use(name, type))
        {
            if(type == PTOKEN_VAR)
                print_err(fl, ll, "wrong use, var is an array:\t%s\n", name);
            else
                print_err(fl, ll, "wrong use, var is not an array:\t%s\n", name);

            exit(1);
        }

        /* iff var is array we have another checks */
        if(val->val->body.var->type == VAR_ARR)
        {
            /* check iff array member is out of array */
            if( is_arr_out_of_range(name, val->val->body.var->body.arr->offset))
            {
                print_err(fl, ll, "array member out of array range:\t%s[ %ju ]\n",
                            name, val->val->body.var->body.arr->offset);

                exit(1);
            }

            /* iff array offset is another var check declaration of this var */
            if(val->val->body.var->body.arr->var_offset != NULL)
                if( ! is_declared(val->val->body.var->body.arr->var_offset->name))
                {
                    print_err(fl, ll, "variable undeclared:\t%s\n",
                        val->val->body.var->body.arr->var_offset->name);

                    exit(1);
                }
        }
    }

    /* const val always are use correclty */
}

void pval_check_init(Pvalue *val, uint64_t fl, uint64_t ll)
{
    char *name = NULL;

    TRACE("");

    if(val == NULL || fl > ll)
    {
        LOG("val == NULL || fl > ll\n", "");

        return;
    }

    /* const value is always init */
    if(val->val->type == CONST_VAL)
        return;

    name = variable_get_name(val->val->body.var);

    /* if variable check initialized */
    if(val->val->body.var->type == VAR_NORMAL)
    {
        if( ! is_set(name))
        {
            print_err(fl, ll, "uninitialized variable:\t%s\n", name);

            exit(1);
        }
    }
    else
    {
        if(val->val->body.var->body.arr->var_offset != NULL)
            if( ! is_set(val->val->body.var->body.arr->var_offset->name))
            {
                print_err(fl, ll, "uninitialized variable:\t%s\n",
                        val->val->body.var->body.arr->var_offset->name);

                exit(1);
            }
    }
}

void check_use_it(Value *it, Token *token, uint64_t fl, uint64_t ll)
{
    Pvar *var;

    TRACE("");

    if(it == NULL || token == NULL)
    {
        LOG("it == NULL || token == NULL\n", "");

        return;
    }

    var = pvar_get_by_name(variable_get_name(it->body.var));
    if(var == NULL)
    {
        LOG("pvar_get_by_name error\n", "");

        return;
    }

    if(var->type == PTOKEN_IT)
    {
        switch(token->type)
        {
            case TOKEN_IO:
            {
                if(token->body.io->op == tokens_id.read)
                    if( ! variable_cmp((void*)&it->body.var,  (void*)&token->body.io->res->body.var))
                    {
                        print_err(fl, ll, "iterator overwrite:\t%s\n", variable_get_name(it->body.var));

                        exit(1);
                    }
                break;
            }
            case TOKEN_ASSIGN:
            {
                if( ! variable_cmp((void*)&it->body.var,  (void*)&token->body.assign->res->body.var))
                {
                    if(token->body.assign->expr->op == tokens_id.undefined &&
                        ! variable_cmp((void*)&it->body.var,  (void*)&token->body.assign->expr->left->body.var) )
                        break;

                    print_err(fl, ll, "iterator overwrite:\t%s\n", variable_get_name(it->body.var));

                    exit(1);
                }

                break;
            }
            default:
                break;
        }
    }
}
