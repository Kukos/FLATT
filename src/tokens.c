#include <tokens.h>

token_id tokens_id =
{
    .undefined  =   0,
    .read       =   1,
    .write      =   2,
    .skip       =   3,
    .add        =   4,
    .sub        =   5,
    .div        =   6,
    .mod        =   7,
    .mult       =   8,
    .eq         =   9,
    .ne         =   10,
    .lt         =   11,
    .gt         =   12,
    .le         =   13,
    .ge         =   14,
    .for_inc    =   15,
    .for_dec    =   16,
    .end_for    =   17,
    .while_loop =   18,
    .cond       =   19,
    .if_cond    =   20,
    .else_cond  =   21,
    .end_if     =   22
};

token_io *token_io_create(uint8_t type, Value *val)
{
    token_io *token;

    TRACE("");

    if((type != tokens_id.read && type != tokens_id.write) || val == NULL
        || (type == tokens_id.read && val->type != VARIABLE))
        ERROR(  "(type != tokens_id.read && type != tokens_id.write) || val == NULL\n"
                "|| (type == tokens_id.read && val->type != VARIABLE)\n", NULL, "");

    token = (token_io*)malloc(sizeof(token_io));
    if(token == NULL)
        ERROR("malloc error\n", NULL, "");

    token->op = type;
    token->res = val;

    return token;
}

void token_io_destroy(token_io *token)
{
    TRACE("");

    if(token == NULL)
    {
        LOG("token == NULL\n", "");
        return;
    }

    value_destroy(token->res);

    FREE(token);
}

void token_io_print(token_io *token)
{
    if(token == NULL)
        return;

    if(token->op == tokens_id.read)
        printf("READ\t");
    else
        printf("WRITE\t");

    value_print(token->res);

    printf("\n");
}

char *token_io_str(token_io *token)
{
    char *str;
    char *val_str;
    char *op;

    TRACE("");

    if(token == NULL)
        ERROR("token == NULL\n", NULL, "");

    val_str = value_str(token->res);
    if(val_str == NULL)
        ERROR("value_str error\n", NULL, "");

    if(token->op == tokens_id.read)
        op = "READ";
    else
        op = "WRITE";

    if(asprintf(&str, "%s\t%s", op, val_str) == -1)
    {
        FREE(val_str);

        ERROR("asprintf error\n", NULL, "");
    }

    FREE(val_str);

    return str;
}

token_guard *token_guard_create(uint8_t type)
{
    token_guard *token;

    TRACE("");

    if(type != tokens_id.skip && type != tokens_id.end_for &&
        type != tokens_id.end_if && type != tokens_id.end_while &&
        type != tokens_id.else_cond )
        ERROR(  "type != tokens_id.skip && type != tokens_id.end_for &&"
                "type != tokens_id.end_if && type != tokens_id.end_while"
                "&& type != tokens_id.else_cond \n", NULL, "");

    token = (token_guard*)malloc(sizeof(token_guard));
    if(token == NULL)
        ERROR("malloc error\n", NULL, "");

    token->type = type;

    return token;
}

void token_guard_destroy(token_guard *token)
{
    TRACE("");

    if(token == NULL)
    {
        LOG("token == NULL\n", "");
        return;
    }

    FREE(token);
}

void token_guard_print(token_guard *token)
{
    if(token == NULL)
        return;

    if(token->type == tokens_id.else_cond)
        printf("ELSE\n");
    else if(token->type == tokens_id.end_for)
        printf("ENDFOR\n");
    else if(token->type == tokens_id.end_if)
        printf("ENDIF\n");
    else if(token->type == tokens_id.end_while)
        printf("ENDWHILE\n");
    else
        printf("SKIP\n");
}

char *token_guard_str(token_guard *token)
{
    char *temp;
    char *str;

    TRACE("");

    if(token == NULL)
        ERROR("token == NULL\n", NULL, "");

    if(token->type == tokens_id.else_cond)
        temp = "ELSE";
    else if(token->type == tokens_id.end_for)
        temp = "ENDFOR";
    else if(token->type == tokens_id.end_if)
        temp = "ENDIF";
    else if(token->type == tokens_id.end_while)
        temp = "ENDWHILE";
    else
        temp = "SKIP";

    if(asprintf(&str, "%s", temp) == -1)
        ERROR("asprintf error\n", NULL, "");

    return str;
}

