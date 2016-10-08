#include <stdlib.h>
#include <string.h>
#include "darray.h"
#include "generic.h"
#include "sort.h"
#include "log.h"

#define FREE(T) do{ if( T != NULL) { free(T); T = NULL; } }while(0)

#define INIT_SIZE 1024

/*
    PARAMS
    @IN darray - pointer to darray
    @IN val -searched val

    RETURN:
    %-1 if failure
    %Pos if success
*/
static int darray_unsorted_search_first(Darray *darray,void* val);

/*
    PARAMS
    @IN darray - pointer to darray
    @IN val -searched val

    RETURN:
    %-1 if failure
    %Pos if success
*/
static int darray_unsorted_search_last(Darray *darray,void* val);

/*
    PARAMS
    @IN darray - pointer to darray
    @IN val -searched val

    RETURN:
    %-1 if failure
    %Pos if success
*/
static int darray_sorted_search_first(Darray *darray,void* val);

/*
    PARAMS
    @IN darray - pointer to darray
    @IN val -searched val

    RETURN:
    %-1 if failure
    %Pos if success
*/
static int darray_sorted_search_last(Darray *darray,void* val);

/*
    Find pos where val i MAX ( <= val )

    PARAMS
    @IN darray - pointer to darray
    @IN val - searched val

    RETURN:
    %-1 if failure
    %Pos if success
*/
static int upper_bound(Darray *darray,void* val);


static int darray_unsorted_search_first(Darray *darray,void* val)
{
    BYTE* _t;
    int offset_i;
    int max;

    TRACE("");

    if( darray == NULL || darray->array == NULL || darray->num_entries < 0
        || val == NULL || darray->cmp == NULL)
    {
        ERROR(  "darray == NULL || darray->array == NULL || "
 				"darray->num_entries < 0 || val == NULL || darray->cmp == NULL\n",-1, "");
    }

    _t = darray->array;
    max = darray->num_entries * darray->size_of;

    for( offset_i = 0; offset_i < max; offset_i += darray->size_of )
        if( ! darray->cmp((void*)&_t[offset_i],val))
            return offset_i / darray->size_of;

    return -1;
}

static int darray_unsorted_search_last(Darray *darray,void* val)
{
    BYTE* _t;
    int offset_i;

    TRACE("");

    if( darray == NULL || darray->array == NULL || darray->num_entries < 0
        || val == NULL || darray->cmp == NULL)
    {
        ERROR(  "darray == NULL || darray->array == NULL "
 				"|| darray->num_entries < 0 || val == NULL || darray->cmp == NULL\n",-1,"");
    }

    _t = darray->array;
    offset_i = darray->num_entries * darray->size_of - darray->size_of;

    for( offset_i = darray->num_entries * darray->size_of - darray->size_of; offset_i >= 0; offset_i -= darray->size_of)
        if( ! darray->cmp((void*)&_t[offset_i],val))
            return offset_i / darray->size_of;

    return -1;
}

static int darray_sorted_search_first(Darray *darray,void* val)
{
    BYTE* _t;

    int offset_left;
    int offset_right;

    TRACE("");

    if( darray == NULL || darray->array == NULL || darray->num_entries < 0
        || val == NULL || darray->cmp == NULL)
    {
        ERROR(  "darray == NULL || darray->array == NULL "
			    "|| darray->num_entries < 0 || val == NULL || darray->cmp == NULL\n",-1,"");
    }

    _t = (BYTE*)darray->array;

    offset_left = 0;
    offset_right = (darray->num_entries - 1) * darray->size_of;

    while( offset_left < offset_right)
    {
        int  offset_middle = ((offset_left / darray->size_of + offset_right / darray->size_of) >> 1) * darray->size_of;
        if ( darray->cmp((void*)&_t[offset_middle],val) == -1)
            offset_left = offset_middle + darray->size_of;
        else
            offset_right = offset_middle;
    }

    if( ! darray->cmp((void*)&_t[offset_left],val))
         return offset_left / darray->size_of;
     else
         return -1;
}

