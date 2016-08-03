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

#define CSOUP_DEBUG_LOCAL	SLOG_CWORD(CSOUP_MOD_SLOG, SLOG_LVL_INFO)
#include "libcsoup_debug.h"

#define SLOG_MOD_TEST_1		SLOG_MODUL_ENUM(3)
#define SLOG_MOD_TEST_2		SLOG_MODUL_ENUM(4)
#define SLOG_MOD_TEST_3		SLOG_MODUL_ENUM(5)
#define SLOG_MOD_TEST_4		SLOG_MODUL_ENUM(6)
#define SLOG_MOD_TEST_5		SLOG_MODUL_ENUM(7)
#define SLOG_MOD_TEST_6		SLOG_MODUL_ENUM(8)

int slog_translate_test(SMMDBG *tstdbg)
{
	int my_transl_001(int cw, char *buf, int blen)
	{
		if (cw & SLOG_MOD_TEST_1) {
			csc_strlcat(buf, "[MOD1]", blen);
		}
		if (cw & SLOG_MOD_TEST_2) {
			csc_strlcat(buf, "[MOD2]", blen);
		}
		if (cw & SLOG_MOD_TEST_3) {
			csc_strlcat(buf, "[MOD3]", blen);
		}
		return SMM_ERR_NULL;
	}
	int my_transl_002(int cw, char *buf, int blen)
	{
		if (cw & SLOG_MOD_TEST_4) {
			csc_strlcat(buf, "[MOD4]", blen);
		}
		if (cw & SLOG_MOD_TEST_5) {
			csc_strlcat(buf, "[MOD5]", blen);
		}
		if (cw & SLOG_MOD_TEST_6) {
			csc_strlcat(buf, "[MOD6]", blen);
		}
		return SMM_ERR_NULL;
	}
	int my_transl_003(int cw, char *buf, int blen)
	{
		(void) cw;
		csc_strlcat(buf, "{date}", blen);
		return SMM_ERR_NULL;
	}


	tstdbg->cword = SLOG_MODUL_ALL(SLOG_LVL_DEBUG);
	CDB_SET_LEVEL(SLOG_LVL_WARNING);

	slog_translate_setup(tstdbg, SLOG_TRANSL_MODUL, my_transl_001);
	slog_translate_setup(tstdbg, SLOG_TRANSL_MODUL, my_transl_002);
	slog_translate_setup(tstdbg, SLOG_TRANSL_DATE, my_transl_003);

	slogs(tstdbg, SLOG_CWORD(SLOG_MOD_TEST_1, SLOG_LVL_ERROR), "SLOG_MOD_TEST_1\n");
	slogs(tstdbg, SLOG_CWORD(SLOG_MOD_TEST_2, SLOG_LVL_ERROR), "SLOG_MOD_TEST_2\n");
	slogs(tstdbg, SLOG_CWORD(SLOG_MOD_TEST_3, SLOG_LVL_ERROR), "SLOG_MOD_TEST_3\n");
	slogs(tstdbg, SLOG_CWORD(SLOG_MOD_TEST_4, SLOG_LVL_ERROR), "SLOG_MOD_TEST_4\n");
	slogs(tstdbg, SLOG_CWORD(SLOG_MOD_TEST_5, SLOG_LVL_ERROR), "SLOG_MOD_TEST_5\n");
	slogs(tstdbg, SLOG_CWORD(SLOG_MOD_TEST_6, SLOG_LVL_ERROR), "SLOG_MOD_TEST_6\n");
	slogs(tstdbg, SLOG_CWORD(-1, SLOG_LVL_ERROR), "SLOG_MOD_TEST_all\n");
	return 0;
}


