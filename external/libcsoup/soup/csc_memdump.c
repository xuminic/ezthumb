/*  csc_memdump.c

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
#include "csoup_internal.h"


#define CSC_MEMDUMP_TYPE_SIGNED(f)	\
	(((f) & CSC_MEMDUMP_TYPE_MASK) == CSC_MEMDUMP_TYPE_IDEC)

#define CSC_MEMDUMP_TYPE_HEX(f)		\
	((((f) & CSC_MEMDUMP_TYPE_MASK) == CSC_MEMDUMP_TYPE_HEXL) ||\
		(((f) & CSC_MEMDUMP_TYPE_MASK) == CSC_MEMDUMP_TYPE_HEXU))

static int csc_memdump_find_step(int flags)
{
	/* calculate the unit step of each number */
	switch (flags & CSC_MEMDUMP_BIT_MASK) {
	case CSC_MEMDUMP_BIT_16:
		return 2; 
	case CSC_MEMDUMP_BIT_32:
		return 4;
	case CSC_MEMDUMP_BIT_64:
		return 8;
	case CSC_MEMDUMP_BIT_FLOAT:
		return (int) sizeof(float);
	case CSC_MEMDUMP_BIT_DOUBLE:
		return (int) sizeof(double);
	}
	return 1;
}

int csc_memdump_line(void *mem, int msize, int flags, char *buf, int blen)
{
	char	*mp, format[32], tmp[64];
	int	n, sp, step, amnt;

	step = csc_memdump_find_step(flags);

	/* prepair the print format string */
	strcpy(format, "%");
	if (flags & CSC_MEMDUMP_ALIGN_LEFT) {
		strcat(format, "-");
	}
	/* define the display width of each number */
	n = (flags & CSC_MEMDUMP_WID_MASK) >> 8;
	if (n > 0) {	/* if the width is explicitly given */
		if (flags & CSC_MEMDUMP_NO_FILLING) {
			sprintf(tmp, "%d", n);
		} else {
			sprintf(tmp, "0%d", n);
		}
	} else if (flags & CSC_MEMDUMP_NO_FILLING) {	/* natural width */
		strcpy(tmp, "");
	} else if (CSC_MEMDUMP_TYPE_HEX(flags)) {
		sprintf(tmp, "0%d", step * 2);
	} else if ((flags & CSC_MEMDUMP_TYPE_MASK) == CSC_MEMDUMP_TYPE_OCT) {
		sprintf(tmp, "0%d", step * 3);
	} else {
		strcpy(tmp, "");
	}
	strcat(format, tmp);
	/* 64-bit integer need the prefix */
	if ((flags & CSC_MEMDUMP_BIT_MASK) == CSC_MEMDUMP_BIT_64) {
		strcat(format, "ll");
	}

	switch (flags & CSC_MEMDUMP_TYPE_MASK) {
	case CSC_MEMDUMP_TYPE_HEXL:
		strcat(format, "x");
		break;
	case CSC_MEMDUMP_TYPE_UDEC:
		strcat(format, "u");
		break;
	case CSC_MEMDUMP_TYPE_IDEC:
		strcat(format, "d");
		break;
	case CSC_MEMDUMP_TYPE_OCT:
		strcat(format, "o");
		break;
	default:
		if ((flags & CSC_MEMDUMP_BIT_MASK) == CSC_MEMDUMP_BIT_FLOAT) {
			strcat(format, "f");
			break;
		}
		if ((flags & CSC_MEMDUMP_BIT_MASK) == CSC_MEMDUMP_BIT_DOUBLE){
			strcat(format, "e");
			break;
		}
		strcat(format, "X");
		break;
	}
	if ((flags & CSC_MEMDUMP_NO_SPACE) == 0) {
		strcat(format, " ");
	}

	//printf("%s %d\n", format, step);
	
	/* begin to print; try print the address first */
	amnt = 0;
	if ((flags & CSC_MEMDUMP_NO_ADDR) == 0) {
		n = sprintf(tmp, "%lX: ", (unsigned long) mem);
		if (buf && (blen > n)) {
			strcpy(buf, tmp);
			buf  += n;
			blen -= n;
		}
		amnt += n;
	}

	/* start to dump the data */
	mp = mem;
	sp = msize;
	while (sp >= step) {
		switch (flags & CSC_MEMDUMP_BIT_MASK) {
		case CSC_MEMDUMP_BIT_16:
			if (CSC_MEMDUMP_TYPE_SIGNED(flags)) {
				n = sprintf(tmp, format, *((short *)mp));
			} else {
				n = sprintf(tmp, format, 
						*((unsigned short *)mp));
			}
			break;
		case CSC_MEMDUMP_BIT_32:
			if (CSC_MEMDUMP_TYPE_SIGNED(flags)) {
				n = sprintf(tmp, format, *((int *)mp));
			} else {
				n = sprintf(tmp, format, *((unsigned *)mp));
			}
			break;
		case CSC_MEMDUMP_BIT_64:
			if (CSC_MEMDUMP_TYPE_SIGNED(flags)) {
				n = SMM_SPRINT(tmp, format, 
						*((long long *)mp));
			} else {
				n = SMM_SPRINT(tmp, format,	
						*((unsigned long long *)mp));
			}
			break;
		case CSC_MEMDUMP_BIT_FLOAT:
			n = sprintf(tmp, format, *((float *)mp));
			break;
		case CSC_MEMDUMP_BIT_DOUBLE:
			n = sprintf(tmp, format, *((double *)mp));
			break;
		default:
			if (CSC_MEMDUMP_TYPE_SIGNED(flags)) {
				n = sprintf(tmp, format, *mp);
			} else {
				n = sprintf(tmp, format, 
						*((unsigned char *)mp));
			}
			break;
		}
		if (buf && blen >= n) {
			strcpy(buf, tmp);
			buf  += n;
			blen -= n;
		}
		amnt += n;

		if (flags & CSC_MEMDUMP_REVERSE) {
			mp -= step;
		} else {
			mp += step;
		}
		sp -= step;
	}
	if (flags & CSC_MEMDUMP_NO_GLYPH) {
		return amnt;
	}

	if (buf && blen > 0) {
		strcpy(buf, " ");
		buf++;
		blen--;
	}
	amnt++;
	mp = mem;
	sp = msize;
	while (sp) {
		if (buf && blen > 0) {
			if ((*mp < ' ') || (*mp > 0x7e)) {
				*buf++ = '.';
			} else {
				*buf++ = *mp;
			}
			blen--;
		}
		if (flags & CSC_MEMDUMP_REVERSE) {
			mp--;
		} else {
			mp++;
		}
		sp--;
		amnt++;
	}
	if (buf && blen > 0) {
		*buf++ = 0;
	}
	return amnt;
}

int csc_memdump(void *mem, int msize, int column, int flags)
{
	char	*buf, *mp;
	int	bsize, csize, len;

	csize = csc_memdump_find_step(flags) * column;
	bsize = csize * 6 + 32;		/* should be big enough */
	if ((buf = smm_alloc(bsize)) == NULL) {
		return SMM_ERR_LOWMEM;
	}

	mp = mem;
	while (msize) {
		len = csize < msize ? csize : msize;
		csc_memdump_line(mp, len, flags, buf, bsize);
		CDB_SHOW(("%s\n", buf));

		msize -= len;
		if (flags & CSC_MEMDUMP_REVERSE) {
			mp -= len;
		} else {
			mp += len;
		}
	}
	smm_free(buf);
	return 0;
}
