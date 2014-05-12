/*!\file      csc_path_basename.c
   \brief     Retrieve the basename of a path 

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

/* return the base name from an input path. 
 * Note the returned pointer points to the input string.
 * If 'buffer' were supplied, a copy would be made to it.
 * for example:
 * /abc/def/ghi      return  ghi
 * /abc/def/ghi/     return  ""
 * abc               return  abc
 */
/*!\brief Retrieve the base name of a path

   The csc_path_basename() finds the base name inside a specified path
   according to the structure of the path itself. If the user 'buffer'
   was given, the base name would be copied into the 'buffer' too.

   \param[in]   path The full path
   \param[out]  buffer The buffer where stores the base name if it's not NULL.
   \param[in]   blen The length of the buffer. If the lenght of buffer 
                is shorter than the basename, it will be truncated.
   \return      Pointer to the base name inside the input 'path'.
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


