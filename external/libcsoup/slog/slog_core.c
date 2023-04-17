
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
#include <time.h>
#include <unistd.h>

#include "libcsoup.h"

#define MAX_PREFIX	512

static int slog_unbind_file(SMMDBG *dbgc);
static int slog_save_to_files(SMMDBG *dbgc, char *buf);
static int slog_epoch_day(void);
static int slog_open_by_epoch_day(SMMDBG *dbgc);
static int slog_open_by_quota(SMMDBG *dbgc);
static int slog_serial_rename(char *basename, int lastsn);


SMMDBG *slog_initialize(void *mem, int cword)
{
	SMMDBG	*dbgc = mem;

	memset(dbgc, 0, sizeof(SMMDBG));
	dbgc->magic = SLOG_MAGIC;
	dbgc->cword = SLOG_MODUL_ALL(cword);
	dbgc->option = SLOG_OPT_ALL;
	
	dbgc->stdio = (void*) -1;
	return dbgc;
}

int slog_shutdown(SMMDBG *dbgc)
{
	slog_unbind_file(dbgc);

	if (dbgc->f_inet) {
		dbgc->f_inet(dbgc, NULL, NULL);
		dbgc->f_inet = NULL;
	}

	dbgc->stdio = (void*) -1;
	return 0;
}

int slog_bind_file(SMMDBG *dbgc, char *fname)
{
	if (fname == NULL) {
		return 0;
	}
	if (dbgc->logd || dbgc->filename) {
		slog_unbind_file(dbgc);
	}
	
	/* in non-split mode, the logd would be always open for appending */
	if ((dbgc->logd = fopen(fname, "a")) == NULL) {	
		return SMM_ERR_OPEN;
	}
	
	/* added 16 bytes space for storing date, like logfilename.log.013 */
	dbgc->filename = csc_strcpy_alloc(fname, 16);
	return 0;
}

/* the extension of slog_bind_file()
 * if flen > 0, the log file will be spiltted by nominated size.
 * if flen == 0, the log file will be splitted by calendar date of the year
 * if fnum > 0, the number of log files will be limited by fnum; old log file will be deleted.
 * if fnum <= 0, the number of log files is unlimited.
 */
int slog_bind_split_file(SMMDBG *dbgc, char *fname, size_t flen, int fnum)
{
	int	rc;

	if (fname == NULL) {
		return 0;
	}
	if (dbgc->logd || dbgc->filename) {
		slog_unbind_file(dbgc);
	}
	
	/* added 16 bytes space for storing date, 
	 * like logfilename.log.013 */
	dbgc->filename = csc_strcpy_alloc(fname, 16);
	dbgc->fileday  = slog_epoch_day();
	dbgc->splitlen = flen;
	dbgc->splitnum = fnum;
	dbgc->option |= SLOG_OPT_SPLIT;
	//dbgc->option |= SLOG_OPT_ONCE;

	if (dbgc->splitlen == 0) {	/* splitting log files by date */
		rc = slog_open_by_epoch_day(dbgc);
	} else {			/* splitting log files by size */
		rc = slog_open_by_quota(dbgc);
	}
	return rc == 0 ? rc : SMM_ERR_OPEN;
}


int slog_bind_stdio(SMMDBG *dbgc, FILE *ioptr)
{
	dbgc->stdio = ioptr;
	return 0;
}

int slog_translate_setup(SMMDBG *dbgc, int which, F_PREFIX func)
{
	F_PREFIX  *root;
	int	i;

	switch (which) {
	case SLOG_TRANSL_MODUL:
		root = dbgc->trans_module;
		break;
	case SLOG_TRANSL_DATE:
		root = dbgc->trans_date;
		break;
	default:
		return SMM_ERR_NULL;
	}

	i = SLOG_TRANS_CHAIN - 1;
	if (root[i] != NULL) {
		return SMM_ERR_LOWMEM;	/* the queue is full */
	}
	for ( ; i > 0; i--) root[i] = root[i-1];	/* shift the queue */
	root[0] = func;		/* always inserts to head */
	return SMM_ERR_NONE;
}

int slog_translate_remove(SMMDBG *dbgc, int which, F_PREFIX func)
{
	F_PREFIX  *root;
	int	i;

	switch (which) {
	case SLOG_TRANSL_MODUL:
		root = dbgc->trans_module;
		break;
	case SLOG_TRANSL_DATE:
		root = dbgc->trans_date;
		break;
	default:
		return SMM_ERR_NULL;
	}
	for (i = 0; (i < SLOG_TRANS_CHAIN) && (root[i] != func); i++);
	if (i < SLOG_TRANS_CHAIN) {
		return SMM_ERR_NULL;	/* not found */
	}
	for ( ; i < SLOG_TRANS_CHAIN - 1; i++) {
		root[i] = root[i+1];	/* squeeze the queue */
	}
	root[i] = NULL;
	return SMM_ERR_NONE;
}

