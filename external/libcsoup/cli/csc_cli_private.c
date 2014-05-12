
/*  csc_cli_option.c - command line option utility

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

#include "libcsoup.h"
#include "csc_cli_private.h"


int csc_cli_type(struct cliopt *optbl)
{
	int	rc = 0;

	if (optbl->opt_long && *optbl->opt_long) {
		if (isalnum(optbl->opt_char)) {
			rc = CLI_BOTH;		/* 'o', "option", 0, NULL */
		} else {
			rc = CLI_LONG;		/* 1, "option", 0, NULL */
		}
	} else if (isalnum(optbl->opt_char)) {
		rc = CLI_SHORT;			/* 'o', NULL, 0, NULL */
	} else if (optbl->comment == NULL) {
		rc = CLI_EOL;			/* 0, NULL, 0, NULL */
	} else if (optbl->param == -1) {
		rc = CLI_EXTLINE;		/* 0, NULL, -1, "Half line" */
	} else {
		rc = CLI_COMMENT;		/* 0, NULL, 0, "Full line" */
	}
	return rc;
}

int csc_cli_table_size(struct cliopt *optbl)
{
	int	i, rc;

	for (i = 0; (rc = csc_cli_type(optbl)) != CLI_EOL; optbl++) {
		if (rc & CLI_BOTH) {
			i++;
		}
	}
	return i;
}

int csc_cli_table_type(struct cliopt *optbl)
{
	int	rc, type = 0;

	while ((rc = csc_cli_type(optbl)) != CLI_EOL) {
		if (rc == CLI_LONG) {
			type |= CLI_LONG;
		} else if (rc == CLI_SHORT) {
			type |= CLI_SHORT;
		} else if (rc == CLI_BOTH) {
			type |= CLI_BOTH;
		}
		optbl++;
	}
	return type;
}

