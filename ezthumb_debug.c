
/*  ezthumb_debug.c - debug module of ezthumb

    Copyright (C) 2014  "Andy Xuming" <xuming@users.sourceforge.net>

    This file is part of EZTHUMB, a utility to generate thumbnails

    EZTHUMB is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    EZTHUMB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "ezthumb_debug.h"

static char *ezthumb_slog_prefix(void *self, int cw);

SMMDBG	ezthumb_debug_control = {
	SLOG_MAGIC,				/* the magic word */
	SLOG_MODUL_ALL(SLOG_LVL_ERROR),		/* control word in host */
	SLOG_OPT_ALL,				/* option */
	NULL, NULL,				/* file name and FILEP */
	(void*) -1,				/* standard i/o */
	ezthumb_slog_prefix,			/* generating the prefix */
	NULL, NULL,				/* socket extension */
	NULL, NULL, NULL			/* mutex setting */
};


SMMDBG *ezthumb_slog_open(FILE *stdio, char *fname)
{
	if (stdio) {
		slog_bind_stdio(&ezthumb_debug_control, stdio);
	}
	if (fname) {
		slog_bind_file(&ezthumb_debug_control, fname);
	}
	return &ezthumb_debug_control;
}

int ezthumb_slog_close(void)
{
	return slog_shutdown(&ezthumb_debug_control);
}

int ezthumb_slog_puts(SMMDBG *dbgc, int setcw, int cw, char *buf)
{
	if (!slog_validate(dbgc, setcw, cw)) {
		return -1;
	}
	return slog_output(dbgc, cw, buf);
}

/** better use this as an example becuase it's not thread safe */
char *ezthumb_slog_format(char *fmt, ...)
{
	static	char	logbuf[SLOG_BUFFER];
	va_list ap;

	va_start(ap, fmt);
	SMM_VSNPRINT(logbuf, sizeof(logbuf), fmt, ap);
	va_end(ap);
	return logbuf;
}

static char *ezthumb_slog_prefix(void *self, int cw)
{
	static	char	buffer[256];
	SMMDBG	*dbgc = self;
	struct	tm	*lctm;
	time_t	sec;

	(void) cw;
	if (dbgc->option & SLOG_OPT_TMSTAMP) {
		time(&sec);
		lctm = localtime(&sec);
		sprintf(buffer, "[%d%02d%02d-%02d%02d%02d]", 
				lctm->tm_year + 1900, 
				lctm->tm_mon, lctm->tm_mday,
				lctm->tm_hour, lctm->tm_min, lctm->tm_sec);
	}
	if (dbgc->option) {
		strcat(buffer, " ");
	}
	return buffer;
}

