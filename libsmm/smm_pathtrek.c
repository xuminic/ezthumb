
/*  smm_pathtrek.c - process directories recursively

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
#include <string.h>

#include "libsmm.h"


static int dummy_message(void *option, char *path, int type, void *info);
static int path_recur_fifo(struct smmdir *sdir, char *path);
static int path_recur_first(struct smmdir *sdir, char *path);
static int path_recur_last(struct smmdir *sdir, char *path);

int smm_pathtrek(char *path, int flags, F_DIR message, void *option)
{
	struct	smmdir	sdir;
	int	rc;

	memset(&sdir, 0, sizeof(sdir));
	sdir.message = message;
	if (sdir.message == NULL) {
		sdir.message = dummy_message;
	}
	sdir.option = option;
	sdir.flags  = flags;

	switch (flags & SMM_PATH_DIR_MASK) {
	case SMM_PATH_DIR_FIRST:
		sdir.path_recur = path_recur_first;
		break;
	case SMM_PATH_DIR_LAST:
		sdir.path_recur = path_recur_last;
		break;
	case SMM_PATH_DIR_FIFO:
	default:
		sdir.path_recur = path_recur_fifo;
		break;
	}

	rc = sdir.path_recur(&sdir, path);
	sdir.message(sdir.option, path, SMM_MSG_PATH_STAT, &sdir);
	return rc;
}

static int dummy_message(void *option, char *path, int type, void *info)
{
	return 0;
}


#ifdef  CFG_WIN32_API
#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>

static int win_path_recur_fifo(struct smmdir *sdir, TCHAR *wpath)
{
	WIN32_FIND_DATA	ffdata;
	HANDLE		ffdh;
	DWORD		fattr;
	char		*fname;
	int		rc;

	if (SetCurrentDirectory(wpath) == 0) {
		return smm_errno_update(0);
	}

	sdir->stat_dirs++;

	fname = smm_wcstombs(wpath);
	sdir->message(sdir->option, fname, SMM_MSG_PATH_ENTER, sdir);
	free(fname);

	ffdh = FindFirstFile(TEXT("*"), &ffdata);
	if (ffdh == INVALID_HANDLE_VALUE) {
		smm_errno_update(0);
		SetCurrentDirectory(TEXT(".."));
		return smm_errno();
	}

	do {
		//wprintf(TEXT("> %s\n"), ffdata.cFileName);
		if (!lstrcmp(ffdata.cFileName, TEXT(".")) ||
				!lstrcmp(ffdata.cFileName, TEXT(".."))) {
			continue;
		}
		fattr = GetFileAttributes(ffdata.cFileName);
		if (fattr & FILE_ATTRIBUTE_DIRECTORY) {
			rc = win_path_recur_fifo(sdir, ffdata.cFileName);
		} else {
			sdir->stat_files++;
			fname = smm_wcstombs(ffdata.cFileName);
			rc = sdir->message(sdir->option, fname,
					SMM_MSG_PATH_EXEC, sdir);
			free(fname);
		}
		if (rc < 0) {
			fname = smm_wcstombs(ffdata.cFileName);
			rc = sdir->message(sdir->option, fname,
					SMM_MSG_PATH_BREAK, &rc);
			free(fname);
			break;
		}
	} while (FindNextFile(ffdh, &ffdata));

	if (GetLastError() != ERROR_NO_MORE_FILES) {
		smm_errno_update(0);
		SetCurrentDirectory(TEXT(".."));
		return smm_errno();
	}
	FindClose(ffdh);

	fname = smm_wcstombs(wpath);
	sdir->message(sdir->option, fname, SMM_MSG_PATH_LEAVE, sdir);
	free(fname);
	SetCurrentDirectory(TEXT(".."));
	return rc;
}
	
static int path_recur_fifo(struct smmdir *sdir, char *path)
{
	TCHAR	*wpath;
	int	rc;

	if ((wpath = smm_mbstowcs(path)) == NULL) {
		return smm_errno();
	}
	rc = win_path_recur_fifo(sdir, wpath);
	free(wpath);
	return rc;
}

static int path_recur_first(struct smmdir *sdir, char *path)
{
	TCHAR	*wpath;
	int	rc;

	if ((wpath = smm_mbstowcs(path)) == NULL) {
		return smm_errno();
	}
	rc = win_path_recur_fifo(sdir, wpath);
	free(wpath);
	return rc;
}

static int path_recur_last(struct smmdir *sdir, char *path)
{
	TCHAR	*wpath;
	int	rc;

	if ((wpath = smm_mbstowcs(path)) == NULL) {
		return smm_errno();
	}
	rc = win_path_recur_fifo(sdir, wpath);
	free(wpath);
	return rc;
}
#endif	/* CFG_WIN32_API */


#ifdef  CFG_UNIX_API
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define	PATH_STAT_IGNORE	-1
#define PATH_STAT_REGULAR	0
#define PATH_STAT_DIR		1
#define PATH_STAT_SKIP		2