static int darray_sorted_search_last(Darray *darray,void* val)
{
    BYTE* _t;

    int offset_left;
    int offset_right;

    TRACE("");

    if( darray == NULL || darray->array == NULL || darray->num_entries < 0
        || val == NULL || darray->cmp == NULL )
    {
        ERROR(  "darray == NULL || darray->array == NULL "
 				"|| darray->num_entries < 0 || val == NULL || darray->cmp == NULL\n",-1, "");
    }

    _t = (BYTE*)darray->array;

    offset_left = 0;
    offset_right = (darray->num_entries - 1) * darray->size_of;

    while( offset_left < offset_right)
    {
        int  offset_middle = ((offset_left / darray->size_of + offset_right / darray->size_of + 1) >> 1) * darray->size_of;
        if ( darray->cmp((void*)&_t[offset_middle],val) == 1)
            offset_right = offset_middle - darray->size_of;
        else
            offset_left = offset_middle;
    }

    if( ! darray->cmp((void*)&_t[offset_left],val))
         return offset_left / darray->size_of;
     else
         return -1;
}

static int upper_bound(Darray *darray,void* val)
{
    int offset_left;
    int offset_right;
    int offset_middle;

    BYTE* _t;

    TRACE("");

    if( darray == NULL || darray->array == NULL || val == NULL || darray->cmp == NULL)
    {
        ERROR("darray == NULL || darray->array == NULL || val == NULL || darray->cmp == NULL\n",-1,"");
    }

    offset_left = - darray->size_of;
    offset_right = (darray->num_entries - 1) * darray->size_of;
    offset_middle = 0;

    _t = (BYTE*)darray->array;

    while( offset_left <= offset_right)
    {
        offset_middle = ((offset_left / darray->size_of + offset_right / darray->size_of) >> 1) * darray->size_of;

        if( darray->cmp((void*)&_t[offset_middle],val) == 1)
            offset_right = offset_middle - darray->size_of;
        else
        {
            if( offset_middle + darray->size_of > (darray->num_entries - 1) * darray->size_of ||
                darray->cmp((void*)&_t[offset_middle + darray->size_of],val) == 1)
                break;
            else
                offset_left = offset_middle + darray->size_of;
        }
    }

    return offset_middle / darray->size_of;
}

Darray_iterator *darray_iterator_create(Darray *darray,ITI_MODE mode)
{
    Darray_iterator *iterator;

    TRACE("");

    if( darray == NULL || ( mode != ITI_BEGIN && mode != ITI_END) )
    {
        ERROR("darray == NULL || ( mode != ITI_BEGIN && mode != ITI_END)\n", NULL, "");
    }

    iterator = (Darray_iterator*)malloc(sizeof(Darray_iterator));
    if(iterator == NULL)
    {
        ERROR("malloc error\n", NULL, "");
    }

    iterator->array = darray->array;
    iterator->array_length = darray->num_entries;

	if(mode == ITI_BEGIN)
    	iterator->index = 0;
	else
		iterator->index = darray->num_entries - 1;

    iterator->size_of = darray->size_of;

    return iterator;
}

void darray_iterator_destroy(Darray_iterator *iterator)
{
    TRACE("");

    if(iterator == NULL)
    {
        LOG("iterator == NULL\n","");

        return;
    }

    FREE(iterator);
}

int darray_iterator_init(Darray *darray,Darray_iterator *iterator,ITI_MODE mode)
{
    TRACE("");

    if( darray == NULL || iterator == NULL || ( mode != ITI_BEGIN && mode != ITI_END) )
    {
        ERROR("darray || iterator == NULL || ( mode != ITI_BEGIN && mode != ITI_END)\n",1,"");
    }

    iterator->array = darray->array;
    iterator->array_length = darray->num_entries;

   	if(mode == ITI_BEGIN)
    	iterator->index = 0;
	else
		iterator->index = darray->num_entries - 1;

    iterator->size_of = darray->size_of;

    return 0;
}

int darray_iterator_next(Darray_iterator *iterator)
{
    TRACE("");

    if( iterator == NULL )
    {
        ERROR("iterator == NULL\n",1,"");
    }

    ++iterator->index;

    return 0;
}

int darray_iterator_prev(Darray_iterator *iterator)
{
    TRACE("");

    if( iterator == NULL )
    {
        ERROR("iterator == NULL\n",1,"");
    }

    --iterator->index;

    return 0;
}

int darray_iterator_get_data(Darray_iterator *iterator,void *val)
{
    BYTE* t;

    TRACE("");

    if( iterator == NULL || val == NULL)
    {
        ERROR("iterator || val == NULL\n",1,"");
    }

    t = (BYTE*)iterator->array;
    __ASSIGN__(*(BYTE*)val,t[iterator->index * iterator->size_of],iterator->size_of);

    return 0;
}

