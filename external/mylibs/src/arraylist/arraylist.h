#ifndef ARRAYLIST_H
#define ARRAYLIST_H


/*
    ArrayList [ Unsorted List ]

    Author: Michal Kukowski
    email: michalkukowski10@gmail.com

    LICENCE: GPL 2.0+
*/

#ifndef ITI_MODE
    #define ITI_MODE    unsigned char
#endif

#define ITI_BEGIN   0
#define ITI_END     1

#ifndef BOOL
    #define BOOL    char
    #define TRUE    1
    #define FALSE   0
#endif

typedef struct Arraylist_node
{
    struct Arraylist_node *next;
    struct Arraylist_node *prev;
    void *data;

}Arraylist_node;


/*
   ArrayList_iterator
*/
typedef struct Arraylist_iterator
{
    Arraylist_node *node;
    int size_of;

}Arraylist_iterator;

typedef struct Arraylist
{
    int size_of; /* size of element */
    unsigned long long length; /* length of list */

    Arraylist_node *head;
    Arraylist_node *tail;

}Arraylist;

/*
    Init iterator

    PARAMS
    @IN alist - pointer to arraylist
    @IN mode - iterator mode

    RETURN:
    NULL if failure
    Pointer to new iterator if success
*/
Arraylist_iterator *arraylist_iterator_create(Arraylist *alist, ITI_MODE mode);

/*
    Init iterator

    PARAMS
    @IN alist - pointer to arraylist
    @IN iterator - pointer to iterator
    @IN mode - iterator mode

    RETURN:
    0 if success
	Non-zero value if failure
*/
int arraylist_iterator_init(Arraylist *alist,Arraylist_iterator *iterator, ITI_MODE mode);

/*
    Deallocate memory

    PARAMS
    @iterator - pointer to iterator

    RETURN:
    This is void function
*/
void arraylist_iterator_destroy(Arraylist_iterator *iterator);

/*
    Go to the next value
    PARAMS
    @IN iterator - pointer iterator

    RETURN:
   	0 if success
	Non-zero value if failure
*/
int arraylist_iterator_next(Arraylist_iterator *iterator);

/*
    Go to the prev value
    PARAMS
    @IN iterator - pointer iterator

    RETURN:
   	0 if success
	Non-zero value if failure
*/
int arraylist_iterator_prev(Arraylist_iterator *iterator);

/*
    list data getter using iterator

    PARAMS
    @IN - pointer iterator
    @OUT val - pointer to value

    RETURN:
    0 if success
	Non-zero value if failure
*/
int arraylist_iterator_get_data(Arraylist_iterator *iterator,void *val);

/*
    Check the end of alist

    PARAMS
    @IN iterator - pointer to iterator

    RETURN:
    FALSE if not end
    TRUE if end
*/
BOOL arraylist_iterator_end(Arraylist_iterator *iterator);

/*
    Macro for create a alist, please see function description

    PARAMS
	@OUT PTR - pointer to alist
    @IN TYPE - type of element of list
*/
#define LIST_CREATE(PTR,TYPE) do{ PTR = arraylist_create(sizeof(TYPE)); }while(0)


/*
    Create alist

    PARAMS
    @IN size_of - size of element in list

    RETURN:
    NULL if failure
    Pointer if success
*/
Arraylist *arraylist_create(int size_of);

/*
    Destroy alist

    PARAMS
    @IN alist - pointer to list

	RETURN:
	This is a void function
*/
void arraylist_destroy(Arraylist *alist);


/*
    Insert Data at the begining of the alist

    PARAMS
    @IN alist - pointer to alist
    @IN data - addr of data

    RETURN:
    0 iff success
    Non-zero value of failure
*/
int arraylist_insert_first(Arraylist *alist, void *data);

/*
    Insert Data at the end of the alist

    PARAMS
    @IN alist - pointer to alist
    @IN data - addr of data

    RETURN:
    0 iff success
    Non-zero value of failure
*/
int arraylist_insert_last(Arraylist *alist, void *data);

/*
    Insert Data at @pos

    PARAMS
    @IN alist - pointer to alist
    @IN pos - posision where we insert data
    @IN data - addr of data

    RETURN:
    0 iff success
    Non-zero value of failure
*/
int arraylist_insert_pos(Arraylist *alist, int pos, void *data);

/*
    Delete first data od alist

    PARAMS
    @IN alist - pointer to alist

    RETURN:
    0 iff success
    Non-zero value of failure
*/
int arraylist_delete_first(Arraylist *alist);

/*
    Delete last data od alist

    PARAMS
    @IN alist - pointer to alist

    RETURN:
    0 iff success
    Non-zero value of failure
*/
int arraylist_delete_last(Arraylist *alist);


/*
    Delete data on @pos

    PARAMS
    @IN alist - pointer to alist
    @IN pos - pos from we delete data

    RETURN:
    0 iff success
    Non-zero value of failure
*/
int arraylist_delete_pos(Arraylist *alist, int pos);


/*
    Get data from node at @pos

    PARAMS
    @IN alist - pointer to alist
    @IN pos - posision
    @OUT data - addr of data

    RETURN:
    0 iff success
    Non-zero value of failure
*/
int arraylist_get_pos(Arraylist *alist, int pos, void *data);

/*
    Allocate new alist and merge alist1 & alist2 to the new alist

    PARAMS
    @IN alist1 - pointer to first list
    @IN alist2 - pointer to second list

    RETURN:
    NULL if failure
    Pointer to alist if success
*/
Arraylist *arraylist_merge(Arraylist *alist1, Arraylist *alist2);

/*
    Create array from alist

    PARAMS
    @IN alist - pointer to alist
    @OUT array - pointer to array
    @OUT size - size of  returned array

    RETURN:
    0 if success
	Non-zero value if failure
*/
int arraylist_to_array(Arraylist *alist,void *array,int *size);


#endif
