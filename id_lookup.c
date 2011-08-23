
/*
    Copyright (C) 2011  "Andy Xuming" <xuming@users.sourceforge.net>

    This file is part of EZTHUMB, a utility to generate thumbnails

    EZTHUMB is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    EZTHUMB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "id_lookup.h"

char *id_lookup(struct idtbl *table, int id)
{
	static	char	def[32];
	int	i;

	for (i = 0; table[i].id; i++) {
		if (id == table[i].no) {
			return table[i].id;
		}
	}
	sprintf(def, "[%d]", id);
	return def;
}

#if 0
/* help to generate the array of ffmpeg IDs */

#include <ctype.h>
#include <stdio.h>

int main()
{
	char	buf[256], *p, *q;

	while (fgets(buf, sizeof(buf), stdin)) {
		for (p = buf; isspace(*p); p++);	/* skip whitespaces */
		for (q = p+1; !isspace(*q) && *q!=','; q++);
		*q = 0;
		printf("\t{ %s, \"%s\" },\n", p, p);

		/*if (!strncmp(buf, "#define", 7)) {
			for (p = buf + 7; isspace(*p); p++);
			for (q = p+1; !isspace(*q); q++);
			*q = 0;
			printf("\t{ %s, \"%s\" },\n", p, p);
		} else {
			printf("%s", buf);
		}*/
	}
	return 0;
}
#endif

