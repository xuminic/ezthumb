
/*!\file       csc_strinsert.c
   \brief      Padding a string to specified length.

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

/*!\brief Insert a string to into a specified string.

   \param[in]  buf The string and buffer which is going to be inserted.
   \param[in]  len The length of the full buffer of 'buf'
   \param[in]  ip The Insertion point. If 'ip' is NULL, 'ip' will be used
   as 'buf'. If 'ip' is out of boundry of 'buf' string, it will be placed
   in the end of 'buf' string.

   \param[in]  del The number of deleting characters in string 'buf'
   \param[in]  s The inserting string.

   \return    The length of the string after insertion, or -1 if failed.

   \remark The 'len' parameter is the total length of the buffer for insertion
   which definitely should be larger than the content of 'buf' string. 
   The 'ip' parameter should be between 'buf' and the '\0' in the end of the 
   'buf' string, otherwise the function return -1 and do nothing.
   If the insertion overflowes the buffer of 'buf', the insertion won't be
   happen in case of truncation. You need to check the return length of 
   csc_strinsert() to see if it is larger than or equal to 'len'.
*/
int csc_strinsert(char *buf, int len, char *ip, int del, char *s)
{
	int	acc, rc, slen, tlen;

	slen = strlen(buf);
	if (ip < buf) {
		ip = buf;
	}
	if (ip > buf + slen) {
		ip = buf + slen;
	}

	tlen = s ? strlen(s) : 0;
	acc = tlen - del;
	if ((rc = slen + acc) >= len) {
		return rc;	/* insertion overflow */
	}

	if (acc != 0) {
		char *sour = ip + del;
		if (sour < buf) {	/* out of buffer boundry */
			return -1;
		}
		if (sour < buf + slen) {
			memmove(sour + acc, sour, strlen(sour) + 1);
		} else {	/* delete out of end */
			ip[tlen] = 0;
		}
	}
	if (s) {
		memcpy(ip, s, tlen);
	}
	return rc;
}


