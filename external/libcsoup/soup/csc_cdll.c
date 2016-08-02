/*!\file       csc_cdll.c
   \brief      The management function of circular doubly linked list.

   This file supplies a group of functions to process the circular doubly 
   linked list.

   \author     "Andy Xuming" <xuming@users.sourceforge.net>
   \date       2013-2014
*/
/* Copyright (C) 1998-2014  "Andy Xuming" <xuming@users.sourceforge.net>

   This file is part of CSOUP library, Chicken Soup for the C

   CSOUP is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   CSOUP is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "libcsoup.h"
#include "libcsoup_debug.h"

#define CSC_CDLL_MAGIC          (('D'<<24) | ('L'<<16))
#define CSC_CDLL_BACKGUARD      0xdeadbeef

#define CSC_CDLL_HEAD           -1
#define CSC_CDLL_TAIL           -2

struct	_CSCLNK {
	struct	_CSCLNK	*next;
	struct	_CSCLNK	*prev;
	void	*refp;
#ifdef	CFG_CDLL_SAFE
	/** Size of the CSCLNK structure and the payload.
	 * but it doesn't include the padding magic */
	unsigned	size;
	/** The combine of CSC_CDLL_MAGIC in higher 16-bit and a CRC16 in 
	 * lower 16 bit of the CSCLNK structure while the majesty had been 
	 * set to the CSC_CDLL_MAGIC. */
	unsigned	majesty;
#endif
	/** Payload starts here ... */
};

#ifdef	CFG_CDLL_SAFE
char	*csc_cdl_version_string = "csc_cdl_version-1.0-verified";
#else
char	*csc_cdl_version_string = "csc_cdl_version-1.0";
#endif

#ifdef	CFG_CDLL_SAFE
static int csc_cdl_checksum(CSCLNK *node)
{
	unsigned short	crc;

	node->majesty = CSC_CDLL_MAGIC;
	crc = csc_crc16(0, node, sizeof(CSCLNK));
	node->majesty |= crc;
	CDB_FUNC(("csc_cdl_checksum: %p:%d (+%p:-%p) R:%p S:%d M:%x\n", 
			node, sizeof(CSCLNK), node->next, node->prev, 
			node->refp, node->size, node->majesty));
	return (int) node->majesty;
}

static int csc_cdl_verify(CSCLNK *node)
{
	CSCLNK	tmp;
	int	*ptr;

	if (node == NULL) {
		return 0;
	}
	tmp = *node;
	csc_cdl_checksum(&tmp);

	if (tmp.majesty != node->majesty) {
		CDB_ERROR(("csc_cdl_verify: broken at %p (+%p:-%p) "
				"S:%d M:%x\n", node, node->next, 
				node->prev, node->size, node->majesty));
		return 0;
	}

	/* verify the back guard */
	ptr = (int*) (((char*)node) + node->size);
	if (*ptr != (int) CSC_CDLL_BACKGUARD) {
		CDB_ERROR(("csc_cdl_verify: backguard violated %p (+%p:-%p) "
				"%x\n", node, node->next, node->prev, *ptr));
		return 0;
	}
	return 1;
}
#endif	/* CFG_CDLL_SAFE */


/*!\brief Allocate a CSCLNK node
   
   The csc_cdl_alloc() function returns a CSCLNK structure with the
   payload of the specified size. In CFG_CDLL_SAFE it will be padded by 
   the guarding words.

   \param[in]   size The size of the payload.

   \return  A pointer to the CSCLNK structure of specified size if succeed,
   or NULL if failed.
*/
#ifdef	CFG_CDLL_SAFE
CSCLNK *csc_cdl_alloc(int size)
{
	CSCLNK	*node;
	int	*ptr;

	size += sizeof(CSCLNK);
	size = (size + 3) / 4 * 4;
	if ((node = smm_alloc(size + sizeof(int))) != NULL) {
		node->size = size;
		ptr = (int*) (((char*)node) + node->size);
		*ptr = CSC_CDLL_BACKGUARD;
	}
	return node;
}
#else
CSCLNK *csc_cdl_alloc(int size)
{
	size += sizeof(CSCLNK);
	size = (size + 3) / 4 * 4;
	return smm_alloc(size);
}
#endif	/* CFG_CDLL_SAFE */


