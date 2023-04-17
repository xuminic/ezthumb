
/*!\file       csc_tmem.c
   \brief      The tiny dynamic memory management based on single link list

   The file supports a set of extreme light weight dynamic memory management.
   It would be quite easy to use within a small memory pool in stack.
   The minimum allocation unit is 'int'. Therefore the maximum managable 
   memory is 4GB in 32/64-bit system, or 32KB in 8/16-bit system.

   The memory overhead is the smallest as far as I know, only one standard integer,
   which can be 4 bytes in 32-bit system or 2 byte in 8-bit system.
   It uses single link list so the speed is not as good as doubly link list.
   Do not use it in high frequency of allocating and freeing scenario.

   \author     "Andy Xuming" <xuming@users.sourceforge.net>
   \date       2013-2014
*/
/* Copyright (C) 1998-2018  "Andy Xuming" <xuming@users.sourceforge.net>

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

#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "libcsoup.h"

/* Memory Sight:
 *   [Managing Block][Memory Block][Memory Block]...
 * [Managing Block]
 *   char    magic[4]: 
 *     CRC8 + MAGIC + CONFIG1 + CONFIG2
 *   Control Word of the Heap:
 *     MSB+0: parity bit
 *     MSB+1: usable bit (always 1=used)
 *     MSB+2...n: heap size (excluding the managing block)
 * [Memory Block]
 *   Control Word of the memory block:
 *     MSB+0: parity bit
 *     MSB+1: usable bit (0=free 1=used)
 *     MSB+2...n: block size (excluding the control word)
 *   int Memory[block size]
 */
#if	(UINT_MAX == 0xFFFFFFFFU)
#define TMEM_MASK_PARITY	0x80000000U
#define TMEM_MASK_USED		0x40000000U
#else
#define TMEM_MASK_PARITY	0x8000U
#define TMEM_MASK_USED		0x4000U
#endif
#define TMEM_MASK_USIZE		((int)(UINT_MAX >> 2))

#define TMEM_SIZE(n)		((n) & TMEM_MASK_USIZE)
#define TMEM_BYTES(n)		(TMEM_SIZE(n) * sizeof(int))
#define TMEM_NEXT(p)		((int*)(p) + TMEM_SIZE(*(int*)(p)) + 1)
#define TMEM_UPWORD(n)		(((n) + sizeof(int) - 1) / sizeof(int))
#define TMEM_DNWORD(n)		((n) / sizeof(int))

#define TMEM_SET_USED(n)	((n) | TMEM_MASK_USED)
#define	TMEM_CLR_USED(n)	((n) & ~TMEM_MASK_USED)
#define	TMEM_TEST_USED(n)	((n) & TMEM_MASK_USED)

#define TMEM_OVERHEAD		4	/* reserved for attribution of the heap */
#define TMEM_GUARD(c)		(CSC_MEM_GETPG(c) * 2)


static int tmem_parity(int cw);
static int tmem_verify(void *heap, int *mb);
static int tmem_cword(int uflag, int size);
static void *tmem_find_client(void *heap, int *mb, size_t *osize);
static int *tmem_find_control(void *heap, void *mem);

static inline void tmem_config_set(unsigned char *heap, int config)
{
	heap[2] = (unsigned char)(config & 0xff);
	heap[3] = (unsigned char)((config >> 8) & 0xff);
}

static inline int tmem_config_get(unsigned char *heap)
{
	return (int)((heap[3] << 8) | heap[2]);
}

static inline int *tmem_start(void *heap)
{
	return (int*)(((char*)heap) + TMEM_OVERHEAD);
}

/*!\brief Initialize the memory heap to be allocable.

   \param[in]  hmem the memory heap for allocation.
   \param[in]  len the size of the memory heap.

   \return    The pointer to the memory heap object, or NULL if failed.

   \remark The given memory pool must be started at 'int' boundry.
   The minimum allocation unit is 'int'. Therefore the maximum managable 
   memory is 4GB in 32/64-bit system, or 32KB in 8/16-bit system.
*/
void *csc_tmem_init(void *hmem, size_t len, int flags)
{
	int	*heap, guards;

	if (hmem == NULL) {
		return hmem;	/* CSC_MERR_INIT */
	}

	/* save TMEM_OVERHEAD bytes and one int for heap managing  */
	len -= sizeof(int) + TMEM_OVERHEAD;
	
	/* change size unit to per-int; the remains will be cut off  */
	len /= sizeof(int);
	guards = TMEM_GUARD(flags) / sizeof(int);

	/* make sure the size is not out of range */
	/* Though CSC_MEM_ZERO is practically useless, supporting CSC_MEM_ZERO
	 * in program is for the integrity of the memory logic */
	if ((len <= (size_t)guards) || (len > (UINT_MAX >> 2))) {
		return NULL;	/* CSC_MERR_RANGE */
	}
	if ((len == (size_t)guards + 1) && !(flags & CSC_MEM_ZERO)) {
		return NULL;	/* CSC_MERR_RANGE: no support empty allocation */
	}

	/* create the heap management */
	((char*)hmem)[0] = (char)CSC_MEM_MAGIC_TINY;
	((char*)hmem)[1] = (char)CSC_MEM_MAGIC_TINY;
	tmem_config_set(hmem, flags);

	heap = tmem_start(hmem);
	*heap++ = tmem_cword(1, (int)len--);
	
	/* create the first memory block */
	*heap = tmem_cword(0, (int)len);
	return hmem;
}


