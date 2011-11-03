/* History:
 * 20111103: V1.0.0.0 Port to ezthum project
*/

/*  libsmm.h - head file of the SMM library

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
#ifndef	_LIBSMM_H_
#define _LIBSMM_H_

#define	SMM_VERSION		((1 << 24)| (0 << 16) | (0 << 8) | 0)

#define	SMM_FSTAT_REGULAR	0
#define SMM_FSTAT_DIR		1
#define SMM_FSTAT_DEVICE	2
#define SMM_FSTAT_LINK		3

#define SMM_PATH_DIR_FIFO	0
#define SMM_PATH_DIR_FIRST	1
#define SMM_PATH_DIR_LAST	2
#define SMM_PATH_DIR_MASK	3

#define SMM_MSG_PATH_ENTER	0
#define SMM_MSG_PATH_LEAVE	1
#define SMM_MSG_PATH_EXEC	2
#define SMM_MSG_PATH_STAT	3
#define SMM_MSG_PATH_BREAK	4


struct	smmdir	{
	int	flags;

	int	stat_dirs;
	int	stat_files;

	int     (*message)(void *option, char *path, int type, void *info);
	void	*option;

	int	(*path_recur)(struct smmdir *sdir, char *path);
};

typedef int (*F_DIR)(void*, char*, int, void*);

#ifdef	CFG_WIN32_API
#define	SMM_TIME	FILETIME
#else	/* CFG_UNIX_API */
typedef	struct timeval	SMM_TIME;
#endif

extern	int	smm_error_no;
extern	int	smm_sys_cp;

int smm_chdir(char *path);
int smm_codepage(void);
char *smm_cwd_alloc(void);
int smm_cwd_pop(int cwid);
int smm_cwd_push(void);
int smm_errno(void);
int smm_errno_update(int value);
int smm_fstat(char *fname);
int smm_init(void);
int smm_pathtrek(char *path, int flags, F_DIR message, void *option);
int smm_pwuid(char *uname, long *uid, long *gid);
int smm_signal_break(int (*handle)(int));
int smm_time_diff(SMM_TIME *tmbuf);
int smm_time_get_epoch(SMM_TIME *tmbuf);
void *smm_mbstowcs(char *mbs);
char *smm_wcstombs(void *wcs);

#endif

