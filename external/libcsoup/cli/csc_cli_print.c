
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
#include "libcsoup_debug.h"
#include "csc_cli_private.h"


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
static int csc_cli_find_longest(struct cliopt *optbl, int mask)
{
	int	rc, type, clen, optlong = 0;

	type = csc_cli_table_type(optbl);
	while ((rc = csc_cli_type(optbl)) != CLI_EOL) {
		if (!CSC_CLI_SHOW(optbl, mask)) {
			optbl++;
			continue;
		}
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
		if ((CSC_CLI_PARAM(optbl) == 1) || (CSC_CLI_PARAM(optbl) == 3)) {
			clen += 5;	/* sizeof(" ARGS") */
		} else if ((CSC_CLI_PARAM(optbl) == 2) || (CSC_CLI_PARAM(optbl) == 4)) {
			clen += 7;	/* sizeof(" [ARGS]") */
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

	if (!buf || (blen <= optlen)) {
		return 0;
	}
	if (optbl->comment && (*optbl->comment == '*')) {
		return 0;	/* hidden option */
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
	
	switch (CSC_CLI_PARAM(optbl)) {
	case 1:
		strcat(buf, " ARGS");
		break;
	case 2:
		strcat(buf, " [ARGS]");
		break;
	case 3:
		strcat(buf, " NUMS");
		break;
	case 4:
		strcat(buf, " [NUMS]");
		break;
	}
	csc_strfill(buf, optlen, ' ');

format_end:
	return strlen(buf);
}	

int csc_cli_print(struct cliopt *optbl, int mask, int (*show)(char *))
{
	char	sbuf[256];
	int	optlen, type, pflag;

	optlen = csc_cli_find_longest(optbl, mask);
	type = csc_cli_table_type(optbl);
	while (csc_cli_type(optbl) != CLI_EOL) {
		if (!CSC_CLI_SHOW(optbl, mask)) {
			optbl++;
			continue;
		}
		pflag = 0;
		if (csc_cli_format(optbl, type, optlen, 
					sbuf, sizeof(sbuf) - 1)) {
			if (show) {
				show(sbuf);
			} else {
				CDB_SHOW(("%s", sbuf));
			}
			pflag++;
		}
		if (optbl->comment && (*optbl->comment != '*')) {
			if (show) {
				show(optbl->comment);
			} else {
				CDB_SHOW(("%s", optbl->comment));
			}
			pflag++;
		}
		if (pflag) {
			if (show) {
				show("\n");
			} else {
				CDB_SHOW(("\n"));
			}
		}
		optbl++;
	}
	return 0;
}

