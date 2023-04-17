/*!\file       csc_bmem.c
   \brief      dynamic memory management based on bitmaps

   The file supports a group of functions of dynamic memory 
   management based on bitmaps.

   \author     "Andy Xuming" <xuming@users.sourceforge.net>
   \date       2019
*/
/* Copyright (C) 1998-2019  "Andy Xuming" <xuming@users.sourceforge.net>

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
/*
 Blank memory:
  |-------------------------------------------------------------------------|
 After initialized:
  [BMMCB***]----------------------------------------------------------------|
 Allocated:
  [BMMCB***][BMMPC*][PAGES]----[BMMPC*][PAGES]------------------------------|
 [BMMCB***]:
  [BMMCB+bitmap][bitmap][bitmap]...
 [BMMPC*][PAGES]:
  [BMMPC+frontpad][guards][page1][page2]...[pageN+backpad][guards]
  - frontpad and backpad are always part of guards; 
 In [BMMCB], page has 12 setting: 32/64/128/256/512/1k/2k/4k/8k/16k/32k/64k
   so that the padding size can be limited in 64k 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libcsoup.h"

/*#define BMEM_SLOWMOTION*/
#define BMEM_MAGPIE		0xCA

static	const unsigned char	bmtab[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };
#define BM_CK_PAGE(bm, idx)	((bm)[(idx)/8] & bmtab[(idx)&7])
#define BM_SET_PAGE(bm, idx)	((bm)[(idx)/8] |= bmtab[(idx)&7])
#define BM_CLR_PAGE(bm, idx)	((bm)[(idx)/8] &= ~bmtab[(idx)&7])

#define BMEM_SPAN(f,t)		((size_t)((char*)(f) - (char*)(t)))


/* Bitmap Memory Manager Page Controller */
typedef struct	_BMMPC	{
	unsigned char	magic[4];	/* CRC8 + MAGIC + PAD1 + PAD2 */
	int	pages;		/* occupied pages, includes BMMPC and guards */
} BMMPC;

/* Bitmap Memory Manager Control Block */
typedef	struct	_BMMCB	{
	unsigned char	magic[4];	/* CRC8 + MAGIC + CONFIG1 + CONFIG2 */
	int	pages;		/* control block used pages, inc. BMMCB and bitmap */

	/*char	*trunk;*/	/* point to the head of the page array */
	int	total;		/* number of allocable pages */
	int	avail;		/* number of available pages */

	unsigned char	bitmap[1];
} BMMCB;


static BMMPC *bmem_verify(BMMCB *bmc, void *mem, int *err);
static void bmem_page_alloc(BMMCB *bmc, int idx, int pages);
static void bmem_page_free(BMMCB *bmc, int idx, int pages);
static int bmem_page_find(BMMCB *bmc, int idx);
static void *bmem_find_client(BMMCB *bmc, BMMPC *mpc, size_t *osize);
static BMMPC *bmem_find_control(BMMCB *bmc, void *mem);

static inline void bmem_set_crc(void *mb, int len)
{
	register char   *p = mb;
	p[1] = (char) CSC_MEM_MAGIC_BITMAP;
	p[0] = (char) csc_crc8(0, p+1, len-1);
}

static inline int bmem_check(void *mb, int len)
{
	register char   *p = mb;
	return (p[1] == (char) CSC_MEM_MAGIC_BITMAP) &&
		(p[0] == (char) csc_crc8(0, p+1, len-1));
}

static inline void bmem_config_set(BMMCB *bmc, int config)
{
	bmc->magic[2] = (unsigned char)(config & 0xff);
	bmc->magic[3] = (unsigned char)((config >> 8) & 0xff);
}

static inline int bmem_config_get(BMMCB *bmc)
{
	return (int)((bmc->magic[3] << 8) | bmc->magic[2]);
}

static inline int bmem_service_pages(BMMCB *bmc)
{
	register int	config = bmem_config_get(bmc);

	/* MemCB + front and back guards */
	return 1 + CSC_MEM_GUARD(config) + CSC_MEM_GUARD(config);
}

static inline void bmem_pad_set(BMMPC *mpc, int padding)
{
	mpc->magic[2] = (unsigned char)(padding & 0xff);
	mpc->magic[3] = (unsigned char)((padding >> 8) & 0xff);
}

static inline int bmem_pad_get(BMMPC *mpc)
{
	return (int)((mpc->magic[3] << 8) | mpc->magic[2]);
}

static inline size_t bmem_page_to_size(BMMCB *bmc, int page)
{
	/* no more than 64KB per page */
	return (size_t)page * CSC_MEM_PAGE(bmem_config_get(bmc));
}

static inline int bmem_size_to_page(BMMCB *bmc, size_t size)
{
	/* no more than 64KB per page */
	register int n = CSC_MEM_PAGE(bmem_config_get(bmc));
	return (int)((size + n - 1) / n);
}

static inline int bmem_addr_to_index(BMMCB *bmc, void *mem)
{
	/* no more than 64KB per page */
	return (int)(((char*)mem - (char*)bmc) / 
			CSC_MEM_PAGE(bmem_config_get(bmc)));
}

static inline void *bmem_index_to_addr(BMMCB *bmc, int idx)
{
	return (char*)bmc + idx * CSC_MEM_PAGE(bmem_config_get(bmc));
}

