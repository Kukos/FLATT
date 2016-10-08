%{

/*
    Author: Michal Kukowski
    email: michalkukowski10@gmail.com
*/

#include <parser_helper.h>
#include <common.h>

/* definision of lexer function use in bison */
int yylex(void);

/* input file buffer */
file_buffer *fb_input = NULL;

/* input stream */
FILE *yyin;

Darray *code_lines;

/* check use of variables */
Avl *variables;

Arraylist *tokens;

/* parser built in functions */
static void yyerror(const char *msg);
static int yyparse(void);

%}

%code requires
{
    #include <parser_helper.h>
}

/* override yylval */
%union
{
    parser_token    ptoken;
    Pvalue          val;
    Pexpr           expr;
    Pcond           cond;
    Arraylist       *list;
    Token           *token;
}

%token	YY_EQ YY_NE YY_LT YY_GT YY_LE YY_GE
%token 	YY_ADD YY_SUB
%token	YY_DIV YY_MOD YY_MULT
%token	YY_ASSIGN
%token	YY_VAR YY_BEGIN YY_END
%token	YY_READ YY_WRITE YY_SKIP
%token	YY_FOR YY_FROM YY_TO YY_DOWNTO YY_ENDFOR
%token	YY_WHILE YY_DO YY_ENDWHILE
%token	YY_IF YY_THEN YY_ELSE YY_ENDIF
%token	YY_VARIABLE YY_NUM
%token	YY_L_BRACKET YY_R_BRACKET
%token	YY_ERROR
%token	YY_SEMICOLON

%type <ptoken>  YY_VARIABLE YY_NUM
%type <ptoken>  YY_EQ YY_NE YY_LT YY_GT YY_LE YY_GE
%type <ptoken>  YY_ADD YY_SUB YY_DIV YY_MOD YY_MULT
%type <ptoken>  YY_ASSIGN YY_VAR YY_BEGIN YY_END
%type <ptoken>  YY_READ YY_WRITE YY_SKIP
%type <ptoken>  YY_IF YY_THEN YY_ELSE YY_ENDIF
%type <ptoken>  YY_FOR YY_FROM YY_TO YY_DOWNTO YY_ENDFOR
%type <ptoken>  YY_WHILE YY_DO YY_ENDWHILE
%type <ptoken>  YY_L_BRACKET YY_R_BRACKET
%type <ptoken>  YY_ERROR YY_SEMICOLON

%type <val>     identifier value
%type <expr>    expr
%type <cond>    cond
%type <list>    command commands
%type <token>   fordeclar

%%

/*******************************************************************************
*                                                                              *
*                                                                              *
*                       ##### PROGRAM #####                                    *
*                                                                              *
*                                                                              *
*******************************************************************************/
program[pro]:
	%empty
	| YY_VAR[var] vdeclar[dec] YY_BEGIN[beg] commands[cmd] YY_END[end]
    {
        YY_LOG("[YACC]\tYY_VAR vdeclar YY_BEGIN commands YY_END\n","");

        FREE($var.str);
        FREE($beg.str);
        FREE($end.str);

        /* we read program code so assign pointer to token list */
        tokens = $cmd;
    }
;

/*******************************************************************************
*                                                                              *
*                                                                              *
*                       ##### VDECLAR #####                                    *
*                                                                              *
*                                                                              *
*******************************************************************************/
vdeclar[res]:
	%empty
/*******************************************************************************
*                       ##### VAR #####                                        *
*******************************************************************************/
	| vdeclar[dec] YY_VARIABLE[var]
    {
        YY_LOG("[YACC]\tvdeclar\tYY_VARIABLE:\t%s\n", $var.str);
        declare(PTOKEN_VAR, &$var, 0ull);

        FREE($var.str);
    }

/*******************************************************************************
*                       ##### VAR[NUM] #####                                   *
*******************************************************************************/
	| vdeclar[dec] YY_VARIABLE[var] YY_L_BRACKET[lb] YY_NUM[num] YY_R_BRACKET[rb]
    {
        YY_LOG("[YACC]\tdeclar\tYY_ARRAY:\t%s[ %s ]\n", $var.str, $num.str);
        declare(PTOKEN_ARR, &$var, $num.val);

        FREE($var.str);
        FREE($lb.str);
        FREE($num.str);
        FREE($rb.str);
    }
;

/*******************************************************************************
*                                                                              *
*                                                                              *
*                       ##### COMMANDS #####                                   *
*                                                                              *
*                                                                              *
*******************************************************************************/
commands[list]:
/*******************************************************************************
*                       ##### CMD #####                                        *
*******************************************************************************/
	command[cmd]
    {
        YY_LOG("[YACC]\tcommand\n", "");

        /* last recursive step, no alloc, just copy pointers */
        $list = $cmd;
    }

/*******************************************************************************
*                      ##### COMMANDS CMD #####                                *
*******************************************************************************/
	| commands[cmds] command[cmd]
    {
        /* cmds here is only needed for recursive calls, add only cmd tokens */
        YY_LOG("[YACC]\tcommands command\n", "");

        Token *token;
        Arraylist_iterator it;

        /* we alloc list in last recursive step, so just add to list */

        for( arraylist_iterator_init($cmd, &it, ITI_BEGIN);\
            ! arraylist_iterator_end(&it); \
            arraylist_iterator_next(&it))
        {

            arraylist_iterator_get_data(&it, (void*)&token);

            if(arraylist_insert_last($list, (void*)&token))
                ERROR("arraylist_insert_first error\n", 1, "");
        }


        /* we add here every token so now can destroy not needed list */
        arraylist_destroy($cmd);
    }
;

