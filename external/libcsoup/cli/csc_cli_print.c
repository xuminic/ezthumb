
/*!\file       csc_cli_print.c
   \brief      List the options

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

#include "libcsoup.h"
#include "csoup_internal.h"
#include "csc_cli_private.h"

#define CLI_FIXED_ARGS		"ARGS"
#define CLI_OPT_ARGS		"[ARGS]"

/* The display can be combination of followings
 * ([FF]=Front padding, [BB]=Back padding):
 *
 * Usage: xxx [OPTIONS] xxx xxx xxx ...
 *
 * [FF]-a OPTARG[BB]Comments
 * [FF]-b       [BB]Comments
 *
 * [FF]--long-a OPTARG[BB]Comments
 * [FF]--long-b       [BB]Comments
 *
 * [FF]-a OPTARG         [BB]Comments
 * [FF]-a,--long-a OPTARG[BB]Comments
 * [FF]   --long-b       [BB]Comments
 *
 * [FF]-a OPTARG         [BB]Comments
 * [FF]   --long-b OPTARG[BB]Comments
 */

/* calculate the longest options before comments */
static int csc_cli_find_longest(struct cliopt *optbl)
{
	int	rc, type, clen, optlong = 0;

	type = csc_cli_table_type(optbl);
	while ((rc = csc_cli_type(optbl)) != CLI_EOL) {
		clen = CLI_FRONT_PADDING;
		switch (rc) {
		case CLI_SHORT:
			clen += 2;
			break;
		case CLI_LONG:
			clen += 2 + strlen(optbl->opt_long);
			if (type == CLI_BOTH) {
				clen += 3;
			}
			break;
		case CLI_BOTH:
			clen += 5 + strlen(optbl->opt_long);
			break;
		}
		if (optbl->param == 1) {
			clen += sizeof(CLI_FIXED_ARGS) + 1;
		} else if (optbl->param > 1) {
			clen += sizeof(CLI_OPT_ARGS) + 1;
		}
		if (optbl->comment && (*optbl->comment == '*')) {
			clen = 0;
		}
		if (optlong < clen) {
			optlong = clen;
		}
		optbl++;
	}
	return optlong + CLI_BACK_PADDING;
}

static int csc_cli_format(struct cliopt *optbl, int type, int optlen, 
		char *buf, int blen)
{
	char	tmp[16];
	int	rc;

	if (!buf || (blen <= optlen)) {
		return 0;
	}
	
	*buf = 0;
	switch (csc_cli_type(optbl)) {
	case CLI_COMMENT:
		goto format_end;
	
	case CLI_EXTLINE:
		csc_strfill(buf, optlen, ' ');
		goto format_end;

	case CLI_SHORT:
		csc_strfill(buf, CLI_FRONT_PADDING, ' ');
		sprintf(tmp, "-%c", optbl->opt_char);
		strcat(buf, tmp);
		break;
	case CLI_LONG:
		if (type == CLI_LONG) {
			csc_strfill(buf, CLI_FRONT_PADDING, ' ');
		} else {
			csc_strfill(buf, CLI_FRONT_PADDING + 3, ' ');
		}
		strcat(buf, "--");
		strcat(buf, optbl->opt_long);
		break;
	case CLI_BOTH:
		csc_strfill(buf, CLI_FRONT_PADDING, ' ');
		sprintf(tmp, "-%c,--", optbl->opt_char);
		strcat(buf, tmp);
		strcat(buf, optbl->opt_long);
		break;
	}
	
	if (optbl->param == 1) {
		strcat(buf, " ");
		strcat(buf, CLI_FIXED_ARGS);
	} else if (optbl->param > 1) {
		strcat(buf, " ");
		strcat(buf, CLI_OPT_ARGS);
	}
	csc_strfill(buf, optlen, ' ');

format_end:
	rc = strlen(buf);
	if (optbl->comment != NULL) {
		rc += csc_strlcpy(buf + rc, optbl->comment, blen - rc);
		if (*optbl->comment == '*') {
			rc = 0;
		}
	}
	return rc;
}	

int csc_cli_print(struct cliopt *optbl, int (*show)(char *))
{
	char	sbuf[256];
	int	optlen, type;

	optlen = csc_cli_find_longest(optbl);
	type = csc_cli_table_type(optbl);
	while (csc_cli_type(optbl) != CLI_EOL) {
		if (csc_cli_format(optbl, type, optlen, 
					sbuf, sizeof(sbuf) - 1)) {
			strcat(sbuf, "\n");
			if (show) {
				show(sbuf);
			} else {
				CDB_SHOW(("%s", sbuf));
			}
		}
		optbl++;
	}
	return 0;
}

