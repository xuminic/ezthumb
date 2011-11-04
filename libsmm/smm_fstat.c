
/*  smm_fstat.c - get the status of files

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
int smm_fstat(char *fname)
{
	TCHAR	*wpath;
	DWORD	fattr;

	if ((wpath = smm_mbstowcs(fname)) == NULL) {
		return smm_errno();
	}
	fattr = GetFileAttributes(wpath);
	free(wpath);

	if (fattr == INVALID_FILE_ATTRIBUTES) {
		return smm_errno_update(0);
	}

	if (fattr & (FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_SYSTEM |
				FILE_ATTRIBUTE_TEMPORARY |
				FILE_ATTRIBUTE_VIRTUAL | 
				FILE_ATTRIBUTE_SPARSE_FILE)) {
		return SMM_FSTAT_DEVICE;
	}
	if (fattr & FILE_ATTRIBUTE_REPARSE_POINT) {
		return SMM_FSTAT_LINK;
	}
	if (fattr & FILE_ATTRIBUTE_DIRECTORY) {
		return SMM_FSTAT_DIR;
	}
	return SMM_FSTAT_REGULAR;
}
#endif

#ifdef	CFG_UNIX_API
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

int smm_fstat(char *fname)
{
	struct	stat	fs;

	if (lstat(fname, &fs) < 0)  {
		return smm_errno_update(0);	/* failed < 0 */
	}
	if (S_ISREG(fs.st_mode)) {
		return SMM_FSTAT_REGULAR;
	}
	if (S_ISDIR(fs.st_mode)) {
		return SMM_FSTAT_DIR;
	}
	if (S_ISLNK(fs.st_mode)) {
		return SMM_FSTAT_LINK;
	}
	return SMM_FSTAT_DEVICE;
}
#endif

