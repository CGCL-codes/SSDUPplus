/*
 *  avltree.c:  from http://purists.org/avltree/
 */

#include <stdlib.h>
#include "my_avltree.h"
#include "gossip.h"

/*
 *  avlrotleft: perform counterclockwise rotation
 *
 *  robl: also update pointers to parent nodes
 *
 *  Parameters:
 *
 *    n           Address of a pointer to a node
 */
static void
mavlrotleft(struct mavlnode **n)
{
	struct mavlnode *tmp = *n;

	*n = (*n)->right;
	tmp->right = (*n)->left;
	(*n)->left = tmp;
}

/*
 *  avlrotright: perform clockwise rotation
 *
 *  Parameters:
 *
 *    n           Address of a pointer to a node
 */
static void
mavlrotright(struct mavlnode **n)
{
	struct mavlnode *tmp = *n;	 

	*n = (*n)->left;		
	tmp->left = (*n)->right; 
	(*n)->right = tmp;	
}

/*
 *  avlleftgrown: helper function for avlinsert
 *
 *  Parameters:
 *
 *    n           Address of a pointer to a node. This node's left 
 *                subtree has just grown due to item insertion; its 
 *                "skew" flag needs adjustment, and the local tree 
 *                (the subtree of which this node is the root node) may 
 *                have become unbalanced.
 *
 *  Return values:
 *
 *    OK          The local tree could be rebalanced or was balanced 
 *                from the start. The parent activations of the avlinsert 
 *                activation that called this function may assume the 
 *                entire tree is valid.
 *
 *    BALANCE     The local tree was balanced, but has grown in height.
 *                Do not assume the entire tree is valid.
 */
static enum MAVLRES
mavlleftgrown(struct mavlnode **n)
{
	switch ((*n)->skew) {
	case LEFT:
		if ((*n)->left->skew == LEFT) {
			(*n)->skew = (*n)->left->skew = NONE;
			mavlrotright(n);
		}	
		else {
			switch ((*n)->left->right->skew) {
			case LEFT:
				(*n)->skew = RIGHT;
				(*n)->left->skew = NONE;
				break;

			case RIGHT:
				(*n)->skew = NONE;
				(*n)->left->skew = LEFT;
				break;

			default:
				(*n)->skew = NONE;
				(*n)->left->skew = NONE;
			}
			(*n)->left->right->skew = NONE;
			mavlrotleft(& (*n)->left);
			mavlrotright(n);
		}
		return OK;

	case RIGHT:
		(*n)->skew = NONE;
		return OK;
	
	default:
		(*n)->skew = LEFT;
		return BALANCE;
	}
}

/*
 *  avlrightgrown: helper function for avlinsert
 *
 *  See avlleftgrown for details.
 */
static enum MAVLRES
mavlrightgrown(struct mavlnode **n)
{
	switch ((*n)->skew) {
	case LEFT:					
		(*n)->skew = NONE;
		return OK;

	case RIGHT:
		if ((*n)->right->skew == RIGHT) {	
			(*n)->skew = (*n)->right->skew = NONE;
			mavlrotleft(n);
		}
		else {
			switch ((*n)->right->left->skew) {
			case RIGHT:
				(*n)->skew = LEFT;
				(*n)->right->skew = NONE;
				break;

			case LEFT:
				(*n)->skew = NONE;
				(*n)->right->skew = RIGHT;
				break;

			default:
				(*n)->skew = NONE;
				(*n)->right->skew = NONE;
			}
			(*n)->right->left->skew = NONE;
			mavlrotright(& (*n)->right);
			mavlrotleft(n);
		}
		return OK;

	default:
		(*n)->skew = RIGHT;
		return BALANCE;
	}
}

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
mavlinsert(struct mavlnode **n, Offset_pair_t d)
{
	enum MAVLRES tmp;

	if (!(*n)) {
		if (!((*n) = malloc(sizeof(struct mavlnode)))) {
			gossip_err("point 1\n");
			return ERROR;
		}
		(*n)->left = (*n)->right = NULL;
		(*n)->d = d;
		(*n)->skew = NONE;
		//gossip_err("point 2\n");
		return BALANCE;
	}
	
	if (MY_AVLKEY(d) < MY_AVLKEY((*n)->d)) {
		if ((tmp = mavlinsert(& (*n)->left, d)) == BALANCE) {
			//gossip_err("point 3\n");
			return mavlleftgrown(n);
		}
		//gossip_err("point 4\n");
		return tmp;
	}
	if (MY_AVLKEY(d) > MY_AVLKEY((*n)->d)) {
		if ((tmp = mavlinsert(& (*n)->right,  d)) == BALANCE) {
			//gossip_err("point 5\n");
			return mavlrightgrown(n);
		}
		//gossip_err("point 6\n");
		return tmp;
	}
	//gossip_err("point 7\n");
	return ERROR;
}