BOOL darray_iterator_end(Darray_iterator *iterator)
{
    TRACE("");

    if( iterator == NULL)
    {
        ERROR("iterator == NULL\n",TRUE,"");
    }

    return (iterator->index >= iterator->array_length || iterator->index < 0);
}

Darray *darray_create(ARRAY_TYPE type,int size,int size_of,int (*cmp)(void *a,void*b))
{
    Darray *da;

    TRACE("");

    if( type != SORTED && type != UNSORTED)
    {
        ERROR("type is invalid\n",NULL,"");
    }

    if( type == SORTED && cmp == NULL)
    {
        ERROR("cmp is needed for sorted array\n", NULL, "");
    }

    if(size < 0 || size_of < 0)
    {
        ERROR("malloc error on da\n",NULL, "");
    }

    da = (Darray*)malloc(sizeof(Darray));
    if( da == NULL)
    {
        ERROR("malloc error on da\n",NULL,"");
    }

    if( ! size )
        da->size = INIT_SIZE;
    else
        da->size = size;

    da->init_size = da->size;
    da->size_of = size_of;
    da->type = type;
    da->cmp = cmp;

    da->array = malloc(da->size * da->size_of);

    if(da->array == NULL)
    {
        ERROR("malloc error on da->array\n",NULL,"");
    }

    da->num_entries = 0;

    return da;
}


void darray_destroy(Darray *darray)
{
    TRACE("");

    if(darray == NULL)
    {
        LOG("darray == NULL\n","");

        return;
    }

    FREE((darray)->array);
    FREE(darray);
}

int darray_insert(Darray *darray,void* entry)
{
    int status;
    int pos;

    TRACE("");

    if( darray == NULL || darray->array == NULL || entry == NULL)
    {
        ERROR("darray || array || entry  == NULL\n",1,"");
    }

    if(darray->type == UNSORTED)
        status = darray_insert_pos(darray,entry,darray->num_entries);
    else
    {
        if( darray->cmp == NULL)
        {
            ERROR("CMP is needed\n",1,"");
        }

        if( ! darray->num_entries )
            pos = 0;
        else
            pos = upper_bound(darray,entry) + 1;

        status = darray_insert_pos(darray,entry,pos);
    }

        if( status )
        {
            ERROR("darray_insert_pos error\n",1,"");
        }

    return 0;

}

int darray_insert_pos(Darray *darray,void* entry,int pos)
{
    BYTE *_t;
    BYTE *_entry;

    TRACE("");

    if( darray == NULL || darray->array == NULL || pos < 0 || pos > darray->num_entries )
    {
        ERROR("darray == NULL || darray->array == NULL || pos < 0 || pos > darray->num_entries\n",1,"");
    }

    /* resize array */
    if( darray->num_entries + 1 >= darray->size )
    {
        darray->size <<= 1;

        darray->array = realloc(darray->array,darray->size * (darray->size_of));
        if(darray->array == NULL)
        {
            ERROR("realloc error on darray->array\n",1,"");
        }

    }

    _t = (BYTE*)darray->array;

    /* make empty slot in pos and insert an entry */
    if( (memmove(   (void*)(_t + ((pos + 1) * darray->size_of)),
                    (void*)(_t + (pos * darray->size_of)),
                    (darray->num_entries - pos) * darray->size_of )) == NULL )
    {
        ERROR("memmove error\n",1,"");
    }

    _entry = (BYTE*)entry;

    __ASSIGN__(_t[pos * darray->size_of],*_entry,darray->size_of);

    ++darray->num_entries;

    return 0;
}

int darray_delete(Darray *darray)
{
     TRACE("");

    if(darray == NULL || darray->array == NULL)
    {
        ERROR("darray || darray->array == NULL\n",1,"");
    }

    if( darray_delete_pos(darray,darray->num_entries) )
    {
        ERROR("darray_delete_pos error\n",1,"");
    }
    return 0;
}

