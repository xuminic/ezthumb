/*!\file csc_cli_private.h

   \author "Andy Xuming" <xuming@users.sourceforge.net>
*/
/* Copyright (C) 2013  "Andy Xuming" <xuming@users.sourceforge.net>

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

#ifndef	_CSC_CLI_PRIVATE_H_
#define _CSC_CLI_PRIVATE_H_

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

#define CLI_FRONT_PADDING	2
#define CLI_BACK_PADDING	4


#ifdef __cplusplus
extern "C"
{
#endif

int csc_cli_type(struct cliopt *optbl);
int csc_cli_table_size(struct cliopt *optbl);
int csc_cli_table_type(struct cliopt *optbl);

#ifdef __cplusplus
} // __cplusplus defined.
#endif
#endif	/* _CSC_CLI_PRIVATE_H_ */