/*!\brief allocate a piece of dynamic memory block inside the specified 
   memory heap.

   \param[in]  heap the memory heap for allocation.
   \param[in]  n the size of the expecting allocated memory block.

   \return    point to the allocated memory block in the memory heap. 
              or NULL if not enough space for allocating.

   \remark The strategy of allocation is defined by CSC_MEM_FITNESS
           in libcsoup.h

*/
void *csc_tmem_alloc(void *heap, size_t n)
{
	int	 *found, *next, config, unum;
	
	int loose(void *mem)
	{
		int	*mb = tmem_find_control(heap, mem);

		if (TMEM_SIZE(*mb) >= unum) {
			if (found == NULL) {
				found = mb;
			}
			switch (config & CSC_MEM_FITMASK) {
			case CSC_MEM_BEST_FIT:
				if (TMEM_SIZE(*found) > TMEM_SIZE(*mb)) {
					found = mb;
				}
				break;
			case CSC_MEM_WORST_FIT:
				if (TMEM_SIZE(*found) < TMEM_SIZE(*mb)) {
					found = mb;
				}
				break;
			default:	/* CSC_MEM_FIRST_FIT */
				return 1;
			}
		}
		return 0;
	}

	if (tmem_verify(heap, (int*)-1) < 0) {
		return NULL;	/* CSC_MERR_INIT: memory heap not available */
	}

	/* find the request size in unit of int */
	config = tmem_config_get(heap);
	unum = (int) TMEM_UPWORD(n + TMEM_GUARD(config));

	/* make sure the request is NOT out of size */
	if (unum > TMEM_SIZE(*tmem_start(heap))) {
		return NULL;	/* CSC_MERR_LOWMEM */
	} else if (unum == (int)TMEM_DNWORD(TMEM_GUARD(config)) && !(config & CSC_MEM_ZERO)) {
		return NULL;	/* CSC_MERR_RANGE: not allow empty allocation */
	}

	found = next = NULL;
	if (csc_tmem_scan(heap, NULL, loose)) {
		return NULL;	/* CSC_MERR_BROKEN: chain broken */
	}
	if (found == NULL) {
		return NULL;	/* CSC_MERR_LOWMEM: out of memory */
	}

	n = (config & CSC_MEM_ZERO) ? 0 : 1;	/* reuse the 'n' for size test */
	n += TMEM_DNWORD(TMEM_GUARD(config));	/* make sure the remain no less than guarding area */
	if (TMEM_SIZE(*found) <= unum + n) {	
		/* not worth to split this block */
		*found = tmem_cword(1, *found);
	} else {
		/* split this memory block */
		next = found + unum + 1;
		*next = tmem_cword(0, TMEM_SIZE(*found) - unum - 1);
		*found = tmem_cword(1, unum);
	}

	/* return the client area */
	found = tmem_find_client(heap, found, &n);
	if (config & CSC_MEM_CLEAN) {
		memset(found, 0, n);
	}
	return (void*)found;
}

/*!\brief free the allocated memory block.

   \param[in]  heap the memory heap for allocation.
   \param[in]  mem the memory block.

   \return    0 if freed successfully 
              -1 memory heap not initialized
	      -2 memory chain broken
	      -3 memory not found

   \remark If using csc_tmem_free() to free a free memory block, it returns -3.
*/
int csc_tmem_free(void *heap, void *mem)
{
	int	*last, *found, rc;

	int used(void *fmem)
	{
		int	*mb;

		if (fmem == mem) {
			found = tmem_find_control(heap, fmem);
			*found = tmem_cword(0, *found);	/* free itself */
			
			/* try to down-merge the next memory block */
			mb = TMEM_NEXT(found);
			if (tmem_verify(heap, mb) < 0) {
				return 1;
			}
			if (!TMEM_TEST_USED(*mb)) {
				*found = tmem_cword(0, TMEM_SIZE(*found + *mb + 1));
				*mb = 0;	/* liquidate the control word in middle */
			}
			return 1;
		}
		return 0;
	}
	int loose(void *mb)
	{
		last = tmem_find_control(heap, mb);
		return 0;
	}


	if (mem == NULL) {
		return CSC_MERR_RANGE;
	}

	found = tmem_find_control(heap, mem);
	if ((rc = tmem_verify(heap, found)) < 0) {
		return rc;	/* memory heap not available */
	}

	if (!TMEM_TEST_USED(*found)) {
		return 0;	/* freeing a freed memory */
	}

	last = found = NULL;
	if (csc_tmem_scan(heap, used, loose)) {
		return CSC_MERR_BROKEN;	/* memory chain broken */
	}

	if (found == NULL) {
		return CSC_MERR_RANGE;	/* memory not found */
	}

	/* try to up-merge the previous memory block */
	if (last && (TMEM_NEXT(last) == found)) {
		*last = tmem_cword(0, TMEM_SIZE(*last + *found + 1));
		*found = 0;	/* liquidate the control word in middle */
	}
	return 0;
}

