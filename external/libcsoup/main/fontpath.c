/*  fontpath.c - test harness of smm_fontpath()

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
#include <stdlib.h>

#include "libcsoup.h"
#include "libcsoup_debug.h"

int fontpath_main(void *rtime, int argc, char **argv)
{
	char	*fdir;

	/* stop the compiler complaining */
	(void) rtime; (void) argc; (void) argv;

	while (--argc && (**++argv == '-')) {
		if (!strcmp(*argv, "-h") || !strcmp(*argv, "--help")) {
			CDB_SHOW(("fontpath [font_name]\n"));
			return 0;
		} else {
			CDB_SHOW(("Unknown option. [%s]\n", *argv));
			return -1;
		}
	}
	if (argc > 0) {
		fdir = smm_fontpath(*argv, argv+1);
		CDB_SHOW(("%s\n", fdir));
		free(fdir);
	}
	return 0;
}

struct	clicmd	fontpath_cmd = {
	"fontpath", fontpath_main, NULL, "Testing the smm_fontpath() function"
};

extern  struct  clicmd  fontpath_cmd;