token_expr *token_expr_create(uint8_t op, Value *left, Value *right)
{
    token_expr *token;

    TRACE("");

    if( (   op != tokens_id.add && op != tokens_id.sub && op != tokens_id.mult
            && op != tokens_id.mod && op != tokens_id.div &&op != tokens_id.undefined
        ) || left == NULL )
        ERROR(  "(   op != tokens_id.add && op != tokens_id.sub && op != tokens_id.mult\n"
                "&& op != tokens_id.mod && op != tokens_id.div && op != tokens_id.undefined\n"
                ") || left == NULL\n", NULL, "");

    token = (token_expr*)malloc(sizeof(token_expr));
    if(token == NULL)
        ERROR("malloc error\n", NULL, "");

    token->op = op;
    token->left = left;
    token->right = right;

    return token;
}

void token_expr_destroy(token_expr *token)
{
    TRACE("");

    if(token == NULL)
    {
        LOG("token == NULL\n", "");
        return;
    }

    value_destroy(token->left);

    if(token->op != tokens_id.undefined)
        value_destroy(token->right);

    FREE(token);
}

void token_expr_print(token_expr *token)
{
    if(token == NULL)
        return;

    value_print(token->left);

    if(token->op != tokens_id.undefined)
        printf(" %s ", op_get_str(token->op));

    if(token->right != NULL)
        value_print(token->right);
}

char *token_expr_str(token_expr *token)
{
    char *str;
    char *val1;
    char *val2;

    TRACE("");

    if(token == NULL)
        ERROR("token == NULL\n", NULL, "");

    val1 = value_str(token->left);
    if(val1 == NULL)
        ERROR("value_str error\n", NULL, "");

    if(token->op == tokens_id.undefined)
    {
        if(asprintf(&str, "%s", val1) == -1)
            ERROR("asprintf error\n", NULL, "");

    }
    else
    {
        val2 = value_str(token->right);
        if(val2 == NULL)
            ERROR("value_str error\n", NULL, "");

        if(asprintf(&str, "%s %s %s", val1, op_get_str(token->op), val2) == -1)
            ERROR("asprintf error\n", NULL, "");

        FREE(val2);
    }

    FREE(val1);

    return str;
}

token_assign *token_assign_create(Value *res, token_expr *expr)
{
    token_assign *token;

    TRACE("");

    if( res == NULL || expr == NULL )
        ERROR( " res == NULL || expr == NULL\n", NULL, "");

    token = (token_assign*)malloc(sizeof(token_assign));
    if(token == NULL)
        ERROR("malloc error\n", NULL, "");

    token->res = res;
    token->expr = expr;

    return token;
}

void token_assign_destroy(token_assign *token)
{
    TRACE("");

    if(token == NULL)
    {
        LOG("token == NULL\n", "");
        return;
    }

    value_destroy(token->res);
    token_expr_destroy(token->expr);

    FREE(token);
}

void token_assign_print(token_assign *token)
{
    if(token == NULL)
        return;

    value_print(token->res);
    printf(" := ");
    token_expr_print(token->expr);

    printf("\n");
}

char *token_assign_str(token_assign *token)
{
    char *expr_str;
    char *val;
    char *str;

    TRACE("");

    if(token == NULL)
        ERROR("token == NULL\n", NULL, "");

    expr_str = token_expr_str(token->expr);
    if(expr_str == NULL)
        ERROR("token_expr_str error\n", NULL, "");

    val = value_str(token->res);
    if(val == NULL)
        ERROR("value_str error\n", NULL, "");

    if(asprintf(&str, "%s := %s", val, expr_str) == -1)
        ERROR("asprintf error\n", NULL, "");

    FREE(val);
    FREE(expr_str);

    return str;
}

token_cond *token_cond_create(uint8_t r, Value *left, Value *right)
{
    token_cond *token;

    TRACE("");

    if( (   r != tokens_id.eq && r != tokens_id.ne && r != tokens_id.lt
            && r != tokens_id.gt && r != tokens_id.le && r != tokens_id.ge
        )  || left == NULL || right == NULL )
        ERROR(  "(   r != tokens_id.eq && r != tokens_id.ne && r != tokens_id.lt\n"
                "&& r != tokens_id.gt && r != tokens_id.le && r != tokens_id.ge\n"
                ")  || left == NULL || right == NULL\n", NULL, "");

    token = (token_cond*)malloc(sizeof(token_cond));
    if(token == NULL)
        ERROR("malloc error\n", NULL, "");

    token->r = r;
    token->left = left;
    token->right = right;

    return token;
}

void token_cond_destroy(token_cond *token)
{
    TRACE("");

    if(token == NULL)
    {
        LOG("token == NULL\n", "");
        return;
    }

    value_destroy(token->left);
    value_destroy(token->right);

    FREE(token);
}

