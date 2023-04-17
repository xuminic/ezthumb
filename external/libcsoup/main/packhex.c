/*  packhex.c - test harness of csc_pack_hex.c

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
#include <stdlib.h>

#include "libcsoup.h"
#include "libcsoup_debug.h"

#include "packhex.h"

int packhex_main(void *rtime, int argc, char **argv)
{
	struct	phex_idx	*idx;
	unsigned char	*s;
	long	dlen;
	int	i;

	int pack_hex_counter(void *frame, char *fname, void *data, long dlen)
	{
		s = data;
		cslog("%p(%6ld): %s [%d][%d][%d][%d]\n", frame, dlen, 
					fname, s[0], s[1], s[2], s[3]);
		return 0;
	}

	/* stop the compiler complaining */
	(void) rtime; 

	if (argc < 2) {
		csc_pack_hex_list((void*)packed_hex, pack_hex_counter);
		return 0;
	}

	if ((s = csc_pack_hex_load((void*)packed_hex, argv[1], &dlen)) != NULL) {
		cslog("Found %s (%ld): [%d][%d][%d][%d]\n", argv[1], dlen,
					s[0], s[1], s[2], s[3]);
	} else if ((idx = csc_pack_hex_index((void*)packed_hex)) != NULL) {
		for (i = 0; idx[i].fname; i++) {
			s = idx[i].data;
			cslog("%2d: %s (%ld): [%d][%d][%d][%d]\n", i, 
						idx[i].fname, idx[i].dlen,
						s[0], s[1], s[2], s[3]);
		}
		free(idx);
	}
	return 0;
}

struct	clicmd	packhex_cmd = {
	"packhex", packhex_main, NULL, "Testing the csc_pack_hex_* functions"
};

extern  struct  clicmd  packhex_cmd;


