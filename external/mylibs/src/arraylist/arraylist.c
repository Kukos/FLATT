#include "arraylist.h"
#include "log.h"
#include "generic.h"
#include <stdlib.h>

#define FREE(PTR) do{ if( PTR != NULL) { free(PTR); PTR = NULL; } }while(0)

/*
    Create new alist node with ptr next and value val

    PARAMS
    @IN prev - ptr to prev node
    @IN next - ptr to next node in list
    @IN data - data sotred in node
    @IN size_of - size of val in bytes

    RETURN:
    NULL if failure
    Pointer if success
*/
static Arraylist_node *arraylist_node_create(Arraylist_node *prev,Arraylist_node *next, void* data,int size_of);


/*
    Destory nodeL

    PARAMS
    @IN node -ptr on list_node

    RETURN:
    This is void function
*/
static void arraylist_node_destroy(Arraylist_node *node);

static Arraylist_node *arraylist_node_create(Arraylist_node *prev,Arraylist_node *next,void *data,int size_of)
{
    Arraylist_node *node;

    TRACE("");

    if( data == NULL || size_of < 1)
        ERROR("data == NULL or size_of < 1\n", NULL, "");

    node = (Arraylist_node*)malloc(sizeof(Arraylist_node));
    if( node == NULL )
        ERROR("malloc error\n", NULL, "");

    node->data = malloc(size_of);
    if( node->data == NULL)
	{
		FREE(node);
        ERROR("malloc error\n", NULL, "");
	}

    node->next = next;
    node->prev = prev;
    __ASSIGN__(*(BYTE*)node->data,*(BYTE*)data,size_of);

    return node;
}

static void arraylist_node_destroy(Arraylist_node *node)
{
    TRACE("");

    if( node == NULL)
    {
        LOG("node == NULL\n", "");
        return;
    }

    FREE((node)->data);
    FREE(node);
}

Arraylist_iterator *arraylist_iterator_create(Arraylist *alist, ITI_MODE mode)
{
    Arraylist_iterator *iterator;

    TRACE("");

    if( alist == NULL || (mode != ITI_BEGIN && mode != ITI_END))
        ERROR("alist == NULL || (mode != ITI_BEGIN && mode != ITI_END)\n", NULL, "");

    iterator = (Arraylist_iterator*)malloc(sizeof(Arraylist_iterator));
    if(iterator == NULL)
        ERROR("malloc error\n", NULL, "");

    if(mode == ITI_BEGIN)
        iterator->node = alist->head;
    else
        iterator->node = alist->tail;

    iterator->size_of = alist->size_of;

    return iterator;
}

void arraylist_iterator_destroy(Arraylist_iterator *iterator)
{
    TRACE("");

    if(iterator == NULL)
    {
        LOG("iterator == NULL\n", "");
        return;
    }

    FREE(iterator);
}

int arraylist_iterator_init(Arraylist *alist, Arraylist_iterator *iterator, ITI_MODE mode)
{
    TRACE("");

    if( alist == NULL || iterator == NULL || (mode != ITI_BEGIN && mode != ITI_END))
        ERROR("alist || iterator == NULL || (mode != ITI_BEGIN && mode != ITI_END)\n", 1, "");

    if(mode == ITI_BEGIN)
        iterator->node = alist->head;
    else
        iterator->node = alist->tail;

    iterator->size_of = alist->size_of;

    return 0;
}

int arraylist_iterator_next(Arraylist_iterator *iterator)
{
    TRACE("");

    if( iterator == NULL )
        ERROR("iterator == NULL\n", 1, "");

    iterator->node = iterator->node->next;

    return 0;
}

int arraylist_iterator_prev(Arraylist_iterator *iterator)
{
    TRACE("");

    if( iterator == NULL )
        ERROR("iterator == NULL\n", 1, "");

    iterator->node = iterator->node->prev;

    return 0;
}

int arraylist_iterator_get_data(Arraylist_iterator *iterator,void *val)
{
    TRACE("");

    if( iterator == NULL || val == NULL)
        ERROR("iterator || val  == NULL\n", 1, "");

    __ASSIGN__(*(BYTE*)val,*(BYTE*)iterator->node->data,iterator->size_of);

    return 0;
}

