/*!\file       csc_strcount_char.c
   \brief      counting the number of specified char in a string 

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

/*!\brief counting the occurrance of specified chars.
 
   The csc_strcount_char() is uesed to counting the occurrance of specified 
   chars. It returns the total occurrance of the chars in the source string.

   For example, if the source string is "Hello world!" and the acct string
   is "lo", it then returns 5 because there are 3 'l' and 2 'o'.

   \param[in]   s The source string
   \param[in]   acct Specifies a set of chars that for counting.

   \return The number of total occurrance of the chars
   \remark This function does not change the source string.
*/
int csc_strcount_char(char *s, char *acct)
{
	int	i, n = 0;

	while (*s) {
		for (i = 0; acct[i]; i++) {
			if (*s == acct[i]) {
				n++;
				break;
			}
		}
		s++;
	}
	return n;
}