/*
 *  avlleftshrunk: helper function for avlremove and avlfindlowest
 *
 *  Parameters:
 *
 *    n           Address of a pointer to a node. The node's left
 *                subtree has just shrunk due to item removal; its
 *                "skew" flag needs adjustment, and the local tree
 *                (the subtree of which this node is the root node) may
 *                have become unbalanced.
 *
 *   Return values:
 *
 *    OK          The parent activation of the avlremove activation
 *                that called this function may assume the entire
 *                tree is valid.
 *
 *    BALANCE     Do not assume the entire tree is valid.
 */                
static enum MAVLRES
mavlleftshrunk(struct mavlnode **n)
{
	switch ((*n)->skew) {
	case LEFT:
		(*n)->skew = NONE;
		return BALANCE;

	case RIGHT:
		if ((*n)->right->skew == RIGHT) {
			(*n)->skew = (*n)->right->skew = NONE;
			mavlrotleft(n);
			return BALANCE;
		}
		else if ((*n)->right->skew == NONE) {
			(*n)->skew = RIGHT;
			(*n)->right->skew = LEFT;
			mavlrotleft(n);
			return OK;
		}
		else {
			switch ((*n)->right->left->skew) {
			case LEFT:
				(*n)->skew = NONE;
				(*n)->right->skew = RIGHT;
				break;

			case RIGHT:
				(*n)->skew = LEFT;
				(*n)->right->skew = NONE;
				break;

			default:
				(*n)->skew = NONE;
				(*n)->right->skew = NONE;
			}
			(*n)->right->left->skew = NONE;
			mavlrotright(& (*n)->right);
			mavlrotleft(n);
			return BALANCE;
		}

	default:
		(*n)->skew = RIGHT;
		return OK;
	}
}

/*
 *  avlrightshrunk: helper function for avlremove and avlfindhighest
 *
 *  See avlleftshrunk for details.
 */
static enum MAVLRES
mavlrightshrunk(struct mavlnode **n)
{
	switch ((*n)->skew) {
	case RIGHT:
		(*n)->skew = NONE;
		return BALANCE;

	case LEFT:
		if ((*n)->left->skew == LEFT) {
			(*n)->skew = (*n)->left->skew = NONE;
			mavlrotright(n);
			return BALANCE;
		}
		else if ((*n)->left->skew == NONE) {
			(*n)->skew = LEFT;
			(*n)->left->skew = RIGHT;
			mavlrotright(n);
			return OK;
		}
		else {
			switch ((*n)->left->right->skew) {
			case LEFT:
				(*n)->skew = RIGHT;
				(*n)->left->skew = NONE;
				break;

			case RIGHT:
				(*n)->skew = NONE;
				(*n)->left->skew = LEFT;	
				break;
			
			default:
				(*n)->skew = NONE;
				(*n)->left->skew = NONE;
			}
			(*n)->left->right->skew = NONE;
			mavlrotleft(& (*n)->left);
			mavlrotright(n);
			return BALANCE;
		}

	default:
		(*n)->skew = LEFT;
		return OK;
	}
}

/*
 *  avlfindhighest: replace a node with a subtree's highest-ranking item.
 *
 *  Parameters:
 *
 *    target      Pointer to node to be replaced.
 *
 *    n           Address of pointer to subtree.
 *
 *    res         Pointer to variable used to tell the caller whether
 *                further checks are necessary; analog to the return
 *                values of avlleftgrown and avlleftshrunk (see there). 
 *
 *  Return values:
 *
 *    1           A node was found; the target node has been replaced.
 *
 *    0           The target node could not be replaced because
 *                the subtree provided was empty.
 *
 */