/*!\brief Insert a node after the reference node.

   The csc_cdl_insert_after() function inserts the specified node between
   the reference node and its next node. This function doesn't need the
   anchor pointer so the reference node can not be NULL.

   \param[in]  refn The reference node where to insert after.
   \param[in]  node The node for insertion.
   \return     0 if succeed, or -1 if failed.

   \remark In CFG_CDLL_SAFE mode, it could return -1 if the reference node
   had been damaged.
*/
int csc_cdl_insert_after(CSCLNK *refn, CSCLNK *node)
{
#ifdef	CFG_CDLL_SAFE
	if (!csc_cdl_verify(refn)) {
		return -1;
	}
#endif
	node->next = refn->next;
	node->prev = refn;
#ifdef	CFG_CDLL_SAFE
	csc_cdl_checksum(node);
#endif

	refn->next->prev = node;
#ifdef	CFG_CDLL_SAFE
	csc_cdl_checksum(refn->next);
#endif
	refn->next = node;
#ifdef	CFG_CDLL_SAFE
	csc_cdl_checksum(refn);
#endif
	return 0;
}

/*!\brief  Insert the node into the head of the list.

   The csc_cdl_insert_head() inserts the specified node into the head of
   the circular doubly linked list. If the anchor pointer is NULL, which 
   means an empty list, the node will become the only member in the list.
   Otherwise the node will be inserted in the head of the list and the 
   anchor pointer will be updated to the new head, the node.

   \param[in]  anchor The anchor pointer which points to the head of the list.   
   \param[in]  node  The node for insertion.
   \return  The updated anchor pointer.
*/
CSCLNK *csc_cdl_insert_head(CSCLNK *anchor, CSCLNK *node)
{
	if (anchor == NULL) {
		node->prev = node->next = node;
#ifdef	CFG_CDLL_SAFE
		csc_cdl_checksum(node);
#endif
	} else if (csc_cdl_insert_after(anchor->prev, node) < 0) {
		return NULL;
	}
	return node;	/* return the new anchor */
}

/*!\brief  Insert the node into the tail of the list.

   The csc_cdl_insert_tail() inserts the specified node into the tail of
   the circular doubly linked list. If the anchor pointer is NULL, which 
   means an empty list, the node will become the only member in the list.
   Otherwise the node will be inserted into the end of the list. 

   \param[in]  anchor The anchor pointer which points to the head of the list.   
   \param[in]  node  The node for insertion.
   \return  The updated anchor pointer.

   \remark In the circular doubly linked list, the end of the list is actually
   a logical end because the tail node points to the head of the list.
*/
CSCLNK *csc_cdl_insert_tail(CSCLNK *anchor, CSCLNK *node)
{
	if (anchor == NULL) {
		anchor = node->prev = node->next = node;
#ifdef	CFG_CDLL_SAFE
		csc_cdl_checksum(node);
#endif
	} else if (csc_cdl_insert_after(anchor->prev, node) < 0) {
		return NULL;
	}
	return anchor;
}

/*!\brief  Remove the node from the list.

   The csc_cdl_remove() function removes the specified node from the 
   specified circular doubly linked list. If the anchor pointer is pointing 
   to the specified node, the anchor pointer will move to the next member. 
   If the anchor pointer is pointing to the only member in the list, once
   the member node is removed, the anchor will be reset to empty.

   \param[in]  anchor The anchor pointer which points to the head of the list.   
   \param[in]  node  One of the member in the list pointed by anchor for 
   removing.
   \return  The updated anchor pointer. If the last member in the list was 
   removed, it returns NULL pointer.

   \remark This function only removes node in the list. It doesn't free 
   the memory or other post morton things.
*/
CSCLNK *csc_cdl_remove(CSCLNK *anchor, CSCLNK *node)
{
#ifdef	CFG_CDLL_SAFE	/* double check the validility of the list */
	CSCLNK	*cur;

	if (!csc_cdl_verify(anchor)) {
		return NULL;
	}
	for (cur = anchor; cur; cur = csc_cdl_next(anchor, cur)) {
		if (cur == node) {
			break;
		}
	}
	if (cur == NULL) {
		return NULL;	/* node in the wrong list */
	}
#endif
	node->prev->next = node->next;
	node->next->prev = node->prev;
#ifdef	CFG_CDLL_SAFE
	csc_cdl_checksum(node->prev);
	csc_cdl_checksum(node->next);
#endif
	if (anchor == node) {
		anchor = node->next;
	}
	if (anchor == node) {	/* single node list */
		anchor = NULL;
	}
	return anchor;
}

