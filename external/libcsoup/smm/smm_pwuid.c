
/*  smm_pwuid.c - retrieve the specified user's uid and gid

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

#ifdef	CFG_WIN32_API
int smm_pwuid(char *uname, long *uid, long *gid)
{
	/* stop the gcc complaining */
	(void) uname;
	(void) uid;
	(void) gid;
	return 0;
}
#endif

#ifdef	CFG_UNIX_API
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>

int smm_pwuid(char *uname, long *uid, long *gid)
{
	struct	passwd	pwd, *result;
	char	*buf;
	int	rc;
	size_t	bufsize;

	(void)uname;	/* stop the gcc complaining */
	bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
	if (bufsize == (size_t) -1) {		/* Value was indeterminate */
		bufsize = 16384;	/* Should be more than enough */
	}
	if ((buf = smm_alloc(bufsize)) == NULL) {
		return smm_errno_update(SMM_ERR_LOWMEM);
	}
	if ((rc = getpwnam_r(optarg, &pwd, buf, bufsize, &result)) != 0) {
		rc = smm_errno_update(SMM_ERR_PWNAM);
	} else {
		if (uid) {
			*uid = (long) pwd.pw_uid;
		}
		if (gid) {
			*gid = (long) pwd.pw_gid;
		}
	}
	smm_free(buf);
	return rc;
}
#endif
