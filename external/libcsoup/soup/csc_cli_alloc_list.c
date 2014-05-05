
/*  csc_cli_alloc_list.c - command line option utility

    Copyright (C) 2011-2013  "Andy Xuming" <xuming@users.sourceforge.net>

    This file is part of CSOUP, Chicken Soup library

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
#include <stdlib.h>
#include <string.h>

#include "libcsoup.h"


char *csc_cli_alloc_list(struct cliopt *optbl)
{
	char	*list, *p;
	int	rc;

	if ((list = smm_alloc((csc_cli_table_size(optbl)+1)*2)) == NULL) {
		return NULL;
	}

	for (p = list; (rc = csc_cli_type(optbl)) != CLI_EOL; optbl++) {
		if (rc & CLI_SHORT) {
			*p++ = optbl->opt_char;
			if (optbl->param > 0) {
				*p++ = ':';
			}
		}
	}
	return list;
}

