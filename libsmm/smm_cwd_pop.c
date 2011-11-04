
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
int smm_cwd_pop(int cwid)
{
	TCHAR	*wpath;

	wpath = (TCHAR*) cwid;
	if (wpath == NULL) {
		return 0;
	}
	
	cwid = SetCurrentDirectory(wpath);
	free(wpath);
	if (cwid == 0) {
		return smm_errno_update(0);
	}
	return 0;
}
#endif

#ifdef	CFG_UNIX_API
#include <unistd.h>

int smm_cwd_pop(int cwid)
{
	if (cwid >= 0) {
		if (fchdir(cwid) < 0) {
			close(cwid);
			return smm_errno_update(0);
		}
		close(cwid);
	}
	return 0;
}
#endif
