/*!\file       csc_path_path.c
   \brief      split the path to the directory and the file name.

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
#include <string.h>

#include "libcsoup.h"

/* return the directory from an input path. 
 */
/*!\brief Split the path to the possible directory and the file name.
   
   The csc_path_path() function splits the input path, according to the 
   structure of the path itself, to directory part and the file name part.
   For example:
     /abc/def/ghi      buffer = /abc/def/         return ghi
     /abc/def/ghi/     buffer = /abc/def/ghi/     return ""
     abc               buffer = ./                return abc

   \param[in]  path The full path
   \param[out] buffer The buffer where stores the splitted directory.
               The returned directory should be always having a trailing 
	       path delimiter like '/' or '\\'. If buffer is NULL, nothing
	       would be copied.
   \param[in]  blen The length of the buffer. If the lenght of buffer 
               is shorter than the splitted directory, it will be truncated.

   \return     Pointer to the last member in the path
   \remark     The file name part could be a directory too because this 
               function doesn't check the file attribution.
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
		i += 2;
		if (buffer) {
			csc_strlcpy(buffer, path, i > blen ? blen : i);
		}
		return path + i - 1;
	}
	if (buffer && (blen > 3)) {
		strcpy(buffer, ".");
		strcat(buffer, SMM_DEF_DELIM);
	}
	return path;
}