/*!\brief  Move to next node.

   The csc_cdl_next() function returns the next node of the specified node.
   If the 'node' is already in the logical end of the list, it returns NULL
   pointer.

   \param[in]  anchor The anchor pointer which points to the head of the list.
   \param[in]  node  The reference node.
   \return  The next node after the specified 'node'. If the specified 'node'
   is already in the logical end of the list, it returns NULL pointer.

   \remark In the circular doubly linked list, the end of the list is actually
   a logical end because the tail node points to the head of the list. Using
   the csc_cdl_next() would simplify the process by one loop, for example:

   for (node = anchor; node; node = csc_cdl_next(anchor, node)) ...
*/
CSCLNK *csc_cdl_next(CSCLNK *anchor, CSCLNK *node)
{
	if (node == NULL) {
		return anchor;
	}
	if (node->next == anchor) {
		return NULL;
	}
#ifdef	CFG_CDLL_SAFE
	if (!csc_cdl_verify(node->next)) {
		return NULL;
	}
#endif
	return node->next;
}

/*!\brief Find a node in the list 
 
   The csc_cdl_search() function searches the list by a comparison function 
   pointed to by 'compare', which is called with two arguments that point to
   the objects being compared. The first object is the payload of the node. 
   The second object is supplied by 'refload'.

   The searching can commence from the first member of the list, if 'last'
   is set to NULL, or commence from the specified node, by setting 'last' to
   the last matched node. The node pointed by 'last' will not be compared.
   The comparision starts from the next node of 'last'.

   The comparison function must return 0 if the two objects are match.

   \param[in]  anchor The anchor pointer which points to the head of the list.
   \param[in]  last The last appearance of the matched node, or NULL by head.
   \param[in]  compare Pointer to the comparison function.
   \param[in]  refload Pointer to the content for the comparison function to 
               compare.
   \return     The first appearance of the matched node, or NULL if not found.
*/
CSCLNK *csc_cdl_search(CSCLNK *anchor, CSCLNK *last,
		int(*compare)(void *, void *), void *refload)
{
	CSCLNK	*node;

#ifdef	CFG_CDLL_SAFE
	if (!csc_cdl_verify(anchor)) {
		return NULL;
	}
	if (last && !csc_cdl_verify(last)) {
		return NULL;
	}
#endif
	if (last == NULL) {
		last = anchor;
	} else {
		last = csc_cdl_next(anchor, last);
	}
	for (node = last; node; node = csc_cdl_next(anchor, node)) {
		if (!compare((void*)&node[1], refload)) {
			return node;
		}
	}
	return NULL;
}

/*!\brief Pick up a node by index.
   
   The csc_cdl_goto() function returns the node in the list by index. 
   The first member in the list is counted 0.

   \param[in]   anchor The anchor pointer which points to the head of the list.
   \param[in]   idx Index number indicates which node is wanted.

   \return  The pointer to the node, or NULL if 'idx' is out of range.
*/
CSCLNK *csc_cdl_goto(CSCLNK *anchor, int idx)
{
	CSCLNK	*n;
	int	i;

#ifdef	CFG_CDLL_SAFE
	if (!csc_cdl_verify(anchor)) {
		return NULL;
	}
#endif
	for (i = 0, n = anchor; n; n = csc_cdl_next(anchor, n), i++) {
		if (idx == i) {
			return n;
		}
	}
	return NULL;
}

/*!\brief The index of a node in the list.
   
   The csc_cdl_index() function returns the index of a member in the list. 
   The index starts from 0. If the member can not be found, it returns -1.
   If the 'node' were set to NULL, it will return the length of the list.

   \param[in] anchor  The anchor pointer which points to the head of the list.
   \param[in] node    The node for searching the index. It can be NULL.

   \return  The index of node in the list, or -1 if failed.
   \remark  This function can validate the list if built with CFG_CDLL_SAFE.
*/
int csc_cdl_index(CSCLNK *anchor, CSCLNK *node)
{
	CSCLNK	*n;
	int	i = 0;

#ifdef	CFG_CDLL_SAFE
	if (!csc_cdl_verify(anchor)) {
		return 0;
	}
#endif
	for (i = 0, n = anchor; n; n = csc_cdl_next(anchor, n), i++) {
		if (n == node) {
			return i;
		}
	}
	if (node == NULL) {
		return i;
	}
	return -1;
}