/*!\brief scan the memory chain to process every piece of memory block.

   \param[in]  heap the memory heap for allocation.
   \param[in]  used the callback function when find a piece of used memory
   \param[in]  loose the callback function when find a piece of free memory

   \return     NULL if successfully scanned the memory chain. If the memory
               chain is corrupted, it returns a pointer to the broken point.

   \remark    The scan function will scan the heap from the first memory block to the last.
   When it found an allocated memory block, it will call the used() function with the address
   of the allocated memory block. When it found a freed memory block, it will call the loose() 
   function with the address of the freed memory block.

   \remark The prototype of the callback functions are: int func(void *)
           The scan process will stop in middle if func() returns non-zero.
*/
void *csc_tmem_scan(void *heap, int (*used)(void*), int (*loose)(void*))
{
	int	*mb, *cw;

	if (tmem_verify(heap, (int*)-1) < 0) {
		return heap;	/* memory heap not available */
	}
	cw = tmem_start(heap);
	for (mb = cw + 1; mb < TMEM_NEXT(cw); mb = TMEM_NEXT(mb)) {
		if (tmem_verify(heap, mb) < 0) {
			return (void*)mb;	/* chain broken */
		}
		if (TMEM_TEST_USED(*mb)) {
			if (used && used(tmem_find_client(heap, mb, NULL))) {
				break;
			}
		} else {
			if (loose && loose(tmem_find_client(heap, mb, NULL))) {
				break;
			}
		}
	}
	return NULL;
}

/*!\brief find the attribution of an allocated memory.

   \param[in]  heap the memory heap for allocation.
   \param[in]  mem the memory block.
   \param[out] state the state of the memory block. 
               0=free 1=used, or <0 in error codes.

   \return    the size of the allocated memory without padding when succeed,
              or (size_t) -1 in error. the error code returns in 'state'.
*/
size_t csc_tmem_attrib(void *heap, void *mem, int *state)
{
	int	rc, *mb;

	if (state == NULL) {
		state = &rc;
	}
	
	if (mem == NULL) {
		*state = CSC_MERR_RANGE;
		return (size_t) -1;
	}

	mb = tmem_find_control(heap, mem);
	if ((rc = tmem_verify(heap, mb)) < 0) {
		*state = rc;
		return (size_t) -1;	/* memory heap not available */
	}
	
	*state = TMEM_TEST_USED(*mb) ? 1 : 0;
	
	/* should not happen in theory: the remain memory is smaller than
	 * the guarding area. It should've been sorted out in the allocation 
	 * function already */
	rc = TMEM_BYTES(*mb) - TMEM_GUARD(tmem_config_get(heap));
	if (rc < 0) {
		*state = rc;
		return (size_t) -1;
	}
	return (size_t) rc;
}

/*!\brief find the front guard area.

   \param[in]  heap the memory heap.
   \param[in]  mem the allocated memory block.
   \param[out] xsize the size of the front guard area, or 0=freed memory, 
               or the error code when failed

   \return    the pointer to the front guard area, or NULL if failed.

   \remark    the guarding area is a piece of memories in front of the client area
   and in the end of the client area. It can be used to buffer a possible memory overflow
   while debugging, or store extra information about the memory user. The size of the
   guarding area was defined in the 'flags' parameter when initializing in bit 4-7,
   the page size will multiply the number of guarding pages in bit 8-11.

   \remark    the size of the guarding area may not be the exact same to the size defined in 'flags',
   because the pad area in the memory management block was defined as a part of the front guard,
   and the pad area in the allocated memory block was defined as a part of the back guard.
*/
void *csc_tmem_front_guard(void *heap, void *mem, int *xsize)
{
	int	rc, *mb;

	if (xsize == NULL) {
		xsize = &rc;
	}
	
	if (mem == NULL) {
		*xsize = CSC_MERR_RANGE;
		return NULL;
	}

	mb = tmem_find_control(heap, mem);
	if ((rc = tmem_verify(heap, mb)) < 0) {
		*xsize = rc;
		return NULL;	/* memory heap not available */
	}

	/* make sure the memory block was allocated. pointless to guard a free block */
	if (!TMEM_TEST_USED(*mb)) {
		*xsize = 0;
		return NULL;
	}

	*xsize = TMEM_GUARD(tmem_config_get(heap)) >> 1;
	return mb+1;
}

/*!\brief find the back guard area.

   \param[in]  heap the memory heap.
   \param[in]  mem the allocated memory block.
   \param[out] xsize the size of the back guard area, or 0=freed memory, 
               or the error code when failed

   \return    the pointer to the back guard area, or NULL if failed.

   \remark    the guarding area is a piece of memories in front of the client area
   and in the end of the client area. It can be used to buffer a possible memory overflow
   while debugging, or store extra information about the memory user. The size of the
   guarding area was defined in the 'flags' parameter when initializing in bit 4-7,
   the page size will multiply the number of guarding pages in bit 8-11.

   \remark    the size of the guarding area may not be the exact same to the size defined in 'flags',
   because the pad area in the memory management block was defined as a part of the front guard,
   and the pad area in the allocated memory block was defined as a part of the back guard.
*/
void *csc_tmem_back_guard(void *heap, void *mem, int *xsize)
{
	int	rc, *mb;

	if (xsize == NULL) {
		xsize = &rc;
	}
	
	if (mem == NULL) {
		*xsize = CSC_MERR_RANGE;
		return NULL;
	}

	mb = tmem_find_control(heap, mem);
	if ((rc = tmem_verify(heap, mb)) < 0) {
		*xsize = rc;
		return NULL;	/* memory heap not available */
	}

	/* make sure the memory block was allocated. pointless to guard a free block */
	if (!TMEM_TEST_USED(*mb)) {
		*xsize = 0;
		return NULL;
	}

	*xsize = TMEM_GUARD(tmem_config_get(heap)) >> 1; 
	return TMEM_NEXT(mb) - *xsize / sizeof(int);
}


/* applying odd parity so 15 (16-bit) or 31 (32-bit) 1-bits makes MSB[1]=0,
 * which can easily sorting out -1 as an illegal word. */
/* https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
 * https://stackoverflow.com/questions/30688465/how-to-check-the-number-of-set-bits-in-an-8-bit-unsigned-char
 */
