
/*  csc_extname_filter_match.c

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
#include <stdlib.h>
#include <string.h>

#include "libcsoup.h"


int csc_extname_filter_match(void *efft, char *fname)
{
	CSEFF	*flt = efft;

	if (flt == NULL) {
		return 1;	/* no filter means total matched */
	}
	if (*flt->filter == NULL) {
		return 1;
	}
	if (!csc_cmp_file_extlist(fname, flt->filter)) {
		return 1;
	}
	return 0;	/* not matched */
}

