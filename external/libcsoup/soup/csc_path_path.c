/*  csc_path_path.c

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

/* return the directory from an input path. 
 * for example:
 * /abc/def/ghi      buffer = /abc/def/         return ghi
 * /abc/def/ghi/     buffer = /abc/def/ghi/     return ""
 * abc               buffer = ./                return abc
 */
char *csc_path_path(char *path, char *buffer, int blen)
{
	int	i;

	for (i = strlen(path) - 1; i >= 0; i--) {
		if (csc_isdelim(SMM_PATH_DELIM, path[i])) {
			break;
		}
	}
	if (i >= 0) {
		i++;
		if (buffer) {
			csc_strlcpy(buffer, path, i > blen ? blen : i);
		}
		return path + i;
	}
	if (buffer && (blen > 3)) {
		strcpy(buffer, ".");
		strcat(buffer, SMM_DEF_DELIM);
	}
	return path;
}

