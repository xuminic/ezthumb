
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

/*#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
*/
#include "libcsoup.h"
#include "ezthumb.h"
#include "iup.h"


#define	CFG_SUBPATH	"ezthumb"
#define CFG_FILENAME	"ezthumb.conf"

#define CFG_GRP_MAIN	"main"

#define CFG_KEY_WIN_WIDTH	"window_width"
#define CFG_KEY_WIN_HEIGHT	"window_height"
#define CFG_KEY_PROF_SIMPLE	"simple_profile"
#define CFG_KEY_DIRECTORY	"last_directory"
#define CFG_KEY_GRID		"grid_define"
#define CFG_KEY_GRID_COLUMN	"grid_column"
#define CFG_KEY_GRID_ROW	"grid_row"
#define CFG_KEY_ZOOM		"zoom_define"
#define CFG_KEY_ZOOM_RATIO	"zoom_ratio"
#define CFG_KEY_ZOOM_WIDTH	"zoom_width"
#define CFG_KEY_ZOOM_HEIGHT	"zoom_height"
#define CFG_KEY_CANVAS_WIDTH	"canvas_width"
#define CFG_KEY_TIME_STEP	"time_step"
#define CFG_KEY_DURATION	"duration_mode"
#define CFG_KEY_FILE_FORMAT	"file_format"
#define CFG_KEY_TRANSPARENCY	"transparency"
#define CFG_KEY_WINDOWSTATE	"window_state"

#define	CFG_PIC_GRID_DIM	"Column and Row"	/* Grid 4x4 */
#define CFG_PIC_GRID_STEP	"Column and Step"	/* Grid 4 Step 15 */
#define CFG_PIC_DIS_NUM		"Discrete by Number"	/* DSS No 20 */
#define CFG_PIC_DIS_STEP	"Discrete by Step"	/* DSS Step 15 */
#define CFG_PIC_DIS_KEY		"Discrete key Frames"	/* DSS I-Frame */
#define CFG_PIC_ZOOM_RATIO	"Zoom by Ratio"		/* Zoom 50% */
#define CFG_PIC_ZOOM_DEFINE	"Zoom by Size"		/* Zoom 320x240 */
#define CFG_PIC_ZOOM_SCREEN	"Zoom by Canvas"	/* Res 1024 */
#define CFG_PIC_AUTO		"Auto"			/* Grid/Zoom Auto */

#define CFG_PIC_DFM_HEAD	"File Head"
#define CFG_PIC_DFM_SCAN	"Full Scan"
#define CFG_PIC_DFM_FAST	"Fast Scan"

#define CFG_PIC_FMT_JPEG	"Jpeg"
#define CFG_PIC_FMT_PNG		"Png"
#define CFG_PIC_FMT_GIFA	"Animated GIF"
#define CFG_PIC_FMT_GIF		"GIF"

#define EZGUI_MAGIC		(('E'<<24) | ('Z'<<16) | ('U'<<8) | 'I')

#if 0
typedef	struct		{
	char		*fname;		/* the path of the configure file */
	GKeyFile	*ckey;		/* key entry of the configure file */
	int		mcount;		/* modify counter >0 mean to save */
} EZCFG;
	
typedef	struct		{
	GtkTreeModel	*app_model;
	
	GtkTextBuffer	*discarded;
	int		dis_count;	/* counter of discarded files */
	int		add_count;	/* counter of added files */
} EZADD;

