
/*  csc_cli_getopt.c - command line option utility

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


void *csc_cli_setopt(void *clibuf, int argc, char **argv)
{
	struct	clirun	*rtbuf = clibuf;

	if (rtbuf) {
		rtbuf->optind = 0;
		rtbuf->optarg = NULL;
		rtbuf->argc   = argc;
		rtbuf->argv   = argv;
	}
	return rtbuf;
}

int csc_cli_getopt(void *clibuf, struct cliopt *optbl)
{
	struct	clirun	*rtbuf = clibuf;
	int	i, rc;

	if (!rtbuf || !optbl) {
		return -1;	/* not available */
	}
	if ((rtbuf->optind == 0) && (rtbuf->argv[0][0] != '-')) {
		rtbuf->optind++;
	}
	if (rtbuf->optind >= rtbuf->argc) {
		return -2;	/* end of scan */
	}
	if (rtbuf->argv[rtbuf->optind][0] != '-') {
		return -2;	/* end of scan */
	}

	for (i = 0; (rc = csc_cli_type(optbl + i)) != CLI_EOL; i++) {
		if (rc == CLI_SHORT) {
			if (rtbuf->argv[rtbuf->optind][1] == 
					optbl[i].opt_char) {
				break;
			}
		} else if (rc == CLI_LONG) {
			if (rtbuf->argv[rtbuf->optind][1] == '-') {
				if (!strcmp(&rtbuf->argv[rtbuf->optind][2], 
							optbl[i].opt_long)) {
					break;
				}
			}
		} else if (rc == CLI_BOTH) {
			if (rtbuf->argv[rtbuf->optind][1] == '-') {
				if (!strcmp(&rtbuf->argv[rtbuf->optind][2],
							optbl[i].opt_long)) {
					break;
				}
			} else {
				if (rtbuf->argv[rtbuf->optind][1] == 
						optbl[i].opt_char) {
					break;
				}
			}
		}
	}
	if (rc == CLI_EOL) {
		return -2;	/* end of scan */
	}

	rtbuf->optind++;
	if (optbl[i].param > 0) {	/* require an option */
		if (rtbuf->optind >= rtbuf->argc) {
			return -3;	/* broken parameters */
		}
		rtbuf->optarg = rtbuf->argv[rtbuf->optind];
		rtbuf->optind++;
	}
	return optbl[i].opt_char;
}