/*!\brief Initialize the memory heap to be allocable.

   \param[in]  mem the memory heap for allocation.
   \param[in]  mlen the size of the memory heap.
   \param[in]  flags the bit combination of the memory manage settings.
               See CSC_MEM_xxx macro in libcsoup.h for details.

   \return    The pointer to the memory heap object, or NULL if failed.

   \remark The minimum allocation unit is 'page' which is defined in Bit4-7 
   of the 'flags'. See CSC_MEM_xxx macro in libcsoup.h for details.
*/
void *csc_bmem_init(void *mem, size_t mlen, int flags)
{
	BMMCB	*bmc;
	int	bmpage, allpage, minpage;

	if ((bmc = mem) == NULL) {
		return NULL;	/* CSC_MERR_INIT */
	}

	/* estimate how many page are there in total */
	allpage = (int)(mlen / CSC_MEM_PAGE(flags));

	/* based on page numbers calculate the pages of the heap control block */
	bmpage = (int)(sizeof(BMMCB) + allpage / 8 + CSC_MEM_PAGE(flags) - 1);
	bmpage /= CSC_MEM_PAGE(flags);

	/* minimum required pages: HeapCB + MemCB + FrontGUARD + BackGUARD */
	minpage = bmpage + 1 + CSC_MEM_GUARD(flags) + CSC_MEM_GUARD(flags);

	/* minimum pool size depends on the minimum pages can be allocated */
	if (allpage < minpage) {
		return NULL;	/* CSC_MERR_LOWMEM */
	}
	if ((allpage == minpage) && ((flags & CSC_MEM_ZERO) == 0)) {
		return NULL;	/* CSC_MERR_LOWMEM */
	}

	/* set up the control block.  Note that the control block is also part of 
	 * the memory scheme so it will take some bits in the bitmap. */
	memset((void*)bmc, 0, bmpage * CSC_MEM_PAGE(flags));
	bmem_config_set(bmc, flags);
	bmc->pages = bmpage;
	bmc->total = allpage;
	bmc->avail = allpage - bmpage;
	bmem_page_alloc(bmc, 0, bmpage);		/* set the bitmap */	
	bmem_set_crc(bmc, bmem_page_to_size(bmc, bmc->pages));
	return bmc;
}

/*!\brief allocate a piece of dynamic memory block inside the specified 
    memory heap.

   \param[in]  heap the memory heap for allocation.
   \param[in]  size the size of the expecting allocated memory block.

   \return    point to the allocated memory block in the memory heap. 
              or NULL if not enough space for allocating.

   \remark The strategy of allocation is defined by CSC_MEM_FITNESS field
*/
void *csc_bmem_alloc(void *heap, size_t size)
{
	BMMCB	*bmc = heap;
	BMMPC	*mpc;
	int	pages, config, padding;
	int	fnd_idx, fnd_pages = -1;

	int loose(void *mem)
	{
		mpc = bmem_find_control(bmc, mem);

		/* when it's been called, the "pages" should've been set */
		if (mpc->pages >= pages) {
			if (fnd_pages == -1) {
				fnd_pages = mpc->pages;
				fnd_idx = bmem_addr_to_index(bmc, mpc);
			}
			switch (config & CSC_MEM_FITMASK) {
			case CSC_MEM_BEST_FIT:
				if (fnd_pages > mpc->pages) {
					fnd_pages = mpc->pages;
					fnd_idx = bmem_addr_to_index(bmc, mpc);
				}
				break;
			case CSC_MEM_WORST_FIT:
				if (fnd_pages < mpc->pages) {
					fnd_pages = mpc->pages;
					fnd_idx = bmem_addr_to_index(bmc, mpc);
				}
				break;
			default:	/* CSC_MEM_FIRST_FIT */
				/*printf("goose: now=%d prev=%d\n",  mpc->pages, pages);*/
				return 1;
			}
			/*printf("loose: now=%d prev=%d found=%d\n", mpc->pages, pages, fnd_pages);*/
		}
		return 0;
	}

	if (!bmem_verify(bmc, (void*)-1, NULL)) {
		return NULL;
	}
	config = (int)bmem_config_get(bmc);

	pages = bmem_size_to_page(bmc, size);
	if (!pages && !(config & CSC_MEM_ZERO)) {
		return NULL;	/* CSC_MERR_RANGE: not allow empty allocation */
	}

	/* find the size of the tail padding */
	padding = bmem_page_to_size(bmc, pages) - size;

	/* add up the service pages: the BMMPC, front and back guards */
	pages += bmem_service_pages(bmc);
	if (pages > bmc->avail) {
		return NULL;	/* CSC_MERR_RANGE */
	}

	/* find a group of free pages where meets the requirement */
	fnd_pages = -1;
	if (csc_bmem_scan(heap, NULL, loose)) {
		return NULL;	/* CSC_MERR_BROKEN: chain broken */
	}
	if (fnd_pages == -1) {
		return NULL;	/* CSC_MERR_LOWMEM */
	}

	/* take the free pages */
	bmem_page_alloc(bmc, fnd_idx, pages);
	bmc->avail -= pages;
	bmem_set_crc(bmc, bmem_page_to_size(bmc, bmc->pages));
	
	/* setup the Bitmap Memory Manager Page Controller */
	mpc = (BMMPC*)bmem_index_to_addr(bmc, fnd_idx);
	memset(mpc, 0, sizeof(BMMPC));
	mpc->pages = pages;
	bmem_pad_set(mpc, padding);
	bmem_set_crc(mpc, sizeof(BMMPC));

	/* initial the memory */
	heap = bmem_find_client(bmc, mpc, NULL);	/* find client area */
 	if (config & CSC_MEM_CLEAN) {
		memset(heap, 0, size);
	}
	return heap;
}

/*!\brief free the allocated memory block.

   \param[in]  heap the memory heap for allocation.
   \param[in]  mem the memory block.

   \return    0 if freed successfully, or error code smaller than 0
   \remark    If freeing a freed memory block, it returns 0.
*/
int csc_bmem_free(void *heap, void *mem)
{
	BMMCB	*bmc = heap;
	BMMPC	*mpc;
	size_t	idx;
	int	err;

	if ((mpc = bmem_verify(bmc, mem, &err)) == NULL) {
		return err;	/* invalided memory management */
	}

	/* set free of these pages */
	idx = bmem_addr_to_index(bmc, mpc);
	bmem_page_free(bmc, idx, mpc->pages);
	mpc->magic[1] = (unsigned char) ~CSC_MEM_MAGIC_BITMAP;	/* set free of the page controller */
	bmem_set_crc(mpc, sizeof(BMMPC));

	bmc->avail += mpc->pages;
	bmem_set_crc(bmc, bmem_page_to_size(bmc, bmc->pages));
	return 0;
}

