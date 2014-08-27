
/*!\file       csc_cli_command.c
   \brief      generic functions to execute commands

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

#include "libcsoup.h"
#include "csoup_internal.h"
#include "csc_cli_private.h"


/*!\brief Display the formated command table

   \param[in]   cmdtbl  The pointer to the command table in 'struct clicmd'.
   \param[in]   show    The pointer to the display function.

   \retval      0   successfully displayed
                -1  display entry is too long
   \remark      The display will be listed like 
                ([FF]=Front padding, [BB]=Back padding):
                [FF]command [-OPTIONS][BB]Comments
*/
int csc_cli_cmd_print(struct clicmd **cmdtbl, int (*show)(char *))
{
	char	sbuf[256];
	int	i, len, opl, slen, longest;

	slen = sizeof(sbuf) - 1;

	for (i = longest = 0; cmdtbl[i]; i++) {
		len = strlen(cmdtbl[i]->cmd) + CLI_FRONT_PADDING;
		if (cmdtbl[i]->param) {
			opl = csc_cli_make_list(cmdtbl[i]->param, NULL, 0);
			if (opl) {
				len += opl + 4;	/* ' ' + '[' + '-' + ']' */
			}
		}
		len += CLI_BACK_PADDING;
		if (longest < len) {
			longest = len;
		}
	}

	if (longest >= slen) {
		return -1;
	}

	for (i = 0; cmdtbl[i]; i++) {
		sbuf[0] = 0;
		csc_strfill(sbuf, CLI_FRONT_PADDING, ' ');
		strcat(sbuf, cmdtbl[i]->cmd);

		if ((cmdtbl[i]->param) && csc_cli_make_list(
					cmdtbl[i]->param, NULL, 0)) {
			strcat(sbuf, " [-");
			len = strlen(sbuf);
			csc_cli_make_list(cmdtbl[i]->param, sbuf + len, slen - len);
			strcat(sbuf, "]");
		}
		if (cmdtbl[i]->comment) {
			csc_strfill(sbuf, longest, ' ');
			strncat(sbuf, cmdtbl[i]->comment, slen - strlen(sbuf));
		}

		strcat(sbuf, "\n");
		if (show) {
			show(sbuf);
		} else {
			CDB_SHOW(("%s", sbuf));
		}
	}
	return 0;
}

/*!\brief  Execute a command 
   
   The csc_cli_cmd_run() searches the command table to find the function entry
   of the specified command by 'argv'. The 'argc' and 'argv' assembles the 
   command line similar to that in the Shell. For example, if argc == 4 and
   argv[0] == 'mycmd', argv[1] == 'abc', argv[2] == '123', argv[3] == 'xyz',
   the command is 'mycmd' and arugments are 'abc' '123' and 'xyz'.

   \param[in]      cmdtbl  The pointer to the command table in 'struct clicmd'.
   \param[in,out]  rtime   The pointer to the runtime environment. (Reserved)
   \param[in]      argc    The number of arguments.
   \param[in]      argv    The string list of arguments.

   \return  If the command was not found in the command table, it returns
            CSC_CLI_UNCMD. Otherwise it passes the result of the command.
*/
int csc_cli_cmd_run(struct clicmd **cmdtbl, void *rtime, int argc, char **argv)
{
	int	i;

	for (i = 0; cmdtbl[i]; i++) {
		if (!strcmp(argv[0], cmdtbl[i]->cmd)) {
			return cmdtbl[i]->entry(rtime, argc, argv);
		}
	}
	return CSC_CLI_UNCMD;
}


