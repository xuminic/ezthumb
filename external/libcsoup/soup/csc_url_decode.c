/*  csc_url_decode.c
 
    https://en.wikipedia.org/wiki/Percent-encoding

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
#include <ctype.h>

#include "libcsoup.h"

//#define	CFG_X_WWW_FORM	/* application/x-www-form-urlencoded type */

#define HEX2DEC(n)	\
	(((n)>='a')?((n)-'a'+10):(((n)>='A')?((n)-'A'+10):((n)-'0')))

int csc_url_decode(char *dst, int dlen, char *src)
{
	for (dlen--; *src && (dlen > 0); dlen--) {
		if ((*src == '%') && isxdigit(src[1]) && isxdigit(src[2])) {
			*dst++ = (char)(HEX2DEC(src[1]) * 16 + HEX2DEC(src[2]));
			src+=3;
#ifdef	CFG_X_WWW_FORM
		} else if (*src == '+') {	
			*dst++ = ' ';
			src++;
#endif
		} else {
			*dst++ = *src++;
		}
	}
	*dst++ = '\0';
	return dlen;
}

char *csc_url_decode_alloc(char *src)
{
	char	*tmp, *dst;

	if ((tmp = dst = smm_alloc(strlen(src)+1)) == NULL) {
		return NULL;
	}
	while (*src) {
		if ((*src == '%') && isxdigit(src[1]) && isxdigit(src[2])) {
			*dst++ = (char)(HEX2DEC(src[1]) * 16 + HEX2DEC(src[2]));
			src+=3;
#ifdef	CFG_X_WWW_FORM
		} else if (*src == '+') {	
			*dst++ = ' ';
			src++;
#endif
		} else {
			*dst++ = *src++;
		}
	}
	*dst++ = '\0';
	return tmp;
}


