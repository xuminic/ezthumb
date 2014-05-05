
/*  slog.c - a simple interface for logging/debugging

    Copyright (C) 2011  "Andy Xuming" <xuming@users.sourceforge.net>

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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "libcsoup.h"

static	SMMDBG	*SlogCB = NULL;

void slog_def_open(int cword)
{
	SlogCB = slog_open(cword);
}

void slog_def_close(void)
{
	slog_close(NULL);
	SlogCB = NULL;
}

int slog_def_stdout(int cword, char *buf, int len)
{
	if (buf) {
		len = fwrite(buf, len, 1, stdout);
	}
	if (cword & SLOG_FLUSH) {
		fflush(stdout);
	}
	return len;
}

int slog_def_stderr(int cword, char *buf, int len)
{
	if (buf) {
		len = fwrite(buf, len, 1, stderr);
	}
	if (cword & SLOG_FLUSH) {
		fflush(stderr);
	}
	return len;
}

void *slog_open(int cword)
{
	SMMDBG	*dbgc;

	if ((dbgc = smm_alloc(sizeof(SMMDBG))) != NULL) {
		dbgc->cword  = (unsigned) cword;
		dbgc->device = SLOG_TO_STDOUT;

		dbgc->stdoutput = slog_def_stdout;
		dbgc->stderrput = slog_def_stderr;
	}
	return dbgc;
}

int slog_close(void *control)
{
	SMMDBG	*dbgc;

	if ((dbgc = slog_control(control)) == NULL) {
		return 0;
	}

	if (dbgc->device & SLOG_TO_STDOUT) {
		dbgc->stdoutput(SLOG_FLUSH, NULL, 0);
	}
	if (dbgc->device & SLOG_TO_STDERR) {
		dbgc->stderrput(SLOG_FLUSH, NULL, 0);
	}
	if (dbgc->device & SLOG_TO_FILE) {
		fflush(dbgc->logd);
		fclose(dbgc->logd);
	}

	smm_free(dbgc);
	return 0;
}

SMMDBG *slog_control(void *control)
{
	if (control == NULL) {
		control = SlogCB;
	}
	return control;
}

int slog_validate(SMMDBG *dbgc, int cw)
{
	if (dbgc == NULL) {
		return 1;	/* no control block means always enabled */
	}
	if (SLOG_LEVEL(cw) <= SLOG_LEVEL(dbgc->cword)) {
		return 1;	/* required debug level met */
	}
	if (SLOG_LEVEL(cw) <= SLOG_LVL_ERROR) {
		return 1;	/* non-blocked error level met */
	}
	return 0;
}


