
/*!\file       csc_strcmp_param.c
   \brief      Compare two parameter strings. 
               Empty or blank string is equal to NULL string.

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

/*!\brief Compare two parameter strings while empty or blank string is equal 
   to a NULL string pointer.

   The function compares the two parameter strings 's1' and 's2'.  It returns 
   an integer less than, equal to, or greater than zero if 's1' is found, 
   respectively, to be less than, to match, or be greater than 's2'.

   The difference between csc_strcmp_param() and strcmp() is 's1' and 's2'
   can be NULL and equal to empty string ("") or blank string ("\t\f\v\n\r ")

   \param[in]  s1  string 1, can be NULL.
   \param[in]  s2  string 2, can be NULL.

   \return     an integer less than, equal to, or greater than zero if 's1' is
   found, respectively, to be less than, to match, or be greater than 's2'.
*/
int csc_strcmp_param(char *s1, char *s2)
{
	int	len1, len2;

	//printf("csc_strcmp_param: {%s}{%s}\n", dest, sour);
	len1 = len2 = 0;
	if (s1) {
		s1 = csc_strbody(s1, &len1);
	}
	if (s2) {
		s2 = csc_strbody(s2, &len2);
	}
	if (s1 && s2) {
		return strncmp(s1, s2, len1 < len2 ? len2 : len1);
	} else if (!s1 && !s2) {
		return 0;
	} else if (s1) {
		return *s1;
	}
	return *s2;
}

