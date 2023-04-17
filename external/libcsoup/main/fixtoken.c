/*  fixtoken.c - test harness of fixtoken()

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

static	struct	{
	char	*delim;
	char	*content;
} testbl[] = {
	{ "# :",	"#abc  wdc:have:::#:debug" },
	{ " ",		"  abc bcd 'sad str sf  ' sdf > asdf" },
	{ NULL, NULL }
};

static int fixtoken_test(char *content, char *delim)
{
	char	buf[256], *argv[32];
	int	i, argc;

	CDB_INFO(("PARSING   {%s} by {%s}\n", content, delim));

	strcpy(buf, content);
	argc = csc_fixtoken(buf, argv, sizeof(argv)/sizeof(char*), delim);
	CDB_INFO(("FIXTOKEN: "));
	for (i = 0; i < argc; i++) {
		CDB_CONTI(SLOG_LVL_INFO, ("{%s} ", argv[i]));
	}
	CDB_CONTI(SLOG_LVL_INFO, ("\n"));

	strcpy(buf, content);
	argc = csc_ziptoken(buf, argv, sizeof(argv)/sizeof(char*), delim);
	CDB_INFO(("ZIPTOKEN: "));
	for (i = 0; i < argc; i++) {
		CDB_CONTI(SLOG_LVL_INFO, ("{%s} ", argv[i]));
	}
	CDB_CONTI(SLOG_LVL_INFO, ("\n"));

	strcpy(buf, content);
	/*argc = csc_mkargv(buf, argv, sizeof(argv)/sizeof(char*));
	CDB_INFO(("MKARGV:   "));
	for (i = 0; i < argc; i++) {
		CDB_CONTI(SLOG_LVL_INFO, ("{%s} ", argv[i]));
	}
	CDB_CONTI(SLOG_LVL_INFO, ("\n\n"));*/
	return 0;
}

static int fixtoken_run(void)
{
	char	buf[256];

	cslog("Press Ctrl-D or 'quit' command to quit.\n");
	while (1) {
		cslog("IN> ");
		if (fgets(buf, 256, stdin) == NULL) {
			break;
		}

		buf[strlen(buf) - 1] = 0;
		if (!strcmp(buf, "quit") || !strcmp(buf, "exit")) {
			break;
		}

		fixtoken_test(buf, " ");
	}
	return 0;
}


int fixtoken_main(void *rtime, int argc, char **argv)
{
	int	i;

	/* stop the compiler complaining */
	(void) rtime; (void) argc; (void) argv;

	while (--argc && (**++argv == '-')) {
		if (!strcmp(*argv, "-h") || !strcmp(*argv, "--help")) {
			cslog("fixtoken [--help] [--runtime]\n");
			return 0;
		} else if (!strcmp(*argv, "--runtime")) {
			return fixtoken_run();
		} else {
			cslog("Unknown option. [%s]\n", *argv);
			return -1;
		}
	}

	for (i = 0; testbl[i].delim; i++) {
		fixtoken_test(testbl[i].content, testbl[i].delim);
	}
	return 0;
}

struct	clicmd	fixtoken_cmd = {
	"fixtoken", fixtoken_main, NULL, "Testing the fixtoken and ziptoken"
};

extern  struct  clicmd  fixtoken_cmd;