/*!\brief scan and inspecting the heap.

   \param[in]  heap the memory heap.
   \param[in]  used the callback function when scanned an allocated memory block.
   \param[in]  loose  the callback function when scanned a freed memory block.

   \return    NULL if successfully scanned to the end of heap, or break the scan 
   by the callback functions. Otherwise it returns the pointer to the faulty memory block.
   
   \remark    The scan function will scan the heap from the first memory block to the last.
   When it found an allocated memory block, it will call the used() function with the address
   of the allocated memory block. When it found a freed memory block, it will call the loose() 
   function with the address of the freed memory block.

   \remark The prototype of the callback functions are: int func(void *)
           The scan process will stop in middle if func() returns non-zero.
*/
void *csc_bmem_scan(void *heap, int (*used)(void*), int (*loose)(void*))
{
	BMMCB	*bmc = heap;
	BMMPC	*mpc;
	void	*client;
	int	i;

	if (!bmem_verify(bmc, (void*)-1, NULL)) {
		return bmc;	/* invalided memory management */
	}

	for (i = bmc->pages; i < bmc->total; i++) {
		mpc = bmem_index_to_addr(bmc, i);
		if (BM_CK_PAGE(bmc->bitmap, i)) {
			/* found an allocated memory block */
			client = bmem_find_client(bmc, mpc, NULL);
			if (!bmem_verify(bmc, client, NULL)) {
				return mpc;
			}
			if (used && used(client)) {
				return NULL;
			}
		} else {
			/* re-initialize the freed pages */
			memset(mpc, 0, sizeof(BMMPC));
			mpc->pages = bmem_page_find(bmc, i);
			bmem_set_crc(mpc, sizeof(BMMPC));
			client = bmem_find_client(bmc, mpc, NULL);
			if (loose && loose(client)) {
				return NULL;
			}
		}
		i += mpc->pages - 1; /* skip the allocated pages */
	}
	return NULL;
}

