
/*  smm_fopen.c - open a file

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
FILE *smm_fopen(char *path, char *mode)
{
	TCHAR   *wpath, *wmode;
	FILE    *fp;
			
	if ((wpath = smm_mbstowcs_alloc(path)) == NULL) {		
		smm_errno_update(SMM_ERR_NONE_READ);
		return NULL;
	}
	if ((wmode = smm_mbstowcs_alloc(mode)) == NULL) {
		smm_errno_update(SMM_ERR_NONE_READ);
		smm_free(wpath);
		return NULL;
	}

	if ((fp = _wfopen(wpath, wmode)) == NULL) {
		smm_errno_update(SMM_ERR_FOPEN);
	} else {
		smm_errno_update(SMM_ERR_NONE);
	}

	smm_free(wmode);
	smm_free(wpath);
	return fp;
}
#endif

#ifdef	CFG_UNIX_API
FILE *smm_fopen(char *path, char *mode)
{
	FILE	*fp;

	if ((fp = fopen(path, mode)) == NULL) {
		smm_errno_update(SMM_ERR_FOPEN);
	} else {
		smm_errno_update(SMM_ERR_NONE);
	}
	return fp;
}
#endif

#if 0
void *smm_fopen(char *path, char *mode)
{
	TCHAR	*wpath;
	DWORD	dwDesiredAccess, dwCreationDisposition;
	HANDLE	fp;

	if ((wpath = smm_mbstowcs_alloc(path)) == NULL) {
		smm_errno_update(SMM_ERR_NONE_READ);
		return NULL;
	}

	dwDesiredAccess = GENERIC_READ;
	dwCreationDisposition = OPEN_EXISTING;
	while (*mode) {
		if (!strncmp(mode, "r+", 2)) {
			/* Open for reading and writing.  The stream is 
			 * positioned at the beginning of the file.*/
			dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
			dwCreationDisposition = OPEN_EXISTING;
			mode += 2;
		} else if (!strncmp(mode, "w+", 2)) {
			/* Open for reading and writing.  The file is created
			 * if it does not exist, otherwise it is truncated. The
			 * stream is positioned at the beginning of the file.*/
			dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
			dwCreationDisposition = CREATE_ALWAYS;
			mode += 2;
		} else if (!strncmp(mode, "a+", 2)) {
			/* Open for reading and appending (writing at end of 
			 * file). The file is created if it does not exist.The
			 * initial file position for reading is at the begin-
			 * ning of the file, but output is always appended to 
			 * the end of the file. 
			 * FIXME: this can not be fully simulated */
			dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
			dwCreationDisposition = OPEN_ALWAYS;
			mode += 2;
		} else if (*mode == 'r') {
			/* Open text file for reading. The stream is 
			 * positioned at the beginning of the file. */
			dwDesiredAccess = GENERIC_READ;
			dwCreationDisposition = OPEN_EXISTING;
			mode++;
		} else if (*mode == 'w') {
			/* Truncate file to zero length or create text file 
			 * for writing. The stream is positioned at the begin-
			 * ning of the file. */
			dwDesiredAccess = GENERIC_WRITE;
			dwCreationDisposition = CREATE_ALWAYS;
			mode++;
		} else if (*mode == 'a') {
			/* Open for appending (writing at end of file). The 
			 * file is created if it does not exist. The stream 
			 * is positioned at the end of the file. */
			dwDesiredAccess = GENERIC_WRITE;
			dwCreationDisposition = OPEN_ALWAYS;
			mode++;
		} else if (*mode == 'b') {
			mode++;	/* ignored the 'b' switch */
		} else {
			smm_free(wpath);
			smm_errno_update(SMM_ERR_OBJECT);
			return NULL;
		}
	}

	/* We don't use CREATE_ALWAYS actually. We use it to simulate 
	 * the Posix fopen's doing, create-truncating */
	if (dwCreationDisposition == CREATE_ALWAYS) {
		fp = CreateFile(wpath, dwDesiredAccess, 0, NULL,
			TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fp == INVALID_HANDLE_VALUE) {  /* file not existed */
			fp = CreateFile(wpath, dwDesiredAccess, 0, NULL,
				CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		}
	} else {
		fp = CreateFile(wpath, dwDesiredAccess, 0, NULL, 
			dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
	}

	smm_free(wpath);
	if (fp == INVALID_HANDLE_VALUE) {
		smm_errno_update(SMM_ERR_FOPEN);
		return NULL;
	}

	/* The OPEN_ALWAYS implied the append mode */
	if (dwCreationDisposition == OPEN_ALWAYS) {
		SetFilePointer(fp, 0, NULL, FILE_END);
	}
	smm_errno_update(SMM_ERR_NONE);
	return fp;
}

int smm_fclose(void *fp)
{
	if (fp) {
		return CloseHandle(fp);
	}
	return 0;
}
#endif

