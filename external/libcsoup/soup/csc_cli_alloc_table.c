
/*  csc_cli_alloc_table.c - command line option utility

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
#include <getopt.h>

#include "libcsoup.h"


void *csc_cli_alloc_table(struct cliopt *optbl)
{
	struct	option	*table, *p;
	int	rc;

	table = smm_alloc((csc_cli_table_size(optbl)+1)*sizeof(struct option));
	if (table == NULL) {
		return NULL;
	}

	for (p = table; (rc = csc_cli_type(optbl)) != CLI_EOL; optbl++) {
		if (rc & CLI_LONG) {
			p->name    = optbl->opt_long;
			p->has_arg = optbl->param > 0 ? 1 : 0;
			p->val     = optbl->opt_char;
			p++;
		}
	}
	return (void*) table;
}

