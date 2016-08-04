
/*!\file       csoup_internal.h
   \brief      internally used by libcsoup

   \author     "Andy Xuming" <xuming@users.sourceforge.net>
   \date       2013-2014
*/
/* Copyright (C) 1998-2014  "Andy Xuming" <xuming@users.sourceforge.net>

   This file is part of CSOUP library, Chicken Soup for the C

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

#ifndef	_CSOUP_INTERNAL_
#define _CSOUP_INTERNAL_

#include "libcsoup.h"

/* define the modules of libcsoup for easy debugging */
#define CSOUP_MOD_SLOG		SLOG_MODUL_ENUM(0)
#define CSOUP_MOD_CLI		SLOG_MODUL_ENUM(1)
#define CSOUP_MOD_CONFIG	SLOG_MODUL_ENUM(2)

#define CCW_SHOW		(SLOG_FLUSH | SLOG_LVL_AUTO)

extern	SMMDBG	csoup_debug_control;

SMMDBG *slog_csoup_open(FILE *stdio, char *fname);
int slog_csoup_close(void);
int slog_csoup_puts(int setcw, int cw, char *buf);
char *slog_csoup_format(char *fmt, ...);


#ifndef  DEBUG
#define CDB_OUTPUT(l,args)
#elif	defined(CSOUP_DEBUG_LOCAL)
#define	CDB_OUTPUT(l,args)	slog_csoup_puts(CSOUP_DEBUG_LOCAL, (l), \
					slog_csoup_format args)
#else
#define	CDB_OUTPUT(l,args)	slog_csoup_puts(SLOG_LVL_WARNING, (l),	\
					slog_csoup_format args)
#endif	/* DEBUG */


#ifdef	CSOUP_DEBUG_LOCAL
#define CDB_REACH(l)		slog_validate(&csoup_debug_control, \
					CSOUP_DEBUG_LOCAL, (l))
#define CDB_SHOW(x)		slog_csoup_puts(CSOUP_DEBUG_LOCAL, CCW_SHOW, \
					slog_csoup_format x)
#else
#define CDB_REACH(l)		slog_validate(&csoup_debug_control, \
					SLOG_LVL_WARNING, (l))
#define	CDB_SHOW(x) 		slog_csoup_puts(SLOG_LVL_AUTO, CCW_SHOW, \
					slog_csoup_format x)
#endif	/* CSOUP_DEBUG_LOCAL */

#define CDB_SET_LEVEL(w)	(csoup_debug_control.cword = SLOG_LEVEL_SET(\
					csoup_debug_control.cword, (w)))

#define CDB_SET_MODULE(w)	(csoup_debug_control.cword = SLOG_MODUL_SET(\
					csoup_debug_control.cword, (w)))


/* only for critical error */
#define CDB_ERROR(x)	CDB_OUTPUT(SLOG_LVL_ERROR, x)
/* for non-critical error; remote access error */
#define CDB_WARN(x)	CDB_OUTPUT(SLOG_LVL_WARNING, x)
/* FYI; show conditions */
#define CDB_INFO(x)	CDB_OUTPUT(SLOG_LVL_INFO, x)
/* low frequency debug */
#define CDB_DEBUG(x)	CDB_OUTPUT(SLOG_LVL_DEBUG, x)
/* high frequency debug */
#define CDB_PROG(x)	CDB_OUTPUT(SLOG_LVL_PROGRAM, x)
/* get in/out the function module */
#define CDB_MODL(x)	CDB_OUTPUT(SLOG_LVL_MODULE, x)
/* get in/out the function; print logs in a direct loop */
#define CDB_FUNC(x)	CDB_OUTPUT(SLOG_LVL_FUNC, x)

#define CDB_CONTI(l,x)	CDB_OUTPUT((l)|SLOG_FLUSH, x)

#endif	/* _CSOUP_INTERNAL_ */

