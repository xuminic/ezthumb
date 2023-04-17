/*  tmem.c - test harness of csc_tmem functions

    Copyright (C) 2013  "Andy Xuming" <xuming@users.sourceforge.net>

    This file is part of CSOUP, Chicken Soup library

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
#include <string.h>
#include <unistd.h>

#include "libcsoup.h"
#include "libcsoup_debug.h"

#ifdef	CFG_UNIT_TEST
extern int csc_tmem_unittest(void);
extern int csc_dmem_unittest(void);
extern int csc_bmem_unittest(void);
#endif

static	struct	cliopt	clist[] = {
	{   0, NULL,        0, "OPTIONS:" },
	{ 'h', "help",      0, "This help" },
	{ 'b', "bitmap",    0, "specify the bitmap management" },
	{ 'd', "dlink",     0, "specify the doubly link management" },
	{ 't', "tiny",      0, "specify the tiny mode management" },
#ifdef	CFG_UNIT_TEST
	{ 'u', "unit-test", 0, "doing the unit test" },
#endif
	{ 0, NULL, 0, NULL }
};

static void *(*csc_mem_init)(void *heap, size_t len, int flags);
static void *(*csc_mem_alloc)(void *heap, size_t n);
static int (*csc_mem_free)(void *heap, void *mem);
static void *(*csc_mem_scan)(void *heap, int (*used)(void*), int (*loose)(void*));
static size_t (*csc_mem_attrib)(void *heap, void *mem, int *state);
static void *(*csc_mem_front_guard)(void *heap, void *mem, int *xsize);
static void *(*csc_mem_back_guard)(void *heap, void *mem, int *xsize);
#ifdef	CFG_UNIT_TEST
static int (*csc_mem_unittest)(void);
#endif
static void mem_set_default(int sw);


int mem_main(void *rtime, int argc, char **argv)
{
	int	c;

	if (argc < 2) {
		csc_cli_print(clist, 0, NULL);
		return 0;
	}
	
	if ((rtime = csc_cli_qopt_open(argc, argv)) == NULL) {
		return -1;
	}

	/* set the default method */
	mem_set_default('b');
	while ((c = csc_cli_qopt(rtime, clist)) >= 0) {
		switch (c) {
		case 'h':
			csc_cli_print(clist, 0, NULL);
			break;

		case 'b':
		case 'd':
		case 't':
			mem_set_default(c);
			break;

#ifdef	CFG_UNIT_TEST
		case 'u':
			if (csc_mem_unittest) {
				csc_mem_unittest();
			}
			break;
#endif
		default:
			if (csc_cli_qopt_optopt(rtime) == ':') {
				cslog("%c: missing argument\n", c);
			} else {
				cslog("%c: unknown option\n", c);
			}
			csc_cli_print(clist, 0, NULL);
			break;
		}
	}
	return 0;
}

static void mem_set_default(int sw)
{
	switch (sw) {
	case 'b':
		csc_mem_init = csc_bmem_init;
		csc_mem_alloc = csc_bmem_alloc;
		csc_mem_free = csc_bmem_free;
		csc_mem_scan = csc_bmem_scan;
		csc_mem_attrib = csc_bmem_attrib;
		csc_mem_front_guard = csc_bmem_front_guard;
		csc_mem_back_guard = csc_bmem_back_guard;
#ifdef	CFG_UNIT_TEST
		csc_mem_unittest = csc_bmem_unittest;
#endif
		break;

	case 'd':
		csc_mem_init = csc_dmem_init;
		csc_mem_alloc = csc_dmem_alloc;
		csc_mem_free = csc_dmem_free;
		csc_mem_scan = csc_dmem_scan;
		csc_mem_attrib = csc_dmem_attrib;
		csc_mem_front_guard = csc_dmem_front_guard;
		csc_mem_back_guard = csc_dmem_back_guard;
#ifdef	CFG_UNIT_TEST
		csc_mem_unittest = csc_dmem_unittest;
#endif
		break;

	default:	/* 't' */
		csc_mem_init = csc_tmem_init;
		csc_mem_alloc = csc_tmem_alloc;
		csc_mem_free = csc_tmem_free;
		csc_mem_scan = csc_tmem_scan;
		csc_mem_attrib = csc_tmem_attrib;
		csc_mem_front_guard = csc_tmem_front_guard;
		csc_mem_back_guard = csc_tmem_back_guard;
#ifdef	CFG_UNIT_TEST
		csc_mem_unittest = csc_tmem_unittest;
#endif
		break;
	}
}



struct	clicmd	mem_cmd = {
	"mem", mem_main, NULL, "Testing dynamic memory management"
};

extern  struct  clicmd  mem_cmd;