void token_cond_print(token_cond *token)
{
    if(token == NULL)
        return;

    value_print(token->left);

    printf(" %s ", rel_get_str(token->r));

    value_print(token->right);
}

char *token_cond_str(token_cond *token)
{
    char *str;
    char *val1;
    char *val2;

    TRACE("");

    if(token == NULL)
        ERROR("token == NULL\n", NULL, "");

    val1 = value_str(token->left);
    if(val1 == NULL)
        ERROR("value_str error\n", NULL, "");

    val2 = value_str(token->right);
    if(val2 == NULL)
        ERROR("value_str error\n", NULL, "");

    if(asprintf(&str, "%s %s %s", val1, rel_get_str(token->r), val2) == -1)
        ERROR("asprintf error\n", NULL, "");

    FREE(val1);
    FREE(val2);

    return str;
}

token_if *token_if_create(token_cond *cond)
{
    token_if *token;

    if(cond == NULL)
        ERROR("cond == NULL\n", NULL, "");

    token = (token_if*)malloc(sizeof(token_if));
    if(token == NULL)
        ERROR("malloc error\n", NULL, "");

    token->cond = cond;

    return token;
}


void token_if_destroy(token_if *token)
{
    TRACE("");

    if(token == NULL)
    {
        LOG("token == NULL\n", "");
        return;
    }

    token_cond_destroy(token->cond);

    FREE(token);
}

void token_if_print(token_if *token)
{
    if(token == NULL)
        return;

    printf("IF\t");
    token_cond_print(token->cond);

    printf("\n");
}

char *token_if_str(token_if *token)
{
    char *str;
    char *cond_str;

    TRACE("");

    if(token == NULL)
        ERROR("token == NULL\n", NULL, "");

    cond_str = token_cond_str(token->cond);
    if(cond_str == NULL)
        ERROR("token_cond_str error\n", NULL, "");

    if(asprintf(&str, "IF\t%s", cond_str) == -1)
        ERROR("asprintf error\n", NULL, "");

    FREE(cond_str);

    return str;
}

token_while *token_while_create(token_cond *cond)
{
    token_while *token;

    if(cond == NULL)
        ERROR("cond == NULL\n", NULL, "");

    token = (token_while*)malloc(sizeof(token_while));
    if(token == NULL)
        ERROR("malloc error\n", NULL, "");

    token->cond = cond;

    return token;
}

void token_while_destroy(token_while *token)
{
    TRACE("");

    if(token == NULL)
    {
        LOG("token == NULL\n", "");
        return;
    }

    token_cond_destroy(token->cond);

    FREE(token);
}

void token_while_print(token_while *token)
{
    if(token == NULL)
        return;

    printf("WHILE\t");
    token_cond_print(token->cond);

    printf("\n");
}

char *token_while_str(token_while *token)
{
    char *str;
    char *cond_str;

    TRACE("");

    if(token == NULL)
        ERROR("token == NULL\n", NULL, "");

    cond_str = token_cond_str(token->cond);
    if(cond_str == NULL)
        ERROR("token_cond_str error\n", NULL, "");

    if(asprintf(&str, "WHILE\t%s", cond_str) == -1)
        ERROR("asprintf error\n", NULL, "");

    FREE(cond_str);

    return str;
}

token_for *token_for_create(uint8_t type, Value *it, Value *begin, Value *end)
{
    token_for *token;

    TRACE("");

    if( (type != tokens_id.for_inc && type != tokens_id.for_dec) ||
         it == NULL || it->type != VARIABLE ||
         begin == NULL || end == NULL)
         ERROR( "(type != tokens_id.for_inc && type != tokens_id.for_dec) ||\n"
                "it == NULL || it->type != VARIABLE ||\n"
                "begin == NULL || end == NULL\n", NULL, "");


    token = (token_for*)malloc(sizeof(token_for));
    if(token == NULL)
        ERROR("malloc error\n", NULL, "");

    token->type = type;
    token->iterator = it;
    token->begin_value = begin;
    token->end_value = end;

    return token;
}

void token_for_destroy(token_for *token)
{
    TRACE("");

    if(token == NULL)
    {
        LOG("token == NULL\n", "");
        return;
    }

    value_destroy(token->iterator);
    value_destroy(token->begin_value);
    value_destroy(token->end_value);

    FREE(token);
}

void token_for_print(token_for *token)
{
    if(token == NULL)
        return;

    printf("FOR ");
    value_print(token->iterator);
    printf(" FROM ");
    value_print(token->begin_value);

    if(token->type == tokens_id.for_inc)
        printf(" TO ");
    else
        printf(" DOWNTO ");

    value_print(token->end_value);

    printf("\n");
}

