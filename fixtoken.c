
/* fixtoken - extract tokens from a string
 
   Version 1.1
   Version 1.2 20090401
     remove the safe_strncpy() function; make test program easier
     add mkargv(), a simple command line parser

   Copyright (C) 1998-2009  Xuming <xuming@users.sourceforge.net>
   
   This program is free software; you can redistribute it and/or 
   modify it under the terms of the GNU General Public License as 
   published by the Free Software Foundation; either version 2, or 
   (at your option) any later version.
	   
   This program is distributed in the hope that it will be useful, 
   but WITHOUT ANY WARRANTY; without even the implied warranty of 
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License, the file COPYING in this directory, for
   more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* isspace() macro has a problem in Cygwin when compiling it with -mno-cygwin.
 * I assume it is caused by minGW because it works fine with cygwin head files.
 * The problem is it treats some Chinese characters as space characters.
 * A sample is: 0xC5 0xF3 0xD3 0xD1 */
#define IsSpace(c)	((((c) >= 9) && ((c) <= 0xd)) || ((c) == 0x20))
#define IsPipe(c)	(((c) == '>') || ((c) == '<') || ((c) == '|'))

static int isdelim(char *delim, int ch)
{
	while (*delim) {
		if (*delim == (char) ch) {
			return 1;
		} else if ((*delim == ' ') && IsSpace(ch)) {
			return 1;
		}
		delim++;
	}
	return 0;
}


/* This function splits the string into tokens. It extracts everything 
   between delimiter.
 
   sour  - the input string
   idx   - the string array for storing tokens
   ids   - the maximem number of tokens, the size of "idx"
   delim - the delimiter array, each character in the array is a delimiter.

   It returns the number of extracted tokens.

   For example, fixtoken("#abc  wdc:have:::#:debug", idx, 16, "# :") returns
   10 tokens "", "abc", "", "wdc", "have", "", "", "", "" and "debug".
   
   NOTE:  'sour' will be changed.
*/

int fixtoken(char *sour, char **idx, int ids, char *delim)
{
	int	i;

	for (i = 0; i < ids; idx[i++] = NULL);
    
	i = 0;
	for (idx[i++] = sour; *sour && (i < ids); sour++)  {
		if (isdelim(delim, *sour))  {
			*sour = 0;
			idx[i++] = sour + 1;
		}
    	}
	return i;
}

/* This function splits the string into tokens. It extracts everything
   between delimiter.

   Unlike fixtoken(), it treats continous delimiters as one single delimter.
 
   sour  - the input string 
   idx   - the string array for storing tokens
   ids   - the maximem number of tokens, the size of "idx"
   delim - the delimiter array, each character in the array is a delimiter.

   It returns the number of token extracted.

   For example, fixtoken("#abc  wdc:have:::#:debug", idx, 16, "# :") returns
   4 tokens "abc", "wdc", "have" and "debug".
   
   NOTE:  'sour' will be changed.
*/

int ziptoken(char *sour, char **idx, int ids, char *delim)
{
	int	i, ss;

	for (i = 0; i < ids; idx[i++] = NULL);

	for (i = ss = 0; *sour && (i < ids); sour++)  {
		if (isdelim(delim, *sour))  {
			ss = 0;
			*sour = 0;
		} else if (ss == 0) {
			ss = 1;
			idx[i++] = sour;
		}
	}
	return i;
}


#define MK_ARG_DELIM		0
#define MK_ARG_TOKEN		1
#define MK_ARG_SGL_QU		2
#define MK_ARG_DBL_QU		3
#define MK_ARG_PIPE		4

