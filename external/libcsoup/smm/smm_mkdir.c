
/*  smm_mkdir.c - make directory

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
int smm_mkdir(char *path)
{
	TCHAR	*wpath;

	if (path == NULL) {
		return smm_errno_update(SMM_ERR_NONE);
	}
	if ((wpath = smm_mbstowcs_alloc(path)) == NULL) {
		return smm_errno_update(SMM_ERR_NONE_READ);
	}
	if (CreateDirectory(wpath, NULL)) {
		smm_free(wpath);
		return smm_errno_update(SMM_ERR_NONE);
	}
	smm_free(wpath);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		return smm_errno_update(SMM_ERR_NONE);
	}
	return smm_errno_update(SMM_ERR_MKDIR);
}
#endif

#ifdef	CFG_UNIX_API
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

int smm_mkdir(char *path)
{
	//printf("%s\n", path);
	if (path == NULL) {
		return smm_errno_update(SMM_ERR_NONE);
	}
	if (mkdir(path, 0755) == 0) {
		return smm_errno_update(SMM_ERR_NONE);
	}
	if (errno == EEXIST) {	/* path name already exists */
		return smm_errno_update(SMM_ERR_NONE);
	}
	return smm_errno_update(SMM_ERR_MKDIR);
}
#endif

int smm_mkpath(char *path)
{
	char	*pco, *p, store;
	int	rc;

	if (path == NULL) {
		return smm_errno_update(SMM_ERR_NONE);
	}
	if ((pco = csc_strcpy_alloc(csc_strbody(path, NULL), 4)) == NULL) {
		return SMM_ERR_LOWMEM;
	}

	/* remove the tailing whitespaces and keep one delimiter */
	p = pco + strlen(pco) - 1;
	pco--;		/* setup the end mark */
	while ((p != pco) && csc_isdelim(SMM_PATH_DELIM " ", *p)) {
		*p-- = 0;
	}
	strcat(++pco, SMM_DEF_DELIM);	/* and clear the end mark */

	/* UNC in Windows:
	 *   \\ComputerName[@SSL][@Port]\SharedFolder\Resource
	 *   \\?\UNC\ComputerName\SharedFolder\Resource
	 *   \\?\C:\SharedFolder\Resource
	 * Traditional in Windows:
	 *   C:\SharedFolder\Resource
	 *   C:SharedFolder\Resource
	 *   \SharedFolder\Resource
	 *   .\SharedFolder\Resource
	 * POSIX:
	 *   /root/home/user/folder/file
	 *   ./user/folder/file
	 *   user/folder/file
	 */
	/* skip the leading delimiters, dot path, UNC, etc which could
	 * not be mkdir-ed anyway */
#ifdef  CFG_UNIX_API
	for (p = pco; *p && csc_isdelim(SMM_PATH_DELIM ".", *p); p++);
#else	/* CFG_WIN32_API */
	for (p = pco; *p && csc_isdelim(SMM_PATH_DELIM "?.", *p); p++);
	if (isalpha(*p) && (p[1] == ':')) {
		p += 2;
	}
	for ( ; *p && csc_isdelim(SMM_PATH_DELIM ".", *p); p++);
#endif

	rc = SMM_ERR_NONE;
	while (*p) {
		if (csc_isdelim(SMM_PATH_DELIM, *p)) {
			store = *p;
			*p = 0;
			//puts(pco);
			/* we only store the last status of mkdir()
			 * because it maybe fail in the intermediate 
			 * directories, which we don't care actually.
			 * Things happened in Windows UNC like:
			 *   \\VBOXSVR\Shared\Hello
			 * where VBOXSVR and Shared could not create */
			rc = smm_mkdir(pco);
			*p = store;
		}
		p++;
	}
	smm_free(pco);
	return smm_errno_update(rc);
}


