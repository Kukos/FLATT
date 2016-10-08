#ifndef ARCH_H
#define ARCH_H

#include <common.h>
#include <asm.h>
#include <avl.h>

/*
    File contains spec of architecture described by Maciek Gebala

    Author: Michal Kukowski
    email: michalkukowski10@gmail.com
*/

/* extern definisions of structures from asm.h */
struct Value;

/*****  CPU *****/

/* Registers */
#define REGS_NUMBER   5 /* we have 5 registers */
#define REG_PTR       0 /* first is the special one -> can load data from memory so it is like pointer */

#define REG_FREE      0  /* iff doesnt' have any value  */
#define REG_BUSY      1  /* iff has value but not use in current token */
#define REG_IN_USE    2  /* iff will be use in current token */

typedef struct Register
{
    struct Value *val;

    /* num in cpu regs array */
    uint64_t num;

    /* register state FREE or BUSY or IN_USE*/
    uint8_t state;

}Register;

/* CHECK REG STATE */
#define IS_REG_FREE(reg)        ( reg->state == REG_FREE )
#define IS_REG_BUSY(reg)        ( reg->state == REG_BUSY )
#define IS_REG_IN_USE(reg)      ( reg->state == REG_IN_USE )


/* SET REG AS FREE */
__inline__ void REG_SET_FREE(Register *reg)
{
    reg->state = REG_FREE;
    if(reg->val != NULL)
    {
        reg->val->reg = NULL;
        reg->val = NULL;
    }
}

#define REG_SET_BUSY(reg)       do{ reg->state = REG_BUSY; }while(0)
#define REG_SET_IN_USE(reg)     do{ reg->state = REG_IN_USE; }while(0)

/* set val in reg and null prev reg->val */
__inline__ void reg_set_val(Register *reg, Value *val)
{
    if(val == NULL || reg == NULL)
        return;

    if(reg->val != NULL)
        reg->val->reg = NULL;

    reg->val = val;
    if(val->reg != NULL)
        REG_SET_FREE(val->reg);

    val->reg = reg;
}


typedef struct CPU
{
    Register *registers[REGS_NUMBER];

}CPU;

/* Machine operation cost ( useful for static analyze ) */
typedef struct operation_cost
{
    uint32_t get;
    uint32_t put;
    uint32_t load;
    uint32_t store;
    uint32_t add;
    uint32_t sub;
    uint32_t copy;
    uint32_t shr;
    uint32_t shl;
    uint32_t inc;
    uint32_t dec;
    uint32_t zero;
    uint32_t jump;
    uint32_t jzero;
    uint32_t jodd;
    uint32_t halt;

}operation_cost;

/***** MEMORY *****/

#define ARRAY_MAX_LEN   (1 << 10)

typedef struct mem_chunk
{
    struct Value *val;

    /* addr is unique key */
    uint64_t addr;

}mem_chunk;


typedef struct Memory
{
    /* need dynamic logarithm structures so we use avl tree */
    Avl *memory;

    /***************************************************************************
    *                           MEMORY                                         *
    *           ----------------------------------------------------           *
    *           | LOOP VARIABLES | VARIABLES | ARRAYS | BIG ARRAYS |           *
    *           ----------------------------------------------------           *
    ***************************************************************************/

    /* mem space for variables */
    uint64_t var_first_addr;
    uint64_t var_last_addr;
    uint64_t var_allocated;

    /* mem space for arrays */
    uint64_t loop_var_first_addr;
    uint64_t loop_var_last_addr;
    uint64_t loop_var_allocated;

    /* mem space for arrays */
    uint64_t arrays_first_addr;
    uint64_t arrays_last_addr;
    uint64_t arrays_allocated;

    /* mem space for arrays */
    mpz_t big_arrays_first_addr;
    mpz_t big_arrays_allocated;

}Memory;

/*
    Standart compare function

    PARAMS
    @IN a - 1st mem chunk
    @IN b - 2nd mem chunk

    RETURN:
    %-1 a  < b
    %0  a == b
    %1  a  > b
*/
int mem_chunk_cmp(void *a, void *b) __nonull__(1,2);

/*
    Allocate structure in memory

    PARAMS
    @IN memory - pointer to memory
    @IN type - type of variable structure
    @IN stct - pointer to structure

    RETURN:
    %0 iff success
    %Non-zero value iff failure
*/
int my_malloc(Memory *memory, VAR_TYPE type, void *stct) __nonull__(1, 3);

/*
    FREE chunk at addr

    PARAMS\
    @INmemory - memory
    @IN addr - addr to free

    RETURN:
    %0 iff success
    %Non-zero value iff failure
*/
int my_free(Memory *memory, uint64_t addr) __nonull__(1);

/*
    Free loop_var section

    PARAMS
    IN memory - pointer to memory

    RETURN:
    %0 iff success
    %Non-zero value iff failure
*/
int free_loop_section(Memory *memory) __nonull__(1);

/***** BOARD *****/

/* Machine CPU */
extern CPU *cpu;

/* Machine RAM */
extern Memory *memory;

extern const operation_cost op_cost;

/*
    INIT board

    PARAMS
    NO PARAMS

    RETURN:
    0 iff success
    Non-zero iff failure
*/
int board_init(void);

/*
    Deallocate board structures

    PARAMS
    NO PARAMS

    RETURN:
    This is void function
*/
void board_destroy(void);


/***** INLINE FUNCTIONS ****/

/*
    Function check registers for variable

    PARAMS
    @IN CPU - pointer to CPU

    RETURN:
    -1 iff all regs are busy
    %free reg number iff exist free one
*/
__inline__ int get_free_reg(CPU *cpu)
{
    int i;

    for(i = REG_PTR + 1; i < REGS_NUMBER; ++i)
        if( IS_REG_FREE(cpu->registers[i]) )
            return i;

    return -1;
}

/*
    GET memory chunk by address

    PARAMS
    @IN addr - mem addr

    RETURN
    NULL iff failure
    Pointer to memchunk iff success
*/
__inline__ mem_chunk* memory_get_chunk(uint64_t addr)
{
    mem_chunk *in;
    mem_chunk *out;

    in = (mem_chunk*)malloc(sizeof(mem_chunk));
    if(in == NULL)
        ERROR("malloc error\n", NULL, "");

    in->addr = addr;

    if(avl_search(memory->memory, (void*)&in, (void*)&out))
        ERROR("avl_search error\n", NULL, "");

    FREE(in);

    return out;
}

#endif
