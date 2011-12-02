
/*
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

#ifndef	_EZGUI_H_
#define _EZGUI_H_

#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#define	CFG_SUBPATH	"ezthumb"
#define CFG_FILENAME	"ezthumb.conf"

#define CFG_GRP_MAIN	"main"

#define CFG_KEY_WIN_WIDTH	"window_width"
#define CFG_KEY_WIN_HEIGHT	"window_height"


typedef	struct		{
	GtkWidget	*gw_main;
	GtkWidget	*gw_page;
	GtkWidget	*gw_page_main;
	GtkWidget	*gw_listview;

	/* GUI parameters */
	char		*cfg_filename;	/* the path of the configure file */
	GKeyFile	*cfg_keys;	/* key entry of the configure file */
	int		mod_counter;	/* modify counter >0 mean to save */
	int		gui_width;	/* the width of the GUI interface */
	int		gui_height;	/* the height of the GUI interface */

} EZGUI;

enum	{
	EZUI_COL_NAME = 0,
	EZUI_COL_SIZE,
	EZUI_COL_LENGTH,
	EZUI_COL_SCREEN,
	EZUI_COL_PROGRESS,
	EZUI_COL_MAX
};



int ezgui_init(int *argc, char ***argv);
void *ezgui_create(void);
int ezgui_run(EZGUI *gui);
int ezgui_close(EZGUI *gui);
void *ezgui_list_append_begin(EZGUI *gui);
int ezgui_list_append_end(EZGUI *gui, GtkTreeModel *model);
int ezgui_list_append(EZGUI *gui, GtkTreeModel *model, char *s);

#endif	/* _EZGUI_H_ */

