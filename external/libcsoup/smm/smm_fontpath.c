
/*  smm_fontpath.c - find the full path of a specified font

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
#define CSOUP_DEBUG_LOCAL       SLOG_CWORD(CSOUP_MOD_CONFIG, SLOG_LVL_ERROR)
//#define CSOUP_DEBUG_LOCAL     SLOG_CWORD(CSOUP_MOD_CONFIG, SLOG_LVL_MODULE)
#include "libcsoup_debug.h"

#ifdef	__CYGWIN__
#include <wchar.h>
#endif

struct	FTINF	{
	char	*ftpath;
	char	*ftname;	/* output */
};


#ifdef	CFG_WIN32_API
static	char	*def_font_dir[] = {
	NULL
};
#else
static	char	*def_font_dir[] = {
	"/usr/share/fonts",
	NULL
};
#endif

static int findfont(void *option, char *path, int type, void *info);
static char *find_sep(char *path);
//static char *find_rsep(char *path);

char *smm_fontpath(char *ftname, char **userdir)
{
	struct	FTINF	ftinfo;
	char	*home;
	int	i;
#ifdef	CFG_WIN32_API
	TCHAR	wpbuf[MAX_PATH];
#else
	char	*bpath;
#endif

	if (ftname == NULL) {
		return NULL;
	}

	/* absolute path */
	if (find_sep(ftname)) {
		if (smm_fstat(ftname) != SMM_FSTAT_REGULAR) {
			return NULL;
		}
		return csc_strcpy_alloc(ftname, 0);
	}
	/* try current directory first */
	if (smm_fstat(ftname) == SMM_FSTAT_REGULAR) {
		if ((home = smm_alloc(strlen(ftname)+16)) != NULL) {
#ifdef	CFG_WIN32_API
			strcpy(home, ".\\");
#else
			strcpy(home, "./");
#endif
			strcat(home, ftname);
		}
		return home;
	}

	/* try the user specified search path if existed */
	if (userdir) {
		for (i = 0; userdir[i]; i++) {
			ftinfo.ftpath = ftname;
			ftinfo.ftname = NULL;
			smm_pathtrek(userdir[i], 0, findfont, &ftinfo);
			if (ftinfo.ftname) {
				return ftinfo.ftname;
			}
		}
	}

#ifdef	CFG_WIN32_API
	/* search Windows system font */
	if (GetWindowsDirectory(wpbuf, MAX_PATH)) {
		wcsncat(wpbuf, TEXT("\\Fonts"), MAX_PATH);
		if ((home = smm_wcstombs_alloc(wpbuf)) == NULL) {
			return NULL;
		}

		ftinfo.ftpath = ftname;
		ftinfo.ftname = NULL;
		smm_pathtrek(home, 0, findfont, &ftinfo);
		if (ftinfo.ftname) {
			smm_free(home);	/* FIXME: produced a memory hole */
			return ftinfo.ftname;
		}
		smm_free(home);
	}
#else
	/* try the user fonts in the home directory */
	/* MinGW and Cygwin have $HOME */
	if ((bpath = getenv("HOME")) != NULL) {
		if ((home = csc_strcpy_alloc(bpath, 16)) == NULL) {
			return NULL;
		}
		/* Unix type environment so no need to '\\' */
		strcat(home, "/.fonts");

		ftinfo.ftpath = ftname;
		ftinfo.ftname = NULL;
		smm_pathtrek(home, 0, findfont, &ftinfo);
		if (ftinfo.ftname) {
			smm_free(home);	/* FIXME: produced a memory hole */
			return ftinfo.ftname;
		}
		smm_free(home);
	}
#endif

	/* try the default search path */
	for (i = 0; def_font_dir[i]; i++) {
		ftinfo.ftpath = ftname;
		ftinfo.ftname = NULL;
		smm_pathtrek(def_font_dir[i], 0, findfont, &ftinfo);
		if (ftinfo.ftname) {
			return ftinfo.ftname;
		}
	}
	return NULL;
}

static int findfont(void *option, char *path, int type, void *info)
{
	struct	FTINF	*ftinfo = option;

	(void)info;		/* stop the gcc warning */
	switch (type) {
	case SMM_MSG_PATH_ENTER:
		CDB_MODL(("Entering %s:\n", path));
		break;
	case SMM_MSG_PATH_EXEC:
#ifdef	CFG_WIN32_API
		if (strcasecmp(ftinfo->ftpath, path)) {
#else
		if (strcmp(ftinfo->ftpath, path)) {
#endif
			break;
		}
		ftinfo->ftname = smm_cwd_alloc(strlen(path) + 8);
		if (ftinfo->ftname == NULL) {
			break;
		}
		strcat(ftinfo->ftname, SMM_DEF_DELIM);
		strcat(ftinfo->ftname, path);
		CDB_MODL(("Found %s\n", ftinfo->ftname));
		return SMM_NTF_PATH_EOP;

	case SMM_MSG_PATH_BREAK:
		CDB_MODL(("Failed to process %s\n", path));
		break;
	case SMM_MSG_PATH_LEAVE:
		CDB_MODL(("Leaving %s\n", path));
		break;
	}
	return SMM_NTF_PATH_NONE;
}

static char *find_sep(char *path)
{
	while (*path) {
		if (csc_isdelim(SMM_PATH_DELIM, *path)) {
			return path;
		}
		path++;
	}
	return NULL;
}

/*
static char *find_rsep(char *path)
{
	int	i;

	for (i = strlen(path) - 1; i >= 0; i--) {
		if (csc_isdelim(SMM_PATH_DELIM, path[i])) {
			return &path[i];
		}
	}
	return NULL;
}
*/