/*!\brief find the attribution of the memory block.

   \param[in]  heap the memory heap.
   \param[in]  mem the memory block.
   \param[out] state the state of the memory block. 
               0=free 1=used, or <0 in error codes.

   \return    the size of the allocated memory without padding when succeed,
              or (size_t) -1 in error. the error code returns in 'state'.

   \remark  To avoid ambiguous of the memory block, the csc_bmem_attrib() don't
   accept the uninitialized block. 
   For example: [memblock/4-page][memblock/0-page][A:free pages to end]
   What should csc_bmem_attrib(heap, A, state) return, the [memblock/0-page] or
   [A:free pages to end]? So the best practice is not to accept the [A:free pages to end] 
*/
size_t csc_bmem_attrib(void *heap, void *mem, int *state)
{
	BMMCB	*bmc = heap;
	BMMPC	*mpc;
	int	idx;

	if ((mpc = bmem_verify(bmc, mem, state)) == NULL) {
		return (size_t) -1;	/* invalided memory block */
	}

	/* if the request memory is allocated, then return its attributes */
	idx = bmem_addr_to_index(bmc, mpc);
	if (BM_CK_PAGE(bmc->bitmap, idx)) {
		if (state) {
			*state = 1;	/* occupied */
		}
		idx = mpc->pages - bmem_service_pages(bmc);
		return bmem_page_to_size(bmc, idx) - bmem_pad_get(mpc);
	}

	/* if the request memory is freed, then search to the end in case of
	 * more than one freed blocks were adjacent */
	if (state) {
		*state = 0;
	}

	/* FIXME: can happen in theory: the remain memory is smaller than
         * the guarding area. It can not be sorted out in the allocation
         * function */
	idx = bmem_page_find(bmc, idx) - bmem_service_pages(bmc);
	if (idx < 0) {
		*state = idx;
		return (size_t) -1;
	}
	return bmem_page_to_size(bmc, idx);
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
void *csc_bmem_front_guard(void *heap, void *mem, int *xsize)
{
	BMMCB	*bmc = heap;
	BMMPC	*mpc;
	int	pages;

	if ((mpc = bmem_verify(bmc, mem, xsize)) == NULL) {
		return NULL;	/* invalided memory management */
	}

	/* make sure the memory block was allocated. pointless to guard a free block */
	pages = bmem_addr_to_index(bmc, mpc);
	if (!BM_CK_PAGE(bmc->bitmap, pages)) {
		if (xsize) {
			*xsize = 0;	/* free memory block */
		}
		return NULL;
	}

	if (xsize) {
		pages = 1 + CSC_MEM_GUARD(bmem_config_get(bmc));
		*xsize = (int)(bmem_page_to_size(bmc, pages) - sizeof(BMMPC));
	}
	return (char*)(mpc+1);
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
void *csc_bmem_back_guard(void *heap, void *mem, int *xsize)
{
	BMMCB	*bmc = heap;
	BMMPC	*mpc;
	int	glen;

	if ((mpc = bmem_verify(bmc, mem, xsize)) == NULL) {
		return NULL;	/* invalided memory management */
	}

	/* make sure the memory block was allocated. pointless to guard a free block */
	glen = bmem_addr_to_index(bmc, mpc);
	if (!BM_CK_PAGE(bmc->bitmap, glen)) {
		if (xsize) {
			*xsize = 0;	/* free memory block */
		}
		return NULL;
	}

	glen = CSC_MEM_GUARD(bmem_config_get(bmc));
	glen = (int)bmem_page_to_size(bmc, glen) + bmem_pad_get(mpc);
	if (xsize) {
		*xsize = glen;
	}
	return (char*)mpc + bmem_page_to_size(bmc, mpc->pages) - glen;
}


/*!\brief verify the heap and the memory block.

   \param[in]  heap the memory heap.
   \param[in]  mem the allocated memory block.
   \param[out] err 0 if memory is validate, otherwise the error code.

   \return    the pointer to the memory management block, or NULL if failed.
*/
static BMMPC *bmem_verify(BMMCB *bmc, void *mem, int *err)
{
	int	tmperr;

	if (err == NULL) {
		err = &tmperr;
	}
	if (bmc == NULL) {
		*err = CSC_MERR_INIT; 	/* heap not created */
		return NULL;
	}
	if (!bmem_check(bmc, bmem_page_to_size(bmc, bmc->pages))) {
		*err = CSC_MERR_INIT;
		return NULL;
	}

	/* Only verify the BMMCB. 
	 * Not using NULL because NULL can be a to-be-verified pointer */
	if (mem == (void*) -1) {
		*err = 0;
		return (BMMPC*) bmc;
	}

	/* make sure the client memory is in range */
	if ((mem > bmem_index_to_addr(bmc, bmc->total)) ||
			(mem < bmem_index_to_addr(bmc, bmc->pages))) {
		*err = CSC_MERR_RANGE;	/* memory out of range */
		return NULL;
	}

	mem = bmem_find_control(bmc, mem);	/* mem become mpc */

	/* Note that bmem_verify() only verify the memory blocks which have
	 * a BMMPC structure. Uninitialized will be treated as broken */
	if (!bmem_check(mem, sizeof(BMMPC))) {
		*err = CSC_MERR_BROKEN; /* broken memory controller */
		return NULL;
	}
	*err = 0;
	return mem;
}

static void bmem_page_alloc_slow(BMMCB *bmc, int idx, int pages)
{
	int	i;

	for (i = idx; i < idx + pages; i++) {
		BM_SET_PAGE(bmc->bitmap, i);
	}
}

#ifdef	BMEM_SLOWMOTION
static void bmem_page_alloc(BMMCB *bmc, int idx, int pages)
{
	bmem_page_alloc_slow(bmc, idx, pages);
}
#else
static const char	mapsetfront[8] = { 0xff, 0x7f, 0x3f, 0x1f, 0xf, 7, 3, 1 };
static const char	mapsetback[8]  = { 0, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe };

static void bmem_page_alloc(BMMCB *bmc, int idx, int pages)
{
	//printf("bmem_page_alloc: %d %d\n", idx, pages);
	if (pages <= 8) {	/* too short to be worth of bit trick */
		bmem_page_alloc_slow(bmc, idx, pages);
	} else {
		/* set the first byte in the bitmap, especially when it's uneven */
		bmc->bitmap[idx / 8] |= mapsetfront[idx & 7];
		pages -= 8 - (idx & 7);
		idx   += 8 - (idx & 7);

		memset(&bmc->bitmap[idx / 8], 0xff, pages / 8);
		if (pages & 7) {
			idx += pages / 8 * 8;
			bmc->bitmap[idx / 8] |= mapsetback[pages & 7];
		}
	}
}
#endif	/* BMEM_SLOWMOTION */

static void bmem_page_free_slow(BMMCB *bmc, int idx, int pages)
{
	int	i;

	for (i = idx; i < idx + pages; i++) {
		BM_CLR_PAGE(bmc->bitmap, i);
	}
}

#ifdef	BMEM_SLOWMOTION
static void bmem_page_free(BMMCB *bmc, int idx, int pages)
{
	bmem_page_free_slow(bmc, idx, pages);
}
#else
static const char	mapclrfront[8] = { 0, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe };
static const char	mapclrback[8]  = { 0, 0x7f, 0x3f, 0x1f, 0xf, 0x7, 0x3, 0x1 };

static void bmem_page_free(BMMCB *bmc, int idx, int pages)
{
	if (pages <= 8) {	/* too short to be worth of bit trick */
		bmem_page_free_slow(bmc, idx, pages);
	} else {
		/* clean the first byte in the bitmap, especially when it's uneven */
		bmc->bitmap[idx / 8] &= mapclrfront[idx & 7];
		pages -= 8 - (idx & 7);
		idx   += 8 - (idx & 7);

		memset(&bmc->bitmap[idx / 8], 0, pages / 8);
		if (pages & 7) {
			idx += pages / 8 * 8;
			bmc->bitmap[idx / 8] &= mapclrback[pages & 7];
		}
	}
}
#endif	/* BMEM_SLOWMOTION */

static int bmem_page_find_slow(BMMCB *bmc, int idx)
{
	int	i;
	
	for (i = idx; i < bmc->total; i++) {
		if (BM_CK_PAGE(bmc->bitmap, i)) {
			break;
		}
	}
	return i - idx;
}

#ifdef	BMEM_SLOWMOTION
static int bmem_page_find(BMMCB *bmc, int idx)
{
	return bmem_page_find_slow(bmc, idx);
}
#else
static int bmem_page_find(BMMCB *bmc, int idx)
{
	int	i, n;

	i = idx / 8;
	if ((bmc->bitmap[i] & mapsetfront[idx & 7]) ||
			((bmc->total - idx) <= 8)) {
		return bmem_page_find_slow(bmc, idx);
	}

	n = 8 - (idx & 7);
	for (i++; i < bmc->total / 8; i++) {
		if (bmc->bitmap[i]) {
			return n + bmem_page_find_slow(bmc, i * 8);
		}
		n += 8;
	}
	if (bmc->total & 7) {
		n += bmem_page_find_slow(bmc, i * 8);
	}
	return n;
}
#endif	/* BMEM_SLOWMOTION */

static void *bmem_find_client(BMMCB *bmc, BMMPC *mpc, size_t *osize)
{
	int	idx, pages, config = bmem_config_get(bmc);

	/* service pages are head, extra pages and front guards */
	idx = 1 + CSC_MEM_GUARD(config);
	if (osize) {
		pages = mpc->pages - idx - CSC_MEM_GUARD(config);	/* back guard */
		*osize = bmem_page_to_size(bmc, pages) - bmem_pad_get(mpc); 
	}
	return (char*)mpc + bmem_page_to_size(bmc, idx);
}

static BMMPC *bmem_find_control(BMMCB *bmc, void *mem)
{
	int	pages, config = bmem_config_get(bmc);

	/* find head, extra pages and front guards */
	pages = 1 + CSC_MEM_GUARD(config);
	return (BMMPC*)((char*)mem - bmem_page_to_size(bmc, pages));
}

#ifdef	CFG_UNIT_TEST
#include "libcsoup_debug.h"

static void csc_bmem_function_test(char *buf, int blen);
static void csc_bmem_minimum_test(char *buf, int blen);
static void csc_bmem_fitness_test(char *buf, int blen);
static void csc_bmem_bitmap_test(char *buf, int blen);
static char *show_bitmap(BMMCB *bmc, int len);

int csc_bmem_unittest(void)
{
	char	buf[32*1024];

	csc_bmem_function_test(buf, sizeof(buf));
	csc_bmem_minimum_test(buf, sizeof(buf));
	csc_bmem_fitness_test(buf, sizeof(buf));
	csc_bmem_bitmap_test(buf, sizeof(buf));
	return 0;
}

static void csc_bmem_function_test(char *buf, int len)
{
	BMMCB	*bmc;
	BMMPC	*mpc;
	int	i, k;
	char	*p, *tmp;
	size_t	msize, mpage;
	int	pglist[] = { 16, 32, 64, 128, 256, 512, 1024 };

	/* function tests: bmem_config_set() and bmem_config_get() */
	cclog(-1, "Testing internal functions.\n");
	bmc = (BMMCB*) buf;
	memset(bmc, 0, sizeof(BMMCB));
	bmem_config_set(bmc, 0xc1c2c3c4);
	cclog(bmc->magic[2] == 0xc4 && bmc->magic[3] == 0xc3, 
			"bmem_config_set: %x %x %x %x\n",
			bmc->magic[0], bmc->magic[1], bmc->magic[2], bmc->magic[3]);
	len = bmem_config_get(bmc);
	cclog(len == 0xc3c4, "bmem_config_get: %x\n", len);

	/* bmem_set_crc() and bmem_check() */
	bmem_set_crc(bmc, sizeof(BMMCB));
	cclog(bmc->magic[0] == 0x98, "bmem_set_crc: %x %x\n", bmc->magic[0], bmc->magic[1]);
	len = bmem_check(bmc, sizeof(BMMCB));
	cclog(len == 1, "bmem_check: checksum PASS= %x %x\n", bmc->magic[0], len);
	bmc->magic[0]++;
	len = bmem_check(bmc, sizeof(BMMCB));
	cclog(len != 1, "bmem_check: checksum FAIL= %x %x\n", bmc->magic[0], len);

	/* bmem_pad_set() and bmem_pad_get() */
	mpc = (BMMPC*)buf;
	memset(mpc, 0, sizeof(BMMPC));
	bmem_pad_set(mpc, 0xf1f2f3f4);
	cclog(mpc->magic[2] == 0xf4 && mpc->magic[3] == 0xf3, 
			"bmem_pad_set: %x %x %x %x\n",
			mpc->magic[0], mpc->magic[1], mpc->magic[2], mpc->magic[3]);
	len = bmem_pad_get(mpc);
	cclog(len==0xf3f4, "bmem_pad_get: %x\n", len);

	/* page size test: bmem_page_to_size(), bmem_size_to_page(),
	 * bmem_addr_to_index(), bmem_index_to_addr() */
	for (i = 0; i < 16; i++) {
		bmem_config_set(bmc, CSC_MEM_SETPG(i,0));
		msize = bmem_page_to_size(bmc, 1);
		mpage = bmem_size_to_page(bmc, 128 * 1024);
		len = bmem_addr_to_index(bmc, (char*)bmc + 128 * 1024);
		p = bmem_index_to_addr(bmc, 2);
		cclog(msize == (size_t)(32 << (i%12)), 
			"Page %d: PSize=%d Page=%d Index=%d P2Off=+%d\n", 
			i, msize, mpage, len, BMEM_SPAN(p, bmc));
	}

	/* bmem_find_xxx() family: bmem_find_client(), bmem_find_control(), 
	 * and bmem_service_pages() */
	cclog(-1, "Testing bmem_find_xxx():\n");
	bmc = (BMMCB*)buf;
	memset(bmc, 0, sizeof(BMMCB));
	bmc->pages = 1;
	bmc->total = 25;

	for (i = 0; i < 4; i++) {	/* page size 32/64/128/256 */
		for (k = 0; k < 4; k++) {	/* guardings */
			bmem_config_set(bmc, CSC_MEM_SETPG(i,k));
			bmem_set_crc(bmc, bmem_page_to_size(bmc, bmc->pages));
			mpc = bmem_index_to_addr(bmc, 1);
			memset(mpc, 0, sizeof(BMMPC));
			bmem_pad_set(mpc, 260);
			mpc->pages = 24;
			bmem_set_crc(mpc, sizeof(BMMPC));

			p = bmem_find_client(bmc, mpc, &msize);
			cclog(!!p, "PSize=%d Guard=%d - Client=+%ld:%ld ", 
				i, k, BMEM_SPAN(p, mpc), msize);
		
			tmp = csc_bmem_front_guard(bmc, p, &len);
			cslog("FrontG=+%ld:%d ", BMEM_SPAN(tmp, mpc), len);
			tmp = csc_bmem_back_guard(bmc, p, &len);
			cslog("BackG=+%ld:%d ", BMEM_SPAN(tmp, mpc), len);

			tmp = (char*)bmem_find_control(bmc, p);
			msize = bmem_service_pages(bmc);
			cslog("BMMPC=+%ld:%ld\n", BMEM_SPAN(tmp, bmc), msize);
		}
	}

	/* testing how to find the page index in the bitmap */
	tmp = buf + bmem_page_to_size(bmc, 10) + 12;
	msize = (size_t) bmem_addr_to_index(bmc, tmp);
	cclog(msize==10, "Found page index %ld at +%ld\n", msize, BMEM_SPAN(tmp, bmc));

	/* calculating the bitmap size in pages */
	cclog(-1, "BMMCB: Bitmap Calculator:\n");
	cclog(-1, "BMMCB: Page Size:    ");
	for (i = 0; i < (int)(sizeof(pglist)/sizeof(int)); i++) {
		cslog("%8d", pglist[i]);
	}
	cslog("\n");
	cclog(-1, "BMMCB: Map 1st Page: ");
	for (i = 0; i < (int)(sizeof(pglist)/sizeof(int)); i++) {
		cslog("%8d", (pglist[i] - sizeof(BMMCB)) * 8);
	}
	cslog("\n");
	cclog(-1, "BMMCB: Mapped size:  ");
	for (i = 0; i < (int)(sizeof(pglist)/sizeof(int)); i++) {
		cslog("%8d", (pglist[i] - sizeof(BMMCB)) * 8 * pglist[i]);
	}
	cslog("\n");
	cclog(-1, "BMMCB: Map 2nd Page: ");
	for (i = 0; i < (int)(sizeof(pglist)/sizeof(int)); i++) {
		cslog("%8d", pglist[i] * 8);
	}
	cslog("\n");
	cclog(-1, "BMMCB: Mapped size:  ");
	for (i = 0; i < (int)(sizeof(pglist)/sizeof(int)); i++) {
		cslog("%8d", pglist[i] * 8 * pglist[i]);
	}
	cslog("\n");
}

static void csc_bmem_minimum_test(char *buf, int blen)
{
	BMMCB	*bmc;
	int	config, rc[4];
	char	*p[4];
	
	(void) blen;

	/* failed to create the minimum heap: bmc=1 mpc=1 guard=0 */
	config = CSC_MEM_DEFAULT | CSC_MEM_SETPG(1,0);
	bmc = csc_bmem_init(buf, 2*CSC_MEM_PAGE(config), config);
	cclog(!bmc, "Create heap with empty allocation disabled: null 2 pages\n");

	/* successful to create the minimum heap: bmc=1 mpc=1 extra=0 guard=0 */
	config |= CSC_MEM_ZERO;
	bmc = csc_bmem_init(buf, 2*CSC_MEM_PAGE(config), config);
	if (!bmc) return;
	cclog(-1, "Created Heap(%d,%d) with empty allocation enabled: bmc=%d free=%d map=%s\n",
			CSC_MEM_PAGE(config), CSC_MEM_GUARD(config), bmc->pages, bmc->avail, 
			show_bitmap(bmc, 0));

	/* failed to allocate 1 byte from the empty heap */
	p[0] = csc_bmem_alloc(bmc, 1);
	cclog(!p[0], "Failed to allocate 1 byte from empty heap. [%02x]\n", bmc->bitmap[0]);

	/* succeeded to allocate 0 byte from the empty heap */
	p[0] = csc_bmem_alloc(bmc, 0);
	p[1] = (char*)bmem_find_control(bmc, p[0]);
	cclog(!!p[0], "Allocated an empty memory block. [%02x]\n", bmc->bitmap[0]); 
	rc[1] = (int)csc_bmem_attrib(bmc, p[0], rc);
	cclog(!rc[1], "Memory attribution: size=%d state=%d pad=%d\n", rc[1], rc[0],
			bmem_pad_get((BMMPC*)p[1]));

	/* failed to find the memory guards */
	p[2] = csc_bmem_front_guard(bmc, p[0], rc+2);
	p[3] = csc_bmem_back_guard(bmc, p[0], rc+3);
	cclog((rc[2]>0)&&!rc[3], "Memory guards: FrontGD=+%d/%d BackGD=+%d/%d\n", 
			BMEM_SPAN(p[2], p[1]), rc[2], BMEM_SPAN(p[3], p[1]), rc[3]);

	/* free the empty memory block */
	rc[0] = csc_bmem_free(bmc, p[0]);
	cclog(rc[0] == 0, "Freed the empty memory block. [%02x]\n", bmc->bitmap[0]);
	rc[0] = csc_bmem_free(bmc, p[0]);
	cclog(rc[0] == 0, "Freed twice the empty memory block. [%02x]\n", bmc->bitmap[0]);
	rc[1] = (int)csc_bmem_attrib(bmc, p[0], rc);
	cclog(rc[1] == 0 && bmem_find_control(bmc, p[0])->pages == 1, 
			"Memory destroied: pages=%d size=%d\n", 
			bmem_find_control(bmc, p[0])->pages, rc[1]);

	/* create the minimum heap: bmc=1 mpc=1 guard=1x2 */
	config = CSC_MEM_DEFAULT | CSC_MEM_SETPG(1,1);
	bmc = csc_bmem_init(buf, 4*CSC_MEM_PAGE(config), config);
	cclog(!bmc, "Create heap with empty allocation disabled: null 4 pages\n");

	config |= CSC_MEM_ZERO;
	bmc = csc_bmem_init(buf, 4*CSC_MEM_PAGE(config), config);
	if (!bmc) return;
	cclog(-1, "Created Heap(%d,%d): bmc=%d free=%d map=%s\n",
			CSC_MEM_PAGE(config), CSC_MEM_GUARD(config), 
			bmc->pages, bmc->avail, show_bitmap(bmc, 0));

	/* failed to allocate 1 byte from the empty heap */
	p[0] = csc_bmem_alloc(bmc, 1);
	cclog(!p[0], "Failed to allocate 1 byte from empty heap. [%02x]\n", bmc->bitmap[0]);

	/* allocated an empty memory block from the empty heap */
	p[0] = csc_bmem_alloc(bmc, 0);
	cclog(!!p[0], "Allocated an empty memory block. [%02x]\n", bmc->bitmap[0]); 
	rc[1] = (int)csc_bmem_attrib(bmc, p[0], rc);
	cclog(!rc[1], "Memory attribution: size=%d state=%d pad=%d\n", rc[1], rc[0],
			bmem_pad_get(bmem_find_control(bmc, p[0])));

	/* free the empty memory block */
	rc[0] = csc_bmem_free(bmc, p[0]);
	cclog(rc[0] >= 0, "Freed the empty memory block. [%02x]\n", bmc->bitmap[0]);
	rc[1] = (int)csc_bmem_attrib(bmc, p[0], rc);
	cclog(rc[1] == 0 && bmem_find_control(bmc, p[0])->pages == 3,
			"Memory destroied: pages=%d size=%d\n", 
			bmem_find_control(bmc, p[0])->pages, rc[1]);

	/* create the small heap: bmc=1 mpc=1 guard=0 */
	config = CSC_MEM_DEFAULT | CSC_MEM_SETPG(1,0);
	bmc = csc_bmem_init(buf, 12*CSC_MEM_PAGE(config), config);
	if (!bmc) return;
	cclog(-1, "Created Heap(%d,%d): bmc=%d free=%d map=%s\n",
			CSC_MEM_PAGE(config), CSC_MEM_GUARD(config), 
			bmc->pages, bmc->avail, show_bitmap(bmc, 0));

	/* allocating test */
	p[0] = csc_bmem_alloc(bmc, 1);
	p[1] = csc_bmem_alloc(bmc, CSC_MEM_PAGE(config) + 2);
	p[2] = csc_bmem_alloc(bmc, CSC_MEM_PAGE(config)*2 + 3);
	cclog(p[0]&&p[1]&&p[2], "Allocated 3 memory blocks: free=%d map=%s\n",
			bmc->avail, show_bitmap(bmc, 0));
	rc[1] = (int)csc_bmem_attrib(bmc, p[0], rc);
	cclog(rc[0], "Memory attribution: off=+%d size=%d state=%d pad=%d\n", 
			BMEM_SPAN(p[0], bmc), rc[1], rc[0],
			bmem_pad_get(bmem_find_control(bmc, p[0])));
	rc[1] = (int)csc_bmem_attrib(bmc, p[1], rc);
	cclog(rc[0], "Memory attribution: off=+%d size=%d state=%d pad=%d\n",
			BMEM_SPAN(p[1], bmc), rc[1], rc[0],
			bmem_pad_get(bmem_find_control(bmc, p[1])));
	rc[1] = (int)csc_bmem_attrib(bmc, p[2], rc);
	cclog(rc[0], "Memory attribution: off=+%d size=%d state=%d pad=%d\n",
			BMEM_SPAN(p[2], bmc), rc[1], rc[0],
			bmem_pad_get(bmem_find_control(bmc, p[2])));

	/* free memory test */
	rc[0] = csc_bmem_free(bmc, p[1]);
	cclog(rc[0] >= 0, "Freed the memory block in middle: free=%d map=%s\n", 
			bmc->avail, show_bitmap(bmc, 0));
	rc[1] = (int)csc_bmem_attrib(bmc, p[1], rc);
	cclog(rc[1], "Memory destroied: pages=%d size=%d pad=%d\n", 
			bmem_find_control(bmc, p[1])->pages, rc[1],
			bmem_pad_get(bmem_find_control(bmc, p[1])));
	rc[0] = csc_bmem_free(bmc, NULL);
	cclog(rc[0]==CSC_MERR_RANGE, "Freeing the NULL memory: %d\n", rc[0]);
}

static void csc_bmem_fitness_test(char *buf, int blen)
{
	BMMCB	*bmc;
	BMMPC	*mpc;
	int	config, rc[4];
	char	*p[8];
	
	(void) blen;

	/* successful to create the minimum heap: bmc=2 page=32 guard=0 */
	config = CSC_MEM_DEFAULT | CSC_MEM_SETPG(0,0);
	bmc = csc_bmem_init(buf, 30*CSC_MEM_PAGE(config), config);
	if (!bmc) return;
	cclog(-1, "Created Heap(%d,%d): bmc=%d free=%d map=%s\n",
			CSC_MEM_PAGE(config), CSC_MEM_GUARD(config), 
			bmc->pages, bmc->avail, show_bitmap(bmc, 0));

	/* create memory pattern: P2+P8+P2+P4+P2+P10 */
	p[0] = csc_bmem_alloc(bmc, 1);
	p[1] = csc_bmem_alloc(bmc, CSC_MEM_PAGE(config)*7);
	p[2] = csc_bmem_alloc(bmc, 1);
	p[3] = csc_bmem_alloc(bmc, CSC_MEM_PAGE(config)*3);
	p[4] = csc_bmem_alloc(bmc, 1);
	cclog(p[1]&&p[3], "Allocated 5 memory blocks: free=%d map=%s\n",
			bmc->avail, show_bitmap(bmc, 0));

	/* create memory holes */
	csc_bmem_free(bmc, p[1]);
	csc_bmem_free(bmc, p[3]);

	/* list all candidators */
	rc[1] = (int)csc_bmem_attrib(bmc, p[1], rc);
	cclog(!rc[0], "Candidator 1: off=+%d size=%d state=%d\n", BMEM_SPAN(p[1], bmc), rc[1], rc[0]);
	rc[1] = (int)csc_bmem_attrib(bmc, p[3], rc);
	cclog(!rc[0], "Candidator 2: off=+%d size=%d state=%d\n", BMEM_SPAN(p[3], bmc), rc[1], rc[0]);
	/* manully set the next candidator because it's uninitialized */
	rc[0] = bmem_addr_to_index(bmc, p[4] + CSC_MEM_PAGE(config));
	mpc = bmem_index_to_addr(bmc, rc[0]);
	memset(mpc, 0, sizeof(BMMPC));
	mpc->pages = bmem_page_find(bmc, rc[0]);
	bmem_set_crc(mpc, sizeof(BMMPC));
	p[5] = bmem_find_client(bmc, mpc, NULL);
	rc[1] = (int)csc_bmem_attrib(bmc, p[5], rc);
	cclog(!rc[0], "Candidator 3: off=+%d size=%d state=%d\n", BMEM_SPAN(p[5], bmc), rc[1], rc[0]);
	cclog(bmc->avail==23, "Created 3 memory holes: free=%d map=%s\n",
			bmc->avail, show_bitmap(bmc, 0));

	/* testing first fit */
	p[6] = csc_bmem_alloc(bmc, CSC_MEM_PAGE(config)*3);
	if (!p[6]) return;
	rc[1] = (int)csc_bmem_attrib(bmc, p[6], rc);
	cclog(p[6]==p[1], "First Fit: off=+%d size=%d -- free=%d map=%s\n", 
			BMEM_SPAN(p[6], bmc), rc[1], bmc->avail, show_bitmap(bmc, 0));

	/* testing best fit */
	csc_bmem_free(bmc, p[6]);
	cclog(bmc->avail==23, "Created 3 memory holes: free=%d map=%s\n",
			bmc->avail, show_bitmap(bmc, 0));

	config = (config & ~CSC_MEM_FITMASK) | CSC_MEM_BEST_FIT;
	bmem_config_set(bmc, config);
	bmem_set_crc(bmc, bmem_page_to_size(bmc, bmc->pages));

	p[6] = csc_bmem_alloc(bmc, CSC_MEM_PAGE(config)*3);
	if (!p[6]) return;
	rc[1] = (int)csc_bmem_attrib(bmc, p[6], rc);
	cclog(p[6]==p[3], "Best Fit: off=+%d size=%d -- free=%d map=%s\n", 
			BMEM_SPAN(p[6], bmc), rc[1], bmc->avail, show_bitmap(bmc, 0));

	/* testing worst fit */
	csc_bmem_free(bmc, p[6]);
	cclog(bmc->avail==23, "Created 3 memory holes: free=%d map=%s\n",
			bmc->avail, show_bitmap(bmc, -1));

	config = (config & ~CSC_MEM_FITMASK) | CSC_MEM_WORST_FIT;
	bmem_config_set(bmc, config);
	bmem_set_crc(bmc, bmem_page_to_size(bmc, bmc->pages));

	p[6] = csc_bmem_alloc(bmc, CSC_MEM_PAGE(config)*3);
	if (!p[6]) return;
	rc[1] = (int)csc_bmem_attrib(bmc, p[6], rc);
	cclog(p[6]==p[5], "Worst Fit: off=+%d size=%d -- free=%d map=%s\n", 
			BMEM_SPAN(p[6], bmc), rc[1], bmc->avail, show_bitmap(bmc, -1));
}


#define	BMTEST_ROUND	40
#define BMTEST_PAGES	128

static void csc_bmem_bitmap_test(char *buf, int blen)
{
	BMMCB	*bmc1, *bmc2;
	int	config, verbose=0;

	int bitmap_compare()
	{
		int	i;
		for (i = 0; i < (bmc1->total + 7)/8; i++) {
			if (bmc1->bitmap[i] != bmc2->bitmap[i]) {
				return i+1;
			}
		}
		return 0;
	}

	void bitmap_set()
	{
		memset(bmc1->bitmap, 0xff, (bmc1->total + 7)/8);
		memset(bmc2->bitmap, 0xff, (bmc1->total + 7)/8);
	}

	void bitmap_clr()
	{
		memset(bmc1->bitmap, 0, (bmc1->total + 7)/8);
		bmc1->bitmap[0] = 0xc0;
		memset(bmc2->bitmap, 0, (bmc1->total + 7)/8);
		bmc2->bitmap[0] = 0xc0;
	}

	int bitmap_allocated(int idx)
	{
		int	i;
		for (i = idx; i < bmc1->total; i++) {
			if (!BM_CK_PAGE(bmc1->bitmap, i)) {
				break;	
			}
		}
		return i - idx;
	}

	void bitmap_chk(int used, int freed)
	{
		int	i, k, cnt_used, cnt_free;
		
		cnt_used = cnt_free = 0;
		if (verbose == 1) cclog(1, "Bitmap usage: ");
		for (i = 0; i < bmc1->total; i++) {
			if (BM_CK_PAGE(bmc1->bitmap, i)) {
				k = bitmap_allocated(i);
				cnt_used += k;
				if (verbose == 1) cslog("A%d ", k);
			} else {
				k = bmem_page_find(bmc1, i);
				cnt_free += k;
				if (verbose == 1) cslog("F%d ", k);
				if (k != bmem_page_find_slow(bmc1, i)) {
					cclog(0, "Bit finder finding free failed: idx=%d (%d)\n", i, k);
				}		
			}
			i = i + k - 1;
		}
		if (verbose == 1) cslog("\n");
		if ((used != cnt_used) || (freed != cnt_free)) {
			cclog(0, "Bit finder failed: used=%d freed=%d\n", cnt_used, cnt_free);
		}
	}

	void bitmap_test_bitset(int bitlen)
	{
		int	i, n, k;

		k = (bitlen + BMTEST_ROUND + 17) / 8;
		for (i = 0; i < BMTEST_ROUND; i++) {
			bitmap_clr();
			bmem_page_alloc_slow(bmc1, i + 2, bitlen);
			bmem_page_alloc(bmc2, i + 2, bitlen);
			if ((n = bitmap_compare()) != 0) {
				cclog(0, "Bit Set failed at %d: [%s]\n", 
						n, show_bitmap(bmc1, k)); 
				cclog(0, "Bit Set failed at %d: [%s]\n", 
						n, show_bitmap(bmc2, k));
				break;
			}
			if ((verbose == 2) || (i == 0) || (i == BMTEST_ROUND - 1)) {
				cclog(1, "Set %d Bit succeed: [%s]\n", 
						bitlen, show_bitmap(bmc1, k));
			}
			bitmap_chk(bitlen+2, BMTEST_PAGES-bitlen-2);
		}
	}

	void bitmap_test_bitclr(int bitlen)
	{
		int	i, n, k;

		k = (bitlen + BMTEST_ROUND + 17) / 8;
		for (i = 0; i < BMTEST_ROUND; i++) {
			bitmap_set();
			bmem_page_free_slow(bmc1, i + 2, bitlen);
			bmem_page_free(bmc2, i + 2, bitlen);
			if ((n = bitmap_compare()) != 0) {
				cclog(0, "Bit Clear failed at %d: [%s]\n", 
						n, show_bitmap(bmc1, k)); 
				cclog(0, "Bit Clear failed at %d: [%s]\n", 
						n, show_bitmap(bmc2, k));
				break;
			}
			if ((verbose == 2) || (i == 0) || (i == BMTEST_ROUND - 1)) {
				cclog(1, "Clear %d Bit succeed: [%s]\n", 
						bitlen, show_bitmap(bmc1, k));
			}
			bitmap_chk(BMTEST_PAGES-bitlen, bitlen);
		}
	}

	config = CSC_MEM_DEFAULT | CSC_MEM_SETPG(0,0);
	bmc1 = csc_bmem_init(buf, BMTEST_PAGES*CSC_MEM_PAGE(config), config);
	if (!bmc1) return;
	cclog(-1, "Created Heap(%d,%d): bmc=%d free=%d map=%s\n",
			CSC_MEM_PAGE(config), CSC_MEM_GUARD(config), 
			bmc1->pages, bmc1->avail, show_bitmap(bmc1, 0));

	bmc2 = csc_bmem_init(buf + blen / 2, BMTEST_PAGES*CSC_MEM_PAGE(config), config);
	if (!bmc2) return;
	cclog(-1, "Created Heap(%d,%d): bmc=%d free=%d map=%s\n",
			CSC_MEM_PAGE(config), CSC_MEM_GUARD(config), 
			bmc2->pages, bmc2->avail, show_bitmap(bmc2, 0));

	bitmap_test_bitset(8);
	bitmap_test_bitset(9);
	bitmap_test_bitset(19);
	bitmap_test_bitset(29);

	bitmap_test_bitclr(8);
	bitmap_test_bitclr(9);
	bitmap_test_bitclr(19);
	bitmap_test_bitclr(29);
}


static char *show_bitmap(BMMCB *bmc, int len)
{
	static	char	buf[1024];
	int	i, k, n;

	n = (bmc->total + 7) / 8;
	if ((len < 0) || (len > n)) {
		len = n;
	} else if (len == 0) {
		for (len = n; len && !bmc->bitmap[len - 1]; len--);
	}			
	for (i = n = 0; i < len; i++) {
		for (k = 0x80; k; k >>= 1) {
			buf[n++] = bmc->bitmap[i] & k ? '1' : '0';
		}
	}
	buf[n] = 0;
	return buf;
}
#endif

