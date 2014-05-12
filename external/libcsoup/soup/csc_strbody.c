/*!\file       csc_strbody.c
   \brief      find the body of a string without heading and 
               tailing white spaces

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

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "libcsoup.h"

/*!\brief find the body of a string
 
   The csc_strbody() is uesed to skip the heading and trailing white spaces.
   It returns a pointer to the first non-white-space character in the source
   string and the length of the string without the trailing white-spaces.

   For example, if the input string is "\t\tHello world!   \n\r\v", the return
   pointer points to the 'H' and the length is 12.

   \param[in]   s The source string
   \param[out]  len The effective lenght of the string

   \return The pointer to the first non-white-space character in the string.
   \remark This function does not change the source string.
*/
char *csc_strbody(char *s, int *len)
{
	int	n;

	/* skip the heading white spaces */
	while (*s && SMM_ISSPACE(*s)) s++;

	/* skip the trailing white spaces */
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


