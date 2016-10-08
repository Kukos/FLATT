#ifndef COMPILER_ALGO_H
#define COMPILER_ALGO_H

#include <compiler.h>
#include <common.h>
#include <tokens.h>
#include <asm.h>
#include <arch.h>
#include <arraylist.h>
#include <stack.h>

/*
    Algorithms to generate asm code and trace compiler values

    Author: Michal Kukowski
    email: michalkukowski10@email.com


    ALL functions with prefix "do" generate asm code
*/

#define PUMP_COST(n) (n ? LOG2(n) + HAMM_WEIGHT(n, 0ull) : 0)
__inline int32_t BPUMP_COST(mpz_t n)
{
    if(mpz_cmp_ui(n, 0) == 0)
        return 0;

    return (int32_t)(mpz_sizeinbase(n, 2) +  mpz_popcount(n));
}

extern Stack *labels;
extern Arraylist *token_list;
extern uint64_t token_list_pos;

/*
    Get the best register for value @val iff we are in pos @pos in tokens list

    PARAMS
    @IN tokens - tokens list
    @IN pos - pos in tokens list
    @IN val - value for which we want to get register

    RETURN:
    -1 iff failure
    Reg number iff success
*/
int get_register(Arraylist *tokens, uint64_t pos, Value *val) __nonull__(1);

/*
    Call get_register, synchronize memory iff needed

    PARAMS
    @IN tokens - tokens list
    @IN pos - pos in tokens list
    @IN val - value for which we want to get register
    @IN force_sync - force synchronize variable

    RETURN:
    Reg number iff success
    -1 iff failure
*/

int do_get_register(Arraylist *tokens, uint64_t pos, Value *val, BOOL force_sync) __nonull__(1, 3);

/*
    Better is get reg for val1 or val2 ?

    PARAMS
    @IN tokens - tokens list
    @IN pos - pos in tokens list
    @IN val1 - 1st val
    @IN val2 - 2nd val

    RETURN:
    -1 iff better val1
    0 iff doesn't matter
    1 iff better val2
*/
int better_reg(Arraylist *tokens, uint64_t pos, Value *val1, Value *val2) __nonull__(1 ,3 ,4);

/*
    Check loop for iterator usage

    PARAMS
    @IN tokens - tokens list
    @IN pos - pos in tokens list
    @IN it - iterator

    RETURN
    TRUE iff iterator is needed
    FALSE iff not
*/
BOOL is_iterator_needed(Arraylist *tokens, uint64_t pos, Value *it) __nonull__(1, 3);

