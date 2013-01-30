
/*  smm_filesize.c - get the size of the file

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
long long smm_filesize(char *fname)
{
	TCHAR	*wpath;
	DWORD	sizeh, sizel;
	HANDLE	fhdl;

	if ((wpath = smm_mbstowcs(fname)) == NULL) {
		return -1;
	}

	fhdl = CreateFile(wpath, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	if (fhdl == INVALID_HANDLE_VALUE) {
		free(wpath);
		smm_errno_update(SMM_ERR_OPEN);
		return -1;
	}
	
	sizel = GetFileSize(fhdl, &sizeh);
	free(wpath);
	CloseHandle(fhdl);
	
	if (sizel == 0xffffffff && (GetLastError() != NO_ERROR)) {
		smm_errno_update(SMM_ERR_STAT);
		return -1;
	}
	smm_errno_update(SMM_ERR_NONE);
	return ((long long) sizeh << 32) | sizel;
}
#endif

#ifdef	CFG_UNIX_API
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

long long smm_filesize(char *fname)
{
	struct	stat	fs;

	if (stat(fname, &fs) < 0)  {
		smm_errno_update(SMM_ERR_STAT);
		return -1;	/* failed < 0 */
	}

	smm_errno_update(SMM_ERR_NONE);
	return (long long) fs.st_size;
}
#endif

