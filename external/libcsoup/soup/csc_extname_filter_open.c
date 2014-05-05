
/*  csc_extname_filter_open.c

    Copyright (C) 2013  "Andy Xuming" <xuming@users.sourceforge.net>

    This file is part of CSOUP, Chicken Soup library

    CSOUP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CSOUP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libcsoup.h"


void *csc_extname_filter_open(char *s)
{
	CSEFF	*flt;
	int	len, fno;
	char	*tmp;

	len = strlen(s);
	fno = len / 2;
	len += fno * sizeof(char*) + sizeof(CSEFF) + 16;
	if ((flt = smm_alloc(len)) == NULL) {
		return NULL;
	}

	memset(flt, 0, len);
	tmp = (char*) &flt->filter[fno];
	strcpy(tmp, s);
	len = csc_ziptoken(tmp, flt->filter, fno, ",;:");
	flt->filter[len] = NULL;
	return flt;
}

int csc_extname_filter_close(void *efft)
{
	smm_free(efft);
	return 0;
}

