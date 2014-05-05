/*  csc_strbody.c - find the body of a string without heading and 
    tailing white spaces

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

char *csc_strbody(char *s, int *len)
{
	int	n;

	/* skip the heading white spaces */
	while (*s && SMM_ISSPACE(*s)) s++;

	/* skip the tailing white spaces */
	n = strlen(s);
	if (n > 0) {
		n--;
		while ((n >= 0) && SMM_ISSPACE(s[n])) n--;
		n++;
	}

	if (len) {
		*len = n;
	}
	return s;
}