int slog_translating(SMMDBG *dbgc, int which, int cw, char *buf, int blen)
{
	F_PREFIX  *root;
	int	i;

	switch (which) {
	case SLOG_TRANSL_MODUL:
		root = dbgc->trans_module;
		break;
	case SLOG_TRANSL_DATE:
		root = dbgc->trans_date;
		break;
	default:
		return SMM_ERR_NULL;
	}
	for (i = 0; root[i] && (i < SLOG_TRANS_CHAIN); i++) {
		if (root[i](cw, buf, blen) == SMM_ERR_NONE) {
			return SMM_ERR_NONE;
		}
	}
	return SMM_ERR_NULL;
}

int slog_output(SMMDBG *dbgc, int cw, char *buf)
{
	char	*prefix;
	int	len;

	len = strlen(buf);
	if (dbgc == NULL) {	/* ignore the control */
		printf("%s", buf);
		fflush(NULL);
		return len;
	}

	if ((prefix = csc_strcpy_alloc(buf, MAX_PREFIX)) == NULL) {	/* no memory */
		printf("%s", buf);
		fflush(NULL);
		return len;
	}

	*prefix = 0;	/* ignore the content of buf -- will be copied again soon */
	if ((cw & SLOG_FLUSH) == 0) {
		if (dbgc->option & SLOG_OPT_TMSTAMP) {
			slog_translating(dbgc, SLOG_TRANSL_DATE, 0, prefix, MAX_PREFIX);
		}
		if (dbgc->option & SLOG_OPT_MODULE) {
			slog_translating(dbgc, SLOG_TRANSL_MODUL, cw, prefix, MAX_PREFIX);
		}
		if (*prefix != 0) {
			csc_strlcat(prefix, " ", MAX_PREFIX);
		}
		len += strlen(prefix);
	}
	strcat(prefix, buf);

	/* take the mutex lock */
	if (dbgc->f_lock) {
		dbgc->f_lock(dbgc);
	}

	if (dbgc->filename) {
		slog_save_to_files(dbgc, prefix);
	}
	if (dbgc->stdio == (void*) -1) {
		printf("%s", prefix);
		fflush(NULL);
	} else if (dbgc->stdio) {
		fputs(prefix, dbgc->stdio);
		fflush(dbgc->stdio);
	}
	if (dbgc->f_inet) {
		dbgc->f_inet(dbgc, dbgc->netobj, prefix);
	}

	if (dbgc->f_unlock) {
		dbgc->f_unlock(dbgc);
	}
	free(prefix);
	return len;
}

int slogs(SMMDBG *dbgc, int cw, char *buf)
{
	if (!slog_validate(dbgc, 0, cw)) {
		return -1;
	}
	return slog_output(dbgc, cw, buf);
}

