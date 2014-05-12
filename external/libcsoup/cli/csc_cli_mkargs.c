
/*!\file       cli/csc_cli_mkargs.c
   \brief      a simple command line parser.

   \author     "Andy Xuming" <xuming@users.sourceforge.net>
   \date       1998-2014
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
#include "libcsoup.h"

#define IsPipe(c)	(((c) == '>') || ((c) == '<') || ((c) == '|'))

#define MK_ARG_DELIM		0
#define MK_ARG_TOKEN		1
#define MK_ARG_SGL_QU		2
#define MK_ARG_DBL_QU		3
#define MK_ARG_PIPE		4

/*!\brief make a argc/argv style parameter list 

   This csc_mkargv() function splits the input source string to a parameter 
   list like 'char *argv[]' and return the lenght of the list like 'int argc'.
   The split and parse process will be end at '\0' in the source string, or
   at the pipe characters like '>', '<' and '|'. The substring inside single
   quotation or double quotation mark will be treated as a whole parameter.

   \param[in]   sour  The source string.
   \param[out]  idx   The string array for storing the output parameters.
   \param[in]   ids   The size of 'idx' array.
       
   \retval  The number of parameters
   \remark  The source string 'sour' will be damaged.
   \remark  This function doesn't expand environment variables.
*/
int csc_cli_mkargv(char *sour, char **idx, int ids)
{
	int	i = 0, stat;

	stat = MK_ARG_DELIM;
	while (*sour) {
		switch (stat) {
		case MK_ARG_DELIM:	/* last char is the delimiter */
			if (SMM_ISSPACE(*sour)) {
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
			if (SMM_ISSPACE(*sour)) {
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
	
