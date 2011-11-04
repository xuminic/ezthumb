
/*  smm_signal_break.c - setup the BREAK signal

    Copyright (C) 2011  "Andy Xuming" <xuming@users.sourceforge.net>

    This file is part of LIBSMM, System Masquerade Module library

    LIBSMM is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LIBSMM is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>

#include "libsmm.h"

static int (*sig_break)(int sig);

#ifdef	CFG_WIN32_API
static BOOL siegfried(DWORD signum)
{
	switch (signum) {
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		if (sig_break) {
			sig_break((int) signum);
		}
	}
	return FALSE;
}

int smm_signal_break(int (*handle)(int))
{
	sig_break = handle;
	SetConsoleCtrlHandler((PHANDLER_ROUTINE) siegfried, TRUE);
	return 0;
}
#endif

#ifdef	CFG_UNIX_API
#include <signal.h>
#include <unistd.h>

static void siegfried (int signum)
{
	if (sig_break) {
		sig_break(signum);
	}
	exit(signum);
}

int smm_signal_break(int (*handle)(int))
{
	struct	sigaction	signew, sigold;

	signew.sa_handler = siegfried;
	sigemptyset(&signew.sa_mask);
	signew.sa_flags = 0;

	sigaction(SIGINT, NULL, &sigold);
	if (sigold.sa_handler != SIG_IGN) {
		sigaction(SIGINT, &signew, NULL);
	}
	sigaction(SIGHUP, NULL, &sigold);
	if (sigold.sa_handler != SIG_IGN) {
		sigaction(SIGHUP, &signew, NULL);
	}
	sigaction(SIGTERM, NULL, &sigold);
	if (sigold.sa_handler != SIG_IGN) {
		sigaction(SIGTERM, &signew, NULL);
	}
	return 0;
}
#endif