static int
mavlfindhighest(struct mavlnode *target, struct mavlnode **n, enum MAVLRES *res)
{
	struct mavlnode *tmp;

	*res = BALANCE;
	if (!(*n)) {
		return 0;
	}
	if ((*n)->right) {
		if (!mavlfindhighest(target, &(*n)->right, res)) {
			return 0;
		}
		if (*res == BALANCE) {
			*res = mavlrightshrunk(n);
		}
		return 1;
	}
	free(target->d);
	target->d  = (*n)->d;
	tmp = *n;
	*n = (*n)->left;
	free(tmp);
	return 1;
}

/*
 *  avlfindlowest: replace node with a subtree's lowest-ranking item.
 *
 *  See avlfindhighest for the details.
 */
static int
mavlfindlowest(struct mavlnode *target, struct mavlnode **n, enum MAVLRES *res)
{
	struct mavlnode *tmp;

	*res = BALANCE;
	if (!(*n)) {
		return 0;
	}
	if ((*n)->left) {
		if (!mavlfindlowest(target, &(*n)->left, res)) {
			return 0;
		}
		if (*res == BALANCE) {
			*res =  mavlleftshrunk(n);
		}
		return 1;
	}
	free(target->d);
	target->d = (*n)->d;
	tmp = *n;
	*n = (*n)->right;
	free(tmp);
	return 1;
}

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
enum MAVLRES
mavlremove(struct mavlnode **n, MY_AVLKEY_TYPE key)
{
	enum MAVLRES tmp = BALANCE;

	if (!(*n)) {
		return ERROR;
	}
	if (key < MY_AVLKEY((*n)->d)) {
		if ((tmp = mavlremove(& (*n)->left, key)) == BALANCE) {
			return mavlleftshrunk(n);
		}
		return tmp;
	}
	if (key > MY_AVLKEY((*n)->d)) {
		if ((tmp = mavlremove(& (*n)->right, key)) == BALANCE) {
			return mavlrightshrunk(n);
		}
		return tmp;
	}
	if ((*n)->left) {
		if (mavlfindhighest(*n, &((*n)->left), &tmp)) {
			if (tmp == BALANCE) {
				tmp = mavlleftshrunk(n);
			}
			return tmp;
		}
	}
	if ((*n)->right) {
		if (mavlfindlowest(*n, &((*n)->right), &tmp)) {
			if (tmp == BALANCE) {
				tmp = mavlrightshrunk(n);
			}
			return tmp;
		}
	}
	free((*n)->d);
	(*n)->d = NULL;
	free(*n);
	*n = NULL;
	return BALANCE;
}

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
mavlaccess(struct mavlnode *n, MY_AVLKEY_TYPE key)
{
        if (!n) {
                return NULL;
        }
        if (key < MY_AVLKEY((n)->d)) {
                return mavlaccess(n->left, key);
        }
        if (key > MY_AVLKEY((n)->d)) {
                return mavlaccess(n->right, key);
        }
        return &(n->d);
}

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
mavlgethighest(struct mavlnode *n)
{
	if (!n) {
		return NULL;
	}
	if (n->right) {
		return mavlgethighest(n->right);
	}
	return &(n->d);
}

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
mavldepthfirst(struct mavlnode *n, MAVLWORKER *f, int param, int depth)
{
	if (!n) return;
	mavldepthfirst(n->left, f, param, depth + 1);
	(*f)(n, param, depth);
	mavldepthfirst(n->right, f, param, depth +1);
}

/*
 *  avlpostorder: post-order tree traversal.
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
mavlpostorder(struct mavlnode *n, MAVLWORKER *f, int param, int depth)
{
	if (!n) return;
	mavlpostorder(n->left, f, param, depth + 1);
	mavlpostorder(n->right, f, param, depth +1);
	(*f)(n, param, depth);
}

void destroy_avl(struct mavlnode **n)
{
   if((*n)->left !=NULL)
   	destroy_avl(&(*n)->left);
   if((*n)->right !=NULL)
   	destroy_avl(&(*n)->right);
   //gossip_err("o_offset:%lld\n",(*n)->d->original_offset);
   free((*n)->d);
   (*n)->d = NULL;
   free(*n);
}