/*!\brief Setup a node.
   
   The csc_cdl_setup() function fills in the CSCLNK structure. 

   \param[in] node  The points to the node.
   \param[in] prev  The pointer to the previous node.
   \param[in] next  The pointer to the next node.
   \param[in] rp    The pointer to the reference frame.
   \param[in] size  The size of the node.

   \return  The size of the CSCLNK structure. In CFG_CDLL_SAFE, it would 
   return the size field in the CSCLNK structure. Otherwise or node is NULL, 
   it returns the size of CSCLNK structure.

   \remark  This function can used to verify the build mode of the library.
*/
int csc_cdl_setup(CSCLNK *node, void *prev, void *next, void *rp, int size)
{
	if (node == NULL) {
		return sizeof(CSCLNK);
	}

	node->prev = prev;
	node->next = next;
	node->refp = rp;
#ifdef	CFG_CDLL_SAFE
	node->size = size;
	csc_cdl_checksum(node);
#else
	size = sizeof(CSCLNK);
#endif
	return size;
}


/*!\brief Return the payload of a node.
   
   The csc_cdl_payload() function safely return the payload of a node.
   
   \param[in] node  The points to the node.

   \return  The pointer to the payload in the CSCLNK structure. 
*/
void *csc_cdl_payload(CSCLNK *node)
{
	if (node) {
		node++;
	}
	return node;
}

/*!\brief Return the node structure from the payload pointer.
 
  The csc_cdl_paylink() function safely return the link structure where
  the payload attached.

  \param[in] payload  The pointer to the payload.

  \return The points to the CSCLNK structure which is ahead of the payload
*/
CSCLNK *csc_cdl_paylink(void *payload)
{
	CSCLNK	*node = payload;

	if (node) {
		node--;
	}
	return node;
}

/****************************************************************************
 * An application of the basic csc_cdl_* functions
 ***************************************************************************/
/*!\brief Allocate a node and insert it to the head of the list.

   The csc_cdl_list_alloc_head() function create a brand new node by allocating
   a piece of memory with the payload size of 'size'. The node will be 
   inserted into the head of the list.

   \param[in,out]  anchor The pointer of the anchor pointer which points to 
                   the head of the list.
   \param[in]      size The size of payload by bytes.

   \return         The pointer to the new allocated node if succeed, 
                   or NULL if fail.
   \remark         The anchor pointer could be changed in the call.
   \remark         The 'size' argument define the payload size only so
                   actually allocated size would plus the structure size 
		   of CSCLNK.
*/
CSCLNK *csc_cdl_list_alloc_head(CSCLNK **anchor, int size)
{
	CSCLNK	*node;

	if ((node = csc_cdl_alloc(size)) != NULL) {
		*anchor = csc_cdl_insert_head(*anchor, node);
#ifdef	CFG_CDLL_SAFE
		if (*anchor == NULL) {
			smm_free(node);
			node = NULL;
		}
#endif
	}
	return node;
}

/*!\brief Allocate a node and append it to the end of the list.

   The csc_cdl_list_alloc_tail() function create a brand new node by allocating
   a piece of memory with the payload size of 'size'. The node will be 
   appended to the end of the list. 

   \param[in,out]  anchor The pointer of the anchor pointer which points to 
                   the head of the list.
   \param[in]      size The size of payload by bytes.

   \return         The pointer to the new allocated node if succeed, 
                   or NULL if fail.
   \remark         The anchor pointer could be changed in the call.
   \remark         The 'size' argument define the payload size only so
                   actually allocated size would plus the structure size 
		   of CSCLNK.
*/
CSCLNK *csc_cdl_list_alloc_tail(CSCLNK **anchor, int size)
{
	CSCLNK	*node;

	if ((node = csc_cdl_alloc(size)) != NULL) {
		*anchor = csc_cdl_insert_tail(*anchor, node);
#ifdef	CFG_CDLL_SAFE
		if (*anchor == NULL) {
			smm_free(node);
			node = NULL;
		}
#endif
	}
	return node;
}

