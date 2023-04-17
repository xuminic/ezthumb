/*  dummy.c - the example of the test harness.

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
/* Build:
 * Add dummy.c to SRCS in Makefile, that's all */

#include <stdio.h>

#include "libcsoup.h"
#include "libcsoup_debug.h"

int dummy_main(void *rtime, int argc, char **argv)
{
	int	i;

	(void) rtime; /* stop the compiler complaining */

	printf("Hello World!\n");
	for (i = 0; i < argc; i++) {
		printf("ARG_%d: %s\n", i, argv[i]);
	}
	return 0;
}

struct	clicmd	dummy_cmd = {
	"dummy", dummy_main, NULL, "dummy test case"
};


/* The following ling is used by 'mkclicmd' to generate 'main_define.h' automatically */
extern  struct  clicmd  dummy_cmd;