#if	UINT_MAX == 0xFFFFFFFFU
static int tmem_parity(int cw)
{
	unsigned  x = (unsigned)cw & ~TMEM_MASK_PARITY;

	x = x - ((x >> 1) & 0x55555555);
	x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
	x = (x + (x >> 4)) & 0x0F0F0F0F;
	x = x + (x >> 8);
	x = x + (x >> 16);
	x &= 0x0000003F;
	x++;	/* make odd bit even */
	return (cw & ~TMEM_MASK_PARITY) | (x << 31);
}
#else
static int tmem_parity(int cw)
{
	unsigned  x = (unsigned)cw & ~TMEM_MASK_PARITY;

	x = x - ((x >> 1) & 0x5555);
	x = (x & 0x3333) + ((x >> 2) & 0x3333);
	x = (x + (x >> 4)) & 0x0F0F;
	x = x + (x >> 8);
	x &= 0x7;
	x++;	/* make odd bit even */
	return (cw & ~TMEM_MASK_PARITY) | (x << 15);
}
#endif

static int tmem_verify(void *heap, int *mb)
{
	if (heap == NULL) {
		return CSC_MERR_INIT;
	}
	if (((char*)heap)[1] != (char)CSC_MEM_MAGIC_TINY) {
		return CSC_MERR_INIT;	/* memory block corrupted */
	}

	heap = tmem_start(heap);	/* move to control word */
	if (*((int*)heap) != tmem_parity(*((int*)heap))) {
		return CSC_MERR_BROKEN;	/* memory heap not available */
	}

	/* Only verify the heap; not using NULL because NULL is valid */
	if (mb == (int*) -1) {
		return 0;
	}

	if (((void*)mb < heap) || (mb >= TMEM_NEXT(heap))) {
		return CSC_MERR_RANGE;	/* memory out of range */
	}
	if (*mb != tmem_parity(*mb)) {
		return CSC_MERR_BROKEN;	/* memory block corrupted */
	}
	return 0;
}

static int tmem_cword(int uflag, int size)
{
	if (uflag) {
		size = TMEM_SET_USED(size);
	} else {
		size = TMEM_CLR_USED(size);
	}
	return tmem_parity(size);
}

static void *tmem_find_client(void *heap, int *mb, size_t *osize)
{
	int	guards;

	guards = TMEM_GUARD(tmem_config_get(heap));
	if (osize) {
		*osize = TMEM_BYTES(*mb) - guards;
	}
	return mb + 1 + (guards >> 1) / sizeof(int);
}

static int *tmem_find_control(void *heap, void *mem)
{
	int	guard;

	guard = TMEM_GUARD(tmem_config_get(heap)) >> 1;
	return (int*)mem - guard / sizeof(int) - 1;
}

#ifdef	CFG_UNIT_TEST

#include "libcsoup_debug.h"

#define BMEM_SPAN(f,t)          ((size_t)((char*)(t) - (char*)(f)))
#define GUARD_WORD(heap)	(TMEM_GUARD(tmem_config_get(heap))/sizeof(int))

static void tmem_test_function(void *buf, int len);
static void tmem_test_empty_memory(void *buf, int len);
static void tmem_test_nonempty_memory(void *buf, int len);
static void tmem_test_misc_memory(void *buf, int len);
static void tmem_test_fitness(void *buf, int len);

int csc_tmem_unittest(void)
{
	int	buf[4096];

	tmem_test_function(buf, sizeof(buf));
	tmem_test_empty_memory(buf, sizeof(buf));
	tmem_test_nonempty_memory(buf, sizeof(buf));
	tmem_test_misc_memory(buf, sizeof(buf));
	tmem_test_fitness(buf, sizeof(buf));
	return 0;
}

static void tmem_test_function(void *buf, int len)
{
	int	plist[] = { -1, 0, 1, 0xf0f0f0f0, 0x55555555, 0x0f0f0f0f, 0x66666666 };
	int	i;
	unsigned char	*p;

	short tmem_parity16(short cw)
	{
		unsigned short  x = (unsigned short)cw & ~0x8000;

		x = x - ((x >> 1) & 0x5555);
		x = (x & 0x3333) + ((x >> 2) & 0x3333);
		x = (x + (x >> 4)) & 0x0F0F;
		x = x + (x >> 8);
		x &= 0x1f;
		x++;	/* make odd bit even */
		return (cw & ~0x8000) | (x << 15);
	}

	for (i = 0; i < (int)(sizeof(plist)/sizeof(int)); i++) {
		cclog(tmem_parity(plist[i]) == tmem_parity(tmem_parity(plist[i])),
				"ODD Parity 0x%08x: 0x%08x 0x%08x\n", plist[i], 
				tmem_parity(plist[i]),
				tmem_parity(tmem_parity(plist[i])));
		cclog(tmem_parity16((short) plist[i]) == tmem_parity16(tmem_parity16((short)plist[i])),
				"ODD Parity16   0x%04x: 0x%04x 0x%04x\n", 
				(unsigned short) plist[i], 
				(unsigned short) tmem_parity16((short) plist[i]),
				(unsigned short) tmem_parity16(tmem_parity16((short)plist[i])));
	}

#if	(UINT_MAX == 0xFFFFFFFFU)
	len = 0xc0123456;
	cclog(TMEM_SIZE(len)==0x123456, "TMEM_SIZE(0x%x) == 0x%x\n", len, TMEM_SIZE(len));
	len = 0x12345678;
	cclog(TMEM_SIZE(len)==0x12345678, "TMEM_SIZE(0x%x) == 0x%x\n", len, TMEM_SIZE(len));
#else
	len = 0xc012;
	cclog(TMEM_SIZE(len)==0x12, "TMEM_SIZE(0x%x) == 0x%x\n", len, TMEM_SIZE(len));
	len = 0x1234;
	cclog(TMEM_SIZE(len)==0x1234, "TMEM_SIZE(0x%x) == 0x%x\n", len, TMEM_SIZE(len));
#endif
	memset(buf, 0, 4);
	p = buf;
	tmem_config_set(buf, 0xc1c2c3c4);
	cclog(p[2] == 0xc4 && p[3] == 0xc3, "tmem_config_set: %x %x %x %x\n",
			p[0], p[1], p[2], p[3]);
	len = tmem_config_get(buf);
	cclog(len == 0xc3c4, "tmem_config_get: %x\n", len);

	p = csc_tmem_init(buf, 4*sizeof(int), CSC_MEM_DEFAULT);
	len = tmem_config_get(p);
	cclog(len==CSC_MEM_DEFAULT, "csc_tmem_init: flag=%x ", len);
	len = (int) csc_tmem_attrib(p, tmem_start(p)+1, &i);
	cslog("heap=%d(%d) ", len, i);
	len = (int) csc_tmem_attrib(p, tmem_start(p)+2, &i);
	cslog("freemem=%d(%d)\n", len, i);
}

