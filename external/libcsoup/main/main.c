
/*  main.c

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

#include "main_define.h"

SMMDBG	*tstdbg = NULL;

int main(int argc, char **argv)
{
	tstdbg = slog_open(SLINFO);
	smm_init(0);

	if (argc > 1) {
		csc_cli_cmd_run(cmdlist, NULL, --argc, ++argv);
	} else {
		slogc(tstdbg, SLINFO, "Usage: csoup COMMAND [args ...]\n");
		csc_cli_cmd_print(cmdlist, NULL);
	}
	slog_close(tstdbg);
	return -1;
}