/*******************************************************************************
*                                                                              *
*                                                                              *
*                       ##### COMMANDS #####                                    *
*                                                                              *
*                                                                              *
*******************************************************************************/
command[cmd]:
/*******************************************************************************
*                       ##### VAL := EXPR #####                                *
*******************************************************************************/
	identifier[id] YY_ASSIGN[ass] expr[exp] YY_SEMICOLON[semi]
    {

#ifdef YY_DEBUG_MODE
        char *str1;
        char *str2;
        char *str3;
#endif
        Pvalue wrapper1;
        Pvalue wrapper2;

        token_assign *tass;
        Token *token;

        /* 1st case: ID := VAL */
        if($exp.expr->op == tokens_id.undefined)
        {

#ifdef YY_DEBUG_MODE
            str1 = pval_str(&$id);
            str2 = value_str($exp.expr->left);

            YY_LOG("[YACC]\t%s := %s\n", str1, str2);
#endif
            /* check use correctnes */
            pval_check_use(&$id, $id.line - 1, $semi.line - 1);

            wrapper1.val = $exp.expr->left;
            pval_check_use(&wrapper1, $id.line - 1, $semi.line - 1);

            if(wrapper1.val->type == VARIABLE)
            {
                pval_check_init(&wrapper1, $id.line - 1, $semi.line - 1);
                use(variable_get_name(wrapper1.val->body.var));
            }

            set(variable_get_name($id.val->body.var));

#ifdef YY_DEBUG_MODE
            FREE(str1);
            FREE(str2);
#endif
        }
        /* 2nd case: ID := VAL OP VAL */
        else
        {

#ifdef YY_DEBUG_MODE
            str1 = pval_str(&$id);
            str2 = value_str($exp.expr->left);
            str3 = value_str($exp.expr->right);

            YY_LOG("[YACC]\t%s := %s %s %s\n", str1, str2,
                        op_get_str($exp.expr->op), str3);
#endif

            /* check use correctnes */
            pval_check_use(&$id, $id.line - 1, $semi.line - 1);

            wrapper1.val = $exp.expr->left;
            pval_check_use(&wrapper1, $id.line - 1, $semi.line - 1);

            if(wrapper1.val->type == VARIABLE)
            {
                pval_check_init(&wrapper1, $id.line - 1, $semi.line - 1);
                use(variable_get_name(wrapper1.val->body.var));
            }

            wrapper2.val = $exp.expr->right;
            pval_check_use(&wrapper2, $id.line - 1, $semi.line - 1);

            if(wrapper2.val->type == VARIABLE)
            {
                pval_check_init(&wrapper2, $id.line - 1, $semi.line - 1);
                use(variable_get_name(wrapper2.val->body.var));
            }

            set(variable_get_name($id.val->body.var));

#ifdef YY_DEBUG_MODE
            FREE(str1);
            FREE(str2);
            FREE(str3);
#endif
        }

        tass = token_assign_create($id.val, $exp.expr);
        if(tass == NULL)
            ERROR("token_assign_create error\n", 1, "");

        token = token_create(TOKEN_ASSIGN, (void*)tass);
        if(token == NULL)
            ERROR("token_create error\n", 1, "");

        check_use_it($id.val, token, $id.line - 1, $semi.line - 1);

        /* create new list */
        $cmd = arraylist_create(sizeof(Token*));
        if($cmd == NULL)
            ERROR("arraylist_create error\n", 1, "");

        /* ADD token to list */
        if( arraylist_insert_last($cmd, (void*)&token))
            ERROR("arraylist_insert_last error\n", 1, "");

        FREE($semi.str);
        FREE($ass.str);
    }

/*******************************************************************************
*                   ##### IF COND CMD ELSE CMD ENDIF #####                     *
*******************************************************************************/
	| YY_IF[tif] cond[co] YY_THEN[then] commands[cmd1] YY_ELSE[telse] commands[cmd2] YY_ENDIF[endif]
    {

#ifdef YY_DEBUG_MODE
            char *str1 = value_str($co.cond->left);
            char *str2 = value_str($co.cond->right);
#endif
            token_if *tiff;
            token_guard *telsee;
            token_guard *endiff;

            Token *token1;
            Token *token2;
            Token *token3;

            Arraylist_iterator it;
            Token *token;

#ifdef YY_DEBUG_MODE
            YY_LOG("[YACC]\tIF %s %s %s\n", str1, rel_get_str($co.cond->r) ,str2);
#endif
            tiff = token_if_create($co.cond);
            if(tiff == NULL)
                ERROR("token_if_create error\n", 1, "");

            token1 = token_create(TOKEN_IF, (void*)tiff);
            if(token1 == NULL)
                ERROR("token_create error\n", 1, "");

            telsee = token_guard_create(tokens_id.else_cond);
            if(telsee == NULL)
                ERROR("token_guard_create error\n", 1, "");

            token2 = token_create(TOKEN_GUARD, (void*)telsee);
            if(token2 == NULL)
                ERROR("token_create error\n", 1, "");

            endiff = token_guard_create(tokens_id.end_if);
            if(endiff == NULL)
                ERROR("token_guard_create error\n", 1, "");

            token3 = token_create(TOKEN_GUARD, (void*)endiff);
            if(token3 == NULL)
                ERROR("token_create error\n", 1, "");

            /* create new list */
            $cmd = arraylist_create(sizeof(Token*));
            if($cmd == NULL)
                ERROR("arraylist_create error\n", 1, "");

            /* ADD IF token to list */
            if( arraylist_insert_last($cmd, (void*)&token1))
                ERROR("arraylist_insert_last error\n", 1, "");

            /* add if statement */
            for( arraylist_iterator_init($cmd1, &it, ITI_BEGIN);\
                ! arraylist_iterator_end(&it); \
                arraylist_iterator_next(&it))
            {

                arraylist_iterator_get_data(&it, (void*)&token);

                if(arraylist_insert_last($cmd, (void*)&token))
                    ERROR("arraylist_insert_first error\n", 1, "");
            }

            /* ADD ELSE token to list */
            if( arraylist_insert_last($cmd, (void*)&token2))
                ERROR("arraylist_insert_last error\n", 1, "");

            /* add ELSE statement */
            for( arraylist_iterator_init($cmd2, &it, ITI_BEGIN);\
                ! arraylist_iterator_end(&it); \
                arraylist_iterator_next(&it))
            {

                arraylist_iterator_get_data(&it, (void*)&token);

                if(arraylist_insert_last($cmd, (void*)&token))
                    ERROR("arraylist_insert_first error\n", 1, "");
            }

            /* ADD ENDIF token to list */
            if( arraylist_insert_last($cmd, (void*)&token3))
                ERROR("arraylist_insert_last error\n", 1, "");

#ifdef YY_DEBUG_MODE
            FREE(str1);
            FREE(str2);
#endif
            FREE($tif.str);
            FREE($then.str);
            FREE($telse.str);
            FREE($endif.str);

            /* we copy every tokens from cmd1 and cmd2 so destroy lists*/
            arraylist_destroy($cmd1);
            arraylist_destroy($cmd2);
    }

