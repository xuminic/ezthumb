/*  memdump.c - test harness of memdump()

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

#include "libcsoup.h"
#include "libcsoup_debug.h"

int memdump_main(void *rtime, int argc, char **argv)
{
	char	user[384], buf[256];
	int	i, len;
	int	flags[] = {
		CSC_MEMDUMP_BIT_16,
		CSC_MEMDUMP_BIT_32,
		CSC_MEMDUMP_BIT_64,
		CSC_MEMDUMP_BIT_FLOAT | CSC_MEMDUMP_TYPE_EE,
		CSC_MEMDUMP_BIT_DOUBLE | CSC_MEMDUMP_TYPE_EE,
		CSC_MEMDUMP_TYPE_HEXL | CSC_MEMDUMP_WIDTH(3) | 
			CSC_MEMDUMP_ALIGN_LEFT | CSC_MEMDUMP_NO_FILLING,
		CSC_MEMDUMP_TYPE_UDEC,
		CSC_MEMDUMP_TYPE_IDEC | CSC_MEMDUMP_BIT_64,
		CSC_MEMDUMP_TYPE_OCT,
		CSC_MEMDUMP_NO_GLYPH,
		CSC_MEMDUMP_NO_ADDR,
		CSC_MEMDUMP_NO_SPACE,
		0
	};

	/* stop the compiler complaining */
	(void) rtime; (void) argc; (void) argv;

	for (i = 0; i < (int)sizeof(user); user[i] = i, i++);

	for (i = 0; i < (int)(sizeof(flags)/sizeof(int)); i++) {
		len = csc_memdump_line(user, 16, flags[i], buf, sizeof(buf));
		cslog("%s\n", buf);
		if (len != (int)strlen(buf)) {
			cslog("BOOM!\n");
			break;
		}
	}

	/* reverse test */
	csc_memdump_line(user + sizeof(user), 16, 
			CSC_MEMDUMP_BIT_32 | CSC_MEMDUMP_REVERSE, 
			buf, sizeof(buf));
	cslog("%s\n", buf);

	/* length test */
	cslog("SIZE=%d\n", csc_memdump_line(user + sizeof(user), 16, 
			CSC_MEMDUMP_BIT_32 | CSC_MEMDUMP_REVERSE, NULL, 0));

	csc_memdump(user, sizeof(user), 16, 0);
	csc_memdump(user, sizeof(user), 4, CSC_MEMDUMP_BIT_32 | 
			CSC_MEMDUMP_NO_GLYPH | CSC_MEMDUMP_NO_ADDR | CSC_MEMDUMP_NO_SPACE);
	csc_memdump(user + sizeof(user), sizeof(user), 16, CSC_MEMDUMP_REVERSE);
	cslog("Sizeof: int=%ld long=%ld short=%ld longlong=%ld float=%d double=%d\n",
			sizeof(int), sizeof(long), sizeof(short), 
			sizeof(long long), sizeof(float), sizeof(double));
	return 0;
}

struct	clicmd	memdump_cmd = {
	"memdump", memdump_main, NULL, "Testing the memory dump"
};

extern  struct  clicmd  memdump_cmd;

