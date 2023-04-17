/*  memdump.c - test harness of memdump()

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
#include "libcsoup_debug.h"

int strings_strbody(void)
{
	struct	dicts	{
		char	*dest;
		char	*sour;
	} testcase[] = {
		{ "abc",  "       abc" },
		{ NULL, "  \t\t   " },
		{ " abc ", "   abc   " },
		{ NULL, NULL }
	};
	char	*p;
	int	i, rc;

	cslog("csc_strcmp_param() testing:\n");
	for (i = 0; i < (int)(sizeof(testcase)/sizeof(struct dicts)); i++) {
		cslog("Comparing {%s} and {%s} ... ", testcase[i].dest, testcase[i].sour); 
		rc = csc_strcmp_param(testcase[i].dest, testcase[i].sour);
		cslog("%d\n", rc);
	}
	cslog("\ncsc_strbody() testing:\n");
	for (i = 0; i < (int)(sizeof(testcase)/sizeof(struct dicts)); i++) {
		if (testcase[i].dest) {
			p = csc_strbody(testcase[i].dest, &rc);
			cslog("Picking from {%s} ... %s (%d)\n", testcase[i].dest, p, rc);
		}
		if (testcase[i].sour) {
			p = csc_strbody(testcase[i].sour, &rc);
			cslog("Picking from {%s} ... %s (%d)\n", testcase[i].sour, p, rc);
		}
	}				

	return 0;
}

static int strings_strbival(void)
{
	char	*testcase[] = {
		"1024x768x24",
		"  1024  768  24  ",
		"0x100x0x200+0xff",
		"0123",
		NULL
	};
	int	i, v1, v2;

	cslog("\ncsc_strbival_int() testing: xX+*\n");
	for (i = 0; testcase[i]; i++) {
		v1 = csc_strbival_int(testcase[i], "xX+*", &v2);
		cslog("[%s]:  %d %d\n", testcase[i], v1, v2);
	}
	cslog("csc_strbival_int() testing:\n");
	for (i = 0; testcase[i]; i++) {
		v1 = csc_strbival_int(testcase[i], NULL, &v2);
		cslog("[%s]:  %d %d\n", testcase[i], v1, v2);
	}

	return 0;
}

static int strings_strinsert(void)
{
#define	STRINSERTSMPL	"Alpha AXP of DEC"
	char	buf[24];
	int	rc;

	cslog("\nFrom [Alpha AXP of DEC] to [Alpha RISC of DEC]: ");
	strcpy(buf, STRINSERTSMPL);
	csc_strinsert(buf, sizeof(buf), &buf[6], 3, "RISC");
	cslog("%s\n", buf);

	cslog("From [Alpha AXP of DEC] to [RISCAlpha AXP of DEC]: ");
	strcpy(buf, STRINSERTSMPL);
	csc_strinsert(buf, sizeof(buf), NULL, 0, "RISC");
	cslog("%s\n", buf);

	cslog("From [Alpha AXP of DEC] to [Alpha AXP of DECCISC]: ");
	strcpy(buf, STRINSERTSMPL);
	csc_strinsert(buf, sizeof(buf), buf+sizeof(buf), 0, "CISC");
	cslog("%s\n", buf);

	cslog("From [Alpha AXP of DEC] to [Alpha RISCCISAXP of DEC]: ");
	strcpy(buf, STRINSERTSMPL);
	csc_strinsert(buf, sizeof(buf), &buf[6], 0, "RISCCIS");
	cslog("%s\n", buf);

	cslog("From [Alpha AXP of DEC] to buffer overflow: ");
	strcpy(buf, STRINSERTSMPL);
	rc = csc_strinsert(buf, sizeof(buf), &buf[6], 0, "RISCCISC");
	cslog("%s [%d]\n", buf, rc);

	cslog("From [Alpha AXP of DEC] to [Alpha AXP _AXP of DEC]: ");
	strcpy(buf, STRINSERTSMPL);
	csc_strinsert(buf, sizeof(buf), buf+10, -4, "_");
	cslog("%s\n", buf);

	cslog("From [Alpha AXP of DEC] to out of boundry: ");
	strcpy(buf, STRINSERTSMPL);
	rc = csc_strinsert(buf, sizeof(buf), &buf[3], -4, "_");
	cslog("%s [%d]\n", buf, rc);

	cslog("From [Alpha AXP of DEC] to [Alpha is gone]: ");
	strcpy(buf, STRINSERTSMPL);
	csc_strinsert(buf, sizeof(buf), buf+6, 100, "is gone");
	cslog("%s\n", buf);

	cslog("From [Alpha AXP of DEC] to [Alpha of DEC]: ");
	strcpy(buf, STRINSERTSMPL);
	csc_strinsert(buf, sizeof(buf), buf+6, 4, NULL);
	cslog("%s\n", buf);

	return 0;
}


int strings_main(void *rtime, int argc, char **argv)
{
	/* stop the compiler complaining */
	(void) rtime; (void) argc; (void) argv;

	strings_strbody();
	strings_strbival();
	strings_strinsert();
	return 0;
}


struct	clicmd	strings_cmd = {
	"strings", strings_main, NULL, "Testing the string process functions"
};

extern  struct  clicmd  strings_cmd;

