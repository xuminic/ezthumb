/*  csc_path_basename.c

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
#include <string.h>

#include "libcsoup.h"

/* return the base name from an input path. 
 * Note the returned pointer points to the input string.
 * If 'buffer' were supplied, a copy would be made to it.
 * for example:
 * /abc/def/ghi      return  ghi
 * /abc/def/ghi/     return  ""
 * abc               return  abc
 */
char *csc_path_basename(char *path, char *buffer, int blen)
{
	char	*p;

	for (p = path + strlen(path) - 1; p >= path; p--) {
		if (csc_isdelim(SMM_PATH_DELIM, *p)) {
			break;
		}
	}
	p++;
	if (buffer) {
		csc_strlcpy(buffer, p, blen);
	}
	return p;
}


