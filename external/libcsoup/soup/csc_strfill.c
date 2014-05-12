
/*!\file       csc_strfill.c
   \brief      Padding a string to specified length.

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

/*!\brief Padding a string to specified length.

   \param[in]  s The string which is going to be padded.
   \param[in]  padto The length of the string after padding.
   \param[in]  ch The character to fill in.

   \return    The pointer to the string.
*/
char *csc_strfill(char *s, int padto, int ch)
{
	int	i;

	if ((i = strlen(s)) < padto) {
		for ( ; i < padto; i++) {
			s[i] = (char) ch;
		}
		s[i] = 0;
	}
	return s;
}

