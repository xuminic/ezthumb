
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

#include "libcsoup.h"

#ifdef	CFG_WIN32_API
int smm_cwd_pop(void *cwid)
{
	TCHAR	*wpath;

	if ((wpath = (TCHAR*) cwid) == NULL) {
		return SMM_ERR_NONE;
	}
	
	if (SetCurrentDirectory(wpath) == 0) {
		smm_free(wpath);
		return smm_errno_update(SMM_ERR_CHDIR);
	}
	smm_free(wpath);
	return smm_errno_update(SMM_ERR_NONE);
}
#endif

#ifdef	CFG_UNIX_API
#include <unistd.h>

int smm_cwd_pop(void *cwid)
{
	/* based on the assumption that it is always 
	 *  sizeof(void*) >= sizeof(int) */
	union   {
		void	*u_poi;
		int	u_int;
	} fd;

	fd.u_poi = cwid;

	if (fd.u_int <= 0) {
		return smm_errno_update(SMM_ERR_NONE);
	}

	if (fchdir(fd.u_int) < 0) {
		close(fd.u_int);
		return smm_errno_update(SMM_ERR_CHDIR);
	}
	close(fd.u_int);
	return smm_errno_update(SMM_ERR_NONE);
}
#endif
