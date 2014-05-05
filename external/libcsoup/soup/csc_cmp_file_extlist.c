
/*  csc_cmp_file_extlist.c

    Copyright (C) 2013  "Andy Xuming" <xuming@users.sourceforge.net>

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
#include <stdio.h>
#include <string.h>

#include "libcsoup.h"


/* the extension name must be ended by a NULL, otherwise it goes nasty. 
 * for example: 
 *   char *ext[] = { ".c", ".h", ".cc", NULL }
 */
int csc_cmp_file_extlist(char *fname, char **ext)
{
	int	i;

	if (!ext || !*ext) {
		return -1;
	}
	for (i = 0; ext[i]; i++) {
		if (!csc_cmp_file_extname(fname, ext[i])) {
			return 0;
		}
	}
	return i;
}



