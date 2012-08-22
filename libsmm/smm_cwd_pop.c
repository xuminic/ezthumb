
/*  smm_cwd_pop.c - change current working directory to the pre-saved
    directory and free the buffer

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
int smm_cwd_pop(void *cwid)
{
	TCHAR	*wpath;

	if ((wpath = (TCHAR*) cwid) == NULL) {
		return SMM_ERR_NONE;
	}
	
	if (SetCurrentDirectory(wpath) == 0) {
		free(wpath);
		return smm_errno_update(SMM_ERR_CHDIR);
	}
	free(wpath);
	return smm_errno_update(SMM_ERR_NONE);
}
#endif

#ifdef	CFG_UNIX_API
#include <unistd.h>

int smm_cwd_pop(void *cwid)
{
	int	fid;

	if (cwid == NULL) {
		return SMM_ERR_NONE;
	}

#if	UINTPTR_MAX == 0xffffffff
	fid = (int) cwid;
#else
	fid = (int)(int64_t) cwid;
#endif

	if (fchdir(fid) < 0) {
		close(fid);
		return smm_errno_update(SMM_ERR_CHDIR);
	}
	close(fid);
	return smm_errno_update(SMM_ERR_NONE);
}
#endif
