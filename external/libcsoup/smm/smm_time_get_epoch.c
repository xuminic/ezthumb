
/*  smm_time_get_epoch.c - get time in second since Epoch

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

#include "libcsoup.h"


#ifdef  CFG_WIN32_API
int smm_time_get_epoch(SMM_TIME *tmbuf)
{
	GetSystemTimeAsFileTime(tmbuf);
	return 0;
}
#endif

#ifdef	CFG_UNIX_API
#include <stdio.h>
#include <sys/time.h>

int smm_time_get_epoch(SMM_TIME *tmbuf)
{
	gettimeofday(tmbuf, NULL);
	return 0;
}
#endif

