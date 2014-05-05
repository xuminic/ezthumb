
/*  smm_init.c - initialize the SMM library

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

#include "libcsoup.h"

/* Error mask is always 1000 0000 ... in 32-bit error code
 * libsmm error mask uses 0100 0000 ... in 32-bit error code */
int	smm_error_no;


int smm_init(int logcw)
{
	slog_def_open(logcw);
	smm_error_no = SMM_ERR_NONE;
	smm_codepage_reset();
	return SMM_ERR_NONE;
}

int smm_destroy(void)
{
	smm_error_no = 0;
	slog_def_close();
	return 0;
}

int smm_errno(void)
{
	return - smm_error_no;
}

int smm_errno_zip(int err)
{
	if (err == (int) SMM_ERR_NONE_READ) {
		err = smm_error_no;
	}
	return ((err >> 24) | err) & 0xff;
}

int smm_errno_update(int value)
{
	if (value == (int)SMM_ERR_NONE_READ) {
		value = smm_error_no;		/* do nothing */
	} else if (value == (int) SMM_ERR_NONE) {
		smm_error_no = SMM_ERR_NONE;
	} else {
		smm_error_no = SMM_ERR(value);
	}
	return smm_error_no;
}

#if 0
#ifdef  CFG_WIN32_API
int smm_errno_update(int value)
{
	if (value) {
		smm_error_no = value;
	} else {
		smm_error_no = (int) GetLastError();
	}
	return smm_errno();
}
#endif

#ifdef	CFG_UNIX_API
#include <errno.h>

int smm_errno_update(int value)
{
	if (value) {
		smm_error_no = value;
	} else {
		smm_error_no = errno;
	}
	return smm_errno();
}
#endif
#endif	/* 0 */
