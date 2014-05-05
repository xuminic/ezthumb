
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


SMMDBG	*tstdbg = NULL;

extern int fixtoken_main(int argc, char **argv);
extern int fontpath_main(int argc, char **argv);
extern int smm_main(int argc, char **argv);
extern int memdump_main(int argc, char **argv);
extern int slog_main(int argc, char **argv);
extern int crc_main(int argc, char **argv);
extern int config_main(int argc, char **argv);
extern int csc_cdll_main(int argc, char **argv);

static	struct	{
	char	*cmd;
	int	(*entry)(int argc, char **argv);
	char	*comment;
} cmdlist[] = {
	{ "cdll", csc_cdll_main, NULL },
	{ "config", config_main, NULL },
	{ "fixtoken", fixtoken_main, NULL },
	{ "fontpath", fontpath_main, NULL },
	{ "smm", smm_main, NULL },
	{ "memdump", memdump_main, NULL },
	{ "slog", slog_main, NULL },
	{ "crc", crc_main, NULL },
	{ NULL, NULL, NULL }
};


static void usage(void)
{
	int	i;

	slogc(tstdbg, SLINFO, "Usage: csoup COMMAND [args ...]\n");
	for (i = 0; cmdlist[i].cmd; i++) {
		slogc(tstdbg, SLINFO, "  %-20s %s\n", cmdlist[i].cmd,
				cmdlist[i].comment ? cmdlist[i].comment : "");
	}
}

int main(int argc, char **argv)
{
	int	i;

	tstdbg = slog_open(SLINFO);
	smm_init(0);

	if (argc > 1) {
		for (i = 0; cmdlist[i].cmd; i++) {
			if (!strcmp(argv[1], cmdlist[i].cmd)) {
				return cmdlist[i].entry(--argc, ++argv);
			}
		}
	}
	usage();
	
	slog_close(tstdbg);
	return -1;
}

