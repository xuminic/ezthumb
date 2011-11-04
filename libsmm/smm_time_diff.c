
/*  smm_time_diff.c - get the differecnce of times in seconds

    Copyright (C) 2011  "Andy Xuming" <xuming@users.sourceforge.net>

    This file is part of LIBSMM, System Masquerade Module library

    LIBSMM is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LIBSMM is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "libsmm.h"


#ifdef  CFG_WIN32_API
int smm_time_diff(SMM_TIME *tmbuf)
{
	ULARGE_INTEGER	tmnow, tmlast;

	tmlast.LowPart  = tmbuf->dwLowDateTime;
	tmlast.HighPart = tmbuf->dwHighDateTime;
	GetSystemTimeAsFileTime((FILETIME *) &tmnow.u);
	return (int)((tmnow.QuadPart - tmlast.QuadPart) / 10000);
}
#endif

#ifdef	CFG_UNIX_API
#include <stdio.h>
#include <sys/time.h>

int smm_time_diff(SMM_TIME *tmbuf)
{
	SMM_TIME	tmnow;
	int		diff;

	gettimeofday(&tmnow, NULL);

	diff = tmnow.tv_usec - tmbuf->tv_usec;
	if (diff < 0) {
		diff += 1000000;
		tmnow.tv_sec--;
	}
	diff += (tmnow.tv_sec - tmbuf->tv_sec) * 1000000;
	return diff / 1000;
}
#endif