/*******************************************************************************
*                   ##### WHILE COND LOOP ENDWHILE #####                       *
*******************************************************************************/
	| YY_WHILE[twhile] cond[co] YY_DO[tdo] commands[cmd1] YY_ENDWHILE[tend]
    {

#ifdef YY_DEBUG_MODE
        char *str1 = value_str($co.cond->left);
        char *str2 = value_str($co.cond->right);
#endif
        token_while *twhilee;
        token_guard *endwhile;

        Token *token1;
        Token *token2;

        Arraylist_iterator it;
        Token *token;

#ifdef YY_DEBUG_MODE
        YY_LOG("[YACC]\tWHILE %s %s %s\n", str1, rel_get_str($co.cond->r) ,str2);
#endif
        twhilee = token_while_create($co.cond);
        if(twhilee == NULL)
            ERROR("token_while_create error\n", 1, "");

        token1 = token_create(TOKEN_WHILE, (void*)twhilee);
        if(token1 == NULL)
            ERROR("token_create error\n", 1, "");

        endwhile = token_guard_create(tokens_id.end_while);
        if(endwhile == NULL)
            ERROR("token_guard_create error\n", 1, "");

        token2 = token_create(TOKEN_GUARD, (void*)endwhile);
        if(token2 == NULL)
            ERROR("token_create error\n", 1, "");


        /* create new list */
        $cmd = arraylist_create(sizeof(Token*));
        if($cmd == NULL)
            ERROR("arraylist_create error\n", 1, "");

        /* ADD WHILE token to list */
        if( arraylist_insert_last($cmd, (void*)&token1))
            ERROR("arraylist_insert_last error\n", 1, "");

        /* add while statement */
        for( arraylist_iterator_init($cmd1, &it, ITI_BEGIN);\
            ! arraylist_iterator_end(&it); \
            arraylist_iterator_next(&it))
        {

            arraylist_iterator_get_data(&it, (void*)&token);

            if(arraylist_insert_last($cmd, (void*)&token))
                ERROR("arraylist_insert_first error\n", 1, "");
        }

        /* ADD ENDWHILE token to list */
        if( arraylist_insert_last($cmd, (void*)&token2))
            ERROR("arraylist_insert_last error\n", 1, "");

#ifdef YY_DEBUG_MODE
        FREE(str1);
        FREE(str2);
#endif
        FREE($twhile.str);
        FREE($tdo.str);
        FREE($tend.str);

        /* we copy every tokens from cmd so destroy list */
        arraylist_destroy($cmd1);
    }

/*******************************************************************************
*                       ##### FOR LOOP ENDFOR #####                            *
*******************************************************************************/
	| fordeclar[tfor] commands[cmd1] YY_ENDFOR[endfor]
    {
#ifdef YY_DEBUG_MODE
        char *str1 = value_str($tfor->body.for_loop->iterator);
        char *str2 = value_str($tfor->body.for_loop->begin_value);
        char *str3 = value_str($tfor->body.for_loop->end_value);
#endif

#ifdef YY_DEBUG_MODE
        if($tfor->body.for_loop->type == tokens_id.for_inc)
            YY_LOG("[YACC]\tFOR LOOP FOR %s FROM %s TO %s\n", str1, str2, str3);
        else
            YY_LOG("[YACC]\tFOR LOOP FOR %s FROM %s DOWNTO %s\n", str1, str2, str3);
#endif

        token_guard *endforr;
        Token *token1;

        Arraylist_iterator it;
        Token *token;

        endforr = token_guard_create(tokens_id.end_for);
        if(endforr == NULL)
            ERROR("token_guard error\n", 1, "");

        token1 = token_create(TOKEN_GUARD, (void*)endforr);
        if(token1 == NULL)
            ERROR("token_create error\n", 1, "");

        /* create new list */
        $cmd = arraylist_create(sizeof(Token*));
        if($cmd == NULL)
            ERROR("arraylist_create error\n", 1, "");

        /* ADD FOR token to list */
        if( arraylist_insert_last($cmd, (void*)&$tfor))
            ERROR("arraylist_insert_last error\n", 1, "");

        /* add FOR statement */
        for( arraylist_iterator_init($cmd1, &it, ITI_BEGIN);\
            ! arraylist_iterator_end(&it); \
            arraylist_iterator_next(&it))
        {

            arraylist_iterator_get_data(&it, (void*)&token);

            if(arraylist_insert_last($cmd, (void*)&token))
                ERROR("arraylist_insert_first error\n", 1, "");
        }

        /* ADD ENDFOR token to list */
        if( arraylist_insert_last($cmd, (void*)&token1))
            ERROR("arraylist_insert_last error\n", 1, "");

#ifdef YY_DEBUG_MODE
        FREE(str1);
        FREE(str2);
        FREE(str3);
#endif

        /* end loop, so undeclare iterator */
        undeclare(variable_get_name($tfor->body.for_loop->iterator->body.var));

        FREE($endfor.str);

        /* we copy every tokens from cmd so destroy list */
        arraylist_destroy($cmd1);
    }

