/*
 *  avltree.h : from http://purists.org/avltree/
 */

#ifndef _MY_AVLTREE_H
#define _MY_AVLTREE_H
#include "trove-types.h"


#define MY_AVLKEY_TYPE TROVE_offset

//???
#define MY_AVLKEY(p) p->original_offset

/*
 *  Which of a given node's subtrees is higher?
 */
enum MAVLSKEW 
{
    NONE,	
    LEFT,
    RIGHT
};

/*
 *  Did a given insertion/deletion succeed, and what do we do next?
 */
enum MAVLRES
{
    ERROR = 0,
    OK,
    BALANCE,
};

struct Offset_pair
{
	TROVE_offset original_offset;
	TROVE_offset new_offset;
	TROVE_size access_size;
 	char cache[10];
};

typedef struct Offset_pair *  Offset_pair_t;

/*
 *  AVL tree node structure
 */
struct mavlnode
{
    struct mavlnode *left, *right;
    Offset_pair_t d;
    enum MAVLSKEW skew;
};

/*
 *  avlinsert: insert a node into the AVL tree.
 *
 *  Parameters:
 *
 *    n           Address of a pointer to a node.
 *
 *    d           Item to be inserted.
 *
 *  Return values:
 *
 *    nonzero     The item has been inserted. The excact value of 
 *                nonzero yields is of no concern to user code; when
 *                avlinsert recursively calls itself, the number 
 *                returned tells the parent activation if the AVL tree 
 *                may have become unbalanced; specifically:
 *
 *      OK        None of the subtrees of the node that n points to 
 *                has grown, the AVL tree is valid.
 *
 *      BALANCE   One of the subtrees of the node that n points to 
 *                has grown, the node's "skew" flag needs adjustment,
 *                and the AVL tree may have become unbalanced.
 *
 *    zero        The datum provided could not be inserted, either due 
 *                to AVLKEY collision (the tree already contains another
 *                item with which the same AVLKEY is associated), or
 *                due to insufficient memory.
 */   
enum MAVLRES
mavlinsert(struct mavlnode **n, Offset_pair_t d);

/*
 *  avlremove: remove an item from the tree.
 *
 *  Parameters:
 *
 *    n           Address of a pointer to a node.
 *
 *    key         AVLKEY of item to be removed.
 *
 *  Return values:
 *
 *    nonzero     The item has been removed. The exact value of 
 *                nonzero yields if of no concern to user code; when
 *                avlremove recursively calls itself, the number
 *                returned tells the parent activation if the AVL tree
 *                may have become unbalanced; specifically:
 *
 *      OK        None of the subtrees of the node that n points to
 *                has shrunk, the AVL tree is valid.
 *
 *      BALANCE   One of the subtrees of the node that n points to
 *                has shrunk, the node's "skew" flag needs adjustment,
 *                and the AVL tree may have become unbalanced.
 *
 *   zero         The tree does not contain an item yielding the
 *                AVLKEY value provided by the caller.
 */
 //???
enum MAVLRES
mavlremove(struct mavlnode **n, MY_AVLKEY_TYPE key);

/*
 *  avlaccess: retrieve the datum corresponding to a given AVLKEY.
 *
 *  Parameters:
 *
 *    n           Pointer to the root node.
 *
 *    key         TKEY of item to be accessed.
 *
 *  Return values:
 *
 *    non-NULL    An item yielding the AVLKEY provided has been found,
 *                the return value points to the AVLKEY attached to it.
 *
 *    NULL        The item could not be found.
 */
Offset_pair_t *
mavlaccess(struct mavlnode *n, MY_AVLKEY_TYPE key);


/*
 * avlgethighest: retrieve the datum from the highest weighted node
 *
 * Parameters:
 *
 *   n		pointer to the root node
 *
 * Return values:
 *
 *   non-NULL	the return value points to the AVLKEY attached to the node
 *
 *   NULL	tree has no nodes
 */
Offset_pair_t *
mavlgethighest(struct mavlnode *n);

/*
 *  Function to be called by the tree traversal functions.
 *
 *  Parameters:
 *
 *    n           Pointer to a node.
 *
 *    param       Value provided by the traversal function's caller.
 *
 *    depth       Recursion depth indicator. Allows the function to
 *                determine how many levels the node bein processed is
 *                below the root node. Can be used, for example,
 *                for selecting the proper indentation width when
 *                avldepthfirst is used to print a tree dump to 
 *                the screen.
 */
typedef void MAVLWORKER(struct mavlnode *n, int param, int depth);

/*
 *  avldepthfirst: depth-first tree traversal.
 *
 *  Parameters:
 *
 *    n          Pointer to the root node.
 *
 *    f          Worker function to be called for every node.
 *
 *    param      Additional parameter to be passed to the
 *               worker function
 *
 *    depth      Recursion depth indicator. Allows the worker function
 *               to determine how many levels the node being processed
 *               is below the root node. Can be used, for example,
 *               for selecting the proper indentation width when
 *               avldepthfirst ist used to print a tree dump to
 *               the screen.
 *
 *               Most of the time, you will want to call avldepthfirst
 *               with a "depth" value of zero.
 */
void
mavldepthfirst(struct mavlnode *n, MAVLWORKER *f, int param, int depth);

/*
 *  avlpreorder: depth-first tree traversal.
 *
 *  Parameters:
 *
 *    n          Pointer to the root node.
 *
 *    f          Worker function to be called for every node.
 *
 *    param      Additional parameter to be passed to the
 *               worker function
 *
 *    depth      Recursion depth indicator. Allows the worker function
 *               to determine how many levels the node being processed
 *               is below the root node. Can be used, for example,
 *               for selecting the proper indentation width when
 *               avldepthfirst ist used to print a tree dump to
 *               the screen.
 *
 *               Most of the time, you will want to call avlpostorder
 *               with a "depth" value of zero.
 */
void
mavlpostorder(struct mavlnode *n, MAVLWORKER *f, int param, int depth);

void destroy_avl(struct mavlnode **n);
#endif


