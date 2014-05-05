
/* csc_fixtoken - extract tokens from a string
 
   Version 1.1
   Version 1.2 20090401
     remove the safe_strncpy() function; make test program easier
     add mkargv(), a simple command line parser

   Copyright (C) 1998-2013  Xuming <xuming@users.sourceforge.net>
   
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
#include "libcsoup.h"


/* This function splits the string into tokens. It extracts everything 
   between delimiter.
 
   sour  - the input string
   idx   - the string array for storing tokens
   ids   - the maximem number of tokens, the size of "idx"
   delim - the delimiter array, each character in the array is a delimiter.

   It returns the number of extracted tokens.

   For example, fixtoken("#abc  wdc:have:::#:debug", idx, 16, "# :") returns
   10 tokens "", "abc", "", "wdc", "have", "", "", "", "" and "debug".
   
   NOTE:  'sour' will be changed.
*/

int csc_fixtoken(char *sour, char **idx, int ids, char *delim)
{
	int	i;

	for (i = 0; i < ids; idx[i++] = NULL);
    
	i = 0;
	for (idx[i++] = sour; *sour && (i < ids); sour++)  {
		if (csc_isdelim(delim, *sour))  {
			*sour = 0;
			idx[i++] = sour + 1;
		}
    	}
	return i;
}

char **csc_fixtoken_copy(char *sour, char *delim, int *ids)
{
	char	**token;
	int	i, n;

	for (i = n = 0; sour[i]; i++) {
		if (csc_isdelim(delim, sour[i])) {
			n++;
		}
	}
	n++;	/* safty edge */

	if ((token = smm_alloc(n*sizeof(char*) + strlen(sour) + 16)) == NULL) {
		return NULL;
	}
	strcpy((char*)&token[n], sour);

	n = csc_fixtoken((char*)&token[n], token, n, delim);
	if (ids) {
		*ids = n;
	}
	return token;
}