/*******************************************************************************
*                           ##### READ VAL; #####                              *
*******************************************************************************/
	| YY_READ[read] identifier[id] YY_SEMICOLON[semi]
    {
        Token *token;
        token_io *io;

#ifdef YY_DEBUG_MODE
        YY_LOG("[YACC]\tREAD %s ;\n", variable_get_name($id.val->body.var));
#endif

        /* check use correctnes */
        pval_check_use(&$id, $read.line - 1, $semi.line - 1);

        io = token_io_create(tokens_id.read, $id.val);
        if(io == NULL)
            ERROR("token_io_create error\n", 1 ,"");

        token = token_create(TOKEN_IO, (void*)io);
        if(token == NULL)
            ERROR("token_create error\n", 1 ,"");

        check_use_it($id.val, token,  $read.line - 1, $semi.line - 1);

        /* read from stdin so we init value */
        set(variable_get_name($id.val->body.var));

        /* user want to use this variable */
        use(variable_get_name($id.val->body.var));

        /* create new list */
        $cmd = arraylist_create(sizeof(Token*));
        if($cmd == NULL)
            ERROR("arraylist_create error\n", 1, "");

        /* ADD token to list */
        if( arraylist_insert_last($cmd, (void*)&token))
            ERROR("arraylist_insert_last error\n", 1, "");

        FREE($read.str);
        FREE($semi.str);
    }

/*******************************************************************************
*                           ##### WRITE VAL; #####                             *
*******************************************************************************/
	| YY_WRITE[write] value[val] YY_SEMICOLON[semi]
    {
        Token *token;
        token_io *io;

#ifdef YY_DEBUG_MODE
        char *str = pval_str(&$val);

        YY_LOG("[YACC]\tWRITE %s ;\n", str);
#endif

        /* check use correctnes */
        pval_check_use(&$val, $write.line - 1, $semi.line - 1);

        /* WRITE required init var */
        pval_check_init(&$val, $write.line - 1, $semi.line - 1);

        io = token_io_create(tokens_id.write, $val.val);
        if(io == NULL)
            ERROR("token_io_create error\n", 1 ,"");

        token = token_create(TOKEN_IO, (void*)io);
        if(token == NULL)
            ERROR("token_create error\n", 1 ,"");

        /* we use this variable */
        if($val.val->type == VARIABLE)
            use(variable_get_name($val.val->body.var));

        /* create new list */
        $cmd = arraylist_create(sizeof(Token*));
        if($cmd == NULL)
            ERROR("arraylist_create error\n", 1, "");

        /* ADD token to list */
        if( arraylist_insert_last($cmd, (void*)&token))
            ERROR("arraylist_insert_last error\n", 1, "");

        FREE($write.str);
        FREE($semi.str);

#ifdef YY_DEBUG_MODE
        FREE(str);
#endif

    }

/*******************************************************************************
*                           ##### SKIP; #####                                  *
*******************************************************************************/
	| YY_SKIP[skip] YY_SEMICOLON[semi]
    {
        Token *token;
        token_guard *guard;

        YY_LOG("[YACC]\tSKIP ;\n", "");

        guard = token_guard_create(tokens_id.skip);
        if(guard == NULL)
            ERROR("token_guard_create error\n", 1, "");

        token = token_create(TOKEN_GUARD, (void*)guard);
        if(token == NULL)
            ERROR("token_create error\n", 1, "");

        /* create new list */
        $cmd = arraylist_create(sizeof(Token*));
        if($cmd == NULL)
            ERROR("arraylist_create error\n", 1, "");

        /* ADD token to list */
        if( arraylist_insert_last($cmd, (void*)&token))
            ERROR("arraylist_insert_last error\n", 1, "");

        FREE($skip.str);
        FREE($semi.str);
    }
;