BOOL arraylist_iterator_end(Arraylist_iterator *iterator)
{
    TRACE("");

    if( iterator == NULL)
        ERROR("iterator ==NULL\n", TRUE, "");

    return iterator->node == NULL;
}

Arraylist *arraylist_create(int size_of)
{
    Arraylist *alist;

    TRACE("");

    if( size_of < 1)
        ERROR("size_of < 1\n", NULL, "");

    alist = (Arraylist*)malloc(sizeof(Arraylist));
    if( alist == NULL )
        ERROR("malloc error\n", NULL, "");

    alist->size_of = size_of;

    alist->length = 0;
    alist->head = NULL;
    alist->tail = NULL;

    return alist;
}

void arraylist_destroy(Arraylist *alist)
{
    Arraylist_node *ptr;
    Arraylist_node *next;

    TRACE("");

    if( alist == NULL)
    {
        LOG("alist == NULL\n", "");
        return;
    }

    ptr = (alist)->head;

    while( ptr != NULL)
    {
        next = ptr->next;
        arraylist_node_destroy(ptr);

        ptr = next;
    }

    FREE(alist);

    return;
}

int arraylist_insert_first(Arraylist *alist, void *data)
{
    Arraylist_node *node;

    TRACE("");

    if(alist == NULL || data == NULL)
        ERROR("alist == NULL || data == NULL\n", 1 ,"");

    /* create node and insert at begining */
    node = arraylist_node_create(NULL, alist->head, data, alist->size_of);
    if(node == NULL)
        ERROR("arraylist_node_create error\n", 1, "");

    /* list was empty  */
    if(alist->head == NULL)
    {
        alist->head = node;
        alist->tail = node;
    }
    else
    {
        alist->head->prev = node;
        alist->head = node;
    }

    ++alist->length;

    return 0;
}

int arraylist_insert_last(Arraylist *alist, void *data)
{
    Arraylist_node *node;

    TRACE("");

    if(alist == NULL || data == NULL)
        ERROR("alist == NULL || data == NULL\n", 1, "");

    /* create node and insert at the end */
    node = arraylist_node_create(alist->tail,NULL, data, alist->size_of);
    if(node == NULL)
        ERROR("arraylist_node_create error\n", 1, "");

    if(alist->head == NULL)
    {
        alist->head = node;
        alist->tail = node;
    }
    else
    {
        alist->tail->next = node;
        alist->tail = node;
    }

    ++alist->length;

    return 0;
}

int arraylist_insert_pos(Arraylist *alist, int pos, void *data)
{
    Arraylist_node *node;
    Arraylist_node *ptr;

    TRACE("");

    if(alist == NULL || data == NULL)
        ERROR("alist == NULL || data == NULL\n", 1, "");

    if(pos < 0 || pos > alist->length)
        ERROR("invalid pos\n", 1, "");

    /* at the begining */
    if(pos == 0)
        return arraylist_insert_first(alist, data);

    /* at the end */
    if(pos == alist->length)
        return arraylist_insert_last(alist, data);

    /* go from begin */
    if (pos < (alist->length >> 1))
    {
        ptr = alist->head;
        for(long i = 0; i < pos; ++i)
            ptr = ptr->next;

    }
    /* go from end */
    else
    {
        pos = alist->length - pos - 1;
        ptr = alist->tail;
        for(long i = 0; i < pos; ++i)
            ptr = ptr->prev;
    }

    node = arraylist_node_create(ptr->prev,ptr, data, alist->size_of);
        if(node == NULL)
            ERROR("arraylist_node_create error\n", 1, "");

    ptr->prev->next = node;
    ptr->prev = node;

    ++alist->length;

    return 0;
}

int arraylist_delete_first(Arraylist *alist)
{
    Arraylist_node *node;

    TRACE("");

    if(alist == NULL)
        ERROR("alist == NULL\n", 1, "");

    if(alist->head == NULL)
        ERROR("alist is empty, nothing to delete\n", 1, "");

    node = alist->head;

    alist->head = alist->head->next;
    alist->head->prev = NULL;
    arraylist_node_destroy(node);

    --alist->length;

    return 0;
}

