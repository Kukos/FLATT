#include <common.h>
#include <arraylist.h>

/*
    Static analysis for code flow before compiling it to asm

    Author: Michal Kukowski
    email: michalkukowski10@gmail.com
*/

/*
    main function for code optimalization

    PARAMS
    @IN in_tokens - basic token list
    @OUT out_tokens - token list after optimalization

    RETURN:
    0 iff success
    1 iff failure
*/
int main_optimizing(Arraylist *in_tokens, Arraylist **out_tokens) __nonull__(1, 2);
