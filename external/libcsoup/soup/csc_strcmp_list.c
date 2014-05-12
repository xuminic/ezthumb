/*!\file       csc_strcmp_list.c
   \brief      compare a list of strings.

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
#include <stdarg.h>

/*!\brief Compare a list of strings.

   The csc_strcmp_list() is used to compare a target string to a list of 
   strings. If one of the string is identical to the target string, The 
   csc_strcmp_list() would return 0 for matching.

   \param[in]  dest The target string.
   \param[in]  src  The first string in the string list. The last member in
               the string list must be NULL.

   \retval     0 if one of the strings are match. 
               non-zero if none of the strings are match.
   \remark     for example: csc_strcmp_list(myext, ".c", ".h", ".cc", NULL);
*/
int csc_strcmp_list(char *dest, char *src, ...)
{
	va_list	ap;
	char	*s;
	int	n;

	va_start(ap, src);
	n = 1;
	s = va_arg(ap, char *);
	while (s) {
		if (!strcmp(dest, s)) {
			va_end(ap);
			return 0;	/* return succeed */
		}
		s = va_arg(ap, char *);
		n++;
	}
	va_end(ap);
	return n;
}