static int path_state(char *path)
{
	struct	stat	fs;

	if (!strcmp(path, ".") || !strcmp(path, "..")) {
		return PATH_STAT_SKIP;	/* skip */
	}
	if (lstat(path, &fs) < 0)  {
		return PATH_STAT_IGNORE;	/* ignore */
	}
	if (S_ISDIR(fs.st_mode)) {
		return PATH_STAT_DIR;	/* directory */
	}
	return PATH_STAT_REGULAR;	/* regular file */
}


static int path_recur_fifo(struct smmdir *sdir, char *path)
{
	DIR	*dir;
	struct	dirent	*de;
	int	rc = 0;

	if (chdir(path) < 0)  {
		return smm_errno_update(0);
	}

	sdir->stat_dirs++;
	sdir->message(sdir->option, path, SMM_MSG_PATH_ENTER, sdir);

	if ((dir = opendir(".")) == NULL)  {
		smm_errno_update(0);
		rc = chdir("..");
		return smm_errno();
	}
	while ((de = readdir(dir)) != NULL)  {
		switch (path_state(de->d_name)) {
		case PATH_STAT_IGNORE:
		case PATH_STAT_SKIP:
			continue;	/* maybe permission denied */
		case PATH_STAT_DIR:
			rc = sdir->path_recur(sdir, de->d_name);
			break;
		case PATH_STAT_REGULAR:
			sdir->stat_files++;
			rc = sdir->message(sdir->option, de->d_name, 
					SMM_MSG_PATH_EXEC, sdir);
			break;
		}
		if (rc < 0) {
			sdir->message(sdir->option, de->d_name,
					SMM_MSG_PATH_BREAK, &rc);
			break;
		}
	}
	closedir(dir);

	sdir->message(sdir->option, path, SMM_MSG_PATH_LEAVE, sdir);

	if (chdir("..") < 0) {
		return smm_errno_update(0);
	}
	return rc;
}

static int path_recur_last(struct smmdir *sdir, char *path)
{
	DIR	*dir;
	struct	dirent	*de = NULL;
	int	rc = 0;

	if (chdir(path) < 0)  {
		return smm_errno_update(0);
	}

	sdir->stat_dirs++;
	sdir->message(sdir->option, path, SMM_MSG_PATH_ENTER, sdir);

	if ((dir = opendir(".")) == NULL)  {
		rc = chdir("..");
		smm_errno_update(0);
		return smm_errno();
	}
	while ((rc >= 0) && ((de = readdir(dir)) != NULL))  {
		if (path_state(de->d_name) == PATH_STAT_REGULAR) {
			sdir->stat_files++;
			rc = sdir->message(sdir->option, de->d_name,
					SMM_MSG_PATH_EXEC, sdir);
		}
	}

	rewinddir(dir);
	while ((rc >= 0) && ((de = readdir(dir)) != NULL))  {
		if (path_state(de->d_name) == PATH_STAT_DIR) {
			rc = sdir->path_recur(sdir, de->d_name);
		}
	}
	if (rc < 0) {
		sdir->message(sdir->option, de->d_name, 
				SMM_MSG_PATH_BREAK, &rc);
	}
	closedir(dir);

	sdir->message(sdir->option, path, SMM_MSG_PATH_LEAVE, sdir);

	if (chdir("..") < 0) {
		return smm_errno_update(0);
	}
	return rc;
}

static int path_recur_first(struct smmdir *sdir, char *path)
{
	DIR	*dir;
	struct	dirent	*de = NULL;
	int	rc = 0;

	if (chdir(path) < 0)  {
		return smm_errno_update(0);
	}

	sdir->stat_dirs++;
	sdir->message(sdir->option, path, SMM_MSG_PATH_ENTER, sdir);

	if ((dir = opendir(".")) == NULL)  {
		smm_errno_update(0);
		rc = chdir("..");
		return smm_errno();
	}

	while ((rc >= 0) && ((de = readdir(dir)) != NULL))  {
		if (path_state(de->d_name) == PATH_STAT_DIR) {
			rc = sdir->path_recur(sdir, de->d_name);
		}
	}

	rewinddir(dir);
	while ((rc >= 0) && ((de = readdir(dir)) != NULL))  {
		if (path_state(de->d_name) == PATH_STAT_REGULAR) {
			sdir->stat_files++;
			rc = sdir->message(sdir->option, de->d_name,
					SMM_MSG_PATH_EXEC, sdir);
		}
	}
	if (rc < 0) {
		sdir->message(sdir->option, de->d_name,
				SMM_MSG_PATH_BREAK, &rc);
	}
	closedir(dir);

	sdir->message(sdir->option, path, SMM_MSG_PATH_LEAVE, sdir);

	if (chdir("..") < 0) {
		return smm_errno_update(0);
	}
	return rc;
}
#endif	/* CFG_UNIX_API */


