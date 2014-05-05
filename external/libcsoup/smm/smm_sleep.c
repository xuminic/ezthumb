
/*  smm_sleep.c - task sleep

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

#include <stdio.h>
#include <stdlib.h>

#include "libcsoup.h"

#ifdef	CFG_WIN32_API
int smm_sleep(int sec, int usec)
{
	DWORD	ms;

	ms = sec * 1000 + usec / 1000;
	Sleep(ms);
	return smm_errno_update(SMM_ERR_NONE);
}
#endif

#ifdef	CFG_UNIX_API
#include <time.h>

int smm_sleep(int sec, int usec)
{
	struct	timespec	require, remain;

	require.tv_sec  = sec;
	require.tv_nsec = usec * 1000;
	while (nanosleep(&require, &remain) < 0) {
		require = remain;
	}
	return smm_errno_update(SMM_ERR_NONE);
}
#endif