int slog_main(void *rtime, int argc, char **argv)
{
	SMMDBG  *tstdbg, localdbgc;
	int	i;

	/* stop the compiler complaining */
	(void) rtime; (void) argc; (void) argv;

	CDB_SHOW(("Testing slog can be used without control block\n"));
	slogs(NULL, SLOG_LVL_ERROR, "OK: Error\n");
	slogs(NULL, SLOG_LVL_FUNC, "OK: Func\n");

	CDB_SHOW(("Testing slog can NOT be used with uninitialized "
				"control block\n"));
	slogs(&localdbgc, SLOG_LVL_ERROR, "FAILED\n");

	tstdbg = slog_initialize(&localdbgc, SLOG_MODUL_ALL(SLOG_LVL_DEBUG));
	CDB_SHOW(("Testing slog by a control block (%x)\n", tstdbg->cword));
	for (i = 0; i < 8; i++) {
		slogf(tstdbg, i, "%d/%d: debug level test\n", 
				i, SLOG_LVL_DEBUG);
	}

	tstdbg->cword = SLOG_LEVEL_GET(tstdbg->cword) | CSOUP_MOD_SLOG | 
		CSOUP_MOD_CLI | CSOUP_MOD_CONFIG;
	CDB_SHOW(("Testing slog by module (%x)\n", tstdbg->cword));
	for (i = 0; i < 8; i++) {
		slogf(tstdbg, SLOG_MODUL_ENUM(i) | i,
				"%d/%x: debug level test\n", i, 
				SLOG_MODUL_ENUM(i) | i);//SLOG_LVL_DEBUG);
	}

	tstdbg->cword = SLOG_LEVEL_SET(tstdbg->cword, SLOG_LVL_FUNC);
	CDB_SHOW(("Raising the debug level (%x)\n", tstdbg->cword));
	for (i = 0; i < 8; i++) {
		slogf(tstdbg, SLOG_CWORD(CSOUP_MOD_SLOG, i), 
				"%d: debug level test\n", i);
	}
	
	CDB_SHOW(("Binding logfile\n"));
	slog_bind_file(tstdbg, "logfile");
	for (i = 0; i < 8; i++) {
		slogf(tstdbg, SLOG_CWORD(CSOUP_MOD_SLOG, i), 
				"%d: debug level test\n", i);
	}

	CDB_SHOW(("UnBind the stdout\n"));
	slog_bind_stdio(tstdbg, NULL);
	tstdbg->cword = SLOG_LEVEL_SET(tstdbg->cword, SLOG_LVL_DEBUG);
	for (i = 0; i < 8; i++) {
		slogf(tstdbg, SLOG_CWORD(CSOUP_MOD_SLOG, i),
				"%d: debug level test\n", i);
	}

	CDB_SHOW(("Unbind the logfile\n"));
	slog_bind_file(tstdbg, NULL);
	for (i = 0; i < 8; i++) {
		slogf(tstdbg, SLOG_CWORD(CSOUP_MOD_SLOG, i),
				"%d: debug level test\n", i);
	}
	
	CDB_SHOW(("Bind to stderr\n"));
	slog_bind_stdio(tstdbg, stderr);
	for (i = 0; i < 8; i++) {
		slogf(tstdbg, SLOG_CWORD(CSOUP_MOD_SLOG, i),
				"%d: debug level test\n", i);
	}

	CDB_SHOW(("Internal test %d\n", CSOUP_DEBUG_LOCAL));
	CDB_ERROR(("Internal: ERROR\n"));
	CDB_WARN(("Internal: Warning\n"));
	CDB_INFO(("Internal: INFO\n"));
	CDB_DEBUG(("Internal: DEBUG\n"));
	CDB_PROG(("Internal: PROG\n"));
	CDB_MODL(("Internal: MODule\n"));
	CDB_FUNC(("Internal: function\n"));
	CDB_SET_LEVEL(SLOG_LVL_FUNC);
	CDB_ERROR(("Internal: ERROR\n"));
	CDB_WARN(("Internal: Warning\n"));
	CDB_INFO(("Internal: INFO\n"));
	CDB_DEBUG(("Internal: DEBUG\n"));
	CDB_PROG(("Internal: PROG\n"));
	CDB_MODL(("Internal: MODule\n"));
	CDB_FUNC(("Internal: function\n"));


#ifdef	CFG_SLOG_SOCKET
	CDB_SHOW(("Socket test: connecting to 6666\n"));
	slog_bind_tcp(tstdbg, 6666);
	while (1) {
		for (i = 0; i < 8; i++) {
			slogf(tstdbg, SLOG_CWORD(CSOUP_MOD_SLOG, i),
					"%d: debug test\n", i);
			smm_sleep(1,0);
		}
	}
#endif

	slog_translate_test(tstdbg);
	slog_shutdown(tstdbg);
	return 0;
}

struct	clicmd	slog_cmd = {
	"slog", slog_main, NULL, "Testing the slog functions"
};
extern  struct  clicmd  slog_cmd;

