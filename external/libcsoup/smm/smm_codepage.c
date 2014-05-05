
/*  smm_codepage.c - get codepage of the console

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
#include <string.h>

#include "libcsoup.h"

int	smm_sys_cp;

int smm_codepage(void)
{
	return smm_sys_cp;
}

int smm_codepage_set(int cpno)
{
	int	tmp;

	tmp = smm_sys_cp;
	smm_sys_cp = cpno;
	return tmp;
}

#ifdef	CFG_WIN32_API
int smm_codepage_reset(void)
{
	//return GetConsoleOutputCP();
	smm_sys_cp = GetACP();
	return smm_sys_cp;
}
#endif


#ifdef	CFG_UNIX_API
int smm_codepage_reset(void)
{
	smm_sys_cp = 65001;	/* UTF-8 in WIN32 API */
	return smm_sys_cp;
}
#endif

