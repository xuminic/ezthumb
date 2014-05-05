
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


int slog_level_read(void *control)
{
	SMMDBG	*dbgc;

	if ((dbgc = slog_control(control)) == NULL) {
		return 0;
	}
	return SLOG_LEVEL(dbgc->cword);
}

int slog_level_write(void *control, int dbg_lvl)
{
	SMMDBG	*dbgc;
	int	tmp;

	if ((dbgc = slog_control(control)) == NULL) {
		return 0;
	}

	tmp = slog_level_read(dbgc);
	dbgc->cword = SLOG_MODULE(dbgc->cword) | SLOG_LEVEL(dbg_lvl);
	return tmp;
}