int arraylist_delete_last(Arraylist *alist)
{
    Arraylist_node *node;

    TRACE("");

    if(alist == NULL)
        ERROR("alist == NULL\n", 1, "");

    if(alist->head == NULL)
        ERROR("alist is empty, nothing to delete\n", 1, "");

    node = alist->tail;

    alist->tail = alist->tail->prev;
    alist->tail->next = NULL;

    arraylist_node_destroy(node);

    --alist->length;

    return 0;
}

int arraylist_delete_pos(Arraylist *alist, int pos)
{
    Arraylist_node *ptr;

    TRACE("");

    if(alist == NULL)
        ERROR("alist == NULL\n", 1, "");

    if(pos < 0 || pos > alist->length)
        ERROR("invalid pos\n", 1, "");

    /* at the begining */
    if(pos == 0)
        return arraylist_delete_first(alist);

    /* at the end */
    if(pos == alist->length - 1)
        return arraylist_delete_last(alist);

    /* go from begin */
    if (pos < (alist->length >> 1))
    {
        ptr = alist->head;
        for(long i = 0; i < pos; ++i)
            ptr = ptr->next;

    }
    /* go from end */
    else
    {
        pos = alist->length - pos - 1;
        ptr = alist->tail;
        for(long i = 0; i < pos; ++i)
            ptr = ptr->prev;
    }

    ptr->prev->next = ptr->next;
    ptr->next->prev = ptr->prev;

    arraylist_node_destroy(ptr);

    --alist->length;

    return 0;
}

int arraylist_get_pos(Arraylist *alist, int pos, void *data)
{
    Arraylist_node *ptr;

    TRACE("");

    if(alist == NULL)
        ERROR("alist == NULL\n", 1, "");

    if(pos < 0 || pos > alist->length)
        ERROR("invalid pos\n", 1, "");

    /* go from begin */
    if (pos < (alist->length >> 1))
    {
        ptr = alist->head;
        for(long i = 0; i < pos; ++i)
            ptr = ptr->next;

    }
    /* go from end */
    else
    {
        pos = alist->length - pos - 1;
        ptr = alist->tail;
        for(long i = 0; i < pos; ++i)
            ptr = ptr->prev;
    }

    __ASSIGN__(*(BYTE*)data,*(BYTE*)ptr->data,alist->size_of);

    return 0;
}

Arraylist *arraylist_merge(Arraylist *alist1, Arraylist *alist2)
{
    Arraylist_node *ptr1;
    Arraylist_node *ptr2;

    Arraylist *result;

    TRACE("");

    if(alist1 == NULL || alist2 == NULL)
        ERROR("alist1 == NULL || alist2 == NULL\n", NULL, "");

    result = arraylist_create(alist1->size_of);
    if(result == NULL)
        ERROR("arraylist_create error\n", NULL, "");

    ptr1 = alist1->head;
    while(ptr1 != NULL)
    {
        arraylist_insert_last(result, ptr1->data);
        ptr1 = ptr1->next;
    }

    ptr2 = alist2->head;
    while(ptr2 != NULL)
    {
        arraylist_insert_last(result, ptr2->data);
        ptr2 = ptr2->next;
    }

    return result;
}

int arraylist_to_array(Arraylist *alist,void *array,int *size)
{
    Arraylist_node *ptr;
    BYTE *t;

    int offset;

    TRACE("");

    if( alist == NULL || array == NULL || size == NULL)
        ERROR("alist == NULL || array == NULL || size  == NULL\n", 1, "");

    t = (BYTE*)malloc(alist->length * alist->size_of);
    if( t == NULL )
        ERROR("malloc error\n", 1, "");

    ptr = alist->head;

    offset = 0;

    while( ptr != NULL)
    {
        __ASSIGN__(t[offset],*(BYTE*)ptr->data,alist->size_of);
        offset += alist->size_of;

        ptr = ptr->next;
    }

    *(void**)array = t;

    *size = alist->length;

    return 0;
}
