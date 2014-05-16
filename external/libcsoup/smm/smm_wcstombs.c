
/*  smm_wcstombs - convert the wide character string to multibyte string

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

#ifdef  CFG_WIN32_API
char *smm_wcstombs_alloc(void *wcs)
{
	char	*buf;
	int	len;

	smm_errno_update(SMM_ERR_NONE);
	if (wcs == NULL) {
		return NULL;
	}
	len = WideCharToMultiByte(smm_codepage(), 
			0, wcs, -1, NULL, 0, NULL, NULL);
	if (len <= 0) {
		smm_errno_update(SMM_ERR_LENGTH);
		return NULL;
	}
	if ((buf = smm_alloc(len + 1)) == NULL) {
		smm_errno_update(SMM_ERR_LOWMEM);
		return NULL;
	}
	WideCharToMultiByte(smm_codepage(), 0, wcs, -1, buf, len, NULL, NULL);
	return buf;
}
#endif

#ifdef  CFG_UNIX_API
#include <errno.h>

char *smm_wcstombs_alloc(void *wcs)
{
	char	*buf;
	int	len;

	smm_errno_update(SMM_ERR_NONE);
	if (wcs == NULL) {
		return NULL;
	}
	if ((len = wcstombs(NULL, wcs, 0)) == 0) {
		smm_errno_update(SMM_ERR_LENGTH);
		return NULL;
	}
	if ((buf = smm_alloc(len + 1)) == NULL) {
		smm_errno_update(SMM_ERR_LOWMEM);
		return NULL;
	}
	wcstombs(buf, wcs, len + 1);
	return buf;
}
#endif
	

