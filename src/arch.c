#include <arch.h>
#include <asm.h>

/*
    Init cpu and registers

    PARAMS
    NO PARAMS

    RETURN:
    %NULL iff failure
    %Pointer to CPU iff success
*/
static CPU* cpu_init(void);

/*
    Detach cpu

    PARAMS
    @IN cpu - pointer to cpu

    RETURN:
    This is a void function
*/
static void cpu_destroy(CPU *cpu) __nonull__(1);

/*
    Init memory

    PARAMS
    NO PARAMS

    RETURN:
    %NULL iff failure
    %Pointer to memory iff success
*/
static Memory *memory_init(void);

/*
    Detach memory

    PARAMS
    @IN memory - pointer to Memory

    RETURN:
    This is a void function
*/
static void memory_destroy(Memory *memory) __nonull__(1);

/*
    Alloc mem_chunk in memory

    PARAMS
    @IN memory - pointer to memory
    @IN chunk - pointer to memory chunk

    RETURN:
    %0 iff success
    %Non-zero value iff failure
*/
static __inline__ int alloc(Memory *memory, mem_chunk *chunk);

/*
    Free mem_chunk in memory

    PARAMS
    @IN chunk - pointer to mem chunk

    RETURN:
    This is void function
*/
static void __inline__ dealloc(mem_chunk *chunk);

const operation_cost op_cost =
{
    .get    =   100,
    .put    =   100,
    .load   =   10,
    .store  =   10,
    .add    =   10,
    .sub    =   10,
    .copy   =   1,
    .shr    =   1,
    .shl    =   1,
    .inc    =   1,
    .dec    =   1,
    .zero   =   1,
    .jump   =   1,
    .jzero  =   1,
    .jodd   =   1,
    .halt   =   0
};

Memory *memory = NULL;
CPU *cpu = NULL;

static CPU *cpu_init(void)
{
    int i;
    int j;
    CPU *cpu;

    TRACE("");

    cpu = (CPU*)malloc(sizeof(CPU));
    if(cpu == NULL)
        ERROR("malloc error\n", NULL, "");

    for(i = 0; i < REGS_NUMBER; ++i)
    {
        cpu->registers[i] = (Register*)calloc(1,sizeof(Register));
        if(cpu->registers[i] == NULL)
        {
            for(j = 0; j < i; ++j)
                FREE(cpu->registers[i]);

            FREE(cpu);

            ERROR("calloc error\n", NULL, "");
        }
        cpu->registers[i]->val = NULL;
        cpu->registers[i]->num = i;
        REG_SET_FREE(cpu->registers[i]);
    }

    return cpu;
}

static void cpu_destroy(CPU *cpu)
{
    int i;

    TRACE("");
    if(cpu == NULL)
    {
        LOG("cpu == NULL\n", "");
        return;
    }

    for(i = 0; i < REGS_NUMBER; ++i)
        FREE(cpu->registers[i]);

    FREE(cpu);
}

static Memory *memory_init(void)
{
    Memory *memory;

    TRACE("");

    memory = (Memory*)malloc(sizeof(Memory));
    if(memory == NULL)
        ERROR("malloc error\n", NULL, "");

    memory->memory = avl_create(sizeof(mem_chunk*), mem_chunk_cmp);
    if(memory->memory == NULL)
        ERROR("avl_create error\n", NULL, "");

    memory->arrays_allocated = 0;
    memory->arrays_first_addr = 0;
    memory->arrays_last_addr = 0;

    memory->var_allocated = 0;
    memory->var_first_addr = 0;
    memory->var_last_addr = 0;

    memory->loop_var_allocated = 0;
    memory->loop_var_first_addr = 0;
    memory->loop_var_last_addr = 0;

    mpz_init(memory->big_arrays_allocated);
    mpz_init(memory->big_arrays_first_addr);

    return memory;
}

static void memory_destroy(Memory *memory)
{
    Avl_iterator it;
    mem_chunk *chunk;

    TRACE("");

    if(memory == NULL)
    {
        LOG("memory == NULL", "");

        return;
    }

    for(  avl_iterator_init(memory->memory, &it, ITI_BEGIN);
        ! avl_iterator_end(&it);
          avl_iterator_next(&it) )
        {
            avl_iterator_get_data(&it, (void*)&chunk);

            dealloc(chunk);
        }

    mpz_clear(memory->big_arrays_allocated);
    mpz_clear(memory->big_arrays_first_addr);

    avl_destroy(memory->memory);
    FREE(memory);
}

static int __inline__ alloc(Memory *memory, mem_chunk *chunk)
{
    return avl_insert(memory->memory, (void*)&chunk);
}

static void __inline__ dealloc(mem_chunk *chunk)
{
    value_destroy(chunk->val);

    FREE(chunk);
}

int mem_chunk_cmp(void *a, void *b)
{
    mem_chunk *_a = *(mem_chunk**)a;
    mem_chunk *_b = *(mem_chunk**)b;

    if(_a->addr < _b->addr)
        return -1;

    if(_a->addr > _b->addr)
        return 1;

    return 0;
}

int board_init(void)
{
    TRACE("");

    cpu = cpu_init();
    if(cpu == NULL)
        ERROR("cpu_init error\n",1,"");

    memory = memory_init();
    if(memory == NULL)
        ERROR("memory_init error\n",1,"");

    return 0;
}

void board_destroy(void)
{
    TRACE("");

    memory_destroy(memory);
    cpu_destroy(cpu);
}