static void tmem_test_empty_memory(void *buf, int len)
{
	int	n, *p, *ctl;
	size_t	msize;
	
	int memc(void *mem)
	{
		int	*mb = tmem_find_control(buf, mem);

		cslog("(+%d:%c:%lu)", BMEM_SPAN(buf,mb), 
				TMEM_TEST_USED(*mb)?'u':'f', TMEM_BYTES(*mb));
		return 0;
	}

	/* create a smallest heap where has only one heap control and 
	 * one block control */
	len = CSC_MEM_DEFAULT;
	msize = TMEM_OVERHEAD + sizeof(int) + sizeof(int) + TMEM_GUARD(len) + 1;
	p = csc_tmem_init(buf, msize, len);
	cclog(!p, "Create heap(%d,%d) with %ld bytes: empty allocation disabled.\n", 
			CSC_MEM_PAGE(len), CSC_MEM_GUARD(len), msize);

	len = CSC_MEM_DEFAULT | CSC_MEM_SETPG(1,2);
	msize = TMEM_OVERHEAD + sizeof(int) + sizeof(int) + TMEM_GUARD(len) + 1;
	p = csc_tmem_init(buf, msize, len);
	cclog(!p, "Create heap(%d,%d) with %ld bytes: empty allocation disabled.\n", 
			CSC_MEM_PAGE(len), CSC_MEM_GUARD(len), msize);

	len |= CSC_MEM_ZERO;
	msize = TMEM_OVERHEAD + sizeof(int) + sizeof(int) + TMEM_GUARD(len) + 1;
	p = csc_tmem_init(buf, msize, len);
	cclog(-1, "Create heap(%d,%d) with %ld bytes: empty allocation enabled.\n", 
			CSC_MEM_PAGE(len), CSC_MEM_GUARD(len), msize);
	if (p == NULL) return;
	
	p = tmem_start(buf);
	n = TMEM_SIZE(*p);
	cclog(n == (int)((msize - TMEM_OVERHEAD)/sizeof(int) - 1), 
			"TMEM_SIZE of the heap: +%d %d\n", BMEM_SPAN(buf,p), n);
	p = TMEM_NEXT(p);
	cclog(p == (int*)buf + msize/sizeof(int), "TMEM_NEXT of the heap: +%d\n", BMEM_SPAN(buf,p));

	p = tmem_start(buf) + 1;	/* move to the first block */
	n = TMEM_SIZE(*p);
	cclog(n == (int)GUARD_WORD(buf), "TMEM_SIZE of the first block: +%d %d\n", BMEM_SPAN(buf,p), n);
	p = TMEM_NEXT(p);
	cclog(p == TMEM_NEXT(tmem_start(buf)), "TMEM_NEXT of the first block: +%d\n", BMEM_SPAN(buf,p));

	p = csc_tmem_alloc(buf, 1);
	cclog(p == NULL, "Can not allocate 1 byte from the full heap\n");

	p = csc_tmem_alloc(buf, 0);
	cclog(p != NULL, "Can allocate 0 byte from the full heap %p\n", p);
	ctl = tmem_find_control(buf, p);
	cclog(TMEM_SIZE(*ctl) == GUARD_WORD(buf), "TMEM_SIZE of the first block: %d\n", TMEM_SIZE(*ctl));

	msize = csc_tmem_attrib(buf, p, &n);
	cclog((n == 1) && (msize == 0), "Attribution of the first block: %d %ld\n", n, msize);
	ctl = csc_tmem_front_guard(buf, p, &n);
	cclog(ctl && (n == TMEM_GUARD(len)/2), 
			"Front guard of the first block: +%d %d\n", BMEM_SPAN(buf, ctl), n);
	ctl = csc_tmem_back_guard(buf, p, &n);
	cclog(ctl && (n == TMEM_GUARD(len)/2), 
			"Back guard of the first block: +%d %d\n", BMEM_SPAN(buf, ctl), n);

	csc_tmem_free(buf, p);
	msize = csc_tmem_attrib(buf, p, &n);
	cclog((n == 0) && (msize == 0), "Attribution of the freed block: %d %ld\n", n, msize);

	/* create heap with 3 empty block: HEAP+MB0+MB1+MB2 */
	len = CSC_MEM_DEFAULT | CSC_MEM_ZERO | CSC_MEM_SETPG(1,2);
	msize = TMEM_OVERHEAD + sizeof(int) + (sizeof(int) + TMEM_GUARD(len)) * 3 + 1;
	p = csc_tmem_init(buf, msize, len);
	cclog(-1, "Create heap(%d,%d) with %ld bytes: empty allocation enabled.\n", 
			CSC_MEM_PAGE(len), CSC_MEM_GUARD(len), msize);
	if (p == NULL) return;

	/* split test: split the memory by allocation 1 byte */
	p = csc_tmem_alloc(buf, 1);
	ctl = tmem_find_control(buf, p);
	cclog(!!p, "Allocated memory +%d %x\n", BMEM_SPAN(buf,p), *ctl);
	cclog(TMEM_NEXT(ctl) == ctl + GUARD_WORD(buf) + 2, 
			"Next memory +%d\n", BMEM_SPAN(buf, TMEM_NEXT(ctl)));
	ctl = TMEM_NEXT(ctl);
	cclog(TMEM_NEXT(ctl) == TMEM_NEXT(tmem_start(buf)), 
			"End of the memory +%d\n", BMEM_SPAN(buf, TMEM_NEXT(ctl)));

	msize = csc_tmem_attrib(buf, p, &n);
	cclog((n ==1) && (msize == sizeof(int)), "Attribution of the allocated memory: %d %ld\n", n, msize);
	msize = csc_tmem_attrib(buf, tmem_find_client(buf, ctl, NULL), &n);
	cclog(n == 0, "Attribution of the free memory: %d %ld\n", n, msize);

	csc_tmem_free(buf, p);
	msize = csc_tmem_attrib(buf, p, &n);
	cclog((n == 0) && (msize == (sizeof(int) + TMEM_GUARD(len)) * 2), 
			"Attribution of the freed memory: %d %ld\n", n, msize);

	/* split test: maximum split; the remain is empty */
	p = csc_tmem_alloc(buf, sizeof(int) + TMEM_GUARD(len));
	msize = csc_tmem_attrib(buf, p, NULL);
	cclog(msize == sizeof(int) + TMEM_GUARD(len), "Splitting memory by allocating %ld bytes\n", msize);
	ctl = tmem_find_control(buf, p);
	ctl = TMEM_NEXT(ctl);
	cclog((TMEM_NEXT(ctl) == TMEM_NEXT(tmem_start(buf))) && (TMEM_SIZE(*ctl) == GUARD_WORD(buf)),
			"Remains are %d bytes\n",  TMEM_BYTES(*ctl));
	csc_tmem_free(buf, p);

	/* split test: unworthy */
	n = sizeof(int) + TMEM_GUARD(len) + 1;
	p = csc_tmem_alloc(buf, (size_t) n);
	msize = csc_tmem_attrib(buf, p, NULL);
	ctl = tmem_find_control(buf, p);
	cclog((msize > n + sizeof(int)) && (TMEM_NEXT(ctl) == TMEM_NEXT(tmem_start(buf))),
			"Unworth to split memory: %ld for %d bytes\n", msize, n);
	csc_tmem_free(buf, p);

	/* create a memory hole and scan it */
	n = csc_tmem_alloc(buf, 0) ? 1 : 0;
	n += (p = csc_tmem_alloc(buf, 0)) ? 1 : 0;
	n += csc_tmem_alloc(buf, 0) ? 1 : 0;
	cclog(n == 3, "Allocated %d empty memories: ", n);
	csc_tmem_free(buf, p);
	csc_tmem_scan(buf, memc, memc);
	cslog("\n");
}

