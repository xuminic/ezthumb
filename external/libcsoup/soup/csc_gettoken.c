
/* csc_gettoken - extract tokens from a string (damageless/copy)

   Copyright (C) 1998-2014  Xuming <xuming@users.sourceforge.net>
   
   This program is free software; you can redistribute it and/or 
   modify it under the terms of the GNU General Public License as 
   published by the Free Software Foundation; either version 2, or 
   (at your option) any later version.
	   
   This program is distributed in the hope that it will be useful, 
   but WITHOUT ANY WARRANTY; without even the implied warranty of 
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License, the file COPYING in this directory, for
   more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <ctype.h>
#include <stdio.h>
#include <string.h>

extern int csc_isdelim(char *delim, int ch);

char *csc_gettoken(char *sour, char *buffer, int blen, char *delim)
{
	if (*sour == 0) {	/* end of process */
		return NULL;
	}

	blen--;		/* leave a space for asc0 */
	while (*sour && blen && !csc_isdelim(delim, *sour)) {
		*buffer++ = *sour++;
		blen--;
	}
	*buffer++ = 0;

	if (*sour != 0) {	/* more tokens are expected */
		sour++;
	}
	return sour;	/* return the address of the next token */ 
}

