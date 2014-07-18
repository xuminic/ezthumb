
/*!\file       csc_extname_filter.c
   \brief      selection files by extension name

   Extended File Name Filter setting
   Input string could be "avi,wmv,mkv" or "+avi:wmv:-mkv"
   Stored format will be "*.avi", "*.wmv", "*.mkv"

   \author     "Andy Xuming" <xuming@users.sourceforge.net>
   \date       2013-2014
*/
/* Copyright (C) 1998-2014  "Andy Xuming" <xuming@users.sourceforge.net>

   This file is part of CSOUP library, Chicken Soup for the C

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

typedef struct  {
	char    *filter[1];
} CSEFF;

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

int csc_extname_filter_match(void *efft, char *fname)
{
	CSEFF	*flt = efft;

	if (flt == NULL) {
		return 1;	/* no filter means total matched */
	}
	if (*flt->filter == NULL) {
		return 1;
	}
	if (!csc_cmp_file_extlist(fname, flt->filter)) {
		return 1;
	}
	return 0;	/* not matched */
}

int csc_extname_filter_export(void *efft, char *buf, int blen)
{
	CSEFF	*flt = efft;
	int	i, idx, ftlen;

	for (i = idx = 0; flt->filter[i]; i++) {
		ftlen = (int) strlen(flt->filter[i]) + 1;
		if (buf && (idx + ftlen < blen)) {
			strcpy(buf + idx, flt->filter[i]);
			strcat(buf + idx, ",");
		}
		idx += ftlen;
	}
	if (idx) {
		idx--;
		if (buf) {
			buf[idx] = 0;
		}
	}
	return idx;
}

char *csc_extname_filter_export_alloc(void *efft)
{
	char	*s;
	int	len;

	len = csc_extname_filter_export(efft, NULL, 0) + 4;
	if ((s = smm_alloc(len)) != NULL) {
		csc_extname_filter_export(efft, s, len);
	}
	return s;
}



