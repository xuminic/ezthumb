
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

#include "libcsoup.h"

static int slog_unbind_file(SMMDBG *dbgc);


SMMDBG *slog_initialize(void *mem, int cword)
{
	SMMDBG	*dbgc = mem;

	memset(dbgc, 0, sizeof(SMMDBG));
	dbgc->magic = SLOG_MAGIC;
	dbgc->cword = SLOG_MODUL_ALL(cword);
	dbgc->option = SLOG_OPT_ALL;
	
	dbgc->stdio = stdout;
	return dbgc;
}

int slog_shutdown(SMMDBG *dbgc)
{
	slog_unbind_file(dbgc);

	if (dbgc->f_inet) {
		dbgc->f_inet(dbgc, NULL, NULL);
		dbgc->f_inet = NULL;
	}
	return 0;
}

int slog_bind_file(SMMDBG *dbgc, char *fname)
{
	if (dbgc->logd) {
		slog_unbind_file(dbgc);
	}
	if (fname == NULL) {
		return 0;	/* unbind */
	}
	
	/* always appending */
	if ((dbgc->logd = fopen(fname, "a+")) == NULL) {	
		return SMM_ERR_OPEN;
	}
	
	dbgc->filename = csc_strcpy_alloc(fname, 0);
	return 0;
}

int slog_bind_stdio(SMMDBG *dbgc, FILE *ioptr)
{
	dbgc->stdio = ioptr;
	return 0;
}

int slog_output(SMMDBG *dbgc, int cw, char *buf)
{
	char	*mypre;
	int	len;

	len = strlen(buf);
	if (dbgc == NULL) {	/* ignore the control */
		fputs(buf, stdout);
		fflush(stdout);
		return len;
	}

	/* take the mutex lock */
	if (dbgc->f_lock) {
		dbgc->f_lock(dbgc);
	}

	mypre = NULL;
	if (dbgc->f_prefix && ((cw & SLOG_FLUSH) == 0)) {
		mypre = dbgc->f_prefix(dbgc, cw);
		len += strlen(mypre);
	}

	if (dbgc->logd) {
		if (mypre) {
			fputs(mypre, dbgc->logd);
		}
		fputs(buf, dbgc->logd);
		fflush(dbgc->logd);
	}
	if (dbgc->stdio == (void*) -1) {
		if (mypre) {
			fputs(mypre, stdout);
		}
		fputs(buf, stdout);
		fflush(stdout);
	} else if (dbgc->stdio) {
		if (mypre) {
			fputs(mypre, dbgc->stdio);
		}
		fputs(buf, dbgc->stdio);
		fflush(dbgc->stdio);
	}
	if (dbgc->f_inet) {
		if (mypre) {
			dbgc->f_inet(dbgc, dbgc->netobj, mypre);
		}
		dbgc->f_inet(dbgc, dbgc->netobj, buf);
	}

	if (dbgc->f_unlock) {
		dbgc->f_unlock(dbgc);
	}
	return len;
}

int slogs(SMMDBG *dbgc, int cw, char *buf)
{
	if (!slog_validate(dbgc, 0, cw)) {
		return -1;
	}
	return slog_output(dbgc, cw, buf);
}

int slogf(SMMDBG *dbgc, int cw, char *fmt, ...)
{
	char	logbuf[SLOG_BUFFER];
	va_list	ap;

	if (!slog_validate(dbgc, 0, cw)) {
		return -1;
	}

	va_start(ap, fmt);
	SMM_VSNPRINT(logbuf, sizeof(logbuf), fmt, ap);
	va_end(ap);
	return slog_output(dbgc, cw, logbuf);
}

int slog_validate(SMMDBG *dbgc, int setcw, int cw)
{
	int level;

	if (dbgc == NULL) {
		return 1;	/* no control block means always enabled */
	}
	if (dbgc->magic != SLOG_MAGIC) {
		return 0;	/* uninitialized control block means disable*/
	}
	if (SLOG_MODUL_GET(cw)) {
		if ((SLOG_MODUL_GET(cw) & SLOG_MODUL_GET(dbgc->cword)) == 0) {
			return 0;	/* discard disabled modules */
		}
	}

	if (setcw) {
		level = SLOG_LEVEL_GET(setcw);
	} else {
		level = SLOG_LEVEL_GET(dbgc->cword);
	}
	if (SLOG_LEVEL_GET(cw) <= level) {
		return 1;	/* required debug level met */
	}
	if (SLOG_LEVEL_GET(cw) <= SLOG_LVL_ERROR) {
		return 1;	/* non-blocked error level met */
	}
	return 0;
}

static int slog_unbind_file(SMMDBG *dbgc)
{
	if (dbgc->logd) {
		fflush(dbgc->logd);
		fclose(dbgc->logd);
		dbgc->logd = NULL;
	}
	if (dbgc->filename) {
		smm_free(dbgc->filename);
		dbgc->filename = NULL;
	}
	return 0;
}

