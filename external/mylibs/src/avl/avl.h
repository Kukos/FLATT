#ifndef AVL_H
#define AVL_H

/*
    AVL tree implementation without recursive function, but with parent pointer

    Author: Michal Kukowski
    email: michalkukowski10@gmail.com

    LICENCE: GPL2.0+
*/

#ifndef ITI_MODE
    #define ITI_MODE unsigned char
#endif

#define ITI_BEGIN   0
#define ITI_END     1
#define ITI_ROOT    2

#ifndef BOOL
    #define BOOL    char
    #define TRUE    1
    #define FALSE   0
#endif

#ifndef INT_8
    #define INT_8 char
#endif

typedef struct Avl_node
{
    void *data;
    struct Avl_node *left_son;
    struct Avl_node *right_son;
    struct Avl_node *parent;

    INT_8 bf; /* balanced factor */

}Avl_node;

typedef struct Avl_iterator
{
    int size_of;
    Avl_node *node;
}Avl_iterator;

typedef struct Avl
{
    Avl_node *root;

    unsigned long long nodes;

    int size_of;

    int (*cmp)(void* a,void *b);
}Avl;

/*
    Create AVL

    PARAMS
    @IN size_of - size_of data in tree
    @IN cmp - cmp function

    RETURN:
    NULL if failure
    Pointer to bst if success
*/
Avl* avl_create(int size_of,int (*cmp)(void* a,void *b));

#define AVL_CREATE(PTR,TYPE,CMP) do{ PTR = avl_create(sizeof(TYPE),CMP); }while(0)

/*
    Destroy all nodes in tree

    PARAMS
    @IN tree - pointer to AVL

    RETURN:
    This is a void function
*/
void avl_destroy(Avl *tree);

/*
    Insert data to AVL IFF data with key ( using cmp ) is not in tree

    PARAMS
    @IN tree - pointer to tree
    @IN data - addr of data

    RETURN:
    0 if success
    Positive value if failure
*/
int avl_insert(Avl *tree,void *data);

/*
    Getter of min value ( using cmp ) in tree

    PARAMS
    @IN tree - pointer to tree
    @OUT data - returned data by addr

    RETURN:
    0 if success
    Positive value if failure
*/
int avl_min(Avl *tree,void *data);

/*
    Getter of max value ( using cmp ) in tree

    PARAMS
    @IN tree - pointer to tree
    @OUT data - returned data by addr

    RETURN:
    0 if success
    Positive value if failure
*/
int avl_max(Avl *tree,void *data);

/*
    Search for data with key equals key @data_key ( using cmp )

    PARAMS
    @IN tree - pointer to tree
    @IN data_key - addr of data with search key
    @OUT data_out - returned data by addr

    RETURN:
    0 if success
    Positive value if failure
*/
 int avl_search(Avl *tree,void *data_key,void *data_out);

/*
	Check existing of key in tree

 	PARAMS
    @IN tree - pointer to tree
    @IN data_key - addr of data with search key

	RETURN:
	FALSE iff key doesn't exist in tree
	TRUE iff key exists in tree
*/
BOOL avl_key_exist(Avl *tree, void *data_key);

/*
    Delete data with key equals @data_key ( using cmp )

    PARAMS
    @IN tree - pointer to tree
    @IN data_key = addr of data with key to delete

    RETURN:
    0 if success
    Positive value if failure
*/
int avl_delete(Avl *tree,void *data_key);

/*
    Convert bst to sorted array

    PARAMS
    @IN tree - pointer to tree
    @OUT addr of array
    @IN / OUT - size - size of array

    RETURN:
    0 if success
    Positive value if failure
*/
int avl_to_array(Avl *tree,void *array,int *size);

/*
    Init iterator

    PARAMS
    @IN tree - pointer to AVL
    @IN mode - iterator init mode

    RETURN:
    NULL if failure
    Pointer to new iterator if success
*/
Avl_iterator *avl_iterator_create(Avl *tree,ITI_MODE mode);

/*
    Init iterator

    PARAMS
    @IN tree - pointer to AVL
    @IN iterator - pointer to iterator
    @IN mode - iterator init mode

    RETURN:
    0 if failure
    Positive value if success
*/
int avl_iterator_init(Avl *tree,Avl_iterator *iterator,ITI_MODE mode);

/*
    Deallocate memory

    PARAMS
    @iterator - pointer to iterator

    RETURN:
    This is void function
*/
void avl_iterator_destroy(Avl_iterator *iterator);

/*
    Go to the next value

    PARAMS
    @IN iterator - pointer iterator

    RETURN:
    0 if failure
    Positive value if success
*/
int avl_iterator_next(Avl_iterator *iterator);

/*
    Go to the prev value

    PARAMS
    @IN iterator - pointer iterator

    RETURN:
    0 if failure
    Positive value if success
*/
int avl_iterator_prev(Avl_iterator *iterator);

/*
    data getter using iterator

    PARAMS
    @IN - pointer iterator
    @OUT val - pointer to value

    RETURN:
    0 if failure
    Positive value if success
*/
int avl_iterator_get_data(Avl_iterator *iterator,void *val);

/*
    Check the end of AVL

    PARAMS
    @IN iterator - pointer to iterator

    RETURN:
    FALSE if not end
    TRUE if end
*/
BOOL avl_iterator_end(Avl_iterator *iterator);

#endif
