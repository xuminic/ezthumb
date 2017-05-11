/*!\file       csc_strcount_str.c
   \brief      counting the occurrance of specified string in a string 

   \author     "Andy Xuming" <xuming@users.sourceforge.net>
   \date       2017-05-11
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

/*!\brief counting the occurrance of specified substring.
 
   The csc_strcount_str() is uesed to counting the occurrance of specified 
   string. It returns the total occurrance of the string in the source string.
   Once it matches a string, it would skip the matched string to look for
   the next match, therefore for example, 

   if the source string is "ssssss" and the substring is "sss", it will return
   2 rather than 4.

   \param[in]   s The source string
   \param[in]   needle The substring.

   \return The number of total occurrance of the substring.
   \remark This function does not change the source string.
*/
int csc_strcount_str(char *s, char *needle)
{
	int	n = 0, span = strlen(needle);

	while (strstr(s, needle) != NULL) {
		n++;
		s += span;
	}
	return n;
}