typedef	struct		{
	GtkWidget	*gw_main;
	GtkWidget	*gw_page;
	GtkWidget	*gw_page_main;
	GtkWidget	*gw_listview;
	GdkWindowState	gw_win_state;	/* window state from events */
	int		w_width;
	int		w_height;

	GtkWidget	*button_del;	/* the delete button on main page */
	GtkWidget	*button_run;	/* the start button on main page */
	GtkWidget	*button_box;
	GtkWidget	*prof_group;

	GtkWidget	*entry_col;
	GtkWidget	*entry_row;
	GtkWidget	*entry_step;
	GtkWidget	*entry_zoom_ratio;
	GtkWidget	*entry_zoom_wid;
	GtkWidget	*entry_zoom_hei;
	GtkWidget	*entry_width;

	/* Setup Page: the two main buttons in the bottom */
	GtkWidget	*butt_setup_apply;
	GtkWidget	*butt_setup_cancel;

	/* Setup Page: the choise of profiles */
	GtkWidget	*prof_grid;
	GtkWidget	*prof_zoom;
	
	/* Setup Page: the duration finding mode */
	GtkWidget	*dfm_head;	/* no scan */
	GtkWidget	*dfm_fast;	/* fast tail scan */
	GtkWidget	*dfm_slow;	/* slow full scan */

	/* Setup Page: the output file format */
	GtkWidget	*off_jpg;
	GtkWidget	*off_jpg_qf;	/* quality fact */
	GtkWidget	*off_gif;
	GtkWidget	*off_gifa;
	GtkWidget	*off_gifa_fr;	/* frame rate */
	GtkWidget	*off_png;
	GtkWidget	*off_transp;	/* transparent */
	int		tmp_jpg_qf;
	int		tmp_gifa_fr;

	/* feed to progress */
	int		list_count;	/* counter of files in the listview */
	int		list_index;	/* current node index */
	GtkTreeModel	*list_model;
	GtkTreeIter	*list_iter;

	EZCFG		*config;
} EZGUI;
#else
typedef	struct		{
	unsigned	magic;		/* "EZUI" */
	EZOPT		*sysopt;	/* system parameters */
	char		inst_id[64];	/* instant identity */

	/* Main Page: work place */
	Ihandle		*list_fname;	/* the list control of file names */
	Ihandle		*list_size;	/* the list control of file sizes */
	Ihandle		*list_length;	/* the list control of media length */
	Ihandle		*list_resolv;	/* the list control of resolution */
	Ihandle		*list_prog;	/* the list control of progress */
	int		list_idx;	/* current active item start from 1 */
	int 		list_count;	/* total items in the list */
	CSCLNK		*list_cache;

	/* Main Page: Progress Bar and Buttons */
	Ihandle		*prog_bar;
	Ihandle		*stat_bar;
	Ihandle		*ps_zbox;
	Ihandle		*button_del;	/* the delete button on main page */
	Ihandle		*button_run;	/* the start button on main page */
	Ihandle		*button_add;
	char		status[256];	/* displaying the status on bottom */
	char		*filefilter;	/* don't forget to release it */

	/* Main Page: dialog of Open File */
	Ihandle		*dlg_main;	/* the main dialog itself */
	Ihandle		*dlg_open;	/* dialog of Open File */
	char		*cur_dir;	/* current directory */

	/* Setup Page: the two main buttons in the bottom */
	Ihandle		*butt_setup_apply;
	Ihandle		*butt_setup_cancel;

	/* Setup Page: the choise of profiles */
	Ihandle		*prof_grid;
	Ihandle		*prof_zoom;
	int		grid_idx;	/* starts from 0 */
	int		zoom_idx;	/* starts from 0 */
	
	/* Main Page: Profile selection */
	Ihandle		*entry_col_grid;
	Ihandle		*entry_col_step;
	Ihandle		*entry_row;
	Ihandle		*entry_step;
	Ihandle		*entry_dss_amnt;
	Ihandle		*entry_dss_step;
	Ihandle		*entry_zoom_ratio;
	Ihandle		*entry_zoom_wid;
	Ihandle		*entry_zoom_hei;
	Ihandle		*entry_width;
	Ihandle		*entry_zbox_grid;
	Ihandle		*entry_zbox_zoom;

	/* Setup Page: the duration finding mode */
	Ihandle		*dfm_list;
	int		dfm_idx;

	/* Setup Page: the output file format */
	Ihandle		*fmt_list;
	Ihandle		*fmt_gif_fr;	/* GIF frame rate */
	Ihandle		*fmt_jpg_qf;	/* quality fact */
	Ihandle		*fmt_transp;	/* transparent */
	Ihandle		*fmt_zbox;
	int		fmt_idx;
	int		fmt_idx_tmp;
	int		tmp_jpg_qf;
	int		tmp_gifa_fr;
} EZGUI;
#endif

typedef	struct	{
	char	fsize[16];
	char	vidlen[16];
	char	resolv[16];
	char	progr[8];
	char	fname[1];
} EZMEDIA;


enum	{
	EZUI_COL_NAME = 0,
	EZUI_COL_SIZE,
	EZUI_COL_LENGTH,
	EZUI_COL_SCREEN,
	EZUI_COL_PROGRESS,
	EZUI_COL_MAX
};

enum	{
	EZUI_FMR_RDWR = 0,
	EZUI_FMR_RDRSET,
	EZUI_FMR_RDONLY
};

enum	{
	EZUI_FMT_PNG = 0,
	EZUI_FMT_GIF,
	EZUI_FMT_GIFA,
	EZUI_FMT_JPEG
};


EZGUI *ezgui_init(EZOPT *ezopt, int *argc, char ***argv);
int ezgui_run(EZGUI *gui, char *flist[], int fnum);
int ezgui_close(EZGUI *gui);
void ezgui_version(void);

#endif	/* _EZGUI_H_ */

