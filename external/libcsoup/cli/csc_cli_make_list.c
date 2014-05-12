
/*!\file       csc_cli_make_list.c
   \brief      Generate a short form option string.

   \author     "Andy Xuming" <xuming@users.sourceforge.net>
   \date       2011-2014
*/
/* Copyright (C) 2011-2014  "Andy Xuming" <xuming@users.sourceforge.net>

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


static inline void store_char(char *buf, int idx, int blen, int ch)
{
	if (buf && (idx < (blen - 1))) {
		buf[idx] = (char) ch;
	}
}

/*!\brief Generate a short form option string by option table

   \param[in]  optbl Pointer to the option table list of 'struct cliopt'.
   \param[out] list  Pointer to the buffer of generated option string.
                     It can be ignored by setting NULL pointer.
   \param[in]  len   The size of the buffer 'list'.

   \return  The length of the generated option string.
   \remark  This function can be used along with getopt().
*/
int csc_cli_make_list(struct cliopt *optbl, char *list, int len)
{
	int	i, rc;

	for (i = 0; (rc = csc_cli_type(optbl)) != CLI_EOL; optbl++) {
		if (rc & CLI_SHORT) {
			store_char(list, i++, len, optbl->opt_char);
			if (optbl->param > 0) {
				store_char(list, i++, len, ':');
			}
			if (optbl->param > 1) {
				store_char(list, i++, len, ':');
			}
		}
	}
	store_char(list, i, len, 0);	/* the option string ends by \0 */
	return i;
}

