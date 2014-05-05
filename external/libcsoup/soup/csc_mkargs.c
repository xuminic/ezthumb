
/* csc_mkargs - make command line arguments
 
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
#include <string.h>

/* isspace() macro has a problem in Cygwin when compiling it with -mno-cygwin.
 * I assume it is caused by minGW because it works fine with cygwin head files.
 * The problem is it treats some Chinese characters as space characters.
 * A sample is: 0xC5 0xF3 0xD3 0xD1 */
#define IsSpace(c)	((((c) >= 9) && ((c) <= 0xd)) || ((c) == 0x20))
#define IsPipe(c)	(((c) == '>') || ((c) == '<') || ((c) == '|'))

#define MK_ARG_DELIM		0
#define MK_ARG_TOKEN		1
#define MK_ARG_SGL_QU		2
#define MK_ARG_DBL_QU		3
#define MK_ARG_PIPE		4

/* mkargv - make argument list
 
   This function splits the string into an argument list in the traditional
   (int argc, char *argv[]) form. It uses a very simple state machine so you
   couldn't expect too much features from here. 

   sour  - the input string
   idx   - the string array for storing tokens
   ids   - the maximem number of tokens, the size of "idx"

   It returns the number of token extracted.

   NOTE:  'sour' will be changed.
*/
int csc_mkargv(char *sour, char **idx, int ids)
{
	int	i = 0, stat;

	stat = MK_ARG_DELIM;
	while (*sour) {
		switch (stat) {
		case MK_ARG_DELIM:	/* last char is the delimiter */
			if (IsSpace(*sour)) {
				*sour = 0;
			} else if (*sour == '\'') {
				*sour = 0;
				idx[i++] = sour + 1;
				stat = MK_ARG_SGL_QU;
			} else if (*sour == '"') {
				*sour = 0;
				idx[i++] = sour + 1;
				stat = MK_ARG_DBL_QU;
			} else if (IsPipe(*sour)) {
				*sour = 0;
				stat = MK_ARG_PIPE;
			} else {
				idx[i++] = sour;
				stat = MK_ARG_TOKEN;
			}
			break;

		case MK_ARG_TOKEN:
			if (IsSpace(*sour)) {
				*sour = 0;
				stat = MK_ARG_DELIM;
			} else if (*sour == '\'') {
				*sour = 0;
				idx[i++] = sour + 1;
				stat = MK_ARG_SGL_QU;
			} else if (*sour == '"') {
				*sour = 0;
				idx[i++] = sour + 1;
				stat = MK_ARG_DBL_QU;
			} else if (IsPipe(*sour)) {
				*sour = 0;
				stat = MK_ARG_PIPE;
			}
			break;

		case MK_ARG_SGL_QU:
			if (*sour == '\'') {
				*sour = 0;
				stat = MK_ARG_DELIM;
			}
			break;

		case MK_ARG_DBL_QU:
			if (*sour == '"') {
				*sour = 0;
				stat = MK_ARG_DELIM;
			}
			break;
		}
		if (stat == MK_ARG_PIPE) {
			break;
		}
		if (i >= ids) {
			i = ids - 1;	/* the last argment must be NULL */
			break;
		}
		sour++;
	}
	idx[i] = NULL;
	return i;
}
	
