
/*  cliopt.h - command line option helps

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

#ifndef	_CLI_OPTION_H_
#define _CLI_OPTION_H_

#include <getopt.h>

#define CLIOPT_VERSION		1

#define CLIOPT_PARAM_NONE	0
#define CLIOPT_PARAM_NUMBER	1
#define CLIOPT_PARAM_STRING	2
#define CLIOPT_PARAM_CHAR	3

#define CLI_SHORT		1
#define CLI_LONG		2
#define CLI_BOTH		3
#define CLI_COMMENT		4
#define CLI_EXTLINE		8
#define CLI_EOL			16

#define CLI_LF_GATE		20


struct	cliopt	{
	int	opt_char;
	char	*opt_long;
	int	param;
	char	*comment;
};

int  cli_type(struct cliopt *optbl);
int  cli_table_size(struct cliopt *optbl);
char *cli_alloc_list(struct cliopt *optbl);
void *cli_alloc_table(struct cliopt *optbl);
int  cli_print(struct cliopt *optbl);


#endif	/* _CLI_OPTION_H_ */
