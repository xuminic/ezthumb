/*  csc_file_load.c - load file contents into memory

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

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "libcsoup.h"

char *csc_file_load(char *path, char *buf, long *len)
{
	FILE	*fp;
	long	n, flen, amnt;

	if ((fp = fopen(path, "r")) == NULL) {
		return NULL;
	}
	if (fseek(fp, 0, SEEK_END) == 0) {	
		flen = ftell(fp);
		rewind(fp);
		if (len && *len && (*len < flen)) {
			flen = *len;
		}
	} else if (buf && len && *len) {	/* unseekable */
		flen = *len;
	} else {
		/* unseekable file need buffer and size */
		fclose(fp);
		return NULL;
	}

	if (buf == NULL) {
		buf = smm_alloc(flen);
	}
	if (buf == NULL) {
		fclose(fp);
		return NULL;
	}

	/* read contents */
	amnt = 0;
	while (amnt < flen) {
		n = fread(buf + amnt, 1, flen - amnt, fp);
		amnt += n;	
	}
	if (len) {
		*len = amnt;
	}
	fclose(fp);
	return buf;
}