/*******************************************************************************
*                                                                              *
*                                                                              *
*                       ##### FORDECLAR #####                                  *
*                                                                              *
*                                                                              *
*******************************************************************************/
fordeclar[declar]:
/*******************************************************************************
*                  ##### FOR IT FROM VAL TO VAL DO #####                       *
*******************************************************************************/
     YY_FOR[tfor] YY_VARIABLE[var] YY_FROM[from] value[val1] YY_TO[to] value[val2] YY_DO[tdo]
     {

#ifdef YY_DEBUG_MODE
        char *str1 = value_str($val1.val);
        char *str2 = value_str($val2.val);
#endif
        Variable *variable;
        var_normal *vn;
        Value *iterator;

        token_for *tforr;

        Token *token;

#ifdef YY_DEBUG_MODE
        YY_LOG("[YACC]\tFOR %s FROM %s TO %s\n", $var.str, str1, str2);
#endif

        /* check prev declaration */
        if(is_declared($var.str))
        {
            print_err($tfor.line - 1, $tdo.line - 1, "iterator is redeclared:\t%s\n",$var.str);

            exit(1);
        }

        /* declared and create iterator*/
        declare(PTOKEN_IT, &$var, 0);

        vn = var_normal_create($var.str);
        if(vn == NULL)
            ERROR("var_normal error\n", 1, "");

        variable = variable_create(VAR_NORMAL, (void*)vn);
        if(variable == NULL)
            ERROR("variable_create error\n", 1, "");

        iterator = value_create(VARIABLE, (void*)variable);
        if(iterator == NULL)
            ERROR("value_create error\n", 1, "");

        set($var.str);
        use($var.str);

        /* check correctnes  */
        pval_check_use(&$val1, $tfor.line - 1, $tdo.line - 1);
        pval_check_init(&$val1, $tfor.line - 1, $tdo.line - 1);

        /* we use this variable */
        if($val1.val->type == VARIABLE)
            use(variable_get_name($val1.val->body.var));

        pval_check_use(&$val2, $tfor.line - 1, $tdo.line - 1);
        pval_check_init(&$val2, $tfor.line - 1, $tdo.line - 1);

        /* we use this variable */
        if($val2.val->type == VARIABLE)
            use(variable_get_name($val2.val->body.var));

        tforr = token_for_create(tokens_id.for_inc, iterator, $val1.val, $val2.val);
        if(tforr == NULL)
            ERROR("token_for_create error\n", 1, "");

        token = token_create(TOKEN_FOR, (void*)tforr);
        if(token == NULL)
            ERROR("token_create error\n", 1, "");

        $declar = token;

#ifdef YY_DEBUG_MODE
        FREE(str1);
        FREE(str2);
#endif

        FREE($tfor.str);
        FREE($var.str);
        FREE($from.str);
        FREE($to.str);
        FREE($tdo.str);
    }

/*******************************************************************************
*                  ##### FOR IT FROM VAL DOWNTO VAL DO #####                   *
*******************************************************************************/
    |  YY_FOR[tfor] YY_VARIABLE[var] YY_FROM[from] value[val1] YY_DOWNTO[to] value[val2] YY_DO[tdo]
    {

#ifdef YY_DEBUG_MODE
        char *str1 = value_str($val1.val);
        char *str2 = value_str($val2.val);
#endif
        Variable *variable;
        var_normal *vn;
        Value *iterator;

        token_for *tforr;

        Token *token;

#ifdef YY_DEBUG_MODE
        YY_LOG("[YACC]\tFOR %s FROM %s DOWNTO %s\n", $var.str, str1, str2);
#endif
        /* check prev declaration */
        if(is_declared($var.str))
        {
            print_err($tfor.line - 1, $tdo.line - 1, "iterator is redeclared:\t%s\n",$var.str);

            exit(1);
        }

        /* declared and create iterator*/
        declare(PTOKEN_IT, &$var, 0);

        vn = var_normal_create($var.str);
        if(vn == NULL)
            ERROR("var_normal error\n", 1, "");

        variable = variable_create(VAR_NORMAL, (void*)vn);
        if(variable == NULL)
            ERROR("variable_create error\n", 1, "");

        iterator = value_create(VARIABLE, (void*)variable);
        if(iterator == NULL)
            ERROR("value_create error\n", 1, "");

        set($var.str);
        use($var.str);

        /* check correctnes  */
        pval_check_use(&$val1, $tfor.line - 1, $tdo.line - 1);
        pval_check_init(&$val1, $tfor.line - 1, $tdo.line - 1);

        /* we use this variable */
        if($val1.val->type == VARIABLE)
            use(variable_get_name($val1.val->body.var));

        pval_check_use(&$val2, $tfor.line - 1, $tdo.line - 1);
        pval_check_init(&$val2, $tfor.line - 1, $tdo.line - 1);

        /* we use this variable */
        if($val2.val->type == VARIABLE)
            use(variable_get_name($val2.val->body.var));

        tforr = token_for_create(tokens_id.for_dec, iterator, $val1.val, $val2.val);
        if(tforr == NULL)
            ERROR("token_for_create error\n", 1, "");

        token = token_create(TOKEN_FOR, (void*)tforr);
        if(token == NULL)
            ERROR("token_create error\n", 1, "");

        $declar = token;

#ifdef YY_DEBUG_MODE
        FREE(str1);
        FREE(str2);
#endif
        FREE($tfor.str);
        FREE($var.str);
        FREE($from.str);
        FREE($to.str);
        FREE($tdo.str);
     }
;
/*******************************************************************************
*                                                                              *
*                                                                              *
*                       ##### EXPRESION #####                                  *
*                                                                              *
*                                                                              *
*******************************************************************************/
expr[exp]:
/*******************************************************************************
*                           ##### VAL #####                                    *
*******************************************************************************/
	value[val]
    {

#ifdef YY_DEBUG_MODE
        char *str1 = pval_str(&$val);
        YY_LOG("[YACC]\texpr:\t %s\n", str1);
#endif

        $exp.expr = token_expr_create(tokens_id.undefined, $val.val, NULL);
        if($exp.expr == NULL)
            ERROR("token_expr_create error\n", 1, "");

        $exp.line = $val.line;

#ifdef YY_DEBUG_MODE
        FREE(str1);
#endif
    }

/*******************************************************************************
*                       ##### VAL + VAL #####                                  *
*******************************************************************************/
	| value[val1] YY_ADD[add] value[val2]
    {

#ifdef YY_DEBUG_MODE
        char *str1 = pval_str(&$val1);
        char *str2 = $add.str;
        char *str3 = pval_str(&$val2);
        YY_LOG("[YACC]\texpr:\t %s %s %s\n", str1, str2, str3);
#endif

        $exp.expr = token_expr_create(tokens_id.add, $val1.val, $val2.val);
        if($exp.expr == NULL)
            ERROR("token_expr_create error\n", 1, "");

        $exp.line = $val2.line;

#ifdef YY_DEBUG_MODE
        FREE(str1);
        FREE(str2);
        FREE(str3);
#else
        FREE($add.str);
#endif
    }