char *token_for_str(token_for *token)
{
    char *str;
    char *it;
    char *val1;
    char *val2;
    char *type;

    TRACE("");

    if(token == NULL)
        ERROR("token == NULL\n", NULL, "");

    it = value_str(token->iterator);
    if(it == NULL)
        ERROR("value_str error\n", NULL, "");

    val1 = value_str(token->begin_value);
    if(val1 == NULL)
        ERROR("value_str error\n", NULL, "");

    val2 = value_str(token->end_value);
    if(val2 == NULL)
        ERROR("value_str error\n", NULL, "");

    if(token->type == tokens_id.for_inc)
        type = "TO";
    else
        type = "DOWNTO";

    if(asprintf(&str, "FOR %s FROM %s %s %s", it, val1, type, val2) == -1)
        ERROR("asprint error\n", NULL, "");

    FREE(it);
    FREE(val1);
    FREE(val2);

    return str;
}

Token *token_create(TOKEN_TYPE type, void *token)
{
    Token *_token;

    TRACE("");

    if(token == NULL)
        ERROR("token == NULL\n", NULL, "");

    _token = (Token*)malloc(sizeof(Token));
    if(_token == NULL)
        ERROR("malloc error\n", NULL, "");

    _token->type = type;

    switch(type)
    {
        case TOKEN_IO:
        {
            _token->body.io = token;
            break;
        }
        case TOKEN_GUARD:
        {
            _token->body.guard = token;
            break;
        }
        case TOKEN_ASSIGN:
        {
            _token->body.assign = token;
            break;
        }
        case TOKEN_IF:
        {
            _token->body.if_cond = token;
            break;
        }
        case TOKEN_FOR:
        {
            _token->body.for_loop = token;
            break;
        }
        case TOKEN_WHILE:
        {
            _token->body.while_loop = token;
            break;
        }
        default:
        {
            FREE(_token);

            ERROR("unrecognized token\n", NULL, "");
        }
    }

    return _token;
}

void token_destroy(Token *token)
{
    TRACE("");

    if(token == NULL)
    {
        LOG("token == NULL\n", "");
        return;
    }

    switch(token->type)
    {
        case TOKEN_IO:
        {
            token_io_destroy(token->body.io);
            break;
        }
        case TOKEN_GUARD:
        {
            token_guard_destroy(token->body.guard);
            break;
        }
        case TOKEN_ASSIGN:
        {
            token_assign_destroy(token->body.assign);
            break;
        }
        case TOKEN_IF:
        {
            token_if_destroy(token->body.if_cond);
            break;
        }
        case TOKEN_FOR:
        {
            token_for_destroy(token->body.for_loop);
            break;
        }
        case TOKEN_WHILE:
        {
            token_while_destroy(token->body.while_loop);
            break;
        }
        default:
        {
            FREE(token);

            LOG("unrecognized token\n", "");
        }
    }

    FREE(token);
}

void token_print(Token *token)
{
    if(token == NULL)
        return;

    switch(token->type)
    {
        case TOKEN_IO:
        {
            token_io_print(token->body.io);
            break;
        }
        case TOKEN_GUARD:
        {
            token_guard_print(token->body.guard);
            break;
        }
        case TOKEN_ASSIGN:
        {
            token_assign_print(token->body.assign);
            break;
        }
        case TOKEN_IF:
        {
            token_if_print(token->body.if_cond);
            break;
        }
        case TOKEN_FOR:
        {
            token_for_print(token->body.for_loop);
            break;
        }
        case TOKEN_WHILE:
        {
            token_while_print(token->body.while_loop);
            break;
        }
        default:
            break;
        }
}

char *token_str(Token *token)
{
    TRACE("");

    if(token == NULL)
        ERROR("token == NULL", NULL, "");

    switch(token->type)
    {
        case TOKEN_IO:
        {
            return token_io_str(token->body.io);
        }
        case TOKEN_GUARD:
        {
            return token_guard_str(token->body.guard);
        }
        case TOKEN_ASSIGN:
        {
            return token_assign_str(token->body.assign);
        }
        case TOKEN_IF:
        {
            return token_if_str(token->body.if_cond);
        }
        case TOKEN_FOR:
        {
            return token_for_str(token->body.for_loop);
        }
        case TOKEN_WHILE:
        {
            return token_while_str(token->body.while_loop);
        }
        default:
            ERROR("unrecognized token type\n", NULL, "");
        }
}
