
/*  smm_cwd_push.c - save the current working directory

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

#include "libsmm.h"

#ifdef	CFG_WIN32_API
int smm_cwd_push(void)
{
	TCHAR	*wpath;
	int	len;

	if ((len = GetCurrentDirectory(0, NULL)) == 0) {
		return smm_errno_update(0);	/* system call failed */
	}
	len++;	/* expend it for the null char */
	if ((wpath = malloc(sizeof(TCHAR) * len)) == NULL) {
		return smm_errno_update(ERROR_NOT_ENOUGH_MEMORY);
	}
	if (GetCurrentDirectory(len, wpath) == 0) {
		return smm_errno_update(0);	/* system call failed */
	}
	return (int) wpath;
}
#endif

#ifdef CFG_UNIX_API
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int smm_cwd_push(void)
{
	int	fd;

	if ((fd = open(".", O_RDONLY)) < 0) {
		return smm_errno_update(0);
	}
	return fd;
}
#endif

