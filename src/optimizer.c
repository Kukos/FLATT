#include <optimizer.h>

int main_optimizing(Arraylist *in_tokens, Arraylist **out_tokens)
{
    int ret;

    TRACE("");

    /* for now we have not optimalization */
    ret = 0;
    *out_tokens = in_tokens;

    return ret;
}
