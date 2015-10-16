
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

#ifdef	CFG_GUI_GTK
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#else
#include "iup.h"
#endif

#include "libcsoup.h"
#include "ezthumb.h"


#define CFG_KEY_WIN_WIDTH	"window_width"
#define CFG_KEY_WIN_HEIGHT	"window_height"
#define CFG_KEY_WINDOWSTATE	"window_state"
#define CFG_KEY_WIN_POS		"window_position"
#define CFG_KEY_DIRECTORY	"last_directory"
#define CFG_KEY_GRID		"grid_define"
#define CFG_KEY_ZOOM		"zoom_define"
#define CFG_KEY_DURATION	"duration_mode"
#define CFG_KEY_TRANSPARENCY	"transparency"
#define CFG_KEY_JPG_QUALITY	"jpeg_quality"
#define CFG_KEY_GIF_FRATE	"gif_anim_delay"
#define CFG_KEY_OUTPUT_METHOD	"output_method"
#define CFG_KEY_OUTPUT_PATH	"output_path"
#define CFG_KEY_FONT_METHOD	"font_method"
#define CFG_KEY_FONT_FACE	"font_face"

#define	CFG_PIC_GRID_DIM	"Column and Row"	/* Grid 4x4 */
#define CFG_PIC_GRID_STEP	"Column and Step"	/* Grid 4 Step 15 */
#define CFG_PIC_DIS_NUM		"Discrete by Number"	/* DSS No 20 */
#define CFG_PIC_DIS_STEP	"Discrete by Step"	/* DSS Step 15 */
#define CFG_PIC_DIS_KEY		"Discrete key Frames"	/* DSS I-Frame */
#define CFG_PIC_ZOOM_RATIO	"Zoom by Ratio"		/* Zoom 50% */
#define CFG_PIC_ZOOM_DEFINE	"Zoom by Size"		/* Zoom 320x240 */
#define CFG_PIC_ZOOM_SCREEN	"Zoom by Canvas"	/* Res 1024 */

#define CFG_PIC_FMT_JPEG	"Jpeg"
#define CFG_PIC_FMT_PNG		"Png"
#define CFG_PIC_FMT_GIFA	"Animated GIF"
#define CFG_PIC_FMT_GIF		"GIF"

#define CFG_PIC_ODIR_CURR	"With the Media Files"
#define CFG_PIC_ODIR_PATH	"To the Diretory Below"

#define CFG_PIC_FONT_SYSTEM	"Use the System Font"
#define CFG_PIC_FONT_BROWSE	"Choose Font"


#if CFG_GUI_GTK

#define	CFG_SUBPATH	"ezthumb"
#define CFG_FILENAME	"ezthumb.conf"
#define CFG_GRP_MAIN	"main"

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
	EZOPT		*sysopt;	/* system parameters */
} EZGUI;
#else

typedef	struct		{
	EZOPT		*sysopt;	/* system parameters */
	char		inst_id[64];	/* instant identity */
	void		*config;
	/* current size of the main window and the decoration */
	int		win_width;	/* window size */
	int		win_height;
	int		win_dec_x;	/* client difference to the window */
	int		win_dec_y;
	int		win_state;	/* window show event */
	char		win_size[32];

	/* Main Page: work place */
	Ihandle		*list_view;

	/* Main Page: Progress Bar and Buttons */
	Ihandle		*prog_bar;	/* progress bar of current file */
	Ihandle		*prog_wait;	/* progress when waiting */
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
	Ihandle		*dlg_odir;	/* dialog of output directory */
	Ihandle		*dlg_font;

	/* Setup Page: the two main buttons in the bottom */
	Ihandle		*butt_setup_apply;
	Ihandle		*butt_setup_cancel;

	/* Setup Page: the choise of profiles */
	Ihandle		*prof_grid;
	Ihandle		*prof_zoom;
	
	/* Setup Page: the media process */
	Ihandle		*dfm_list;	/* duration finding mode */
	int		dfm_idx;
	Ihandle		*mpm_list;	/* media process method */
	int		mpm_idx;

	/* Setup Page: the font */
	Ihandle		*font_list;
	int		font_idx;
	Ihandle		*font_face;

	/* Setup Page: the output directory */
	Ihandle		*dir_list;
	int		dir_idx;
	Ihandle		*dir_path;

	/* Setup Page: the output file format */
	Ihandle		*fmt_list;
	Ihandle		*fmt_gif_fr;	/* GIF frame rate */
	Ihandle		*fmt_jpg_qf;	/* quality fact */
	Ihandle		*fmt_transp;	/* transparent */
	Ihandle		*fmt_zbox;
	Ihandle		*fmt_suffix;
	Ihandle		*fmt_exist;
	int		fmt_idx;
	int		fmt_idx_tmp;
	int		tmp_jpg_qf;
	int		tmp_gifa_fr;
	int		exist_idx;
} EZGUI;
#endif

#define	EZGUI_SVIEW_ACTIVE_MAX		8
#define EZGUI_SVIEW_ACTIVE_CONTENT	0
#define EZGUI_SVIEW_ACTIVE_SELECT	1
#define EZGUI_SVIEW_ACTIVE_PROGRESS	2
#define EZGUI_SVIEW_ACTIVE_BIND		3

typedef	struct		simpleview	{
	Ihandle		*filename;	/* the list control of file names */
	Ihandle		*filesize;	/* the list control of file sizes */
	Ihandle		*medialen;	/* the list control of media length */
	Ihandle		*resolution;	/* the list control of resolution */
	Ihandle		*progress;	/* the list control of progress */
	Ihandle		*attrib;	/* video attribution (invisible) */
	int		svidx;		/* current active item start from 1 */
	int 		svnum;		/* total items in the list */
	int		moused;		/* the recent line being moused to */
	EZGUI		*gui;		/* uplink to the EZGUI */

	/* set active status if contents exist */
	Ihandle		*act_content[EZGUI_SVIEW_ACTIVE_MAX];
	/* set active status if selection was made */
	Ihandle		*act_select[EZGUI_SVIEW_ACTIVE_MAX];
	/* set active status if progress bar moving */
	Ihandle		*act_progress[EZGUI_SVIEW_ACTIVE_MAX];
} SView;

#define EZOBJ_GRID_PROFILE	"GRIDPROFILE"
typedef	struct		{
	Ihandle		*entry_col_grid;
	Ihandle		*entry_col_step;
	Ihandle		*entry_row;
	Ihandle		*entry_step;
	Ihandle		*entry_dss_amnt;
	Ihandle		*entry_dss_step;
	Ihandle		*zbox;
	Ihandle		*hbox;

	EZGUI		*ezgui;
	int		grid_idx;	/* starts from 0 */
	//int		grid_tmp;
} SetGrid;

#define	EZOBJ_ZOOM_PROFILE	"ZOOMPROFILE"
typedef	struct		{
	Ihandle		*entry_zoom_ratio;
	Ihandle		*entry_zoom_wid;
	Ihandle		*entry_zoom_hei;
	Ihandle		*entry_width;
	Ihandle		*zbox;
	Ihandle		*hbox;

	EZGUI		*ezgui;
	int		zoom_idx;	/* starts from 0 */
	//int		zoom_tmp;
} SetZoom;

typedef	struct	{
	char	fsize[16];
	char	vidlen[16];
	char	resolv[16];
	char	progr[8];
	EZTIME	duration;
	int	seekable;
	int	bitrates;
	char	*showing;
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