/*!\brief Insert a node into the specified position.

   The csc_cdl_list_insert() funciton insert a node into the specified 
   position in the list by 'idx'. If 'idx' is 0, it is inserted in the head
   of the list. If 'idx' is less than 0, it will be inserted in the tail of
   the list. Otherwise it will be inserted after the 'idx'-th node.

   \param[in,out]  anchor The pointer of the anchor pointer which points to
                          the head of the list.
   \param[in]      node   The inserting node.
   \param[in]      idx    The index in the list where the node goes after.
   			  If 'idx' < 0, see CSC_CDLL_HEAD and CSC_CDLL_TAIL

   \return         0 if succeed, or -1 if fail.
   \remark         The anchor pointer could be changed in the call.
*/
int csc_cdl_list_insert(CSCLNK **anchor, CSCLNK *node, int idx)
{
	CSCLNK	*n;
	int	i;

	if ((idx == CSC_CDLL_HEAD) || (*anchor == NULL)) {
		*anchor = csc_cdl_insert_head(*anchor, node);
#ifdef	CFG_CDLL_SAFE
		return *anchor ? 0 : -1;
#else
		return 0;
#endif
	}
	if (idx < 0) {	/* CSC_CDLL_TAIL */
		*anchor = csc_cdl_insert_tail(*anchor, node);
#ifdef	CFG_CDLL_SAFE
		return *anchor ? 0 : -1;
#else
		return 0;
#endif
	}

#ifdef	CFG_CDLL_SAFE
	if (!csc_cdl_verify(*anchor)) {
		return -1;
	}
#endif
	for (i = 0, n = *anchor; n; n = csc_cdl_next(*anchor, n), i++) {
		if (i == idx) {
			return csc_cdl_insert_after(n, node);
		}
	}
	return -1;		
}

int csc_cdl_list_insert_head(CSCLNK **anchor, CSCLNK *node)
{
	return csc_cdl_list_insert(anchor, node, CSC_CDLL_HEAD);
}

int csc_cdl_list_insert_tail(CSCLNK **anchor, CSCLNK *node)
{
	return csc_cdl_list_insert(anchor, node, CSC_CDLL_TAIL);
}


/*!\brief Remove the specified node from the list and free its memory.

   \param[in,out]  anchor The pointer of the anchor pointer which points to 
                   the head of the list.
   \param[in]      node The node going to be freed.

   \return         The pointer to the next node in the list, or NULL if there
                   is no more node left.
   \remark         The anchor pointer could be changed in the call.
*/
CSCLNK *csc_cdl_list_free(CSCLNK **anchor, CSCLNK *node)
{
	CSCLNK	*next;

#ifdef	CFG_CDLL_SAFE
	if (!csc_cdl_verify(node)) {
		return NULL;
	}
#endif
	if ((next = node->next) == node) {
		next = NULL;	/* node is the last member in the queue */
	}
	*anchor = csc_cdl_remove(*anchor, node);
	smm_free(node);
	return next;
}

/*!\brief Destroy a whole list and free all its members.

   \param[in,out]  anchor The pointer of the anchor pointer which points to 
                   the head of the list.

   \return         0 if succeed, or -1 if failed.
   \remark         The anchor pointer will be reset to NULL.
   \remark         The list should be built by calling csc_cdl_list_alloc_head()
                   or csc_cdl_list_alloc_tail().
*/
int csc_cdl_list_destroy(CSCLNK **anchor)
{
	CSCLNK	*node;

	for (node = *anchor; node; node = csc_cdl_list_free(anchor, node)) {
#ifdef	CFG_CDLL_SAFE
		if (!csc_cdl_verify(node)) {
			return -1;
		}
#endif
	}
	*anchor = NULL;
	return 0;
}

/*!\brief Check the heath of the list

   \param[in,out]  anchor The pointer of the anchor pointer which points to 
                   the head of the list.

   \return         the number of nodes in the list, or -1 if failed.
*/
int csc_cdl_list_state(CSCLNK **anchor)
{
	CSCLNK	*node;
	int	i = 0;

	for (node = *anchor; node; node = csc_cdl_next(*anchor, node)) {
#ifdef	CFG_CDLL_SAFE
		if (!csc_cdl_verify(node)) {
			return -1;
		}
#endif
		i++;
	}
	return i;
}