int my_malloc(Memory *memory, VAR_TYPE type, void *stct)
{
    mem_chunk *chunk;
    mem_chunk *fake_chunk;

    Darray_iterator it;

    var_arr *va;
    Variable *var;
    Value *val;
    Array *arr;

    BOOL first_time = TRUE;

    mpz_t len;
    mpz_t addr;

    uint64_t i;

    TRACE("");

    if(stct == NULL)
        ERROR("struct == NULL\n", 1, "");

    switch(type)
    {
        case VALUE:
        {
            chunk = (mem_chunk*)malloc(sizeof(mem_chunk));
            if(chunk == NULL)
                ERROR("malloc error\n", 1, "");

            if(memory->var_allocated + memory->var_first_addr > memory->var_last_addr)
                ERROR("not enough memory for variable\n", 1, "");

            chunk->val = (Value*)stct;
            chunk->addr = memory->var_first_addr + memory->var_allocated;

            ++memory->var_allocated;

            if( alloc(memory, chunk) )
                ERROR("alloc error\n", 1, "");

            ((Value*)stct)->chunk = chunk;

            break;
        }
        case ARRAY:
        {
            arr = (Array*)stct;

            for(  darray_iterator_init(arr->arr, &it, ITI_BEGIN);
                ! darray_iterator_end(&it);
                  darray_iterator_next(&it))
                {
                    darray_iterator_get_data(&it, (void*)&va);

                    var = variable_create(VAR_ARR, (void*)va);
                    if(var == NULL)
                        ERROR("variable create error\n", 1 ,"");

                    val = value_create(VARIABLE, (void*)var);
                    if(val == NULL)
                        ERROR("value create error\n", 1, "");

                    chunk = (mem_chunk*)malloc(sizeof(mem_chunk));
                    if(chunk == NULL)
                        ERROR("malloc error\n", 1, "");

                    if(memory->arrays_allocated + memory->arrays_first_addr > memory->arrays_last_addr)
                        ERROR("not enough memory for arrays\n", 1, "");

                    chunk->val = val;
                    chunk->addr = memory->arrays_first_addr + memory->arrays_allocated;

                    if(first_time)
                    {
                        first_time = FALSE;
                        arr->chunk = chunk;

                        mpz_init(addr);

                        ull2mpz(addr, chunk->addr);

                        mpz_set(arr->addr, addr);

                        mpz_clear(addr);
                    }

                    ++memory->arrays_allocated;

                    if(alloc(memory, chunk) )
                        ERROR("alloc error\n", 1, "");

                    val->chunk = chunk;
                }

            break;
        }
        case LOOP_VAR:
        {
            fake_chunk = (mem_chunk*)malloc(sizeof(mem_chunk));
            if(fake_chunk == NULL)
                ERROR("malloc error\n", 1, "");

            chunk = (mem_chunk*)malloc(sizeof(mem_chunk));
            if(chunk == NULL)
                ERROR("malloc error\n", 1, "");

            /*
            if(memory->loop_var_allocated + memory->loop_var_first_addr > memory->loop_var_last_addr)
                ERROR("not enough memory for loop variable\n", 1, "");
            */

            chunk->val = (Value*)stct;

            /* find place for new chunk */
            for(i = memory->loop_var_first_addr; i <= memory->loop_var_last_addr; ++i)
            {
                fake_chunk->addr = i;
                if( ! avl_key_exist(memory->memory, (void*)&fake_chunk))
                    break;
            }

            chunk->addr = i;

            ++memory->loop_var_allocated;

            if( alloc(memory, chunk) )
                ERROR("alloc error\n", 1, "");

            ((Value*)stct)->chunk = chunk;

            FREE(fake_chunk);

            break;
        }
        case BIG_ARRAY:
        {
            mpz_init(len);
            mpz_init(addr);

            ull2mpz(len, ((Array*)stct)->len);

            /* set big array addr */
            mpz_add(addr, memory->big_arrays_first_addr, memory->big_arrays_allocated);
            mpz_set(((Array*)stct)->addr, addr);

            /* alloc big array in memory */
            mpz_add(memory->big_arrays_allocated,memory->big_arrays_allocated, len);


            mpz_clear(addr);
            mpz_clear(len);

            break;
        }
        default:
        {
            ERROR("unrecognized type\n", 1, "");

            break;
        }
    }

    return 0;
}

int free_loop_section(Memory *memory)
{
    uint64_t i;

    for(i = memory->loop_var_first_addr; i <= memory->loop_var_last_addr; ++i)
        if(my_free(memory, i))
            ERROR("my_free error\n", 1, "");

    return 0;
}

int my_free(Memory *memory, uint64_t addr)
{
    mem_chunk *key;
    mem_chunk *chunk;

    key = (mem_chunk*)malloc(sizeof(mem_chunk));
    key->addr = addr;

    if(avl_key_exist(memory->memory, (void*)&key))
    {
        if( avl_search(memory->memory, (void*)&key, (void*)&chunk) )
            ERROR("avl_search error\n", 1, "");

            dealloc(chunk);

        if( avl_delete(memory->memory, (void*)&key) )
            ERROR("avl_delete error\n", 1, "");

        if(addr >= memory->var_first_addr && addr <= memory->var_last_addr)
            --memory->var_allocated;
        else if (addr >= memory->loop_var_first_addr && addr <= memory->loop_var_last_addr)
            --memory->loop_var_allocated;
        else if(addr >= memory->arrays_first_addr && addr <= memory->arrays_first_addr)
            --memory->arrays_allocated;
    }

    FREE(key);

    return 0;
}