/*
    set reg value to 0

    PARAMS
    @IN reg - pointer to register
    @IN trace - want to trace value in reg ?

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_zero(Register *reg, BOOL trace) __nonull__(1);

/*
    inc reg value

    PARAMS
    @IN reg - pointer to register
    @IN trace - want to trace value in reg ?

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_inc(Register *reg, BOOL trace) __nonull__(1);

/*
    dec reg value

    PARAMS
    @IN reg - pointer to register
    @IN trace - want to trace value in reg ?

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_dec(Register *reg, BOOL trace) __nonull__(1);

/*
    reg val = val << 1

    PARAMS
    @IN reg - pointer to register
    @IN trace - want to trace value in reg ?

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_shl(Register *reg, BOOL trace) __nonull__(1);

/*
    reg val = val >> 1

    PARAMS
    @IN reg - pointer to register
    @IN trace - want to trace value in reg ?

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_shr(Register *reg, BOOL trace) __nonull__(1);

/*
    load val from memory to reg

    PARAMS
    @IN reg - pointer to register
    @IN val - value to load

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_load(Register *reg, Value *val) __nonull__(1, 2);

/*
    store reg value in memory

    PARAMS
    @IN reg - pointer to register

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_store(Register *reg) __nonull__(1);

/*
    GET value from stdin to reg

    PARAMS
    @IN reg - pointer to register
    @IN trace - want to trace value in reg ?

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_get(Register *reg, BOOL trace) __nonull__(1);

/*
    PUT reg value to stdout

    PARAMS
    @IN reg - pointer to register

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_put(Register *reg) __nonull__(1);

/*
    Generate halt asm code

    PARAMS
    NO PARAMS

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_halt(void);

/*
    REG = REG + MEM[REG_PTR]

    PARAMS
    @IN reg - pointer to register
    @IN trace - want to trace value in reg ?

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_add(Register *reg, BOOL trace) __nonull__(1);

/*
    REG = MAX(0,REG - MEM[REG_PTR])

    PARAMS
    @IN reg - pointer to register
    @IN trace - want to trace value in reg ?

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_sub(Register *reg, BOOL trace) __nonull__(1);

/*
    REG = REG2 * REG3

    PARAMS
    @IN res - pointer to register
    @IN left - pointer to register
    @IN right - pointer to register
    @IN trace - want to trace value in reg ?

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_mult(Register *res, Register *left, Register *right, BOOL trace) __nonull__(1, 2, 3);


/*
    REG = REG2 / REG3

    PARAMS
    @IN res - pointer to register
    @IN left - pointer to register
    @IN right - pointer to register
    @IN trace - want to trace value in reg ?

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_div(Register *res, Register *left, Register *right, BOOL trace) __nonull__(1, 2, 3);

/*
    REG = REG2 mod REG3

    PARAMS
    @IN res - pointer to register
    @IN left - pointer to register
    @IN right - pointer to register
    @IN trace - want to trace value in reg ?

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_mod(Register *res, Register *left, Register *right, BOOL trace) __nonull__(1, 2, 3);

/*
    JUMP TO LINE

    PARAMS
    @IN line line where we want to jump

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_jump(uint64_t line);

/*
    if REG == 0 JUMP TO LINE

    PARAMS
    @IN reg - pointer to register
    @IN line line where we want to jump

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_jzero(Register *reg, uint64_t line) __nonull__(1);

/*
    if REG & 1 == 1 JUMP TO LINE

    PARAMS
    @IN reg - pointer to register
    @IN line line where we want to jump

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_jodd(Register *reg, uint64_t line) __nonull__(1);

/*
    JUMP TO label

    PARAMS
    @IN type - label type

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_jump_label(int8_t type);

/*
    if REG == 0 JUMP TO label

    PARAMS
    @IN reg - pointer to register
    @IN type - label type

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_jzero_label(Register *reg, int8_t type) __nonull__(1);

/*
    if REG & 1 == 1 JUMP TO LINE

    PARAMS
    @IN reg - pointer to register
    @IN type - label type

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_jodd_label(Register *reg, int8_t type) __nonull__(1);

/*
    REG_PTR = reg

    PARAMS
    @IN reg - pointer to register
    @IN trace - want to trace value in reg ?

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_copy(Register *reg, BOOL trace) __nonull__(1);

/*
    Pump value @val to register

    PARAMS
    @IN reg - register
    @IN val- value to pump
    @IN trace - want to trace value in reg ?

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_pump(Register *reg, uint64_t val, BOOL trace) __nonull__(1);


/*
    Pump BIG value @val to register

    PARAMS
    @IN reg - register
    @IN val- value to pump
    @IN trace - want to trace value in reg ?

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_pump_bigvalue(Register *reg, mpz_t val, BOOL trace) __nonull__(1);

/*
    Set in register @reg addr if @val

    PARAMS
    @IN reg - pointer to register
    @IN val - pointer to value
    @IN trace - want to trace value in reg ?

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_set_val_addr(Register *reg, Value *val, BOOL trace) __nonull__(1 , 2);

/*
    Synchronize value in reg with value in memory
    (do_store WRAPPER)

    PARAMS
    @IN reg - register to synchronize

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_synchronize(Register *reg) __nonull__(1);

/*
    left < right

    left in reg, right in memory

    PARAMS
    @IN left - register
    @IN right - value in memory

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_lt1(Register *left, Value *right) __nonull__(1 , 2);

/*
    left < right

    left in memory, right in register

    PARAMS
    @IN right - register
    @IN left - value in memory

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_lt2(Register *right, Value *left) __nonull__(1 , 2);

/*
    left <= right

    left in reg, right in memory

    PARAMS
    @IN left - register
    @IN right - value in memory

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_le1(Register *left, Value *right) __nonull__(1 , 2);

/*
    left <= right

    left in memory, right in register

    PARAMS
    @IN right - register
    @IN left - value in memory

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_le2(Register *right, Value *left) __nonull__(1 , 2);

/*
    left > right

    left in reg, right in memory

    PARAMS
    @IN left - register
    @IN right - value in memory

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_gt1(Register *left, Value *right) __nonull__(1 , 2);

/*
    left > right

    left in memory, right in register

    PARAMS
    @IN right - register
    @IN left - value in memory

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_gt2(Register *right, Value *left) __nonull__(1 , 2);

/*
    left >= right

    left in reg, right in memory

    PARAMS
    @IN left - register
    @IN right - value in memory

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_ge1(Register *left, Value *right) __nonull__(1 , 2);

/*
    left >= right

    left in memory, right in register

    PARAMS
    @IN right - register
    @IN left - value in memory

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_ge2(Register *right, Value *left) __nonull__(1 , 2);

/*
    left == right

    left in reg, right in memory

    PARAMS
    @IN left - register
    @IN right - value in memory

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_eq1(Register *left, Value *right) __nonull__(1 , 2);

/*
    left == right

    left in memory, right in register

    PARAMS
    @IN right - register
    @IN left - value in memory

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_eq2(Register *right, Value *left) __nonull__(1 , 2);

/*
    left != right

    left in reg, right in memory

    PARAMS
    @IN left - register
    @IN right - value in memory

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_ne1(Register *left, Value *right) __nonull__(1 , 2);

/*
    left != right

    left in memory, right in register

    PARAMS
    @IN right - register
    @IN left - value in memory

    RETURN
    0 iff success
    Non-zero value iff failure
*/
int do_ne2(Register *right, Value *left) __nonull__(1 , 2);

/*
    Check trace status

    PARAMS
    @IN val - value

    RETURN
    FALSE iff can't trace value
    TRUE iff can trace value
*/
__inline__ BOOL value_can_trace(Value *val)
{
    /* can't trace null */
    if(val == NULL)
        return FALSE;

    /* can't trace  */
    if(val->type == CONST_VAL)
        return FALSE;

    /* we can't trace any array */
    if(val->body.var->type == VAR_ARR)
        return FALSE;

    if(value_is_big_array(val))
        return FALSE;

    return TRUE;
}

/* get node from arraylist */
__inline__ Arraylist_node *arraylist_get_node(Arraylist *list, uint64_t pos)
{
    Arraylist_node *node;
    uint64_t i = 0;

    if(list == NULL)
        return NULL;

    node = list->head;
    while(node != NULL && i < pos)
        node = node->next;

    return node;
}

#endif
