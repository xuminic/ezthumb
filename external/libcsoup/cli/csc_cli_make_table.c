
/*!\file       csc_cli_make_table.c
   \brief      generate a getopt_long option table

   \author     "Andy Xuming" <xuming@users.sourceforge.net>
   \date       2011-2014
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
#include "csc_cli_private.h"


/*!\brief Generate a getopt_long table by option table

   \param[in]   optbl Pointer to the option table list of 'struct cliopt'.
   \param[out]  oplst Pointer to the option buffer of 'struct option'.
   \param[in]   len   The length of the option buffer in units.

   \return  Pointer to the option table of 'struct option' if succeed, 
            or NULL if failed.
   \remark  This function can be used along with getopt_long().
*/
int csc_cli_make_table(struct cliopt *optbl, struct option *oplst, int len)
{
	int	i, rc;

	for (i = 0; (rc = csc_cli_type(optbl)) != CLI_EOL; optbl++) {
		if (rc & CLI_LONG) {
			if (oplst && (i < (len - 1))) {
				oplst[i].name    = optbl->opt_long;
				oplst[i].has_arg = optbl->param;
				oplst[i].flag    = NULL;
				oplst[i].val     = optbl->opt_char;
			}
			i++;
		}
	}
	oplst[i].name    = NULL;
	oplst[i].has_arg = 0;
	oplst[i].flag    = NULL;
	oplst[i].val     = 0;
	return i;
}