/*******************************************************************************
*                       ##### VAL - VAL #####                                  *
*******************************************************************************/
	| value[val1] YY_SUB[sub] value[val2]
    {

#ifdef YY_DEBUG_MODE
        char *str1 = pval_str(&$val1);
        char *str2 = $sub.str;
        char *str3 = pval_str(&$val2);
        YY_LOG("[YACC]\texpr:\t %s %s %s\n", str1, str2, str3);
#endif

        $exp.expr = token_expr_create(tokens_id.sub, $val1.val, $val2.val);
        if($exp.expr == NULL)
            ERROR("token_expr_create error\n", 1, "");

        $exp.line = $val2.line;

#ifdef YY_DEBUG_MODE
        FREE(str1);
        FREE(str2);
        FREE(str3);
#else
        FREE($sub.str);
#endif

    }

/*******************************************************************************
*                       ##### VAL * VAL #####                                  *
*******************************************************************************/
	| value[val1] YY_MULT[mult] value[val2]
    {

#ifdef YY_DEBUG_MODE
        char *str1 = pval_str(&$val1);
        char *str2 = $mult.str;
        char *str3 = pval_str(&$val2);
        YY_LOG("[YACC]\texpr:\t %s %s %s\n", str1, str2, str3);
#endif

        $exp.expr = token_expr_create(tokens_id.mult, $val1.val, $val2.val);
        if($exp.expr == NULL)
            ERROR("token_expr_create error\n", 1, "");

        $exp.line = $val2.line;

#ifdef YY_DEBUG_MODE
        FREE(str1);
        FREE(str2);
        FREE(str3);
#else
        FREE($mult.str);
#endif

    }

/*******************************************************************************
*                       ##### VAL / VAL #####                                  *
*******************************************************************************/
	| value[val1] YY_DIV[div] value[val2]
    {

#ifdef YY_DEBUG_MODE
        char *str1 = pval_str(&$val1);
        char *str2 = $div.str;
        char *str3 = pval_str(&$val2);
        YY_LOG("[YACC]\texpr:\t %s %s %s\n", str1, str2, str3);
#endif

        $exp.expr = token_expr_create(tokens_id.div, $val1.val, $val2.val);
        if($exp.expr == NULL)
            ERROR("token_expr_create error\n", 1, "");

        $exp.line = $val2.line;

#ifdef YY_DEBUG_MODE
        FREE(str1);
        FREE(str2);
        FREE(str3);
#else
        FREE($div.str);
#endif

    }

/*******************************************************************************
*                       ##### VAL % VAL #####                                  *
*******************************************************************************/
	| value[val1] YY_MOD[mod] value[val2]
    {

#ifdef YY_DEBUG_MODE
        char *str1 = pval_str(&$val1);
        char *str2 = $mod.str;
        char *str3 = pval_str(&$val2);
        YY_LOG("[YACC]\texpr:\t %s %s %s\n", str1, str2, str3);
#endif

        $exp.expr = token_expr_create(tokens_id.mod, $val1.val, $val2.val);
        if($exp.expr == NULL)
            ERROR("token_expr_create error\n", 1, "");

        $exp.line = $val2.line;

#ifdef YY_DEBUG_MODE
        FREE(str1);
        FREE(str2);
        FREE(str3);
#else
        FREE($mod.str);
#endif
    }
;

/*******************************************************************************
*                                                                              *
*                                                                              *
*                       ##### CONDITION #####                                  *
*                                                                              *
*                                                                              *
*******************************************************************************/
cond[con]:
/*******************************************************************************
*                       ##### VAL = VAL #####                                  *
*******************************************************************************/
	value[val1] YY_EQ[eq] value[val2]
    {

#ifdef YY_DEBUG_MODE
        char *str1 = pval_str(&$val1);
        char *str2 = $eq.str;
        char *str3 = pval_str(&$val2);
        YY_LOG("[YACC]\tcond:\t %s %s %s\n", str1, str2, str3);
#endif
        $con.cond = token_cond_create(tokens_id.eq, $val1.val, $val2.val);
        if($con.cond == NULL)
            ERROR("token_cond_create error\n", 1, "");

        $con.line = $val2.line;

        check_cond(&$con);
        use_cond(&$con);

#ifdef YY_DEBUG_MODE
        FREE(str1);
        FREE(str2);
        FREE(str3);
#else
        FREE($eq.str);
#endif
    }

/*******************************************************************************
*                       ##### VAL <> VAL #####                                 *
*******************************************************************************/
	| value[val1] YY_NE[ne] value[val2]
    {

#ifdef YY_DEBUG_MODE
        char *str1 = pval_str(&$val1);
        char *str2 = $ne.str;
        char *str3 = pval_str(&$val2);
        YY_LOG("[YACC]\tcond:\t %s %s %s\n", str1, str2, str3);
#endif

        $con.cond = token_cond_create(tokens_id.ne, $val1.val, $val2.val);
        if($con.cond == NULL)
            ERROR("token_cond_create error\n", 1, "");

        $con.line = $val2.line;

        check_cond(&$con);
        use_cond(&$con);

#ifdef YY_DEBUG_MODE
        FREE(str1);
        FREE(str2);
        FREE(str3);
#else
        FREE($ne.str);
#endif
    }

