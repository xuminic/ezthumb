
/*!\file       csc_cli_getopt.c
   \brief      Parsing command line

   \author     "Andy Xuming" <xuming@users.sourceforge.net>
   \date       2011-2014
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
#include <stdlib.h>
#include <getopt.h>

#include "libcsoup.h"
#include "csc_cli_private.h"

struct	clirun	{
	char	*optarg;
	struct	option	oplst[1];
};


/*!\brief Open the command line interface option table

   \param[in]  optbl The option table of 'struct cliopt'

   \return  The pointer to an allocated structure if succeed, 
            or NULL otherwise.
   \remark  Must use csc_cli_getopt_close() to close it.       
   \remark  The getopt() and getopt_long() will be reset after this call.
*/
void *csc_cli_getopt_open(struct cliopt *optbl)
{
	struct	clirun	*rtbuf;
	int	n, rc;

	n  = csc_cli_table_size(optbl) + 1;
	rc = n * (sizeof(struct option) + 4) + sizeof(struct clirun);

	if ((rtbuf = smm_alloc(rc)) != NULL) {
		rtbuf->optarg = (char*) &rtbuf->oplst[n];
		csc_cli_make_list(optbl, rtbuf->optarg, n*4);
		csc_cli_make_table(optbl, rtbuf->oplst, n);
	}

	/* reset the getopt() function */
	optind = 1;
	return rtbuf;
}

/*!\brief Close the internal structure

   \param[in]  clibuf The pointer to the allocated structure.

   \return 0
*/
int csc_cli_getopt_close(void *clibuf)
{
	smm_free(clibuf);
	return 0;
}

/*!\brief Parse command-line options

   \param[in]  argc,argv Same to getopt().
   \param[in]  clibuf The pointer to the allocated structure.

   \return Same to getopt().
*/
int csc_cli_getopt(int argc, char * const argv[], void *clibuf)
{
	struct	clirun	*rtbuf = clibuf;

	return getopt_long(argc, argv, rtbuf->optarg, rtbuf->oplst, NULL);
}

