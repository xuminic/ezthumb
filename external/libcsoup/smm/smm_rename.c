
/*  smm_rename.c - rename a file

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
int smm_rename(char *oldname, char *newname)
{
	TCHAR	*woldname, *wnewname;

	if ((woldname = smm_mbstowcs_alloc(oldname)) == NULL) {
		return smm_errno_update(SMM_ERR_NONE_READ);
	}
	if ((wnewname = smm_mbstowcs_alloc(newname)) == NULL) {
		smm_free(woldname);
		return smm_errno_update(SMM_ERR_NONE_READ);
	}

	if (MoveFile(woldname, wnewname) == 0) {
		smm_free(woldname);
		smm_free(wnewname);
		return smm_errno_update(SMM_ERR_CHDIR);
	}

	smm_free(woldname);
	smm_free(wnewname);
	return smm_errno_update(SMM_ERR_NONE);
}
#endif

#ifdef	CFG_UNIX_API
int smm_rename(char *oldname, char *newname)
{
	if (rename(oldname, newname) < 0) {
		return smm_errno_update(SMM_ERR_RENAME);
	}
	return smm_errno_update(SMM_ERR_NONE);
}
#endif

