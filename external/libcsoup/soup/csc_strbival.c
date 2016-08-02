
/* csc_strbival.c - extract two values from a string

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
#include <stdlib.h>
#include <string.h>

extern int csc_isdelim(char *delim, int ch);

int csc_strbival_int(char *s, char *delim, int *opt)
{
	char	*next, defdlm[] = " ";
	int	val;

	val = (int)strtol(s, &next, 0);
	
	if (delim == NULL) {
		delim = defdlm;
	}
	while (*next && csc_isdelim(delim, *next)) next++;
	if (opt) {
		*opt = (int) strtol(next, NULL, 0);
	}
	return val;
}


long csc_strbival_long(char *s, char *delim, long *opt)
{
	char	*next, defdlm[] = " ";
	long	val;

	val = strtol(s, &next, 0);
	
	if (delim == NULL) {
		delim = defdlm;
	}
	while (*next && csc_isdelim(delim, *next)) next++;
	if (opt) {
		*opt = strtol(next, NULL, 0);
	}
	return val;
}


