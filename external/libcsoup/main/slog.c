/*  slog.c - test harness of slog()

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

extern SMMDBG  *tstdbg;
FILE	*fp;

static int my_stdout(int flush, char *buf, int len)
{
	if (fp == NULL) {
		return 0;
	}
	if (buf) {
		len = fwrite(buf, len, 1, fp);
	}
	if (flush) {
		fflush(fp);
	}
	return len;
}

int slog_main(void *rtime, int argc, char **argv)
{
	int	i, level, control;

	/* stop the compiler complaining */
	(void) rtime; (void) argc; (void) argv;

	slogc(tstdbg, SLINFO, "## slog can be used before initialized\n");
	level = SLSHOW;
	slog(level, "%d: this is a test before initialing\n", level);
	level = SLFUNC;
	slog(level, "%d: this is a test before initialing\n", level);

	slogc(tstdbg, SLINFO, "## once opened the slog, it prints by debug level\n");
	control = SLINFO;
	slog_open(control);
	for (i = 0; i < 8; i++) {
		slog(i, "%d/%d: debug level test\n", i, control);
	}

	control = SLFUNC;
	slogc(tstdbg, SLINFO, "## change debug level to %d\n", control);
	slog_level_write(NULL, control);
	for (i = 0; i < 8; i++) {
		slog(i, "%d/%d: debug level test\n", i, control);
	}

#if 1
	slogc(tstdbg, SLINFO, "## unbuffered mode test\n");
	level = SLSHOW;
	slog(level, "%d: buffered ", level);
	for (i = 0; i < 4; i++) {
		slog(level, ".");
		smm_sleep(1, 0);
	}
	slog(level, "\n");

	level = SLSHOW | SLOG_FLUSH;
	slog(level, "%d: unbuffered ", level);
	for (i = 0; i < 4; i++) {
		slog(level, ".");
		smm_sleep(1, 0);
	}
	slog(level, "\n");
#endif

	slogc(tstdbg, SLINFO, "## slog binded to stdout by default. now bind to stderr\n");
	slog_bind_stderr(NULL, NULL);
	level = SLINFO;
	slog_level_write(NULL, level);
	slog(level, "%d/%d: debug level test\n", level, slog_level_read(NULL));	

	slogc(tstdbg, SLINFO, "## unbinded the stdout\n");
	slog_unbind_stdout(NULL);
	slog(level, "%d/%d: debug level test\n", level, slog_level_read(NULL));	

	slogc(tstdbg, SLINFO, "## unbinded the stderr then save everything to file\n");
	slog_unbind_stderr(NULL);
	slog_bind_file(NULL, "logfile", 0);
	for (i = 0; i < 8; i++) {
		slog(i, "%d/%d: debug level test\n", i, slog_level_read(NULL));
	}

	slogc(tstdbg, SLINFO, "## unbinded the file and rebind the stdout\n");
	slog_unbind_file(NULL);
	slog_bind_stdout(NULL, NULL);
	for (i = 8; i >= 0; i--) {
		slog(i, "%d/%d: debug level test\n", i, slog_level_read(NULL));
	}

	slogc(tstdbg, SLINFO, "## bind both file and the stdout\n");
	slog_bind_file(NULL, "logfile", 1);
	for (i = 0; i < 8; i++) {
		slog(i, "%d/%d: debug level test\n", i, slog_level_read(NULL));
	}

	slogc(tstdbg, SLINFO, "## unbind file and rebind stdout to file logfile2\n");
	slog_unbind_file(NULL);
	slog_bind_stdout(NULL, my_stdout);
	for (i = 0; i < 8; i++) {
		slog(i, "%d/%d: debug level test\n", i, slog_level_read(NULL));
	}
	slogc(tstdbg, SLINFO, "## then open logfile2\n");
	fp = fopen("logfile2", "w");
	for (i = 0; i < 8; i++) {
		slog(i, "%d/%d: debug level test\n", i, slog_level_read(NULL));
	}
	fclose(fp);	/* must close it first before unbind it */
	slogc(tstdbg, SLINFO, "## recover the default stdout\n");
	slog_bind_stdout(NULL, (F_STD) -1);
	for (i = 0; i < 8; i++) {
		slog(i, "%d/%d: debug level test\n", i, slog_level_read(NULL));
	}
	

	slog_close(NULL);
	return 0;
}

struct	clicmd	slog_cmd = {
	"slog", slog_main, NULL, "Testing the slog functions"
};
extern  struct  clicmd  slog_cmd;