/* mkargv - make argument list
 
   This function splits the string into an argument list in the traditional
   (int argc, char *argv[]) form. It uses a very simple state machine so you
   couldn't expect too much features from here. 

   sour  - the input string
   idx   - the string array for storing tokens
   ids   - the maximem number of tokens, the size of "idx"

   It returns the number of token extracted.

   NOTE:  'sour' will be changed.
*/
int mkargv(char *sour, char **idx, int ids)
{
	int	i = 0, stat;

	stat = MK_ARG_DELIM;
	while (*sour) {
		switch (stat) {
		case MK_ARG_DELIM:	/* last char is the delimiter */
			if (IsSpace(*sour)) {
				*sour = 0;
			} else if (*sour == '\'') {
				*sour = 0;
				idx[i++] = sour + 1;
				stat = MK_ARG_SGL_QU;
			} else if (*sour == '"') {
				*sour = 0;
				idx[i++] = sour + 1;
				stat = MK_ARG_DBL_QU;
			} else if (IsPipe(*sour)) {
				*sour = 0;
				stat = MK_ARG_PIPE;
			} else {
				idx[i++] = sour;
				stat = MK_ARG_TOKEN;
			}
			break;

		case MK_ARG_TOKEN:
			if (IsSpace(*sour)) {
				*sour = 0;
				stat = MK_ARG_DELIM;
			} else if (*sour == '\'') {
				*sour = 0;
				idx[i++] = sour + 1;
				stat = MK_ARG_SGL_QU;
			} else if (*sour == '"') {
				*sour = 0;
				idx[i++] = sour + 1;
				stat = MK_ARG_DBL_QU;
			} else if (IsPipe(*sour)) {
				*sour = 0;
				stat = MK_ARG_PIPE;
			}
			break;

		case MK_ARG_SGL_QU:
			if (*sour == '\'') {
				*sour = 0;
				stat = MK_ARG_DELIM;
			}
			break;

		case MK_ARG_DBL_QU:
			if (*sour == '"') {
				*sour = 0;
				stat = MK_ARG_DELIM;
			}
			break;
		}
		if (stat == MK_ARG_PIPE) {
			break;
		}
		if (i >= ids) {
			i = ids - 1;	/* the last argment must be NULL */
			break;
		}
		sour++;
	}
	idx[i] = NULL;
	return i;
}
	
 

#ifdef	EXECUTE_FIXTOKEN

/* make:  gcc -Wall -DEXECUTE_FIXTOKEN -o token fixtoken.c */

struct	{
	char	*delim;
	char	*content;
} testbl[] = {
	{ "# :",	"#abc  wdc:have:::#:debug" },
	{ " ",		"  abc bcd 'sad str sf  ' sdf > asdf" },
	{ NULL, NULL }
};

int test_print_token(char *content, char *delim)
{
	char	buf[256], *argv[32];
	int	i, argc;

	printf("PARSING   {%s} by {%s}\n", content, delim);

	strcpy(buf, content);
	argc = fixtoken(buf, argv, sizeof(argv)/sizeof(char*), delim);
	printf("FIXTOKEN: ");
	for (i = 0; i < argc; i++) {
		printf("{%s} ", argv[i]);
	}
	printf("\n");

	strcpy(buf, content);
	argc = ziptoken(buf, argv, sizeof(argv)/sizeof(char*), delim);
	printf("ZIPTOKEN: ");
	for (i = 0; i < argc; i++) {
		printf("{%s} ", argv[i]);
	}
	printf("\n");

	strcpy(buf, content);
	argc = mkargv(buf, argv, sizeof(argv)/sizeof(char*));
	printf("MKARGV:   ");
	for (i = 0; i < argc; i++) {
		printf("{%s} ", argv[i]);
	}
	printf("\n\n");
	return 0;
}

int main(int argc, char **argv)
{
	int	i;
	char	buf[256];

	if (argc < 2) {
		for (i = 0; testbl[i].delim; i++) {
			test_print_token(testbl[i].content, testbl[i].delim);
		}
		return 0;
	}

	printf("Press Ctrl-D or 'quit' command to quit.\n");
	while (1) {
		printf("IN> ");
		if (fgets(buf, 256, stdin) == NULL) {
			break;
		}

		buf[strlen(buf) - 1] = 0;
		if (!strcmp(buf, "quit") || !strcmp(buf, "exit")) {
			break;
		}

		test_print_token(buf, " ");
	}
	return 0;
}

#endif

