
/*!\file       ezthumb_debug.h
   \brief      internally used by ezthumb

   \author     "Andy Xuming" <xuming@users.sourceforge.net>
   \date       2013-2014
*/
/*  Copyright (C) 2011  "Andy Xuming" <xuming@users.sourceforge.net>

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

#ifndef	_EZTHUMB_DEBUG_
#define _EZTHUMB_DEBUG_

#include "libcsoup.h"

/* define the modules of libcsoup for easy debugging */
#define EZTHUMB_MOD_CORE	SLOG_MODUL_ENUM(0)
#define EZTHUMB_MOD_CLI		SLOG_MODUL_ENUM(1)
#define EZTHUMB_MOD_GUI		SLOG_MODUL_ENUM(2)

extern	SMMDBG	ezthumb_debug_control;

SMMDBG *ezthumb_slog_open(FILE *stdio, char *fname);
int ezthumb_slog_close(void);
int ezthumb_slog_setcw(int cw);
int ezthumb_slog_puts(SMMDBG *dbgc, int setcw, int cw, char *buf);
char *ezthumb_slog_format(char *fmt, ...);



#ifndef  DEBUG
#define EDB_OUTPUT(l,args)
#define EDB_STATE(l) 		0
#elif	defined(CSOUP_DEBUG_LOCAL)
#define	EDB_OUTPUT(l,args)	ezthumb_slog_puts(&ezthumb_debug_control, \
					CSOUP_DEBUG_LOCAL, \
					SLOG_LEVEL_SET(CSOUP_DEBUG_LOCAL,(l)),\
					ezthumb_slog_format args)
#define EDB_STATE(l)		slog_validate(&ezthumb_debug_control, \
					CSOUP_DEBUG_LOCAL, \
					SLOG_LEVEL_SET(CSOUP_DEBUG_LOCAL,(l)))
#else
#define	EDB_OUTPUT(l,args)	slogs(&ezthumb_debug_control, (l), \
					ezthumb_slog_format args)
#define EDB_STATE(l)		slog_validate(&ezthumb_debug_control, 0, (l))
#endif


#define EDB_SHOW_CW(cw)		(SLOG_MODUL_GET(cw)|SLOG_FLUSH|SLOG_LVL_SHOWOFF)

#ifdef	CSOUP_DEBUG_LOCAL
#define EDB_SHOW(x)		ezthumb_slog_puts(&ezthumb_debug_control, \
					CSOUP_DEBUG_LOCAL,\
					EDB_SHOW_CW(CSOUP_DEBUG_LOCAL),\
					ezthumb_slog_format x)
#else
#define	EDB_SHOW(x) 		slogs(&ezthumb_debug_control, EDB_SHOW_CW(0),\
					ezthumb_slog_format x)
#endif

/* only for critical error */
#define EDB_ERROR(x)	EDB_OUTPUT(SLOG_LVL_ERROR, x)
/* for non-critical error; remote access error */
#define EDB_WARN(x)	EDB_OUTPUT(SLOG_LVL_WARNING, x)
/* FYI; show conditions */
#define EDB_INFO(x)	EDB_OUTPUT(SLOG_LVL_INFO, x)
/* low frequency debug */
#define EDB_DEBUG(x)	EDB_OUTPUT(SLOG_LVL_DEBUG, x)
/* high frequency debug */
#define EDB_PROG(x)	EDB_OUTPUT(SLOG_LVL_PROGRAM, x)
/* get in/out the function module */
#define EDB_MODL(x)	EDB_OUTPUT(SLOG_LVL_MODULE, x)
/* get in/out the function; print logs in a direct loop */
#define EDB_FUNC(x)	EDB_OUTPUT(SLOG_LVL_FUNC, x)

#define EDB_CONTI(l,x)	EDB_OUTPUT((l)|SLOG_FLUSH, x)

#endif	/* _EZTHUMB_DEBUG_ */

