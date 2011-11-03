
/*  smm_chdir.c - change directory

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
#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>

int smm_chdir(char *path)
{
	TCHAR	*wpath;

	if ((wpath = smm_mbstowcs(path)) == NULL) {
		return smm_errno();
	}

	if (SetCurrentDirectory(wpath) == 0) {
		smm_errno_update(0);
		free(wpath);
		return smm_errno();
	}

	free(wpath);
	return 0;
}
#endif

#ifdef	CFG_UNIX_API
#include <unistd.h>

int smm_chdir(char *path)
{
	if (chdir(path) < 0) {
		return smm_errno_update(0);
	}
	return 0;
}
#endif

