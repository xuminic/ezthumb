/*!\file       csc_strcpy_alloc.c
   \brief      copy a string to a new allocated memory.

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

/*!\brief copy a string to a new allocated memory.

   The csc_strcpy_alloc() function allocates a piece of memory with the same 
   size of string 'src', plus the extra size specified by 'extra', and copies
   the string pointed to by src, including the terminating null byte ('\0'), 
   to the allocated momory.

   \param[in]  src  The pointer to the source string.
   \param[in]  extra Extra bytes required in memory allocation.

   \return  Pointer to the new allocated memory with the copied string, 
            or NULL if the memory can not be allocated.
*/
char *csc_strcpy_alloc(const char *src, int extra)
{
	char	*dst;

	if (src == NULL) {
		return NULL;
	}

	extra = (extra + strlen(src) + 20) / 16 * 16;
	if ((dst = smm_alloc(extra)) == NULL) {
		return NULL;
	}
	return strcpy(dst, src);
}