/*******************************************************************************
*                       ##### VAL < VAL #####                                  *
*******************************************************************************/
	| value[val1] YY_LT[lt] value[val2]
    {

#ifdef YY_DEBUG_MODE
        char *str1 = pval_str(&$val1);
        char *str2 = $lt.str;
        char *str3 = pval_str(&$val2);
        YY_LOG("[YACC]\tcond:\t %s %s %s\n", str1, str2, str3);
#endif

        $con.cond = token_cond_create(tokens_id.lt, $val1.val, $val2.val);
        if($con.cond == NULL)
            ERROR("token_cond_create error\n", 1, "");

        $con.line = $val2.line;

        check_cond(&$con);
        use_cond(&$con);

#ifdef YY_DEBUG_MODE
        FREE(str1);
        FREE(str2);
        FREE(str3);
#else
        FREE($lt.str);
#endif

    }

/*******************************************************************************
*                       ##### VAL > VAL #####                                  *
*******************************************************************************/
	| value[val1] YY_GT[gt] value[val2]
    {

#ifdef YY_DEBUG_MODE
        char *str1 = pval_str(&$val1);
        char *str2 = $gt.str;
        char *str3 = pval_str(&$val2);

        YY_LOG("[YACC]\tcond:\t %s %s %s\n", str1, str2, str3);
#endif

        $con.cond = token_cond_create(tokens_id.gt, $val1.val, $val2.val);
        if($con.cond == NULL)
            ERROR("token_cond_create error\n", 1, "");

        $con.line = $val2.line;

        check_cond(&$con);
        use_cond(&$con);

#ifdef YY_DEBUG_MODE
        FREE(str1);
        FREE(str2);
        FREE(str3);
#else
        FREE($gt.str);
#endif
    }

/*******************************************************************************
*                       ##### VAL <= VAL #####                                 *
*******************************************************************************/
	| value[val1] YY_LE[le] value[val2]
    {

#ifdef YY_DEBUG_MODE
        char *str1 = pval_str(&$val1);
        char *str2 = $le.str;
        char *str3 = pval_str(&$val2);
        YY_LOG("[YACC]\tcond:\t %s %s %s\n", str1, str2, str3);
#endif

        $con.cond = token_cond_create(tokens_id.le, $val1.val, $val2.val);
        if($con.cond == NULL)
            ERROR("token_cond_create error\n", 1, "");

        $con.line = $val2.line;

        check_cond(&$con);
        use_cond(&$con);

#ifdef YY_DEBUG_MODE
        FREE(str1);
        FREE(str2);
        FREE(str3);
#else
        FREE($le.str);
#endif
    }

/*******************************************************************************
*                       ##### VAL >= VAL #####                                 *
*******************************************************************************/
	| value[val1] YY_GE[ge] value[val2]
    {

#ifdef YY_DEBUG_MODE
        char *str1 = pval_str(&$val1);
        char *str2 = $ge.str;
        char *str3 = pval_str(&$val2);
        YY_LOG("[YACC]\tcond:\t %s %s %s\n", str1, str2, str3);
#endif

        $con.cond = token_cond_create(tokens_id.ge, $val1.val, $val2.val);
        if($con.cond == NULL)
            ERROR("token_cond_create error\n", 1, "");

        $con.line = $val2.line;

        check_cond(&$con);
        use_cond(&$con);

#ifdef YY_DEBUG_MODE
        FREE(str1);
        FREE(str2);
        FREE(str3);
#else
        FREE($ge.str);
#endif
    }
;

/*******************************************************************************
*                                                                              *
*                                                                              *
*                       ##### VALUE #####                                  *
*                                                                              *
*                                                                              *
*******************************************************************************/
value[val]:
/*******************************************************************************
*                       ##### NUM #####                                        *
*******************************************************************************/
	YY_NUM[num]
    {
        const_value *cv;

        YY_LOG("[YACC]\tYY_NUM:\t%s\n", $num.str);

        cv = const_value_create($num.val);
        if(cv == NULL)
            ERROR("const_value_create error\n", 1, "");

        $val.val = value_create(CONST_VAL, (void*)cv);
        if($val.val == NULL)
            ERROR("value_create error\n", 1, "");

        $val.line = $num.line;

        FREE($num.str);
    }
/*******************************************************************************
*                       ##### IDENTIFIER #####                                 *
*******************************************************************************/
	| identifier[id]
    {
        YY_LOG("[YACC]\tidentifier: %s\n", variable_get_name($id.val->body.var));

        if( memcpy(&$val, &$id, sizeof(Pvalue)) == NULL)
            ERROR("memcpy error\n", 1, "");

        /* we don't clean str because we copy pointer to $val */
    }
;

/*******************************************************************************
*                                                                              *
*                                                                              *
*                       ##### IDENTIFIER #####                                 *
*                                                                              *
*                                                                              *
*******************************************************************************/
identifier[id]:
/*******************************************************************************
*                       ##### VAR #####                                        *
*******************************************************************************/
	YY_VARIABLE[var]
    {
        Variable *variable;
        var_normal *vn;

        YY_LOG("[YACC]\tYY_VARIABLE:\t%s\n", $var.str);

        vn = var_normal_create($var.str);
        if(vn == NULL)
            ERROR("var_normal error\n", 1, "");

        variable = variable_create(VAR_NORMAL, (void*)vn);
        if(variable == NULL)
            ERROR("variable_create error\n", 1, "");

        $id.val = value_create(VARIABLE, (void*)variable);
        if($id.val == NULL)
            ERROR("value_create error\n", 1, "");

        $id.line = $var.line;

        FREE($var.str);
    }

