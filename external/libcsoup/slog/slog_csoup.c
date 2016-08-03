
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
#include <string.h>
#include <time.h>
#include "libcsoup_debug.h"

static int slog_csoup_trans_date(int cw, char *buf, int blen);
static int slog_csoup_trans_module(int cw, char *buf, int blen);

SMMDBG	csoup_debug_control = {
	SLOG_MAGIC,			/* the magic word */
	SLOG_MODUL_ALL(SLOG_LVL_AUTO),	/* control word in host */
	SLOG_OPT_ALL,			/* option */
	NULL, NULL,			/* file name and FILEP */
	(void*) -1,			/* standard i/o */
	NULL, NULL,			/* generating the prefixes */
	NULL, NULL,			/* socket extension */
	NULL, NULL, NULL		/* mutex setting */
};


SMMDBG *slog_csoup_open(FILE *stdio, char *fname)
{
	if (stdio) {
		slog_bind_stdio(&csoup_debug_control, stdio);
	}
	if (fname) {
		slog_bind_file(&csoup_debug_control, fname);
	}
	if (csoup_debug_control.trans_module == NULL) {
		slog_translate_setup(&csoup_debug_control, 
				SLOG_TRANSL_MODUL, slog_csoup_trans_module);
	}
	if (csoup_debug_control.trans_date == NULL) {
		slog_translate_setup(&csoup_debug_control, 
				SLOG_TRANSL_DATE, slog_csoup_trans_date);
	}
	return &csoup_debug_control;
}

int slog_csoup_close(void)
{
	return slog_shutdown(&csoup_debug_control);
}

int slog_csoup_puts(int setcw, int cw, char *buf)
{
	int	rc;

	/* combine the module value and the debug level */
	cw = SLOG_LEVEL_SET(setcw, cw);

	if (!slog_validate(&csoup_debug_control, setcw, cw)) {
		rc = -1;
	} else {
		rc = slog_output(&csoup_debug_control, cw, buf);
	}
	smm_free(buf);
	return rc;
}

/** better use this as an example becuase it's not thread safe */
char *slog_csoup_format(char *fmt, ...)
{
	char	logbuf[SLOG_BUFFER];
	va_list ap;

	va_start(ap, fmt);
	SMM_VSNPRINT(logbuf, sizeof(logbuf), fmt, ap);
	va_end(ap);
	return csc_strcpy_alloc(logbuf, 0);
}

static int slog_csoup_trans_date(int cw, char *buf, int blen)
{
	struct	tm	*lctm;
	time_t	sec;
	char	tmp[64];
		
	(void) cw;

	time(&sec);
	lctm = localtime(&sec);
	sprintf(tmp, "[%d%02d%02d%02d%02d%02d]", lctm->tm_year + 1900, 
			lctm->tm_mon, lctm->tm_mday,
			lctm->tm_hour, lctm->tm_min, lctm->tm_sec);
	csc_strlcat(buf, tmp, blen);
	return SMM_ERR_NONE;
}

static int slog_csoup_trans_module(int cw, char *buf, int blen)
{
	if (cw & CSOUP_MOD_SLOG) {
		csc_strlcat(buf, "[SLOG]", blen);
	}
	if (cw & CSOUP_MOD_CLI) {
		csc_strlcat(buf, "[CLI]", blen);
	}
	if (cw & CSOUP_MOD_CONFIG) {
		csc_strlcat(buf, "[CONFIG]", blen);
	}
	return SMM_ERR_NONE;	/* end of the chain */
}