static void tmem_test_nonempty_memory(void *buf, int len)
{
	int	n[4], *p[4];
	size_t	msize;

	/* create heap with 1 memory block: HEAP+MB0+FG0+4Byte+BG0 */
	len = CSC_MEM_DEFAULT | CSC_MEM_SETPG(1,2);
	msize = TMEM_OVERHEAD + sizeof(int) + sizeof(int) + TMEM_GUARD(len) + sizeof(int);
	p[0] = csc_tmem_init(buf, msize, len);
	cclog(-1, "Create heap(%d,%d) with %ld bytes: empty allocation disabled.\n", 
			CSC_MEM_PAGE(len), CSC_MEM_GUARD(len), msize);
	if (p[0] == NULL) return;

	p[0] = tmem_start(buf);
	n[0] = TMEM_SIZE(*p[0]);
	n[1] = GUARD_WORD(buf) + 2;
	cclog(n[0] == n[1], "TMEM_SIZE of the heap: %d\n", n[0]);
	p[1] = TMEM_NEXT(p[0]);
	cclog(p[1] == p[0]+n[1]+1, "TMEM_NEXT of the heap: +%d\n", 
			BMEM_SPAN(buf, p[1]));

	p[1] = p[0] + 1;
	n[1] = TMEM_SIZE(*p[1]);
	cclog(n[1] == (int)GUARD_WORD(buf) + 1, "TMEM_SIZE of the first block: %d\n", n[1]);
	p[2] = TMEM_NEXT(p[1]);
	cclog(p[2] == TMEM_NEXT(p[0]), "TMEM_NEXT of the first block: +%d\n", BMEM_SPAN(buf, p[2]));

	p[3] = csc_tmem_alloc(buf, 0);
	cclog(!p[3], "Allocating 0 byte from the heap: disabled %p\n", p[3]);

	p[3] = csc_tmem_alloc(buf, 1);
	cclog(p[3] == tmem_find_client(buf, p[1], NULL), 
			"Allocating 1 byte from the heap: +%d\n", BMEM_SPAN(buf, p[3]));
	msize = csc_tmem_attrib(buf, p[3], &n[3]);
	cclog((n[3] == 1) && (msize == sizeof(int)), "Verify memory +%d: %s %ld bytes\n", 
			BMEM_SPAN(buf, p[3]), n[3] ? "used" : "free", msize);

	p[2] = tmem_find_control(buf, p[3]);
	cclog(TMEM_NEXT(p[2]) == TMEM_NEXT(p[0]), "Verify the end of the heap: +%d\n", 
			BMEM_SPAN(buf, TMEM_NEXT(p[2])));

	csc_tmem_free(buf, p[3]);
	msize = csc_tmem_attrib(buf, p[3], &n[3]);
	cclog((n[3] == 0) && (msize == sizeof(int)), 
			"Attribution of the freed block: %d %ld\n", n[3], msize);

	/* create heap with 3 memory block: HEAP+MB0+PL0+MB1+PL1+MB2+PL2 */
	len = CSC_MEM_DEFAULT | CSC_MEM_SETPG(1,2);
	msize = TMEM_OVERHEAD + sizeof(int) + (sizeof(int) + TMEM_GUARD(len) + sizeof(int)) * 3;
	p[0] = csc_tmem_init(buf, msize, len);
	cclog(-1, "Create heap(%d,%d) with %ld bytes: 3 minimum blocks.\n", 
			CSC_MEM_PAGE(len), CSC_MEM_GUARD(len), msize);
	if (p[0] != buf) return;

	/* testing merging: create a hole in middle  */
	p[0] = csc_tmem_alloc(buf, 1);
	p[1] = csc_tmem_alloc(buf, 1);
	p[2] = csc_tmem_alloc(buf, 1);
	csc_tmem_free(buf, p[1]);
	cclog(p[0]&&p[1]&&p[2], "Create a memory hole in the middle: +%d\n", BMEM_SPAN(buf, p[1]));

	/* testing merging: merging down the next free block */
	csc_tmem_free(buf, p[2]);
	msize = csc_tmem_attrib(buf, p[2], &n[2]);	/* should be cleaned */
	cclog(msize==(size_t)-1, "Merging down the memory: control block cleaned (%d)\n", n[2]);
	msize = csc_tmem_attrib(buf, p[1], &n[1]);
	p[3] = tmem_find_control(buf, p[1]);
	cclog(!n[1] && TMEM_SIZE(*p[3]) == TMEM_DNWORD(TMEM_GUARD(len)) * 2 + 3, 
			"Merged down the memory: %d words (%ld)\n", TMEM_SIZE(*p[3]), msize);

	/* testing merging: merging up the previou free block */
	csc_tmem_free(buf, p[0]);
	msize = csc_tmem_attrib(buf, p[1], &n[1]);	/* should be cleaned */
	cclog(msize==(size_t)-1, "Merging up the memory: control block cleaned (%d)\n", n[1]);
	msize = csc_tmem_attrib(buf, p[0], &n[0]); 
	p[3] = tmem_find_control(buf, p[0]);
	cclog(!n[0] && TMEM_SIZE(*p[3]) == TMEM_DNWORD(TMEM_GUARD(len)) * 3 + 5,
			"Merged up the memory: %d words (%ld)\n", TMEM_SIZE(*p[3]), msize);

	/* testing merging: create a hole in middle  */
	p[0] = csc_tmem_alloc(buf, 1);
	p[1] = csc_tmem_alloc(buf, 1);
	p[2] = csc_tmem_alloc(buf, 1);
	csc_tmem_free(buf, p[1]);
	cclog(p[0]&&p[1]&&p[2], "Create a memory hole in the middle: +%d\n", BMEM_SPAN(buf, p[1]));

	/* testing merging: merging up the previou free block */
	csc_tmem_free(buf, p[0]);
	msize = csc_tmem_attrib(buf, p[1], &n[1]);	/* should be cleaned */
	cclog(msize==(size_t)-1, "Merging up the memory: control block cleaned (%d)\n", n[1]);
	msize = csc_tmem_attrib(buf, p[0], &n[0]); 
	p[3] = tmem_find_control(buf, p[0]);
	cclog(!n[0] && TMEM_SIZE(*p[3]) == TMEM_DNWORD(TMEM_GUARD(len)) * 2 + 3,
			"Merged up the memory: %d words (%ld)\n", TMEM_SIZE(*p[3]), msize);

	/* testing merging: merging down the next free block */
	csc_tmem_free(buf, p[2]);
	msize = csc_tmem_attrib(buf, p[2], &n[2]);	/* should be cleaned */
	cclog(msize==(size_t)-1, "Merging down the memory: control block cleaned (%d)\n", n[2]);
	msize = csc_tmem_attrib(buf, p[0], &n[0]);
	p[3] = tmem_find_control(buf, p[0]);
	cclog(!n[0] && TMEM_SIZE(*p[3]) == TMEM_DNWORD(TMEM_GUARD(len)) * 3 + 5, 
			"Merged down the memory: %d words (%ld)\n", TMEM_SIZE(*p[3]), msize);

	/* testing tri-merging: create an island in middle  */
	p[0] = csc_tmem_alloc(buf, 1);
	p[1] = csc_tmem_alloc(buf, 1);
	p[2] = csc_tmem_alloc(buf, 1);
	csc_tmem_free(buf, p[0]);
	csc_tmem_free(buf, p[2]);
	cclog(!!p[1], "Create a memory island in the middle: +%d\n", BMEM_SPAN(buf, p[1]));

	csc_tmem_free(buf, p[1]);
	msize = csc_tmem_attrib(buf, p[1], &n[1]);	/* should be cleaned */
	cclog(msize==(size_t)-1, "Tri-merging: control block 1 cleaned (%d)\n", n[1]);
	msize = csc_tmem_attrib(buf, p[2], &n[2]);	/* should be cleaned */
	cclog(msize==(size_t)-1, "Tri-merging: control block 2 cleaned (%d)\n", n[2]);
	msize = csc_tmem_attrib(buf, p[0], &n[0]); 
	p[3] = tmem_find_control(buf, p[0]);
	cclog(!n[0] && TMEM_SIZE(*p[3]) == TMEM_DNWORD(TMEM_GUARD(len)) * 3 + 5,
			"Tri-merged: %d words (%ld)\n", TMEM_SIZE(*p[3]), msize);
}