/*******************************************************************************
*                       ##### VAR[VAR] #####                                   *
*******************************************************************************/
	| YY_VARIABLE[var1] YY_L_BRACKET[lb] YY_VARIABLE[var2] YY_R_BRACKET[rb]
    {
        Variable *variable;
        Array *arr = *(&arr);
        var_normal *vn;
        var_normal *offset;
        var_arr *va;

        YY_LOG("[YACC]\tYY_ARRAY:\t%s [ %s ]\n", $var1.str, $var2.str);

        vn = var_normal_create($var1.str);
        if(vn == NULL)
            ERROR("var_normal error\n", 1, "");

        offset = var_normal_create($var2.str);
        if(offset == NULL)
            ERROR("var_normal error\n", 1, "");

        va = var_arr_create(vn, arr, 0);
        if(va == NULL)
            ERROR("var_arr_create error\n", 1, "");

        va->var_offset = offset;

        variable = variable_create(VAR_ARR, (void*)va);
        if(variable == NULL)
            ERROR("variable_create error\n", 1, "");

        $id.val = value_create(VARIABLE, (void*)variable);
        if($id.val == NULL)
            ERROR("value_create error\n", 1, "");

        $id.line = $rb.line;

        /* we assume that array are always used  */
        if(is_declared($var1.str))
        {
            use($var1.str);

            /* array var offset always set as used */
            if(is_declared($var2.str))
                use($var2.str);
        }

        FREE($var1.str);
        FREE($lb.str);
        FREE($var2.str);
        FREE($rb.str);
    }

/*******************************************************************************
*                       ##### VAR[NUM] #####                                   *
*******************************************************************************/
	| YY_VARIABLE[var] YY_L_BRACKET[lb] YY_NUM[num] YY_R_BRACKET[rb]
    {
        Variable *variable;
        Array *arr = *(&arr);
        var_normal *vn;
        var_arr *va;

        YY_LOG("[YACC]\tYY_ARRAY:\t%s [ %s ]\n", $var.str, $num.str);

        vn = var_normal_create($var.str);
        if(vn == NULL)
            ERROR("var_normal error\n", 1, "");

        va = var_arr_create(vn, arr, $num.val);
        if(va == NULL)
            ERROR("var_arr_create error\n", 1, "");

        variable = variable_create(VAR_ARR, (void*)va);
        if(variable == NULL)
            ERROR("variable_create error\n", 1, "");

        $id.val = value_create(VARIABLE, (void*)variable);
        if($id.val == NULL)
            ERROR("value_create error\n", 1, "");

        $id.line = $rb.line;

        /* we assume that array are always used  */
        use($var.str);

        FREE($var.str);
        FREE($lb.str);
        FREE($num.str);
        FREE($rb.str);
    }
;

%%

static void yyerror(const char *msg)
{
    fprintf(stderr,"%sERROR!\t%s\t%s\nLINE: %ju\n%s%s\n",
            RED, msg, yylval.ptoken.str,
            yylval.ptoken.line,
            ((char**)code_lines->array)[yylval.ptoken.line - 1],
            RESET);

    exit(1);
}

int parse(const char *file, Arraylist **out_tokens)
{
    FILE *f;

    int fd;
    int ret;

    Darray_iterator it;
    char *temp;

    Avl_iterator avl_it;
    Pvar *pvar;

#ifdef DEBUG_MODE
    int i;
    Arraylist_iterator ait;
    Token *token;

    char *str;
#endif

    TRACE("");

    /* Create variables to check correctnes of using */
    variables = avl_create(sizeof(Pvar*), pvar_cmp);
    if(variables == NULL)
        ERROR("avl_create error\n", 1, "");

    /* map file */
    if(file == NULL)
        ERROR("file == NULL\n", 1, "");

    fd = open(file, O_RDWR | O_LARGEFILE);
    if( fd == -1)
        ERROR("cannot open %s file\n", 1, file);

    /* create File buffer for this file */
    fb_input = file_buffer_create(fd, PROT_READ | PROT_WRITE | MAP_SHARED);
    if(fb_input == NULL)
        ERROR("file_buffer_create error\n", 1, "");

    close(fd);

    code_lines = darray_create(UNSORTED, 0, sizeof(char*), NULL);
    if(code_lines == NULL)
        ERROR("darray_create error\n", 1, "");

    if(strspl(fb_input->buffer, '\n', code_lines))
        ERROR("strspl error\n", 1, "");

#ifdef DEBUG_MODE
    for(i = 0; i < code_lines->num_entries; ++i )
        LOG("LINE %d\t%s\n",i,((char**)code_lines->array)[i]);
#endif

    /* set file as lex input */
    f = fopen(file, "r");
    yyin = f;

	ret = yyparse();

#ifdef DEBUG_MODE
    for(arraylist_iterator_init(tokens, &ait, ITI_BEGIN); \
        ! arraylist_iterator_end(&ait); \
        arraylist_iterator_next(&ait) )
    {
        arraylist_iterator_get_data(&ait, (void*)&token);
        str = token_str(token);

        LOG("TOKEN: %s\n", str);

        FREE(str);
    }
#endif

    /* check ununused variables */
    for( avl_iterator_init(variables, &avl_it, ITI_BEGIN);
        ! avl_iterator_end(&avl_it);
        avl_iterator_next(&avl_it))
    {
        avl_iterator_get_data(&avl_it, (void*)&pvar);
        if( ! IS_VAR_USE(pvar) )
        {
            if(option.wall)
            {
                if(option.werr)
                {
                    fprintf(stderr,"%sERROR!\tunused variable:\t%s%s\n",
                        RED, pvar->name, RESET);

                    exit(1);
                }
                else
                fprintf(stderr,"%sWARNING!\tunused variable:\t%s%s\n",
                    YELLOW, pvar->name, RESET);
            }
        }
    }

    /* clean up */
    file_buffer_destroy(fb_input);

    for(darray_iterator_init(code_lines,&it,ITI_BEGIN); ! darray_iterator_end(&it); darray_iterator_next(&it))
    {
        darray_iterator_get_data(&it, (void*)&temp);
        FREE(temp);
    }

    fclose(f);
    darray_destroy(code_lines);

    *out_tokens = tokens;

    return ret;
}