int darray_delete_pos(Darray *darray,int pos)
{
    BYTE *_t;

    TRACE("");

    if(darray == NULL || darray->array == NULL || pos < 0 || pos > darray->num_entries)
    {
        ERROR("darray == NULL || darray->array == NULL || pos < 0 || pos > darray->num_entries\n",1,"");
    }

	if( ! darray->num_entries )
	{
		ERROR("Nothing to delete\n",1,"");
	}

    _t = (BYTE*)darray->array;

    if( (memmove( _t + (pos * darray->size_of),
                  _t + ((pos + 1) * darray->size_of),
                  (darray->num_entries - pos) * darray->size_of )) == NULL )
    {
        ERROR("memmove error\n",1,"");
    }

    --darray->num_entries;

    /* resize array */
    if( darray->num_entries <= (darray->size * 0.4) && ((darray->size >> 1) >= darray->init_size))
    {
        darray->size >>= 1;

        darray->array = realloc(darray->array,darray->size * (darray->size_of));
        if(darray->array == NULL)
        {
            ERROR("realloc error on darray->array\n",1,"");
        }
    }
    return 0;
}

int darray_search_first(Darray *darray,void *val)
{
     TRACE("");

     if(darray == NULL || darray->array == NULL || darray->num_entries <= 0 || val == NULL )
     {
         ERROR("darray == NULL || darray->array == NULL || darray->num_entries <= 0 || val == NULL\n",-1,"");
     }

     if(darray->type == UNSORTED)
        return darray_unsorted_search_first(darray,val);
     else
     {
         if( darray->cmp == NULL)
         {
             ERROR("CMP is needed\n",-1,"");
         }

         return darray_sorted_search_first(darray,val);
     }
}

int darray_search_last(Darray *darray,void *val)
{
    TRACE("");

    if(darray == NULL || darray->array == NULL || darray->num_entries <= 0 || val == NULL )
    {
        ERROR("darray == NULL || darray->array == NULL || darray->num_entries <= 0 || val == NULL\n",-1,"");
    }

    if(darray->type == UNSORTED)
        return darray_unsorted_search_last(darray,val);
    else
    {
        if( darray->cmp == NULL)
        {
            ERROR("CMP is needed\n",-1,"");
        }

       return darray_sorted_search_last(darray,val);
    }
}

int darray_sort(Darray *darray)
{
    TRACE("");

    if(darray == NULL || darray->array == NULL || darray->cmp == NULL)
    {
        ERROR("darray == NULL || darray->array == NULL || darray->cmp == NULL\n",1,"");
    }

    if( darray->type == SORTED )
        return 0;

    return sort(darray->array,darray->num_entries,darray->cmp,darray->size_of);
}

int darray_min(Darray *darray)
{
    int pos;

    BYTE min[MAXWORD];
    BYTE *_t;

    int offset_i;
    int offset_max;

    TRACE("");

    if( darray == NULL || darray->array == NULL || darray->cmp == NULL)
    {
        ERROR("DARRAY_MIN\ndarray == NULL || darray->array == NULL || darray->cmp == NULL\n",-1,"");
    }

    pos = 0;
    _t = (BYTE*)darray->array;

    __ASSIGN__(min[0],_t[0],darray->size_of);

    offset_max = darray->num_entries * darray->size_of;

    for(offset_i = darray->size_of; offset_i < offset_max; offset_i += darray->size_of)
        if( darray->cmp((void*)min,(void*)&_t[offset_i]) == 1)
        {
            __ASSIGN__(min[0],_t[offset_i],darray->size_of)
            pos = offset_i;
        }

    return pos / darray->size_of;
}

int darray_max(Darray *darray)
{
    int pos;

    BYTE max[MAXWORD];
    BYTE *_t;

    int offset_i;
    int offset_max;

     TRACE("");

    if( darray == NULL || darray->array == NULL || darray->cmp == NULL)
    {
        ERROR("DARRAY_MIN\ndarray == NULL || darray->array == NULL || darray->cmp == NULL\n",-1,"");
    }

    pos = 0;
    _t = (BYTE*)darray->array;

    __ASSIGN__(max[0],_t[0],darray->size_of);

    offset_max = darray->num_entries * darray->size_of;

    for(offset_i = darray->size_of; offset_i < offset_max; offset_i += darray->size_of)
        if( darray->cmp((void*)max,(void*)&_t[offset_i]) == -1)
        {
            __ASSIGN__(max[0],_t[offset_i],darray->size_of)
            pos = offset_i;
        }

    return pos / darray->size_of;
}