int slogs_long(SMMDBG *dbgc, int setcw, int cw, char *buf)
{
	/* combine the module value and the debug level */
	cw = SLOG_LEVEL_SET(setcw, cw);
	
	if (!slog_validate(dbgc, setcw, cw)) {
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

int slogf_long(SMMDBG *dbgc, int setcw, int cw, char *fmt, ...)
{
	char	logbuf[SLOG_BUFFER];
	va_list	ap;

	/* combine the module value and the debug level */
	cw = SLOG_LEVEL_SET(setcw, cw);

	if (!slog_validate(dbgc, setcw, cw)) {
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

	if ((level = SLOG_LEVEL_GET(dbgc->cword)) == SLOG_LVL_AUTO) {
		level = SLOG_LEVEL_GET(setcw);
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
	dbgc->splitlen = 0;
	dbgc->splitnum = 0;
	dbgc->option = SLOG_OPT_ALL;
	return 0;
}

static int slog_save_to_files(SMMDBG *dbgc, char *buf)
{
	if (!buf || !*buf) {
		return 0;
	}

	if (dbgc->option & SLOG_OPT_SPLIT) {
		if (dbgc->splitlen) {	/* splitlen > 0: split the log files by size */
			slog_open_by_quota(dbgc);
		} else {		/* splitlen == 0: split the log files by date */
			slog_open_by_epoch_day(dbgc);
		}
	}

	if (dbgc->logd) {
		fputs(buf, dbgc->logd);
		if (dbgc->option & SLOG_OPT_ONCE) {
			fclose(dbgc->logd);
			dbgc->logd = NULL;
		} else {
			fflush(dbgc->logd);
		}
	}
	return 0;
}

#define LEAPYEAR(x)	(((x)%400==0) || (((x)%4==0) && ((x)%100!=0)))
/* find the total days since epoch */
static int slog_epoch_day(void)
{
	struct	tm	*tbuf;
	time_t	today;
	int	i, day;

	today = time(NULL);
	tbuf = localtime(&today);

	for (i = day = 0; i < tbuf->tm_year; i++) {
		day += LEAPYEAR(i + 1900) ? 366 : 365;
	}
	day += tbuf->tm_yday;
	return day;
}

static int slog_open_by_epoch_day(SMMDBG *dbgc)
{
	char	*fname;

	/* if it's a same day logging, there's no need to change the log file.
	 * return the already-opened log file */
	if (dbgc->logd && (dbgc->fileday == slog_epoch_day())) {
		return 0;
	}

	/* create a new log file named in the current day */
	if ((fname = malloc(strlen(dbgc->filename) + 16)) == NULL) {
		return -1;
	}
	if (dbgc->logd != NULL) {
		fclose(dbgc->logd);
	}
	dbgc->fileday = slog_epoch_day();
	sprintf(fname, "%s.%d", dbgc->filename, dbgc->fileday);
	if ((dbgc->logd = fopen(fname, "a")) == NULL) {
		free(fname);
		return -2;
	}

	/* delete the obselete log file */
	if (dbgc->splitnum > 0) {
		sprintf(fname, "%s.%d", dbgc->filename, dbgc->fileday - dbgc->splitnum);
		//if (close(open(fname, O_RDONLY)) == 0) {
		if (smm_fstat(fname) == SMM_FSTAT_REGULAR) {
			unlink(fname);
		}
	}

	free(fname);
	return 0;
}

/* only works when dbgc->splitlen > 0
 * if dbgc->splitnum <= 0, i.e unlimited, new logs always the original name,
 * backup logs added a serial number like: file.0, file.1, file.2. The next would be file.3
 * if dbgc->splitnum > 0, new logs always the original name,
 * backup logs rename to lower number like: file.1 -> file.0, file.2 -> file.1, 
 * the last one should always be file.splitnum
 */
static int slog_open_by_quota(SMMDBG *dbgc)
{
	if (dbgc->logd == NULL) {
		if ((dbgc->logd = fopen(dbgc->filename, "a")) == NULL) {	
			return -1;
		}
	}
		
	/* if the log is under quota, there's no need to change the log file.*/
	if ((size_t)ftell(dbgc->logd) < dbgc->splitlen) {
		return 0;
	}

	/* otherwise close the current log then open another one */
	fclose(dbgc->logd);

	/* rename every logs one by one:
	 * Delete logname.0, logname.1 -> logname.0, logname.2 -> logname.1, ...
	 * logname -> logname.N */
	if (slog_serial_rename(dbgc->filename, dbgc->splitnum) < 0) {
		return -2;
	}

	/* create and open the new log file */
	if ((dbgc->logd = fopen(dbgc->filename, "a")) == NULL) {	
		return -1;
	}
	return 0;
}

static int slog_serial_rename(char *basename, int lastsn)
{
	char	*oldname, *newname;
	int	i, len;

	/* allocate a pool for two filenames and two 16 bytes buffer */
	len = strlen(basename);
	if ((oldname = csc_strcpy_alloc(basename, len + 32)) == NULL) {
		return -2;
	}
	newname = oldname + len + 16;	/* point to the middle of the pool */
	strcpy(newname, basename);

	/* find the number of the last log */
	for (i = 0; ; i++) {
		sprintf(newname + len, ".%d", i);
		if (smm_fstat(newname) == SMM_FSTAT_ERROR) {
			break;	/* found the last log file */
		}
	}

	if ((lastsn <= 0) || (i < lastsn)) {
		/* if the log number is unlimited, or hasn't reached the quota */
		smm_rename(oldname, newname);
	} else {
		/* delete the oldest log, should be logfile.ext.0 */
		strcat(oldname, ".0");
		unlink(oldname);

		/* delete the extra log files */
		/* PS no need to delete the extra logs. Admin should do this
		for (i--; i >= lastsn; i--) {
			sprintf(oldname + len, ".%d", i);
			unlink(oldname);
		}
		*/

		/* rename old logs, like 
		 * logfile.ext.1 -> logfile.ext.0, logfile.ext.2 -> logfile.ext.1 */
		for (i = 1; i < lastsn; i++) {
			sprintf(oldname + len, ".%d", i);
			sprintf(newname + len, ".%d", i - 1);
			smm_rename(oldname, newname);
		}

		/* archive the current log to logfile.ext.N */
		sprintf(newname + len, ".%d", i - 1);
		smm_rename(basename, newname);
	}
	free(oldname);
	return 0;
}