static void tmem_test_misc_memory(void *buf, int len)
{
	int	n, k, *p, *q;
	size_t	msize;

	msize = sizeof(int)*20;
	len = 0;
	p = csc_tmem_init(buf, msize, len);
	cclog(-1, "Create heap(%d,%d) with %d bytes for misc test.\n", 
			CSC_MEM_PAGE(len), CSC_MEM_GUARD(len), msize);
	if (p == NULL) return;

	n = csc_tmem_free(NULL, NULL);
	cclog(n == CSC_MERR_RANGE, "Free NULL heap: %d\n", n);
	n = csc_tmem_free(buf, NULL);
	cclog(n == CSC_MERR_RANGE, "Free NULL memory: %d\n", n);

	/* test the memory initial clean function */
	q = csc_tmem_alloc(buf, 1);
	csc_tmem_free(buf, q);
	*q = -1;
	len |= CSC_MEM_CLEAN;
	tmem_config_set(buf, len);
	p = csc_tmem_alloc(buf, 1);
	cclog(*p == 0, "Memory is initialized to %x\n", *p);
	
	/* double free test */
	n = csc_tmem_free(buf, p);
	k = csc_tmem_free(buf, p);
	cclog(n == 0 && k == 0, "Memory is double freed. [%d]\n", k);

	/* test the memory initial non-clean function */
	len &= ~CSC_MEM_CLEAN;
	tmem_config_set(buf, len);
	*q = -1;
	p = csc_tmem_alloc(buf, 1);
	cclog(*p == -1, "Memory is not initialized. [%x]\n", *p);
}

