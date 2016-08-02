
/*  smm_userpath.c - find the default user path

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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libcsoup.h"

#ifdef	CFG_WIN32_API
#include "Shlobj.h"
#endif

char *smm_userpath(char *buffer, int len)
{
#ifdef	CFG_WIN32_API
	TCHAR	wpbuf[MAX_PATH];

	*buffer = 0;
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL,0, wpbuf))) {
		WideCharToMultiByte(smm_codepage(), 0, 
				wpbuf, -1, buffer, len, NULL, NULL);
	}
#else
	char	*bpath;
	
	/* MinGW and Cygwin have $HOME */
	*buffer = 0;
	if ((bpath = getenv("HOME")) != NULL) {
		csc_strlcpy(buffer, bpath, len);
	}
#endif
	return buffer;
}


