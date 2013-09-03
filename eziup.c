
/*  ezgui.c - the graphic user interface

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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "iup.h"
#include "libcsoup.h"
#include "ezthumb.h"
#include "ezgui.h"

EZGUI *ezgui_init(EZOPT *ezopt, int *argcs, char ***argvs)
{
	EZGUI	*gui;
	char	*p;
	int	tmp;

	IupOpen(argcs, argvs);

	IupSetGlobal("SINGLEINSTANCE", "ezthumb");
	if (!IupGetGlobal("SINGLEINSTANCE")) {
		IupClose();
		return NULL;
	}

	if ((gui = calloc(sizeof(EZGUI), 1)) == NULL) {
		return NULL;
	}
	gui->sysopt = ezopt;
	return gui;
}

int ezgui_run(EZGUI *gui, char *flist[], int fnum)
{
	//ezgui_create_window(gui);

	IupMainLoop();
	return 0;
}

int ezgui_close(EZGUI *gui)
{
	if (gui) {
		IupClose();
		free(gui);
	}
	return 0;
}

void ezgui_version(void)
{
	char	*path;

	slogz("GTK: %d.%d.%d\n", GTK_MAJOR_VERSION,
			GTK_MINOR_VERSION, GTK_MICRO_VERSION);
	path = g_build_filename(g_get_user_config_dir(),
			CFG_SUBPATH, CFG_FILENAME, NULL);
	slogz("Profile: %s\n", path);
	g_free(path);
}