static void tmem_test_fitness(void *buf, int len)
{
	int	u = 0, f = 0;

	int used(void *mem)
	{
		int	*mb = tmem_find_control(buf, mem);
		u = (u << 4) | (TMEM_SIZE(*mb) - GUARD_WORD(buf)); 
		return 0;
	}

	int loose(void *mem)
	{
		int	*mb = tmem_find_control(buf, mem);
		f = (f << 4) | (TMEM_SIZE(*mb) - GUARD_WORD(buf)); 
		return 0;
	}

	void *memory_set_pattern(void *heap, int config)
	{
		int	*p[4];

		/* the memory pattern will be: F1+U1+F4+U1+F2+U1+F8
		 * total 7 section so 7 set of guarding areas.
		 * The target memory is 2 words */

		len = sizeof(int)*30 + TMEM_GUARD(config)*7;
		if (csc_tmem_init(heap, len, config) == NULL) {
			return NULL;
		}

		p[0] = csc_tmem_alloc(heap, sizeof(int));
		csc_tmem_alloc(heap, sizeof(int));
		p[1] = csc_tmem_alloc(heap, sizeof(int)*4);
		csc_tmem_alloc(heap, sizeof(int));
		p[2] = csc_tmem_alloc(heap, sizeof(int)*2);
		csc_tmem_alloc(heap, sizeof(int));
		csc_tmem_free(heap, p[0]);
		csc_tmem_free(heap, p[1]);
		csc_tmem_free(heap, p[2]);

		u = f = 0;
		csc_tmem_scan(heap, used, loose);
		cclog((u==0x111)&&(f==0x142b), "Create heap(%d,%d) with 4 holes [%x %x]\n", 
			CSC_MEM_PAGE(config), CSC_MEM_GUARD(config), u, f);
		return heap;
	}


	cclog(-1, "Fitness test of the allocation\n");
	if (memory_set_pattern(buf, CSC_MEM_FIRST_FIT) == NULL) return;
	csc_tmem_alloc(buf, sizeof(int)*2);
	u = f = 0;
	csc_tmem_scan(buf, used, loose);
	cclog(u == 0x1211 && f == 0x112b, "Allocated 2 words by First Fit method [%x %x]\n", u, f);

	if (memory_set_pattern(buf, CSC_MEM_BEST_FIT | CSC_MEM_SETPG(1,2)) == NULL) return;
	csc_tmem_alloc(buf, sizeof(int)*2);
	u = f = 0;
	csc_tmem_scan(buf, used, loose);
	cclog(u == 0x1121 && f == 0x14b, "Allocated 2 words by Best Fit method [%x %x]\n", u, f);

	if (memory_set_pattern(buf, CSC_MEM_WORST_FIT | CSC_MEM_SETPG(0,1)) == NULL) return;
	csc_tmem_alloc(buf, sizeof(int)*2);
	u = f = 0;
	csc_tmem_scan(buf, used, loose);
	cclog(u == 0x111b && f == 0x142, "Allocated 2 words by Worst Fit method [%x %x]\n", u, f);
}

#endif	/* CFG_UNIT_TEST */

