
/*  ezgui.c - the graphic user interface based on IUP

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
#include <ctype.h>

#include "iup.h"

#include "ezconfig.h"
#include "libcsoup.h"
#include "ezthumb.h"
#include "ezgui.h"
#include "ezicon.h"
#include "id_lookup.h"

/* re-use the debug convention in libcsoup */
//#define CSOUP_DEBUG_LOCAL	SLOG_CWORD(EZTHUMB_MOD_GUI, SLOG_LVL_WARNING)
//#define CSOUP_DEBUG_LOCAL	SLOG_CWORD(EZTHUMB_MOD_GUI, SLOG_LVL_MODULE)
#define CSOUP_DEBUG_LOCAL	SLOG_CWORD(EZTHUMB_MOD_GUI, SLOG_LVL_DEBUG)
#include "libcsoup_debug.h"


#define EZOBJ_MAIN	"EZGUIOBJ"
#define EZOBJ_SVIEW	"SVIEWOBJ"

#define EZGUI_MAINKEY	"GUI"		/* for the configure file */

#define EGPS_SETUP_DESCR	"120"
#define EGPS_SETUP_DROPDOWN	"200"	//"200x14"
#define EGPS_SETUP_SHORT_TEXT	"40"	//"40x10"
#define EGPS_GRID_FST_TEXT	"24"	//"24x10"
#define EGPS_GRID_SND_TEXT	"24"	//"18x10"
#define EGPS_SETUP_BUTTON	"60"


static	struct	idtbl	uir_grid[] = {
	{ 0,	CFG_PIC_AUTO }, 
	{ 0,	CFG_PIC_GRID_DIM }, 
	{ 0,	CFG_PIC_GRID_STEP },
	{ 0,	CFG_PIC_DIS_NUM }, 
	{ 0,	CFG_PIC_DIS_STEP }, 
	{ 0,	CFG_PIC_DIS_KEY }, 
	{ 0,	NULL }
};

static	struct	idtbl	uir_zoom[] = {
	{ 0,	CFG_PIC_AUTO }, 
	{ 0,	CFG_PIC_ZOOM_RATIO }, 
	{ 0,	CFG_PIC_ZOOM_DEFINE },
	{ 0,	CFG_PIC_ZOOM_SCREEN }, 
	{ 0,	NULL }
};

static	struct	idtbl	uir_format[] = {
	{ 0,	CFG_PIC_FMT_JPEG },
	{ 0,	CFG_PIC_FMT_PNG }, 
	{ 0,	CFG_PIC_FMT_GIFA },
	{ 0,	CFG_PIC_FMT_GIF }, 
	{ 0, 	NULL }
};

static	struct	idtbl	uir_outdir[] = {
	{ 0,	CFG_PIC_ODIR_CURR },
	{ 0,	CFG_PIC_ODIR_PATH },
	{ 0,	NULL }
};

static	struct	idtbl	uir_choose_font[] = {
	{ 0,	CFG_PIC_FONT_SYSTEM },
	{ 0,	CFG_PIC_FONT_BROWSE },
	{ 0,	NULL }
};

static int ezgui_timer_monitor(Ihandle *ih);
static int ezgui_create_window(EZGUI *gui);
static int ezgui_event_window_resize(Ihandle *ih, int width, int height);
static int ezgui_event_window_show(Ihandle *ih, int state);
static int ezgui_event_window_close(Ihandle *ih);

static Ihandle *ezgui_page_main(EZGUI *gui);
static int ezgui_page_main_reset(EZGUI *gui);
static Ihandle *ezgui_page_main_button(EZGUI *gui);
static int ezgui_event_main_add(Ihandle *ih);
static int ezgui_event_main_remove(Ihandle *ih);
static int ezgui_event_main_run(Ihandle *ih);
static int ezgui_show_progress(EZGUI *gui, int cur, int range);
static int ezgui_notificate(void *v, int eid, long param, long opt, void *b);

static Ihandle *ezgui_page_setup(EZGUI *gui);
static int ezgui_page_setup_reset(EZGUI *gui);

static Ihandle *ezgui_setup_grid_create(EZGUI *gui);
static Ihandle *ezgui_setup_grid_groupbox(Ihandle *gridbox);
static int ezgui_setup_grid_reset(Ihandle *gridbox);
//static int ezgui_setup_grid_read_index(Ihandle *gridbox);
//static int ezgui_setup_grid_write_index(Ihandle *gridbox, int idx);
static int ezgui_setup_grid_update(Ihandle *gridbox, char *status);
static int ezgui_setup_grid_event(Ihandle *ih, char *text, int i, int s);

static Ihandle *ezgui_setup_zoom_create(EZGUI *gui);
static Ihandle *ezgui_setup_zoom_groupbox(Ihandle *zoombox);
static int ezgui_setup_zoom_reset(Ihandle *zoombox);
//static int ezgui_setup_zoom_read_index(Ihandle *zoombox);
//static int ezgui_setup_zoom_write_index(Ihandle *zoombox, int index);
static int ezgui_setup_zoom_update(Ihandle *zoombox, char *status);
static int ezgui_setup_zoom_check(Ihandle *zoombox);
static int ezgui_setup_zoom_event(Ihandle *ih, char *text, int i, int s);

static Ihandle *ezgui_setup_media_create(EZGUI *gui);
static int ezgui_setup_media_reset(EZGUI *gui);
static int ezgui_setup_media_update(EZGUI *gui, char *status);
static int ezgui_setup_media_check(EZGUI *gui);
static int ezgui_setup_media_event(Ihandle *ih, char *text, int i, int s);

static Ihandle *ezgui_setup_outputdir_create(EZGUI *gui);
static int ezgui_setup_outputdir_reset(EZGUI *gui);
static int ezgui_setup_outputdir_update(EZGUI *gui);
static int ezgui_setup_outputdir_check(EZGUI *gui);
static int ezgui_setup_outputdir_event(Ihandle *ih, char *text, int i, int s);

static Ihandle *ezgui_setup_font_create(EZGUI *gui);
static int ezgui_setup_font_reset(EZGUI *gui);
static int ezgui_setup_font_update(EZGUI *gui);
static int ezgui_setup_font_check(EZGUI *gui);
static int ezgui_setup_font_event(Ihandle *ih, char *text, int i, int s);

static Ihandle *ezgui_setup_suffix_create(EZGUI *gui);
static int ezgui_setup_suffix_reset(EZGUI *gui);
static int ezgui_setup_suffix_update(EZGUI *gui);
static int ezgui_setup_suffix_check(EZGUI *gui);
static int ezgui_setup_suffix_event(Ihandle *ih);

static Ihandle *ezgui_setup_dupname_create(EZGUI *gui);
static int ezgui_setup_dupname_reset(EZGUI *gui);
static int ezgui_setup_dupname_update(EZGUI *gui);
static int ezgui_setup_dupname_check(EZGUI *gui);
static int ezgui_setup_dupname_event(Ihandle *ih, char *text, int i, int s);

static Ihandle *ezgui_setup_format_create(EZGUI *gui);
static int ezgui_setup_format_reset(EZGUI *gui);
static int ezgui_setup_format_update(EZGUI *gui);
static int ezgui_setup_format_check(EZGUI *gui);
static int ezgui_setup_format_event_picture(Ihandle *ih, char *text, int item, int);
static int ezgui_setup_format_event_transparent(Ihandle* ih, int state);
static int ezgui_setup_format_event_param(Ihandle* ih);

static Ihandle *ezgui_setup_button_create(EZGUI *gui);
static int ezgui_setup_button_check_status(EZGUI *gui);
static int ezgui_setup_button_event_ok(Ihandle *ih);
static int ezgui_setup_button_event_cancel(Ihandle *ih);

static Ihandle *ezgui_page_about(EZGUI *gui);

static Ihandle *ezgui_sview_create(EZGUI *gui, int dblck);
static int ezgui_sview_progress(Ihandle *ih, int percent);
static int ezgui_sview_resize(Ihandle *ih, int width, int height);
static int ezgui_sview_active_add(Ihandle *ih, int type, Ihandle *ctrl);
//static int ezgui_sview_active_remove(Ihandle *ih, int type, Ihandle *ctrl);
static int ezgui_sview_event_run(Ihandle *ih, int item, char *text);
static int ezgui_sview_event_dropfiles(Ihandle *, const char *,int, int, int);
static int ezgui_sview_event_multi_select(Ihandle *ih, char *value);
static int ezgui_sview_event_moused(Ihandle *ih, 
		int button, int pressed, int x, int y, char *status);
static int ezgui_sview_event_motion(Ihandle *ih, int x, int y, char *status);
static int ezgui_sview_add(SView *sview);
static int ezgui_sview_remove(SView *sview);
static int ezgui_sview_run(SView *sview);
static int ezgui_sview_file_append(SView *sview, char *fname);
static int ezgui_sview_file_remove(SView *sview, int idx);
static int ezgui_sview_active_update(SView *sview, int type, int num);

static Ihandle *xui_label(char *label, char *size, char *font);
static Ihandle *xui_text(Ihandle **xlst, char *label);
static Ihandle *xui_list_setting(Ihandle **xlst, char *label);
static int xui_list_get_idx(Ihandle *ih);
static int xui_text_get_number(Ihandle *ih);
static int xui_get_size(Ihandle *ih, char *attr, int *height);
static Ihandle *xui_text_setting(Ihandle **xtxt, char *label, char *ext);
static Ihandle *xui_text_grid(char *label, 
		Ihandle **xcol, Ihandle **xrow, char *ext);
static Ihandle *xui_button(char *prompt, Icallback ntf);
static int xui_config_status(void *config, char *prompt);
static char *xui_make_filters(char *slist);
static char *xui_make_font(char *face, int *size);


EZGUI *ezgui_init(EZOPT *ezopt, int *argcs, char ***argvs)
{
	EZGUI	*gui;

	IupOpen(argcs, argvs);
	IupImageLibOpen();

	IupSetGlobal("SINGLEINSTANCE", "ezthumb");
	if (!IupGetGlobal("SINGLEINSTANCE")) {
		IupClose();
		return NULL;
	}

	if ((gui = smm_alloc(sizeof(EZGUI))) == NULL) {
		return NULL;
	}

	/* initialize GUI structure with parameters from command line */
	gui->sysopt = ezopt;
	sprintf(gui->inst_id, "EZTHUMB_%p", gui);

	/* load configure from file, or create the file */
	gui->config = csc_cfg_open(SMM_CFGROOT_DESKTOP,
			"ezthumb", "ezthumb.conf", CSC_CFG_RWC);
	if (gui->config) {
		csc_cfg_status(gui->config, NULL);
		ezopt_load_config(ezopt, gui->config);
		xui_config_status(gui->config, "Read");
	}
	return gui;
}

int ezgui_run(EZGUI *gui, char *flist[], int fnum)
{
	SView	*sview;
	Ihandle *timer;
	int	i;

	ezgui_create_window(gui);

	/* 20160727 enable utf-8 mode for Windows.
	 * One case had been found that default setting can only accept utf-16
	 * in Windows, though utf-8 filename can be normally displayed in 
	 * File Explorer. Enabling the following attributions will let 
	 * IupFileDlg() return utf-8 filenames */
	/* Note that these two lines must be kept in this sequence */
	IupSetAttribute(NULL, "UTF8MODE", "YES");
	IupSetAttribute(NULL, "UTF8MODE_FILE", "YES");

	/* filling the work area with file names from command line */
	sview = (SView *) IupGetAttribute(gui->list_view, EZOBJ_SVIEW);
	if ((fnum > 0) && sview) {
		for (i = 0; i < fnum; i++) {
			ezgui_sview_file_append(sview, flist[i]);
			ezgui_show_progress(gui, i, fnum);
		}
		ezgui_show_progress(gui, i, fnum);
	}

	timer = IupTimer();
	IupSetCallback(timer, "ACTION_CB", (Icallback) ezgui_timer_monitor);
	IupSetAttribute(timer, EZOBJ_MAIN, (char*) gui);
	IupSetAttribute(timer, "TIME", "100");
	IupSetAttribute(timer, "RUN", "YES");

	IupMainLoop();
	IupSetAttribute(timer, "RUN", "NO");

	/* 20160719 It is known some event will come behind the CLOSE event,
	 * for example, click in a text control then click the close button,
	 * the KILLFOCUS will come behind the IupMainLoop() */
	/* I set NULL to 'EZOBJ_MAIN' as a flag to notify all events the main
	 * loop is closed. Having all event unhooked may be a better way */
	IupSetAttribute(gui->dlg_main, EZOBJ_MAIN, NULL);
	return 0;
}

int ezgui_close(EZGUI *gui)
{
	if (gui) {
		IupClose();
		if (gui->filefilter) {
			gui->filefilter = smm_free(gui->filefilter);
		}
		if (gui->config) {
			xui_config_status(gui->config, "Finalize");
			csc_cfg_close(gui->config);
		}
		smm_free(gui);
	}
	return 0;
}

static int ezgui_timer_monitor(Ihandle *ih)
{
	EZGUI	*gui;
	int	i;

	if ((gui = (EZGUI*) IupGetAttribute(ih, EZOBJ_MAIN)) == NULL) {
		return IUP_DEFAULT;
	}

	if (gui->dir_ppp_flag) {
		i = lookup_index_string(uir_outdir, 0, CFG_PIC_ODIR_CURR);
		IupSetInt(gui->dir_list, "VALUE", i + 1);
		IupSetAttribute(gui->dir_path, "VISIBLE", "NO");
		gui->dir_ppp_flag = 0;
	}
	if (gui->font_ppp_flag) {
		i = lookup_index_string(uir_choose_font, 0, CFG_PIC_FONT_SYSTEM);
		IupSetInt(gui->font_list, "VALUE", i + 1);
		IupSetAttribute(gui->font_face, "VISIBLE", "NO");
		gui->font_ppp_flag = 0;
	}
	return IUP_DEFAULT;
}

static int ezgui_create_window(EZGUI *gui)
{
	Ihandle		*tabs, *tabox;
	char		*s;
	char		win_size[32];

	/* find the extension name filter of files */
	/* the CFG_KEY_SUFFIX_FILTER has been opened in ezthumb.c before
	 * so make it readonly here. */
	s = csc_cfg_read(gui->config, NULL, CFG_KEY_SUFFIX_FILTER);
	if (s) {
		gui->filefilter = xui_make_filters(s);
	} else {
		gui->filefilter = xui_make_filters(EZ_DEF_FILTER);
	}

	/* create the Open-File dialog in the initialize stage.
	 * so in the event, it can be popup and hide without a real destory */
	gui->dlg_open = IupFileDlg();
	IupSetAttribute(gui->dlg_open, "PARENTDIALOG", gui->inst_id);
	IupSetAttribute(gui->dlg_open, "TITLE", "Open");
	IupSetAttribute(gui->dlg_open, "MULTIPLEFILES", "YES");
	IupSetAttribute(gui->dlg_open, "EXTFILTER", gui->filefilter);

	/* the Open-File dialog for picking an output directory */
	gui->dlg_odir = IupFileDlg();
	IupSetAttribute(gui->dlg_odir, "PARENTDIALOG", gui->inst_id);
	IupSetAttribute(gui->dlg_odir, "TITLE", "Save to");
	IupSetAttribute(gui->dlg_odir, "DIALOGTYPE", "DIR");

	gui->dlg_font = IupFontDlg();
	IupSetAttribute(gui->dlg_font, "PARENTDIALOG", gui->inst_id);
	IupSetAttribute(gui->dlg_font, "TITLE", "Setup Font");

	tabs = IupTabs(ezgui_page_main(gui), 
			ezgui_page_setup(gui), 
			ezgui_page_about(gui), 
			NULL);
	IupSetAttribute(tabs, "TABTITLE0", "&Generate");
	IupSetAttribute(tabs, "TABTITLE1", " &Setup  ");
	//IupSetAttribute(tabs, "TABTITLE2", "Advanced");
	IupSetAttribute(tabs, "TABTITLE2", " &About  ");
	IupSetAttribute(tabs, "PADDING", "6x2");

	tabox = IupHbox(tabs, NULL);
	IupSetAttribute(tabox, "NMARGIN", "8x8");

	IupSetHandle("DLG_ICON", IupImageRGBA(128, 128, ezicon_pixbuf));
	gui->dlg_main = IupDialog(tabox);
	IupSetAttribute(gui->dlg_main, "TITLE", "Ezthumb");
	
	/* retrieve the previous window size */
	gui->win_width = gui->win_height = 0;
	csc_cfg_read_int(gui->config, EZGUI_MAINKEY, 
			CFG_KEY_WIN_WIDTH, &gui->win_width);
	csc_cfg_read_int(gui->config, EZGUI_MAINKEY,
			CFG_KEY_WIN_HEIGHT, &gui->win_height);
	if (gui->win_width && gui->win_height) {
		sprintf(win_size, "%dx%d", gui->win_width, gui->win_height);
		IupSetAttribute(gui->dlg_main, "RASTERSIZE", win_size);
	} else {
		IupSetAttribute(gui->dlg_main, "RASTERSIZE", "x600");
	}

	/* recover the minimized placement of the window */
	s = csc_cfg_read(gui->config, EZGUI_MAINKEY, CFG_KEY_WINDOWSTATE);
	if (s) {
		IupSetAttribute(gui->dlg_main, "PLACEMENT", s);
	}
	
	IupSetAttribute(gui->dlg_main, "ICON", "DLG_ICON");
	IupSetHandle(gui->inst_id, gui->dlg_main);

	/* bind the GUI structure into the current dialog so it can be accessed
	 * in its sub-controls */
	IupSetAttribute(gui->dlg_main, EZOBJ_MAIN, (char*) gui);
	IupSetCallback(gui->dlg_main, "RESIZE_CB",
			(Icallback) ezgui_event_window_resize);
	IupSetCallback(gui->dlg_main, "SHOW_CB",
			(Icallback) ezgui_event_window_show);
	IupSetCallback(gui->dlg_main, "CLOSE_CB",
			(Icallback) ezgui_event_window_close);

	/* show the dialog window at the last location */
	s = csc_cfg_read(gui->config, EZGUI_MAINKEY, CFG_KEY_WIN_POS);
	if (s == NULL) {
		IupShow(gui->dlg_main);
	} else {
		int	y = 0, x = (int) strtol(s, NULL, 0);

		if ((s = strchr(s, ',')) != NULL) {
			y = (int) strtol(s+1, NULL, 0);
		}
		IupShowXY(gui->dlg_main, x, y);
	}

	/* bind the notification function to GUI mode */
	gui->sysopt->notify = ezgui_notificate;

	ezgui_page_setup_reset(gui);
	ezgui_setup_button_event_ok(gui->butt_setup_apply);
	ezgui_page_main_reset(gui);
	return 0;
}

static int ezgui_event_window_resize(Ihandle *ih, int width, int height)
{
	EZGUI	*gui;

	if ((gui = (EZGUI *) IupGetAttribute(ih, EZOBJ_MAIN)) == NULL) {
		return IUP_DEFAULT;
	}

	/* 20160115 setting "RASTERSIZE" again gain the ability of shrink */
	IupSetAttribute(ih, "RASTERSIZE", IupGetAttribute(ih, "RASTERSIZE"));
	gui->win_width = xui_get_size(ih, "RASTERSIZE", &gui->win_height);

	CDB_MODL(("EVT_RESIZE: C:%dx%d W:%dx%d P:%s\n", width, height,
			gui->win_width, gui->win_height, 
			IupGetAttribute(gui->dlg_main, "SCREENPOSITION")));

	if (gui->win_state != IUP_MAXIMIZE) {
		csc_cfg_write_int(gui->config, EZGUI_MAINKEY,
				CFG_KEY_WIN_WIDTH, gui->win_width);
		csc_cfg_write_int(gui->config, EZGUI_MAINKEY,
				CFG_KEY_WIN_HEIGHT, gui->win_height);
	}

	/* 20151223 retrieve the client size of scrollbox instead */
	width = xui_get_size(gui->list_sbox, "CLIENTSIZE", &height);
	ezgui_sview_resize(gui->list_view, width, height);
	return IUP_DEFAULT;
}

static int ezgui_event_window_show(Ihandle *ih, int state)
{
	EZGUI	*gui;

#ifdef	DEBUG
	switch (state) {
	case IUP_HIDE:
		CDB_MODL(("EVT_SHOW(%d): IUP_HIDE\n", state));
		break;
	case IUP_SHOW:
		CDB_MODL(("EVT_SHOW(%d): IUP_SHOW\n", state));
		break;
	case IUP_RESTORE:
		CDB_MODL(("EVT_SHOW(%d): IUP_RESTORE\n", state));
		break;
	case IUP_MINIMIZE:
		CDB_MODL(("EVT_SHOW(%d): IUP_MINIMIZE\n", state));
		break;
	case IUP_MAXIMIZE:
		CDB_MODL(("EVT_SHOW(%d): IUP_MAXIMIZE\n", state));
		break;
	case IUP_CLOSE:
		CDB_MODL(("EVT_SHOW(%d): IUP_CLOSE\n", state));
		break;
	default:
		CDB_MODL(("EVT_SHOW(%d): unknown\n", state));
		break;
	}
#endif
	if ((gui = (EZGUI *) IupGetAttribute(ih, EZOBJ_MAIN)) == NULL) {
		return IUP_DEFAULT;
	}

	/* we don't save the MAXIMIZED status because when IUP set the 
	 * PLACEMENT of MAXIMIZED, IUP won't generate the resize event */
	gui->win_state = state;

	switch (state) {
	case IUP_MINIMIZE:
		csc_cfg_write(gui->config, EZGUI_MAINKEY,
				CFG_KEY_WINDOWSTATE, "MINIMIZED");
		break;
	case IUP_HIDE:
		break;
	/* case IUP_MAXIMIZE:
		csc_cfg_write(gui->config, EZGUI_MAINKEY,
				CFG_KEY_WINDOWSTATE, "MAXIMIZED");
		break;*/
	default:
		csc_cfg_delete_key(gui->config, EZGUI_MAINKEY, 
				CFG_KEY_WINDOWSTATE);
		break;
	}
	return 0;
}

static int ezgui_event_window_close(Ihandle *ih)
{
	EZGUI	*gui;
	char	*s;
	
	if ((gui = (EZGUI *) IupGetAttribute(ih, EZOBJ_MAIN)) == NULL) {
		return IUP_DEFAULT;
	}

	s = IupGetAttribute(ih, "SCREENPOSITION");

	CDB_MODL(("EVT_CLOSE: SCREENPOSITION = %s\n", s));
	csc_cfg_write(gui->config, EZGUI_MAINKEY, CFG_KEY_WIN_POS, s);
	return 0;
}


/****************************************************************************
 * Page Main 
 ****************************************************************************/
static Ihandle *ezgui_page_main(EZGUI *gui)
{
	Ihandle	*vbox, *hbox;

	/* the progress bar of the current processing file */
	gui->prog_bar = IupProgressBar();
	IupSetAttribute(gui->prog_bar, "EXPAND", "HORIZONTAL");
	IupSetAttribute(gui->prog_bar, "DASHED", "YES");
	IupSetAttribute(gui->prog_bar, "SIZE", "x10");

	/* the progress bar of the task list */
	gui->prog_wait = IupProgressBar();
	IupSetAttribute(gui->prog_wait, "EXPAND", "HORIZONTAL");
	IupSetAttribute(gui->prog_wait, "DASHED", "YES");
	IupSetAttribute(gui->prog_wait, "SIZE", "x10");
	IupSetAttribute(gui->prog_wait, "MARQUEE", "YES");

	/* the status bar */
	gui->stat_bar = IupLabel("");
	IupSetAttribute(gui->stat_bar, "EXPAND", "HORIZONTAL");

	/* status bar and progress bars share the same conner. normally it
	 * display the status until in the running mode */
	/* in C, each element must have a name defined by IupSetHandle (why?)
	 * but its children automatically receives a name when the child is 
	 * appended or inserted into the tabs */
	gui->ps_zbox = IupZbox(NULL);
	IupAppend(gui->ps_zbox, gui->stat_bar);
	IupAppend(gui->ps_zbox, gui->prog_bar);
	IupAppend(gui->ps_zbox, gui->prog_wait);
	IupSetAttribute(gui->ps_zbox, "ALIGNMENT", "ACENTER");

	/* progres bar and buttons are in the same bottom line */
	hbox = IupHbox(gui->ps_zbox, ezgui_page_main_button(gui), NULL);
	IupSetAttribute(hbox, "NGAP", "10");
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");

	/* grouping with the work area, a group of lists inside a scroll box */
	gui->list_view = ezgui_sview_create(gui, 1);
	ezgui_sview_active_add(gui->list_view, 
			EZGUI_SVIEW_ACTIVE_CONTENT, gui->button_run);
	ezgui_sview_active_add(gui->list_view, 
			EZGUI_SVIEW_ACTIVE_SELECT, gui->button_del);
	ezgui_sview_active_add(gui->list_view, 
			EZGUI_SVIEW_ACTIVE_PROGRESS, gui->prog_bar);
	ezgui_sview_active_add(gui->list_view,
			EZGUI_SVIEW_ACTIVE_BIND, gui->button_add);
	
	gui->list_sbox = IupScrollBox(gui->list_view);
	IupSetAttribute(gui->list_sbox, "SCROLLBAR", "VERTICAL");

	vbox = IupVbox(gui->list_sbox, hbox, NULL);
	IupSetAttribute(vbox, "NGAP", "4");
	IupSetAttribute(vbox, "NMARGIN", "4x4");
	return vbox;
}

static int ezgui_page_main_reset(EZGUI *gui)
{
	SView	*sview;

	IupSetInt(gui->ps_zbox, "VALUEPOS", 0);
	IupSetAttribute(gui->button_del, "ACTIVE", "NO");

	sview = (SView *) IupGetAttribute(gui->button_run, EZOBJ_SVIEW);
	if (sview) {
		ezgui_sview_active_update(sview, 
				EZGUI_SVIEW_ACTIVE_CONTENT, sview->svnum);
	}
	return 0;
}

static Ihandle *ezgui_page_main_button(EZGUI *gui)
{
	gui->button_add = xui_button("Add", NULL);
	IupSetAttribute(gui->button_add, "IMAGE", "IUP_FileOpen");
	IupSetCallback(gui->button_add, "ACTION",
			(Icallback) ezgui_event_main_add);
	gui->button_del = xui_button("Remove", NULL);
	IupSetAttribute(gui->button_del, "IMAGE", "IUP_EditErase");
	IupSetCallback(gui->button_del, "ACTION",
			(Icallback) ezgui_event_main_remove);
	gui->button_run = xui_button("Run", NULL);
	IupSetAttribute(gui->button_run,"IMAGE", "IUP_ActionOk");
	IupSetCallback(gui->button_run, "ACTION",
			(Icallback) ezgui_event_main_run);
	return IupHbox(gui->button_add, gui->button_del, gui->button_run,NULL);
}

static int ezgui_event_main_add(Ihandle *ih)
{
	SView	*sview;

	if ((sview = (SView*)IupGetAttribute(ih, EZOBJ_SVIEW)) != NULL) {
		ezgui_sview_add(sview);
	}
	return IUP_DEFAULT;
}

static int ezgui_event_main_remove(Ihandle *ih)
{
	SView	*sview;

	if ((sview = (SView*)IupGetAttribute(ih, EZOBJ_SVIEW)) != NULL) {
		ezgui_sview_remove(sview);
	}
	return IUP_DEFAULT;
}

static int ezgui_event_main_run(Ihandle *ih)
{
	SView	*sview;

	if ((sview = (SView*)IupGetAttribute(ih, EZOBJ_SVIEW)) != NULL) {
		ezgui_sview_run(sview);
	}
	return IUP_DEFAULT;
}

static int ezgui_show_progress(EZGUI *gui, int cur, int range)
{
	if (cur == 0) {		/* begin to display progress */
		IupSetInt(gui->prog_bar, "MIN", 0);
		IupSetInt(gui->ps_zbox, "VALUEPOS", 1);
	} else if (cur == range) {	/* end of display */
		IupSetInt(gui->ps_zbox, "VALUEPOS", 0);
	} else if (cur < range) {
		IupSetInt(gui->prog_bar, "MAX", range);
		IupSetInt(gui->prog_bar, "VALUE", cur);
	}
	IupFlush();
	return 0;
}

static int ezgui_show_duration(EZGUI *gui, int state)
{
	if (state == EN_OPEN_BEGIN) {
		IupSetInt(gui->prog_wait, "VALUE", 0);
		IupSetInt(gui->ps_zbox, "VALUEPOS", 2);
	} else if (state == EN_OPEN_END) {
		IupSetInt(gui->ps_zbox, "VALUEPOS", 0);
	} else {
		IupSetInt(gui->prog_wait, "VALUE", 1);
	}
	IupFlush();
	return 0;
}

static int ezgui_notificate(void *v, int eid, long param, long opt, void *b)
{
	EZGUI	*gui = ((EZOPT*) v)->gui;

	(void)b;

	switch (eid) {
	case EN_PROC_BEGIN:
		ezgui_show_progress(gui, 0, 0);	/* show/reset progress bar */
		break;
	case EN_PROC_CURRENT:
		ezgui_sview_progress(gui->prog_bar, (int)(opt * 100 / param));
		ezgui_show_progress(gui, opt, param);
		break;
	case EN_PROC_END:
		ezgui_sview_progress(gui->prog_bar, 100);
		ezgui_show_progress(gui, param, param);
		break;
	case EN_OPEN_BEGIN:
	case EN_OPEN_GOING:
	case EN_OPEN_END:
		ezgui_show_duration(gui, eid);
		break;
	default:
		return EN_EVENT_PASSTHROUGH;
	}
	return eid;
}


/****************************************************************************
 * Page Setup 
 ****************************************************************************/
static Ihandle *ezgui_page_setup(EZGUI *gui)
{
	Ihandle	*vbox, *sbox;

	/* create the setup working area */
	sbox = IupVbox(xui_label("Profile Selection", NULL, "Bold"), NULL);
	IupSetAttribute(sbox, "NGAP", "8");
	IupSetAttribute(sbox, "NMARGIN", "16x16");

	/* adding profiles of grid and zoom */
	gui->prof_grid = ezgui_setup_grid_create(gui);
	gui->prof_zoom = ezgui_setup_zoom_create(gui);
	vbox = IupVbox(ezgui_setup_grid_groupbox(gui->prof_grid),
			ezgui_setup_zoom_groupbox(gui->prof_zoom), NULL);
	IupSetAttribute(vbox, "NMARGIN", "16x4");
	IupSetAttribute(vbox, "NGAP", "4");
	IupAppend(sbox, vbox);

	/* adding media processing */
	IupAppend(sbox, xui_label("Media Processing", NULL, "Bold"));
	IupAppend(sbox, ezgui_setup_media_create(gui));

	/* adding font setup */
	IupAppend(sbox, xui_label("Font", NULL, "Bold"));
	IupAppend(sbox, ezgui_setup_font_create(gui));

	/* adding output directory */
	IupAppend(sbox, xui_label("Output Directory", NULL, "Bold"));
	IupAppend(sbox, ezgui_setup_outputdir_create(gui));

	/* adding thumbnail output */
	IupAppend(sbox, xui_label("Output Thumbnails", NULL, "Bold"));
	vbox = IupVbox(ezgui_setup_suffix_create(gui), 
			ezgui_setup_dupname_create(gui),
			ezgui_setup_format_create(gui), NULL);
	IupSetAttribute(vbox, "NMARGIN", "16x4");
	IupSetAttribute(vbox, "NGAP", "4");
	IupAppend(sbox, vbox);

	/* create the scroll box around the setup working area */
	sbox = IupScrollBox(sbox);
	IupSetAttribute(sbox, "SCROLLBAR", "VERTICAL");

	/* pack the scroll box with the button area */
	vbox = IupVbox(sbox, ezgui_setup_button_create(gui), NULL);
	IupSetAttribute(vbox, "NGAP", "4");
	IupSetAttribute(vbox, "NMARGIN", "4x4");
	return vbox;
}

static int ezgui_page_setup_reset(EZGUI *gui)
{
	ezgui_setup_grid_reset(gui->prof_grid);
	ezgui_setup_zoom_reset(gui->prof_zoom);
	ezgui_setup_media_reset(gui);
	ezgui_setup_font_reset(gui);
	ezgui_setup_outputdir_reset(gui);
	ezgui_setup_suffix_reset(gui);
	ezgui_setup_dupname_reset(gui);
	ezgui_setup_format_reset(gui);

	/* store the default configures */
	//ezgui_setup_button_event_ok(gui->butt_setup_apply);
	return 0;
}

static Ihandle *ezgui_setup_grid_create(EZGUI *gui)
{
	static	SetGrid	setgridbuffer;		//FIXME: not reenterable
	Ihandle	*hbox, *gridbox;
	SetGrid	*grid = &setgridbuffer;
	EZOPT	*ezopt;
	char	*s;
	int	i;

	/* create the zbox of grid parameters */
	grid->zbox = IupZbox(IupFill(), NULL);

	hbox = xui_text_grid("Grid", 
			&grid->entry_col_grid, &grid->entry_row, NULL);
	IupAppend(grid->zbox, hbox);
	
	hbox = xui_text_grid("Column", 
			&grid->entry_col_step, &grid->entry_step, "(s)");
	IupAppend(grid->zbox, hbox);

	hbox = xui_text_grid("Total", &grid->entry_dss_amnt, NULL, NULL);
	IupAppend(grid->zbox, hbox);

	hbox = xui_text_grid("Every", &grid->entry_dss_step, NULL, "(s)");
	IupAppend(grid->zbox, hbox);

	IupAppend(grid->zbox, IupFill());

	/* create grid drop list */
	grid->hbox = xui_list_setting(&gridbox, "Grid Setting");
	IupAppend(grid->hbox, grid->zbox);

	for (i = 0; uir_grid[i].s; i++) {
		IupSetAttributeId(gridbox, "", i + 1, uir_grid[i].s);
	}
	IupSetAttribute(gridbox, EZOBJ_GRID_PROFILE, (char*) grid);
	IupSetCallback(gridbox, "ACTION", (Icallback) ezgui_setup_grid_event);

	/* find the index of drop down lists: the grid drop down */
	ezopt = gui->sysopt;
	s = csc_cfg_read(gui->config, EZGUI_MAINKEY, CFG_KEY_GRID);
	if (s) {
		grid->grid_idx = lookup_index_string(uir_grid, 0, s);
	} else if (ezopt->pro_grid != NULL) {
		grid->grid_idx = lookup_index_string(uir_grid, 0, CFG_PIC_AUTO);
	} else if (ezopt->grid_col && ezopt->grid_row) {
		grid->grid_idx = lookup_index_string(uir_grid, 0, CFG_PIC_GRID_DIM);
	} else if (ezopt->grid_col && ezopt->tm_step) {
		grid->grid_idx = lookup_index_string(uir_grid, 0, CFG_PIC_GRID_STEP);
	} else if (!ezopt->grid_col && ezopt->grid_row) {
		grid->grid_idx = lookup_index_string(uir_grid, 0, CFG_PIC_DIS_NUM);
	} else if (!ezopt->grid_col && !ezopt->grid_row && ezopt->tm_step) {
		grid->grid_idx = lookup_index_string(uir_grid, 0, CFG_PIC_DIS_STEP);
	} else if (!ezopt->grid_col && !ezopt->grid_row && !ezopt->tm_step) {
		grid->grid_idx = lookup_index_string(uir_grid, 0, CFG_PIC_DIS_KEY);
	} else {
		grid->grid_idx = lookup_index_string(uir_grid, 0, CFG_PIC_AUTO);
	}

	grid->ezgui = gui;
	return gridbox;
}

static Ihandle *ezgui_setup_grid_groupbox(Ihandle *gridbox)
{
	SetGrid	*grid;

	grid = (SetGrid *) IupGetAttribute(gridbox, EZOBJ_GRID_PROFILE);
	if (grid != NULL) {
		return grid->hbox;
	}
	return NULL;
}

static int ezgui_setup_grid_reset(Ihandle *gridbox)
{
	SetGrid	*grid;
	EZOPT	*sopt;

	grid = (SetGrid *) IupGetAttribute(gridbox, EZOBJ_GRID_PROFILE);
	if (grid != NULL) {
		sopt = ((EZGUI*)grid->ezgui)->sysopt;
		IupSetInt(gridbox, "VALUE", grid->grid_idx + 1);
		IupSetInt(grid->entry_col_grid, "VALUE", sopt->grid_col);
		IupSetInt(grid->entry_col_step, "VALUE", sopt->grid_col);
		IupSetInt(grid->entry_row, "VALUE", sopt->grid_row);
		IupSetInt(grid->entry_step, "VALUE", sopt->tm_step / 1000);
		IupSetInt(grid->entry_dss_amnt, "VALUE", sopt->grid_row);
		IupSetInt(grid->entry_dss_step, "VALUE", sopt->tm_step / 1000);
		IupSetInt(grid->zbox, "VALUEPOS", grid->grid_idx);
	}
	return IUP_DEFAULT;
}

/*
static int ezgui_setup_grid_read_index(Ihandle *gridbox)
{
	SetGrid	*grid;

	grid = (SetGrid *) IupGetAttribute(gridbox, EZOBJ_GRID_PROFILE);
	if (grid != NULL) {
		return grid->grid_idx;
	}
	return -1;
}

static int ezgui_setup_grid_write_index(Ihandle *gridbox, int idx)
{
	SetGrid	*grid;

	grid = (SetGrid *) IupGetAttribute(gridbox, EZOBJ_GRID_PROFILE);
	if (grid != NULL) {
		grid->grid_idx = idx;
		return idx;
	}
	return -1;
}
*/
static int ezgui_setup_grid_update(Ihandle *gridbox, char *status)
{
	SetGrid	*grid;
	EZGUI	*gui;
	EZOPT	*opt;
	char	tmp[128];

	grid = (SetGrid *) IupGetAttribute(gridbox, EZOBJ_GRID_PROFILE);
	if (grid == NULL) {
		return -1;
	}

	gui = grid->ezgui;
	opt = gui->sysopt;
	grid->grid_idx = xui_list_get_idx(gridbox);
	csc_cfg_write(gui->config, EZGUI_MAINKEY, 
			CFG_KEY_GRID, uir_grid[grid->grid_idx].s);

	/* FIXME: not quite readible */
	switch (grid->grid_idx) {
	case 0:
		strcpy(tmp, "Auto Grid ");
		break;
	case 1:
		opt->grid_col = xui_text_get_number(grid->entry_col_grid);
		opt->grid_row = xui_text_get_number(grid->entry_row);
		sprintf(tmp, "Grid:%dx%d ", opt->grid_col, opt->grid_row);
		ezopt_profile_disable(opt, EZ_PROF_LENGTH);
		break;
	case 2:
		opt->grid_col = xui_text_get_number(grid->entry_col_step);
		opt->tm_step  = xui_text_get_number(grid->entry_step);
		sprintf(tmp, "Column:%d Step:%d(s) ", 
				opt->grid_col, opt->tm_step);
		opt->tm_step *= 1000;
		ezopt_profile_disable(opt, EZ_PROF_LENGTH);
		break;
	case 3:
		opt->grid_col = 0;
		opt->grid_row = xui_text_get_number(grid->entry_dss_amnt);
		sprintf(tmp, "Total %d snaps ", opt->grid_row);
		ezopt_profile_disable(opt, EZ_PROF_LENGTH);
		break;
	case 4:
		opt->grid_col = 0;
		opt->grid_row = 0;
		opt->tm_step  = xui_text_get_number(grid->entry_dss_step);
		sprintf(tmp, "Snap every %d(s) ", opt->tm_step);
		opt->tm_step *= 1000;
		ezopt_profile_disable(opt, EZ_PROF_LENGTH);
		break;
	case 5:
		opt->grid_col = 0;
		opt->grid_row = 0;
		opt->tm_step  = 0;
		strcpy(tmp, "Separate I-Frames ");
		ezopt_profile_disable(opt, EZ_PROF_LENGTH);
		break;
	default:
		strcpy(tmp, "Oops; ");
		break;
	}
	strcat(status, tmp);
	IupSetInt(grid->zbox, "VALUEPOS", grid->grid_idx);

	/* save all related parameters into the configure file */
	csc_cfg_write_int(gui->config, NULL, CFG_KEY_GRID_COLUMN, 
			opt->grid_col);
	csc_cfg_write_int(gui->config, NULL, CFG_KEY_GRID_ROW, 
			opt->grid_row);
	csc_cfg_write_int(gui->config, NULL, CFG_KEY_TIME_STEP, 
			opt->tm_step);
	return 0;
}

static int ezgui_setup_grid_check(Ihandle *gridbox)
{
	SetGrid	*grid;
	EZOPT	*opt;

	grid = (SetGrid *) IupGetAttribute(gridbox, EZOBJ_GRID_PROFILE);
	if (grid == NULL) {
		return 0;
	}

	if (grid->grid_idx != xui_list_get_idx(gridbox)) {
		return 1;
	}
	
	opt = grid->ezgui->sysopt;
	switch (grid->grid_idx) {
	case 1:
		if (opt->grid_col != xui_text_get_number(grid->entry_col_grid)) {
			return 1;
		}
		if (opt->grid_row != xui_text_get_number(grid->entry_row)) {
			return 1;
		}
		break;
	case 2:
		if (opt->grid_col != xui_text_get_number(grid->entry_col_step)) {
			return 1;
		}
		if (opt->tm_step  != xui_text_get_number(grid->entry_step) * 1000) {
			return 1;
		}
		break;
	case 3:
		if (opt->grid_row != xui_text_get_number(grid->entry_dss_amnt)) {
			return 1;
		}
		break;
	case 4:
		if (opt->tm_step != xui_text_get_number(grid->entry_dss_step) * 1000) {
			return 1;
		}
		break;
	}
	return 0;
}

static int ezgui_setup_grid_event(Ihandle *ih, char *text, int i, int s)
{
	SetGrid	*grid;

	(void) text;	/* stop the gcc complaining */
	//printf("ezgui_setup_grid_event: %s %d %d\n", text, i, s);
	
	/* we don't care about the leaving event */
	if (s == 0) {
		return IUP_DEFAULT;
	}

	grid = (SetGrid *) IupGetAttribute(ih, EZOBJ_GRID_PROFILE);
	if (grid != NULL) {
		IupSetInt(grid->zbox, "VALUEPOS", i - 1);
		ezgui_setup_button_check_status(grid->ezgui);
	}
	return IUP_DEFAULT;
}

static Ihandle *ezgui_setup_zoom_create(EZGUI *gui)
{
	static	SetZoom	setzoombuffer;
	Ihandle	*hbox, *zoombox;
	SetZoom	*zoom = &setzoombuffer;
	EZOPT	*ezopt;
	char	*s;
	int	i;

	/* create the zbox of zoom parameters */
	zoom->zbox = IupZbox(IupFill(), NULL);

	hbox = xui_text_grid("Ratio", &zoom->entry_zoom_ratio, NULL, "%");
	//IupSetAttribute(zoom->entry_zoom_ratio, "SIZE", "24x11");
	IupSetAttribute(zoom->entry_zoom_ratio, "SIZE", "30");
	IupSetAttribute(zoom->entry_zoom_ratio, "SPIN", "YES");
	IupSetAttribute(zoom->entry_zoom_ratio, "SPINMIN", "5");
	IupSetAttribute(zoom->entry_zoom_ratio, "SPINMAX", "200");
	IupSetAttribute(zoom->entry_zoom_ratio, "SPININC", "5");
	IupSetAttribute(zoom->entry_zoom_ratio, "SPINALIGN", "LEFT");
	IupSetAttribute(zoom->entry_zoom_ratio, "SPINVALUE", "50");
	IupAppend(zoom->zbox, hbox);

	hbox = xui_text_grid("Pixel", 
			&zoom->entry_zoom_wid, &zoom->entry_zoom_hei, NULL);
	IupAppend(zoom->zbox, hbox);
	
	hbox = xui_text_grid("Width", &zoom->entry_width, NULL, NULL);
	IupAppend(zoom->zbox, hbox);
	
	/* create zoom drop list */
	zoom->hbox = xui_list_setting(&zoombox, "Zoom Setting");
	IupAppend(zoom->hbox, zoom->zbox);

	for (i = 0; uir_zoom[i].s; i++) {
		IupSetAttributeId(zoombox, "", i + 1, uir_zoom[i].s);
	}
	IupSetAttribute(zoombox, EZOBJ_ZOOM_PROFILE, (char*) zoom);
	IupSetCallback(zoombox, "ACTION", (Icallback) ezgui_setup_zoom_event);

	/* find the index of drop down lists: the zoom drop down */
	ezopt = gui->sysopt;
	s = csc_cfg_read(gui->config, EZGUI_MAINKEY, CFG_KEY_ZOOM);
	if (s) {
		zoom->zoom_idx = lookup_index_string(uir_zoom, 0, s);
	} else if (ezopt->pro_size != NULL) {
		zoom->zoom_idx = lookup_index_string(uir_zoom, 0, CFG_PIC_AUTO);
	} else if (ezopt->tn_facto) {
		zoom->zoom_idx = lookup_index_string(uir_zoom, 0, CFG_PIC_ZOOM_RATIO);
	} else if (ezopt->tn_width && ezopt->tn_height) {
		zoom->zoom_idx = lookup_index_string(uir_zoom, 0, CFG_PIC_ZOOM_DEFINE);
	} else if (ezopt->canvas_width) {
		zoom->zoom_idx = lookup_index_string(uir_zoom, 0, CFG_PIC_ZOOM_SCREEN);
	} else {
		zoom->zoom_idx = lookup_index_string(uir_zoom, 0, CFG_PIC_AUTO);
	}

	zoom->ezgui = gui;
	return zoombox;
}

static Ihandle *ezgui_setup_zoom_groupbox(Ihandle *zoombox)
{
	SetZoom	*zoom;

	zoom = (SetZoom *) IupGetAttribute(zoombox, EZOBJ_ZOOM_PROFILE);
	if (zoom != NULL) {
		return zoom->hbox;
	}
	return NULL;
}

static int ezgui_setup_zoom_reset(Ihandle *zoombox)
{
	SetZoom	*zoom;
	EZOPT	*sopt;

	zoom = (SetZoom *) IupGetAttribute(zoombox, EZOBJ_ZOOM_PROFILE);
	if (zoom != NULL) {
		sopt = zoom->ezgui->sysopt;
		IupSetInt(zoombox, "VALUE", zoom->zoom_idx + 1);
		IupSetInt(zoom->entry_zoom_ratio, "VALUE", sopt->tn_facto);
		IupSetInt(zoom->entry_zoom_wid, "VALUE", sopt->tn_width);
		IupSetInt(zoom->entry_zoom_hei, "VALUE", sopt->tn_height);
		IupSetInt(zoom->entry_width, "VALUE", sopt->canvas_width);
		IupSetInt(zoom->zbox, "VALUEPOS", zoom->zoom_idx);
	}
	return IUP_DEFAULT;
}

/*
static int ezgui_setup_zoom_read_index(Ihandle *zoombox)
{
	SetZoom	*zoom;

	zoom = (SetZoom *) IupGetAttribute(zoombox, EZOBJ_ZOOM_PROFILE);
	if (zoom != NULL) {
		return zoom->zoom_idx;
	}
	return -1;
}

static int ezgui_setup_zoom_write_index(Ihandle *zoombox, int index)
{
	SetZoom	*zoom;

	zoom = (SetZoom *) IupGetAttribute(zoombox, EZOBJ_ZOOM_PROFILE);
	if (zoom != NULL) {
		zoom->zoom_idx = index;
		return index;
	}
	return -1;
}
*/

static int ezgui_setup_zoom_update(Ihandle *zoombox, char *status)
{
	SetZoom	*zoom;
	EZGUI	*gui;
	EZOPT	*opt;
	char	tmp[128];

	zoom = (SetZoom *) IupGetAttribute(zoombox, EZOBJ_ZOOM_PROFILE);
	if (zoom == NULL) {
		return -1;
	}

	gui = zoom->ezgui;
	opt = gui->sysopt;
	zoom->zoom_idx = xui_list_get_idx(zoombox);
	csc_cfg_write(gui->config, EZGUI_MAINKEY,
			CFG_KEY_ZOOM, uir_zoom[zoom->zoom_idx].s);

	switch (zoom->zoom_idx) {
	case 0:
		strcpy(tmp, "Auto Zoom ");
		break;
	case 1:
		opt->tn_facto  = xui_text_get_number(zoom->entry_zoom_ratio);
		sprintf(tmp, "Zoom to %d%% ", opt->tn_facto);
		ezopt_profile_disable(opt, EZ_PROF_WIDTH);
		break;
	case 2:
		opt->tn_width  = xui_text_get_number(zoom->entry_zoom_wid);
		opt->tn_height = xui_text_get_number(zoom->entry_zoom_hei);
		sprintf(tmp, "Zoom to %dx%d ", 
				opt->tn_width, opt->tn_height);
		ezopt_profile_disable(opt, EZ_PROF_WIDTH);
		break;
	case 3:
		opt->canvas_width = xui_text_get_number(zoom->entry_width);
		sprintf(tmp, "Canvas Width %d ", opt->canvas_width);
		ezopt_profile_disable(opt, EZ_PROF_WIDTH);
		break;
	default:
		strcpy(tmp, "Oops; ");
		break;
	}
	strcat(status, tmp);
	IupSetInt(zoom->zbox, "VALUEPOS", zoom->zoom_idx);

	/* save all related parameters into the configure file */
	csc_cfg_write_int(gui->config, NULL, CFG_KEY_ZOOM_RATIO, 
			opt->tn_facto);
	csc_cfg_write_int(gui->config, NULL, CFG_KEY_ZOOM_WIDTH, 
			opt->tn_width);
	csc_cfg_write_int(gui->config, NULL, CFG_KEY_ZOOM_HEIGHT, 
			opt->tn_height);
	csc_cfg_write_int(gui->config, NULL, CFG_KEY_CANVAS_WIDTH, 
			opt->canvas_width);
	return 0;
}

static int ezgui_setup_zoom_check(Ihandle *zoombox)
{
	SetZoom	*zoom;
	EZOPT	*opt;

	zoom = (SetZoom *) IupGetAttribute(zoombox, EZOBJ_ZOOM_PROFILE);
	if (zoom == NULL) {
		return 0;
	}

	if (zoom->zoom_idx != xui_list_get_idx(zoombox)) {
		return 1;
	}

	opt = zoom->ezgui->sysopt;
	switch (zoom->zoom_idx) {
	case 1:
		if (opt->tn_facto != xui_text_get_number(zoom->entry_zoom_ratio)) {
			return 1;
		}
		break;
	case 2:
		if (opt->tn_width != xui_text_get_number(zoom->entry_zoom_wid)) {
			return 1;
		}
		if (opt->tn_height != xui_text_get_number(zoom->entry_zoom_hei)) {
			return 1;
		}
		break;
	case 3:
		if (opt->canvas_width != xui_text_get_number(zoom->entry_width)) {
			return 1;
		}
		break;
	}
	return 0;
}
	
static int ezgui_setup_zoom_event(Ihandle *ih, char *text, int i, int s)
{
	SetZoom	*zoom;

	(void) text;	/* stop the gcc complaining */

	zoom = (SetZoom *) IupGetAttribute(ih, EZOBJ_ZOOM_PROFILE);
	if ((zoom != NULL) && (s != 0)) {
		IupSetInt(zoom->zbox, "VALUEPOS", i - 1);
		ezgui_setup_button_check_status(zoom->ezgui);
	}
	return IUP_DEFAULT;
}


static Ihandle *ezgui_setup_media_create(EZGUI *gui)
{
	Ihandle	*hbox1, *hbox2, *vbox;
	int	i;

	hbox1 = xui_list_setting(&gui->dfm_list, "Find Media Duration By");
	for (i = 0; id_duration_long[i].s; i++) {
		IupSetAttributeId(gui->dfm_list, "", i + 1, 
				id_duration_long[i].s);
	}
	IupSetCallback(gui->dfm_list, "ACTION", (Icallback) ezgui_setup_media_event);

	/* find the index of drop down lists: the duration drop down */
	gui->dfm_idx = lookup_index_idnum(id_duration_long, 0, 
			GETDURMOD(gui->sysopt->flags));

	
	hbox2 = xui_list_setting(&gui->mpm_list, "Media Process Method");
	for (i = 0; id_mprocess[i].s; i++) {
		IupSetAttributeId(gui->mpm_list, "", i + 1, id_mprocess[i].s);
	}
	IupSetCallback(gui->mpm_list, "ACTION", (Icallback) ezgui_setup_media_event);

	/* find the media process method */
	gui->mpm_idx = lookup_index_idnum(id_mprocess, 
			0, EZOP_PROC(gui->sysopt->flags));


	vbox = IupVbox(hbox1, hbox2, NULL);
	IupSetAttribute(vbox, "NMARGIN", "16x4");
	IupSetAttribute(vbox, "NGAP", "4");
	return vbox;
}

static int ezgui_setup_media_reset(EZGUI *gui)
{
	IupSetInt(gui->dfm_list, "VALUE", gui->dfm_idx + 1);
	IupSetInt(gui->mpm_list, "VALUE", gui->mpm_idx + 1);
	return 0;
}

static int ezgui_setup_media_update(EZGUI *gui, char *status)
{
	char	tmp[128];

	gui->dfm_idx  = xui_list_get_idx(gui->dfm_list);
	csc_cfg_write(gui->config, EZGUI_MAINKEY,
			CFG_KEY_DURATION, id_duration_long[gui->dfm_idx].s);

	switch (gui->dfm_idx) {
	case 0:
		SETDURMOD(gui->sysopt->flags, EZOP_DUR_AUTO);
		strcpy(tmp, "Auto detect");
		break;
	case 1:
		SETDURMOD(gui->sysopt->flags, EZOP_DUR_HEAD);
		strcpy(tmp, "Detect by Head");
		break;
	case 2:
		SETDURMOD(gui->sysopt->flags, EZOP_DUR_FSCAN);
		strcpy(tmp, "Detect by Full Scan");
		break;
	case 3:
		SETDURMOD(gui->sysopt->flags, EZOP_DUR_QSCAN);
		strcpy(tmp, "Detect by Partial Scan");
		break;
	default:
		strcpy(tmp, "Oops; ");
		break;
	}
	strcat(status, tmp);

	gui->mpm_idx  = xui_list_get_idx(gui->mpm_list);
	EZOP_PROC_MAKE(gui->sysopt->flags, id_mprocess[gui->mpm_idx].id);
	csc_cfg_write(gui->config, NULL, CFG_KEY_MEDIA_PROC,
			id_mprocess[gui->mpm_idx].s);
	csc_cfg_write(gui->config, NULL, CFG_KEY_DURATION, 
			lookup_string_idnum(id_duration_long, -1, 
				GETDURMOD(gui->sysopt->flags)));
	return 0;
}

static int ezgui_setup_media_check(EZGUI *gui)
{
	if (gui->dfm_idx != xui_list_get_idx(gui->dfm_list)) {
		return 1;
	}
	if (gui->mpm_idx != xui_list_get_idx(gui->mpm_list)) {
		return 1;
	}
	return 0;
}

static int ezgui_setup_media_event(Ihandle *ih, char *text, int i, int s)
{
	EZGUI	*gui;

	(void) text; (void) i;

	if (s == 0) {
		return IUP_DEFAULT;	/* ignore the leaving item */
	}
	if ((gui = (EZGUI *) IupGetAttribute(ih, EZOBJ_MAIN)) != NULL) {
		ezgui_setup_button_check_status(gui);
	}
	return IUP_DEFAULT;
}


static Ihandle *ezgui_setup_outputdir_create(EZGUI *gui)
{
	Ihandle	*hbox1, *hbox2, *vbox;
	int	i;

	hbox1 = xui_list_setting(&gui->dir_list, "Save Thumbnails ");
	for (i = 0; uir_outdir[i].s; i++) {
		IupSetAttributeId(gui->dir_list, "", i + 1, uir_outdir[i].s);
	}
	IupSetAttribute(gui->dir_list, EZOBJ_MAIN, (char*) gui);
	IupSetCallback(gui->dir_list, "ACTION",
			(Icallback) ezgui_setup_outputdir_event);

	hbox2 = xui_text(&gui->dir_path, "");
	IupSetAttribute(gui->dir_path, "VISIBLE", "NO");

	vbox = IupVbox(hbox1,  hbox2, NULL);
	IupSetAttribute(vbox, "NMARGIN", "16x4");
	IupSetAttribute(vbox, "NGAP", "4");
	return vbox;
}

static int ezgui_setup_outputdir_reset(EZGUI *gui)
{
	char	*s;

	/* find the thumbnail output method and path */
	gui->dir_idx = 0;
	s = csc_cfg_read(gui->config, EZGUI_MAINKEY, CFG_KEY_OUTPUT_METHOD);
	if (s) {
		gui->dir_idx = lookup_index_string(uir_outdir, 0, s);
	}
	IupSetInt(gui->dir_list, "VALUE", gui->dir_idx + 1);

	/* the default output directory entry */
	if (!strcmp(uir_outdir[gui->dir_idx].s, CFG_PIC_ODIR_CURR)) {
		IupSetAttribute(gui->dir_path, "VISIBLE", "NO");
	} else {	/* CFG_PIC_ODIR_PATH */
		IupSetAttribute(gui->dir_path, "VISIBLE", "YES");
		gui->sysopt->pathout = csc_cfg_read(gui->config, 
					EZGUI_MAINKEY, CFG_KEY_OUTPUT_PATH);
		if (gui->sysopt->pathout) {
			IupSetAttribute(gui->dir_path, "VALUE", 
						gui->sysopt->pathout);
		} else {
			IupSetAttribute(gui->dir_path, "VALUE", "");
		}
	}
	return 0;
}

static int ezgui_setup_outputdir_update(EZGUI *gui)
{
	gui->dir_idx  = xui_list_get_idx(gui->dir_list);
	csc_cfg_write(gui->config, EZGUI_MAINKEY,
			CFG_KEY_OUTPUT_METHOD, uir_outdir[gui->dir_idx].s);
	
	if (!strcmp(uir_outdir[gui->dir_idx].s, CFG_PIC_ODIR_PATH)) {
		gui->sysopt->pathout = IupGetAttribute(gui->dir_path,"VALUE");
		csc_cfg_write(gui->config, EZGUI_MAINKEY, 
				CFG_KEY_OUTPUT_PATH, gui->sysopt->pathout);
	}
	return 0;
}

static int ezgui_setup_outputdir_check(EZGUI *gui)
{
	char	*val;

	if (gui->dir_idx != xui_list_get_idx(gui->dir_list)) {
		return 1;
	}
	if (!strcmp(uir_outdir[gui->dir_idx].s, CFG_PIC_ODIR_PATH)) {
		val = IupGetAttribute(gui->dir_path, "VALUE");
		if (csc_strcmp_param(val, gui->sysopt->pathout)) {
			return 1;
		}
	}
	return 0;
}

static int ezgui_setup_outputdir_event(Ihandle *ih, char *text, int i, int s)
{
	EZGUI	*gui;
	char	*val;

	(void) i;
	if (s == 0) {
		return IUP_DEFAULT;	/* ignore the leaving item */
	}
	if ((gui = (EZGUI *) IupGetAttribute(ih, EZOBJ_MAIN)) == NULL) {
		return IUP_DEFAULT;
	}
	
	if (strcmp(text, CFG_PIC_ODIR_PATH)) {
		IupSetAttribute(gui->dir_path, "VISIBLE", "NO");
		ezgui_setup_button_check_status(gui);
		return IUP_DEFAULT;
	}

	val = IupGetAttribute(gui->dir_path, "VALUE");
	if (val) {
		IupSetAttribute(gui->dlg_odir, "DIRECTORY", val);
	}
	IupPopup(gui->dlg_odir, IUP_CENTERPARENT, IUP_CENTERPARENT);
	if (IupGetInt(gui->dlg_odir, "STATUS") < 0) {
		/* 20151110 can not update list control inside the event 
		 * callback, otherwise the list control will miss calculate 
		 * the change and confuse the window manager */
		gui->dir_ppp_flag = 1;
		return IUP_DEFAULT;	/* cancelled */
	}
	
	val = IupGetAttribute(gui->dlg_odir, "VALUE");
	CDB_DEBUG(("Open File VALUE: %s\n", val));
	CDB_DEBUG(("Last  DIRECTORY: %s\n", 
			IupGetAttribute(gui->dlg_odir, "DIRECTORY")));

	IupSetAttribute(gui->dir_path, "VISIBLE", "YES");
	if (val) {
		IupSetAttribute(gui->dir_path, "VALUE", val);
	}
	ezgui_setup_button_check_status(gui);	
	return IUP_DEFAULT;
}

static Ihandle *ezgui_setup_font_create(EZGUI *gui)
{
	Ihandle	*hbox1, *hbox2, *vbox;
	int	i;

	hbox1 = xui_list_setting(&gui->font_list, "Choose Font");
	for (i = 0; uir_choose_font[i].s; i++) {
		IupSetAttributeId(gui->font_list, "", i + 1, 
				uir_choose_font[i].s);
	}
	IupSetAttribute(gui->font_list, EZOBJ_MAIN, (char*) gui);
	IupSetCallback(gui->font_list, "ACTION",
			(Icallback) ezgui_setup_font_event);

	hbox2 = xui_text(&gui->font_face, "");
	IupSetAttribute(gui->font_face, "READONLY", "YES");

	vbox = IupVbox(hbox1,  hbox2, NULL);
	IupSetAttribute(vbox, "NMARGIN", "16x4");
	IupSetAttribute(vbox, "NGAP", "4");

#ifdef  HAVE_GD_USE_FONTCONFIG
	gdFTUseFontConfig(1);
#endif
	return vbox;
}

static int ezgui_setup_font_reset(EZGUI *gui) 
{
	char	*s;

	/* find the font choosing method */
	gui->font_idx = 0;
	s = csc_cfg_read(gui->config, EZGUI_MAINKEY, CFG_KEY_FONT_METHOD);
	if (s) {
		gui->font_idx = lookup_index_string(uir_choose_font, 0, s);
	}
	IupSetInt(gui->font_list, "VALUE", gui->font_idx + 1);

	/* find the default font face */
	if (!strcmp(uir_choose_font[gui->font_idx].s, CFG_PIC_FONT_SYSTEM)) {
		/* just disable the entry control if system font used */
		IupSetAttribute(gui->font_face, "VISIBLE", "NO");
	} else {	/* CFG_PIC_FONT_BROWSE */
		IupSetAttribute(gui->font_face, "VISIBLE", "YES");
		s = csc_cfg_read(gui->config, EZGUI_MAINKEY, CFG_KEY_FONT_FACE);
		if (s) {
			IupSetAttribute(gui->font_face, "VALUE", s);
		} else {
			IupSetAttribute(gui->font_face, "VALUE", "");
		}
		gui->font_gtk_name = IupGetAttribute(gui->font_face, "VALUE");
	}
	return 0;
}

static int ezgui_setup_font_update(EZGUI *gui)
{
	gui->font_idx = xui_list_get_idx(gui->font_list);
	csc_cfg_write(gui->config, EZGUI_MAINKEY,
			CFG_KEY_FONT_METHOD, uir_choose_font[gui->font_idx].s);

	if (gui->sysopt->mi_font) {
		smm_free(gui->sysopt->mi_font);
	}
	if (!strcmp(uir_choose_font[gui->font_idx].s, CFG_PIC_FONT_SYSTEM)) {
		/* don't update configure if using system font */
		gui->sysopt->mi_font = gui->sysopt->ins_font = NULL;
	} else {	/* CFG_PIC_FONT_BROWSE */
		gui->font_gtk_name = IupGetAttribute(gui->font_face, "VALUE");
		csc_cfg_write(gui->config, EZGUI_MAINKEY,
				CFG_KEY_FONT_FACE, gui->font_gtk_name);

		gui->sysopt->mi_font = gui->sysopt->ins_font = 
			xui_make_font(gui->font_gtk_name, &gui->sysopt->mi_size);
	}
	return 0;
}

static int ezgui_setup_font_check(EZGUI *gui)
{
	char	*val;

	if (gui->font_idx != xui_list_get_idx(gui->font_list)) {
		return 1;
	}
	if (!strcmp(uir_choose_font[gui->font_idx].s, CFG_PIC_FONT_BROWSE)) {
		val = IupGetAttribute(gui->font_face, "VALUE");
		if (csc_strcmp_param(val, gui->font_gtk_name)) {
			return 1;
		}
	}
	return 0;
}

static int ezgui_setup_font_event(Ihandle *ih, char *text, int i, int s)
{
	EZGUI	*gui;
	char	*val;

	(void) i;

	if (s == 0) {
		return IUP_DEFAULT;	/* ignore the leaving item */
	}
	if ((gui = (EZGUI *) IupGetAttribute(ih, EZOBJ_MAIN)) == NULL) {
		return IUP_DEFAULT;
	}

	if (strcmp(text, CFG_PIC_FONT_BROWSE)) {
		IupSetAttribute(gui->font_face, "VISIBLE", "NO");
		ezgui_setup_button_check_status(gui);
		return IUP_DEFAULT;
	}
	
	val = IupGetAttribute(gui->font_face, "VALUE");
	if (val) {
		IupSetAttribute(gui->dlg_font, "VALUE", val);
	}
	IupSetAttribute(gui->dlg_font, "COLOR", "128 0 255");
	IupPopup(gui->dlg_font, IUP_CENTERPARENT, IUP_CENTERPARENT);

	if (IupGetAttribute(gui->dlg_font, "STATUS") == NULL) {
		/* 20151110 can not update list control inside the event 
		 * callback, otherwise the list control will miss calculate 
		 * the change and confuse the window manager */
		gui->font_ppp_flag = 1;
		return IUP_DEFAULT;	/* cancelled */
	}

	val = IupGetAttribute(gui->dlg_font, "VALUE");
	CDB_DEBUG(("Font Face: %s\n", val));
	CDB_DEBUG(("Font Color: %s\n",
				IupGetAttribute(gui->dlg_font, "COLOR")));

	IupSetAttribute(gui->font_face, "VISIBLE", "YES");
	if (val) {
		IupSetAttribute(gui->font_face, "VALUE", val);
	}
	ezgui_setup_button_check_status(gui);
	return IUP_DEFAULT;
}

static Ihandle *ezgui_setup_suffix_create(EZGUI *gui)
{
	Ihandle *hbox;

	hbox = xui_text(&gui->fmt_suffix, "Suffix of Thumbnails");
	IupSetAttribute(gui->fmt_suffix, "VALUE", gui->sysopt->suffix);
	IupSetAttribute(gui->fmt_suffix, EZOBJ_MAIN, (char*) gui);
	IupSetCallback(gui->fmt_suffix, "VALUECHANGED_CB",
			(Icallback) ezgui_setup_suffix_event);
	return hbox;
}

static int ezgui_setup_suffix_reset(EZGUI *gui)
{
	IupSetAttribute(gui->fmt_suffix, "VALUE", gui->sysopt->suffix);
	return 0;
}

static int ezgui_setup_suffix_update(EZGUI *gui)
{
	char	*val;

	val = IupGetAttribute(gui->fmt_suffix, "VALUE");
	if (csc_strcmp_param(val, gui->sysopt->suffix)) {
		csc_strlcpy(gui->sysopt->suffix, val, 64);
	}
	return 0;
}

static int ezgui_setup_suffix_check(EZGUI *gui)
{
	char	*val;

	val = IupGetAttribute(gui->fmt_suffix, "VALUE");
	if (csc_strcmp_param(val, gui->sysopt->suffix)) {
		return 1;
	}
	return 0;
}

static int ezgui_setup_suffix_event(Ihandle *ih)
{
	EZGUI	*gui;

	if ((gui = (EZGUI *) IupGetAttribute(ih, EZOBJ_MAIN)) == NULL) {
		return IUP_DEFAULT;
	}
	ezgui_setup_button_check_status(gui);
	return IUP_DEFAULT;
}

static Ihandle *ezgui_setup_dupname_create(EZGUI *gui)
{
	Ihandle *hbox;
	int	i;

	/* find the process to existed thumbnails */
	gui->exist_idx = lookup_index_idnum(id_existed, 0, 
			EZOP_THUMB_GET(gui->sysopt->flags));

	hbox = xui_list_setting(&gui->fmt_exist, "Existed Thumbnails");
	for (i = 0; id_existed[i].s; i++) {
		IupSetAttributeId(gui->fmt_exist, "", i+1, id_existed[i].s);
	}
	IupSetAttribute(gui->fmt_exist, EZOBJ_MAIN, (char*) gui);
	IupSetCallback(gui->fmt_exist, "ACTION",
			(Icallback) ezgui_setup_dupname_event);
	return hbox;
}

static int ezgui_setup_dupname_reset(EZGUI *gui)
{
	IupSetInt(gui->fmt_exist, "VALUE", gui->exist_idx + 1);
	return 0;
}

static int ezgui_setup_dupname_update(EZGUI *gui)
{
	gui->exist_idx = xui_list_get_idx(gui->fmt_exist);
	EZOP_THUMB_SET(gui->sysopt->flags, id_existed[gui->exist_idx].id);
	csc_cfg_write(gui->config, NULL, CFG_KEY_FILE_EXISTED, 
			id_existed[gui->exist_idx].s);
	return 0;
}

static int ezgui_setup_dupname_check(EZGUI *gui)
{
	if (gui->exist_idx != xui_list_get_idx(gui->fmt_exist)) {
		return 1;
	}
	return 0;
}

static int ezgui_setup_dupname_event(Ihandle *ih, char *text, int i, int s)
{
	EZGUI	*gui;

	(void) i; (void) text;

	if (s == 0) {
		return IUP_DEFAULT;	/* ignore the leaving item */
	}
	if ((gui = (EZGUI *) IupGetAttribute(ih, EZOBJ_MAIN)) == NULL) {
		return IUP_DEFAULT;
	}
	ezgui_setup_button_check_status(gui);
	return IUP_DEFAULT;
}

static Ihandle *ezgui_setup_format_create(EZGUI *gui)
{
	Ihandle	*hbox3, *hbox4, *vbox;
	int	i;

	/* third line: file format of thumbnails */
	hbox3 = xui_list_setting(&gui->fmt_list, "Save Picture As");
	for (i = 0; uir_format[i].s; i++) {
		IupSetAttributeId(gui->fmt_list, "", i+1, 
				uir_format[i].s);
	}
	IupSetAttribute(gui->fmt_list, EZOBJ_MAIN, (char*) gui);
	IupSetCallback(gui->fmt_list, "ACTION",
			(Icallback) ezgui_setup_format_event_picture);
	
	/* append the transparent control for png/gif */
	gui->fmt_transp = IupToggle("Transparent", NULL);
	IupSetAttribute(gui->fmt_transp, EZOBJ_MAIN, (char*) gui);
	IupSetCallback(gui->fmt_transp, "ACTION",
			(Icallback) ezgui_setup_format_event_transparent);
	IupAppend(hbox3, gui->fmt_transp);

	/* optional line: attribute of the file format */
	gui->fmt_zbox = IupZbox(IupFill(), 
			xui_text_setting(&gui->fmt_gif_fr, "FRate:", "(ms)"),
			xui_text_setting(&gui->fmt_jpg_qf, "Quality:", NULL),
			NULL);
	IupSetAttribute(gui->fmt_gif_fr, EZOBJ_MAIN, (char*) gui);
	IupSetCallback(gui->fmt_gif_fr, "VALUECHANGED_CB",
			(Icallback) ezgui_setup_format_event_param);
	IupSetAttribute(gui->fmt_jpg_qf, EZOBJ_MAIN, (char*) gui);
	IupSetCallback(gui->fmt_jpg_qf, "VALUECHANGED_CB",
			(Icallback) ezgui_setup_format_event_param);

	hbox4 = IupHbox(xui_label("", EGPS_SETUP_DESCR, NULL), 
			gui->fmt_zbox, NULL);
	IupSetAttribute(hbox4, "NGAP", "4");

	/* assemble the Picture Format area */
	vbox = IupVbox(hbox3, hbox4, NULL);
	//IupSetAttribute(vbox, "NMARGIN", "16x4");
	IupSetAttribute(vbox, "NGAP", "4");

	/* find the index of drop down lists: the file format drop down */
	if (!strcmp(gui->sysopt->img_format, "png")) {
		gui->fmt_idx = lookup_index_string(uir_format, 0, 
				CFG_PIC_FMT_PNG);
	} else if (strcmp(gui->sysopt->img_format, "gif")) {
		gui->fmt_idx = lookup_index_string(uir_format, 0, 
				CFG_PIC_FMT_JPEG);
	} else if (gui->sysopt->img_quality) {
		gui->fmt_idx = lookup_index_string(uir_format, 0, 
				CFG_PIC_FMT_GIFA);
	} else {
		gui->fmt_idx = lookup_index_string(uir_format, 0, 
				CFG_PIC_FMT_GIF);
	}

	/* seperate the image quality and frame rate */
	gui->tmp_jpg_qf  = 85;
	if (!csc_strcmp_list(gui->sysopt->img_format, "jpg", "jpeg", NULL)) {
		gui->tmp_jpg_qf  = gui->sysopt->img_quality;
	}
	csc_cfg_read_int(gui->config, EZGUI_MAINKEY, 
			CFG_KEY_JPG_QUALITY, &gui->tmp_jpg_qf);

	gui->tmp_gifa_fr = 1000;
	if (!strcmp(gui->sysopt->img_format, "gif") && 
			gui->sysopt->img_quality) {
		gui->tmp_gifa_fr = gui->sysopt->img_quality;
	}
	csc_cfg_read_int(gui->config, EZGUI_MAINKEY, 
			CFG_KEY_GIF_FRATE, &gui->tmp_gifa_fr);
	return vbox;
}

static int ezgui_setup_format_reset(EZGUI *gui)
{
	IupSetInt(gui->fmt_list, "VALUE", gui->fmt_idx + 1);
	
	IupSetInt(gui->fmt_gif_fr, "VALUE", gui->tmp_gifa_fr);
	IupSetInt(gui->fmt_jpg_qf, "VALUE", gui->tmp_jpg_qf);

	if (gui->sysopt->flags & EZOP_TRANSPARENT) {
		IupSetAttribute(gui->fmt_transp, "VALUE", "ON");
	} else {
		IupSetAttribute(gui->fmt_transp, "VALUE", "OFF");
	}

	ezgui_setup_format_event_picture(gui->fmt_list,
			uir_format[gui->fmt_idx].s, gui->fmt_idx + 1, 1);
	return 0;
}

static int ezgui_setup_format_update(EZGUI *gui)
{
	char	*val;

	gui->fmt_idx  = xui_list_get_idx(gui->fmt_list);
	csc_cfg_write(gui->config, EZGUI_MAINKEY,
			CFG_KEY_FILE_FORMAT, uir_format[gui->fmt_idx].s);

	gui->tmp_jpg_qf = (int) strtol(
			IupGetAttribute(gui->fmt_jpg_qf, "VALUE"), NULL, 10);
	csc_cfg_write_int(gui->config, EZGUI_MAINKEY,
			CFG_KEY_JPG_QUALITY, gui->tmp_jpg_qf);
	gui->tmp_gifa_fr = (int) strtol(
			IupGetAttribute(gui->fmt_gif_fr, "VALUE"), NULL, 10);
	csc_cfg_write_int(gui->config, EZGUI_MAINKEY,
			CFG_KEY_GIF_FRATE, gui->tmp_gifa_fr);

	val = IupGetAttribute(gui->fmt_transp, "VALUE");
	if (!strcmp(val, "ON")) {
		gui->sysopt->flags |= EZOP_TRANSPARENT;
	} else {
		gui->sysopt->flags &= ~EZOP_TRANSPARENT;
	}
	csc_cfg_write(gui->config, EZGUI_MAINKEY, CFG_KEY_TRANSPARENCY, val);
	
	CDB_DEBUG(("EVT_SETUP: Fmt=%d JPG=%d GIFA=%d Tra=%s\n",
			gui->fmt_idx, gui->tmp_jpg_qf, gui->tmp_gifa_fr, val));
	return 0;
}

static int ezgui_setup_format_check(EZGUI *gui)
{
	char	*val;

	if (gui->fmt_idx != xui_list_get_idx(gui->fmt_list)) {
		return 1;
	}

	val = IupGetAttribute(gui->fmt_jpg_qf, "VALUE");
	if (gui->tmp_jpg_qf != (int) strtol(val, NULL, 10)) {
		return 1;
	}
	val = IupGetAttribute(gui->fmt_gif_fr, "VALUE");
	if (gui->tmp_gifa_fr != (int) strtol(val, NULL, 10)) {
		return 1;
	}
	val = IupGetAttribute(gui->fmt_transp, "VALUE");
	if (!strcmp(val, "ON")) {
		if ((gui->sysopt->flags & EZOP_TRANSPARENT) == 0) {
			return 1;
		}
	} else {
		if (gui->sysopt->flags & EZOP_TRANSPARENT) {
			return 1;
		}
	}
	return 0;
}

static int ezgui_setup_format_event_picture(Ihandle *ih, char *text, int i, int s)
{
	EZGUI	*gui;

	(void) i;

	if (s == 0) {
		return IUP_DEFAULT;	/* ignore the leaving item */
	}
	if ((gui = (EZGUI *) IupGetAttribute(ih, EZOBJ_MAIN)) == NULL) {
		return IUP_DEFAULT;
	}

	/* hide the transparent toggle and quality editboxes */
	IupSetAttribute(gui->fmt_transp, "VISIBLE", "NO");
	IupSetInt(gui->fmt_zbox, "VALUEPOS", 0);

	if (!strcmp(text, CFG_PIC_FMT_JPEG)) {
		IupSetInt(gui->fmt_zbox, "VALUEPOS", 2);
	} else {
		IupSetAttribute(gui->fmt_transp, "VISIBLE", "YES");
		if (!strcmp(text, CFG_PIC_FMT_GIFA)) {
			IupSetInt(gui->fmt_zbox, "VALUEPOS", 1);
		}
	}
	ezgui_setup_button_check_status(gui);
	return IUP_DEFAULT;
}

static int ezgui_setup_format_event_transparent(Ihandle* ih, int state)
{
	EZGUI	*gui;

	(void) state;
	if ((gui = (EZGUI *) IupGetAttribute(ih, EZOBJ_MAIN)) == NULL) {
		return IUP_DEFAULT;
	}
	ezgui_setup_button_check_status(gui);
	return IUP_DEFAULT;
}

static int ezgui_setup_format_event_param(Ihandle* ih)
{
	EZGUI	*gui;

	if ((gui = (EZGUI *) IupGetAttribute(ih, EZOBJ_MAIN)) == NULL) {
		return IUP_DEFAULT;
	}
	ezgui_setup_button_check_status(gui);
	return IUP_DEFAULT;
}


static Ihandle *ezgui_setup_button_create(EZGUI *gui)
{
	gui->butt_setup_apply = 
		xui_button("OK", (Icallback) ezgui_setup_button_event_ok);
	IupSetAttribute(gui->butt_setup_apply, EZOBJ_MAIN, (char*) gui);
	IupSetAttribute(gui->butt_setup_apply, "ACTIVE", "NO");
	IupSetAttribute(gui->butt_setup_apply, "IMAGE", "IUP_ActionOk");

	gui->butt_setup_cancel = 
		xui_button("Cancel", (Icallback) ezgui_setup_button_event_cancel);
	IupSetAttribute(gui->butt_setup_cancel, EZOBJ_MAIN, (char*) gui);
	IupSetAttribute(gui->butt_setup_cancel, "ACTIVE", "NO");
	IupSetAttribute(gui->butt_setup_cancel, "IMAGE", "IUP_ActionCancel");

	return IupHbox(xui_label("", "320", NULL),  
			gui->butt_setup_cancel, gui->butt_setup_apply, NULL);
}

static int ezgui_setup_button_check_status(EZGUI *gui)
{
	int	counter = 0;

	counter += ezgui_setup_grid_check(gui->prof_grid);
	counter += ezgui_setup_zoom_check(gui->prof_zoom);
	counter += ezgui_setup_media_check(gui);
	counter += ezgui_setup_outputdir_check(gui);
	counter += ezgui_setup_font_check(gui);
	counter += ezgui_setup_suffix_check(gui);
	counter += ezgui_setup_dupname_check(gui);
	counter += ezgui_setup_format_check(gui);
	if (counter == 0) {
		IupSetAttribute(gui->butt_setup_apply, "ACTIVE", "NO");
		IupSetAttribute(gui->butt_setup_cancel, "ACTIVE", "NO");
	} else {
		IupSetAttribute(gui->butt_setup_apply, "ACTIVE", "YES");
		IupSetAttribute(gui->butt_setup_cancel, "ACTIVE", "YES");
	}
	return counter;
}

static int ezgui_setup_button_event_ok(Ihandle *ih)
{
	EZGUI	*gui;

	if ((gui = (EZGUI *) IupGetAttribute(ih, EZOBJ_MAIN)) == NULL) {
		return IUP_DEFAULT;
	}

	gui->status[0] = 0;
	ezgui_setup_grid_update(gui->prof_grid, gui->status);
	ezgui_setup_zoom_update(gui->prof_zoom, gui->status);
	ezgui_setup_media_update(gui, gui->status);
	ezgui_setup_font_update(gui);
	ezgui_setup_outputdir_update(gui);
	ezgui_setup_suffix_update(gui);
	ezgui_setup_dupname_update(gui);
	ezgui_setup_format_update(gui);

	IupSetAttribute(gui->stat_bar, "TITLE", gui->status);
	//puts(gui->status);

	ezgui_setup_button_check_status(gui);
	return IUP_DEFAULT;
}

static int ezgui_setup_button_event_cancel(Ihandle *ih)
{
	EZGUI	*gui;

	if ((gui = (EZGUI *) IupGetAttribute(ih, EZOBJ_MAIN)) != NULL) {
		ezgui_page_setup_reset(gui);
		ezgui_setup_button_check_status(gui);
	}
	return IUP_DEFAULT;
}


/****************************************************************************
 * Page About
 ****************************************************************************/
static Ihandle *ezgui_page_about(EZGUI *gui)
{
	Ihandle	*icon, *name, *descr, *thanks;
	Ihandle	*vbox, *sbox;

	(void) gui;

	/* show the icon */
	icon = IupLabel(NULL);
	IupSetAttribute(icon, "IMAGE", "DLG_ICON");

	/* show name and the version */
	name = IupLabel("Ezthumb " EZTHUMB_VERSION);
	IupSetAttribute(name, "FONTSIZE", "20");
	IupSetAttribute(name, "FONTSTYLE", "Bold");
	
	/* show the simple description */
	descr = IupLabel(description);
	IupSetAttribute(descr, "ALIGNMENT", "ACENTER:ACENTER");

	/* show the credits */
	thanks = IupLabel(credits);
	IupSetAttribute(thanks, "ALIGNMENT", "ACENTER:ACENTER");
	
	/* group these elements inside a vertical box */
	vbox = IupVbox(icon, name, descr, thanks, NULL);
	IupSetAttribute(vbox, "NGAP", "8");
	IupSetAttribute(vbox, "NMARGIN", "16x16");
	IupSetAttribute(vbox, "ALIGNMENT", "ACENTER");

	/* fill the right side of the veritcal box with blank and pack
	 * into a scrollbox */
	sbox = IupScrollBox(IupHbox(vbox, IupHbox(IupFill(), NULL), NULL));
	IupSetAttribute(sbox, "SCROLLBAR", "VERTICAL");
	return sbox;
}


/****************************************************************************
 * New control for workarea
 ****************************************************************************/

static Ihandle *ezgui_sview_create(EZGUI *gui, int dblck)
{
	Ihandle	*vb_main, *vb_size, *vb_len, *vb_res, *vb_prog, *hbox;
	SView	*sview;

	if ((sview = smm_alloc(sizeof(SView))) == NULL) {
		return NULL;
	}
	sview->gui = gui;

	sview->filename = IupList(NULL);
	IupSetAttribute(sview->filename, "EXPAND", "YES");
	IupSetAttribute(sview->filename, "MULTIPLE", "YES");
	IupSetAttribute(sview->filename, "SCROLLBAR", "NO");
	IupSetAttribute(sview->filename, "DROPFILESTARGET", "YES");
	IupSetAttribute(sview->filename, "ALIGNMENT", "ARIGHT");
	IupSetAttribute(sview->filename, EZOBJ_SVIEW, (char*) sview);
	vb_main = IupVbox(xui_label("Files", NULL, NULL), 
			sview->filename, NULL);

	IupSetCallback(sview->filename, "DROPFILES_CB",
			(Icallback) ezgui_sview_event_dropfiles);
	IupSetCallback(sview->filename, "MULTISELECT_CB",
			(Icallback) ezgui_sview_event_multi_select);
	IupSetCallback(sview->filename, "BUTTON_CB",
			(Icallback) ezgui_sview_event_moused);
	IupSetCallback(sview->filename, "MOTION_CB",
			(Icallback) ezgui_sview_event_motion);
	if (dblck) {
		IupSetCallback(sview->filename, "DBLCLICK_CB", 
				(Icallback) ezgui_sview_event_run);
	}

	sview->filesize = IupList(NULL);
	IupSetAttribute(sview->filesize, "SIZE", "50");
	IupSetAttribute(sview->filesize, "EXPAND", "VERTICAL");
	IupSetAttribute(sview->filesize, "SCROLLBAR", "NO");
	IupSetAttribute(sview->filesize, "ACTIVE", "NO");
	vb_size = IupVbox(xui_label("Size", "50", NULL), 
			sview->filesize, NULL);

	sview->medialen = IupList(NULL);
	IupSetAttribute(sview->medialen, "SIZE", "48");
	IupSetAttribute(sview->medialen, "EXPAND", "VERTICAL");
	IupSetAttribute(sview->medialen, "SCROLLBAR", "NO");
	IupSetAttribute(sview->medialen, "ACTIVE", "NO");
	vb_len = IupVbox(xui_label("Length", "48", NULL), 
			sview->medialen, NULL);

	sview->resolution = IupList(NULL);
	IupSetAttribute(sview->resolution, "SIZE", "50");
	IupSetAttribute(sview->resolution, "EXPAND", "VERTICAL");
	IupSetAttribute(sview->resolution, "SCROLLBAR", "NO");
	IupSetAttribute(sview->resolution, "ACTIVE", "NO");
	vb_res = IupVbox(xui_label("Resolution", "50", NULL), 
			sview->resolution, NULL);
	
	sview->progress = IupList(NULL);
	IupSetAttribute(sview->progress, "SIZE", "40");
	IupSetAttribute(sview->progress, "SCROLLBAR", "NO");
	IupSetAttribute(sview->progress, "EXPAND", "VERTICAL");
	IupSetAttribute(sview->progress, "ACTIVE", "NO");
	vb_prog = IupVbox(xui_label("Progress", "40", NULL), 
			sview->progress, NULL);

	sview->attrib = IupList(NULL);
	IupSetAttribute(sview->attrib, "SIZE", "50");
	IupSetAttribute(sview->attrib, "EXPAND", "VERTICAL");
	IupSetAttribute(sview->attrib, "SCROLLBAR", "NO");
	IupSetAttribute(sview->attrib, "ACTIVE", "NO");
	IupVbox(xui_label("Attribution", "50", NULL), 
			sview->attrib, NULL);

	hbox = IupHbox(vb_main, vb_size, vb_len, vb_res, vb_prog, NULL);
	IupSetAttribute(hbox, EZOBJ_SVIEW, (char*) sview);
	return hbox;
}

static int ezgui_sview_progress(Ihandle *ih, int percent)
{
	SView	*sview;
	char	*tmp;

	if ((sview = (SView *) IupGetAttribute(ih, EZOBJ_SVIEW)) == NULL) {
		return EZ_ERR_PARAM;
	}
	tmp = IupGetAttributeId(sview->progress, "", sview->svidx);
	if (tmp) {
		sprintf(tmp, "%d%%",  percent);
		IupSetAttributeId(sview->progress, "", sview->svidx, tmp);
	}
	return EZ_ERR_NONE;
}

static int ezgui_sview_active_add(Ihandle *ih, int type, Ihandle *ctrl)
{
	SView	*sview;
	Ihandle	**clist;	/* control list */
	int	i;

	if ((sview = (SView*)IupGetAttribute(ih, EZOBJ_SVIEW)) == NULL) {
		return EZ_ERR_PARAM;
	}

	switch (type) {
	case EZGUI_SVIEW_ACTIVE_CONTENT:
		clist = sview->act_content;
		break;
	case EZGUI_SVIEW_ACTIVE_SELECT:
		clist = sview->act_select;
		break;
	case EZGUI_SVIEW_ACTIVE_PROGRESS:
		clist = sview->act_progress;
		break;
	case EZGUI_SVIEW_ACTIVE_BIND:
		IupSetAttribute(ctrl, EZOBJ_SVIEW, (char*) sview);
		return EZ_ERR_NONE; 
	default:
		return EZ_ERR_PARAM;	/* wrong type */
	}

	for (i = 0; i < EZGUI_SVIEW_ACTIVE_MAX; i++) {
		if (clist[i]) {
			continue;
		}
		clist[i] = ctrl;
		IupSetAttribute(clist[i], EZOBJ_SVIEW, (char*) sview);
		return EZ_ERR_NONE;	/* successful */
	}
	return EZ_ERR_LOWMEM;	/* full */ 
}

static int ezgui_sview_resize(Ihandle *ih, int width, int height)
{
	SView	*sview;
	char	tmp[32];

	(void) height;

	if ((sview = (SView *) IupGetAttribute(ih, EZOBJ_SVIEW)) == NULL) {
		return EZ_ERR_PARAM;
	}

	if (sview->car_size == 0) {
		sview->car_size  = xui_get_size(sview->filesize, "RASTERSIZE", NULL);
		sview->car_size += xui_get_size(sview->medialen, "RASTERSIZE", NULL);
		sview->car_size += xui_get_size(sview->resolution, "RASTERSIZE", NULL);
		sview->car_size += xui_get_size(sview->progress, "RASTERSIZE", NULL);
		sview->car_size += 4;
		//printf("ezgui_sview_resize: %d %d\n", width, sview->car_size);
	}
	sprintf(tmp, "%d", width - sview->car_size);
	IupSetAttribute(sview->filename, "RASTERSIZE", tmp);
	return 0;
}

#if 0
static int ezgui_sview_active_remove(Ihandle *ih, int type, Ihandle *ctrl)
{
	SView	*sview;
	Ihandle	**clist;	/* control list */
	int	i;

	if ((sview = (SView*)IupGetAttribute(ih, EZOBJ_SVIEW)) == NULL) {
		return EZ_ERR_PARAM;
	}

	switch (type) {
	case EZGUI_SVIEW_ACTIVE_CONTENT:
		clist = sview->act_content;
		break;
	case EZGUI_SVIEW_ACTIVE_SELECT:
		clist = sview->act_select;
		break;
	case EZGUI_SVIEW_ACTIVE_PROGRESS:
		clist = sview->act_progress;
		break;
	default:
		return EZ_ERR_PARAM;	/* wrong type */
	}

	for (i = 0; i < EZGUI_SVIEW_ACTIVE_MAX && clist[i]; i++) {
		if (ctrl && (ctrl != clist[i])) {
			continue;
		}
		IupSetAttribute(clist[i], EZOBJ_SVIEW, NULL);
		clist[i] = NULL;
	}
	return EZ_ERR_NONE;	/* successful */
}
#endif

static int ezgui_sview_event_run(Ihandle *ih, int item, char *text)
{
	EZGUI	*gui;
	SView	*sview;
	char	*attr, *fname;

	if ((sview = (SView *) IupGetAttribute(ih, EZOBJ_SVIEW)) == NULL) {
		return IUP_DEFAULT;
	}

	sview->svidx = item;	/* store the current index */
	gui = sview->gui;

	if ((attr = IupGetAttributeId(sview->attrib, "", item)) == NULL) {
		return IUP_DEFAULT;
	}

	CDB_DEBUG(("EVT_Action %d: %s %s\n", item, attr, text));

	gui->sysopt->pre_seek = (int) strtol(attr, NULL, 0);
	attr = strchr(attr, ':');
	gui->sysopt->pre_br = (int) strtol(++attr, NULL, 0);
	attr = strchr(attr, ':');
	gui->sysopt->pre_dura = (EZTIME) strtoll(++attr, NULL, 0);

	/* 20160115 content in 'text' is not stable */
	smm_codepage_set(65001);
	fname = csc_strcpy_alloc(text, 0);
	ezthumb(fname, gui->sysopt);
	smm_free(fname);
	smm_codepage_reset();

	gui->sysopt->pre_seek = 0;
	gui->sysopt->pre_br   = 0;
	gui->sysopt->pre_dura = 0;
	return IUP_DEFAULT;
}

static int ezgui_sview_event_dropfiles(Ihandle *ih, 
		const char* filename, int num, int x, int y)
{
	SView	*sview;

	(void)num; (void)x; (void)y;

	if ((sview = (SView *) IupGetAttribute(ih, EZOBJ_SVIEW)) == NULL) {
		return IUP_DEFAULT;
	}

	CDB_DEBUG(("EVT_Dropfiles: fname=%s number=%d %dx%d\n",
				filename, num, x, y));
	ezgui_sview_file_append(sview, (char*) filename);
	/* highlight the RUN button when the list is not empty */
	ezgui_sview_active_update(sview, 
			EZGUI_SVIEW_ACTIVE_CONTENT, sview->svnum);
	return IUP_DEFAULT;
}

static int ezgui_sview_event_multi_select(Ihandle *ih, char *value)
{
	SView	*sview;
	int	i, n;

	if ((sview = (SView *) IupGetAttribute(ih, EZOBJ_SVIEW)) == NULL) {
		return IUP_DEFAULT;
	}

	CDB_DEBUG(("EVT_Multi_select: %s\n", value));
	CDB_DEBUG(("List value=%s\n", 
				IupGetAttribute(sview->filename, "VALUE")));
	
	/* the parameter 'value' is useless. grab list myself */
	/*value = IupGetAttribute(sview->list_fname, "VALUE");
	for (i = 0; value[i]; i++) {
		if (value[i] == '+') {
			IupSetAttribute(gui->button_del, "ACTIVE", "YES");
			return IUP_DEFAULT;
		}
	}
	IupSetAttribute(gui->button_del, "ACTIVE", "NO");*/
	value = IupGetAttribute(sview->filename, "VALUE");
	for (i = n = 0; value[i]; i++) {
		if (value[i] == '+') {
			n++;
		}
	}
	ezgui_sview_active_update(sview, EZGUI_SVIEW_ACTIVE_SELECT, n);
	return IUP_DEFAULT;
}

static int ezgui_sview_event_moused(Ihandle *ih, 
		int button, int pressed, int x, int y, char *status)
{
	SView	*sview;

	(void)x; (void)y; (void)status;	/* stop compiler complains */

	if ((sview = (SView *) IupGetAttribute(ih, EZOBJ_SVIEW)) == NULL) {
		return IUP_DEFAULT;
	}

	CDB_MODL(("EVT_Mouse: %d %d %dx%d %s\n", 
				button, pressed, x, y, status));
	if (pressed) {	/* only act when button is released */
		return IUP_DEFAULT;
	}
	/* deselect every thing if the right button was released */
	if (button == IUP_BUTTON3) {
		IupSetAttribute(sview->filename, "VALUE", "");
		ezgui_sview_active_update(sview, 
				EZGUI_SVIEW_ACTIVE_SELECT, 0);
	}
	return IUP_DEFAULT;
}

static int ezgui_sview_event_motion(Ihandle *ih, int x, int y, char *status)
{
	SView	*sview;
	char	*fname;
	int	line;

	(void)status;	/* stop compiler complains */

	line  = IupConvertXYToPos(ih, x, y);

	CDB_MODL(("EVT_Motion: %d %d %d\n", x, y, line));

	if ((sview = (SView *) IupGetAttribute(ih, EZOBJ_SVIEW)) == NULL) {
		return IUP_DEFAULT;
	}
	
	if (line != sview->moused) {
		sview->moused = line;
		if (line < 1) {
			IupSetAttribute(ih, "TIP", NULL);
		} else {
			fname = IupGetAttributeId(ih, "",  line);
			if (fname) {
				IupSetAttribute(ih, "TIP", fname);
			}
		}
	}
	return IUP_DEFAULT;
}

static int ezgui_sview_add(SView *sview)
{
	EZGUI	*gui = (EZGUI *) sview->gui;
	char	*flist, *fname, *path, *sdir;
	int	i, amnt;
	
	/* Store the recent visited diretory so it can be used next time.
	 * The file open dialog can not go to the last directory in gtk */
	path = csc_cfg_read(gui->config, EZGUI_MAINKEY, CFG_KEY_DIRECTORY);
	if (path) {
		IupSetAttribute(gui->dlg_open, "DIRECTORY", path);
	}
	IupPopup(gui->dlg_open, IUP_CENTERPARENT, IUP_CENTERPARENT);

	if (IupGetInt(gui->dlg_open, "STATUS") < 0) {
		return IUP_DEFAULT;	/* cancelled */
	}

	/* IUP generate different file list between one file and more files:
	 * Open File VALUE: /home/xum1/dwhelper/lan_ke_er.flv
	 * Last  DIRECTORY: /home/xum1/dwhelper/
	 * Open File VALUE: /home/xum1/dwhelper|file-602303262.flv|
	 * 			lan_ke_er.flv|Powered_by_Discuz.flv|
	 * Last  DIRECTORY: /home/xum1/dwhelper0 */
	CDB_DEBUG(("Open File VALUE: %s\n", 
			IupGetAttribute(gui->dlg_open, "VALUE")));
	CDB_DEBUG(("Last  DIRECTORY: %s\n", 
			IupGetAttribute(gui->dlg_open, "DIRECTORY")));
	/* duplicate the path and filename list */
	flist = csc_strcpy_alloc(IupGetAttribute(gui->dlg_open, "VALUE"), 16);
	/* find out how many files is in */
	for (i = amnt = 0; flist[i]; i++) {
		amnt += (flist[i] == '|') ? 1 : 0;
	}
	if (amnt == 0) {
		amnt++;
	} else {
		amnt--;
	}

	/* store the current directory. note that the tailing '/' or '0' 
	 * need to be cut out first.
	 * Is the tailing '0' a bug of IUP? */
	path = csc_strcpy_alloc(
			IupGetAttribute(gui->dlg_open, "DIRECTORY"), 4);
	i = strlen(path) - 1;
	if ((path[i] == '/') || (path[i] == '\\') || (path[i] == '0')) {
		path[i] = 0;
	}
	csc_cfg_write(gui->config, EZGUI_MAINKEY, CFG_KEY_DIRECTORY, path);
	smm_free(path);

	/* process the single file list */
	if (amnt == 1) {
		ezgui_sview_file_append(sview, flist);
		/* highlight the RUN button when the list is not empty */
		ezgui_sview_active_update(sview, 
				EZGUI_SVIEW_ACTIVE_CONTENT, sview->svnum);
		smm_free(flist);
		return IUP_DEFAULT;
	}

	/* cut out the path first */
	flist = csc_cuttoken(flist, &sdir, "|");
	/* extract the file names */
	while ((flist = csc_cuttoken(flist, &fname, "|")) != NULL) {
		if (fname[0] == 0) {
			break;	/* end of list */
		}
		path = csc_strcpy_alloc(sdir, strlen(fname)+8);
		strcat(path, SMM_DEF_DELIM);
		strcat(path, fname);
		//printf("%s\n", path);
		ezgui_sview_file_append(sview, path);
		/* highlight the RUN button when the list is not empty */
		ezgui_sview_active_update(sview, 
				EZGUI_SVIEW_ACTIVE_CONTENT, sview->svnum);
		smm_free(path);
	}
	smm_free(flist);
	return IUP_DEFAULT;
}

static int ezgui_sview_remove(SView *sview)
{
	char	*value;
	int	i;

	while (1) {
		value = IupGetAttribute(sview->filename, "VALUE");
		CDB_PROG(("EVT_Remove: %s\n", value));
		for (i = 0; value[i]; i++) {
			if (value[i] == '+') {
				ezgui_sview_file_remove(sview, i+1);
				break;
			}
		}
		if (!value[i]) {
			break;
		}
	}
	ezgui_sview_active_update(sview, 
			EZGUI_SVIEW_ACTIVE_SELECT, 0);
	ezgui_sview_active_update(sview, 
			EZGUI_SVIEW_ACTIVE_CONTENT, sview->svnum);
	return IUP_DEFAULT;
}

static int ezgui_sview_run(SView *sview)
{
	char	*value, *fname;
	int	i, n;

	value = IupGetAttribute(sview->filename, "VALUE");
	for (i = n = 0; value[i]; i++) {
		if (value[i] == '+') {
			fname = IupGetAttributeId(sview->filename, "",  i+1);
			ezgui_sview_event_run(sview->filename, i+1, fname);
			n++;
		}
	}
	if (n == 0) {
		for (i = 0; i < sview->svnum; i++) {
			fname = IupGetAttributeId(sview->filename, "",  i+1);
			ezgui_sview_event_run(sview->filename, i+1, fname);
		}
	}
	return IUP_DEFAULT;
}


static int ezgui_sview_file_append(SView *sview, char *fname)
{
	EZVID	vobj;
	char	buf[64];
	int	lnext;

	/* preset the filename to make it look better */
	lnext = sview->svnum + 1;
	IupSetStrAttributeId(sview->filename, "",  lnext, fname);

	/* 20120903 Bugfix: set the codepage to utf-8 before calling
	 * ezthumb core. In Win32 version, the ezthumb core uses the 
	 * default codepage to process file name. However the GTK 
	 * converted the file name to UTF-8 so the Windows version 
	 * could not find the file. 
	 * There's no such problem in linux.*/
	smm_codepage_set(65001);
	if (ezinfo(fname, sview->gui->sysopt, &vobj) != EZ_ERR_NONE) {
		smm_codepage_reset();
		/* FIXME: disaster control */
		IupSetInt(sview->filename, "REMOVEITEM", lnext);
		return EZ_ERR_FORMAT;
	}
	smm_codepage_reset();

	meta_filesize(vobj.filesize, buf);
	IupSetStrAttributeId(sview->filesize,   "", lnext, buf);
	meta_timestamp(vobj.duration, 0, buf);
	IupSetStrAttributeId(sview->medialen,   "", lnext, buf);
	sprintf(buf, "%dx%d", vobj.width, vobj.height);
	IupSetStrAttributeId(sview->resolution, "", lnext, buf);
	SMM_SPRINT(buf, "%d:%d:%lld", vobj.seekable, vobj.bitrates, 
			(long long) vobj.duration);
	IupSetStrAttributeId(sview->attrib, "", lnext, buf);
	/* hope the internal allocated memory is reusable */
	IupSetStrAttributeId(sview->progress,   "", lnext, "0%     ");

	/* increase the list index first because the IUP list control
	 * starts from 1 */
	sview->svnum++;

	IupFlush();
	return sview->svnum;
}

static int ezgui_sview_file_remove(SView *sview, int idx)
{
	CDB_INFO(("Removing: %s\n", 
			IupGetAttributeId(sview->filename, "",  idx)));

	IupSetInt(sview->filename, "REMOVEITEM", idx);
	IupSetInt(sview->filesize, "REMOVEITEM", idx);
	IupSetInt(sview->medialen, "REMOVEITEM", idx);
	IupSetInt(sview->resolution, "REMOVEITEM", idx);
	IupSetInt(sview->progress, "REMOVEITEM", idx);
	IupSetInt(sview->attrib, "REMOVEITEM", idx);
	sview->svnum--;
	return sview->svnum;
}

static int ezgui_sview_active_update(SView *sview, int type, int num)
{
	Ihandle	**clist;	/* control list */
	int	i;

	switch (type) {
	case EZGUI_SVIEW_ACTIVE_CONTENT:
		clist = sview->act_content;
		break;
	case EZGUI_SVIEW_ACTIVE_SELECT:
		clist = sview->act_select;
		break;
	case EZGUI_SVIEW_ACTIVE_PROGRESS:
		clist = sview->act_progress;
		break;
	default:
		return EZ_ERR_PARAM;
	}

	for (i = 0; i < EZGUI_SVIEW_ACTIVE_MAX; i++) {
		if (clist[i] == NULL) {
			continue;
		}
		if (num) {
			IupSetAttribute(clist[i], "ACTIVE", "YES");
		} else {
			IupSetAttribute(clist[i], "ACTIVE", "NO");
		}
	}
	return EZ_ERR_NONE;
}


/****************************************************************************
 * Support Functions 
 ****************************************************************************/
static Ihandle *xui_label(char *label, char *size, char *font)
{
	Ihandle	*ih;

	ih = IupLabel(label);
	if (size) {
		IupSetAttribute(ih, "SIZE", size);
	}
	if (font) {
		IupSetAttribute(ih, "FONTSTYLE", font);
	}
	return ih;
}

static Ihandle *xui_text(Ihandle **xlst, char *label)
{
	Ihandle	*text, *hbox;

	text = IupText(NULL);
	IupSetAttribute(text, "SIZE", EGPS_SETUP_DROPDOWN);

	hbox = IupHbox(xui_label(label, EGPS_SETUP_DESCR, NULL), text, NULL);
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
	IupSetAttribute(hbox, "NGAP", "4");

	if (xlst) {
		*xlst = text;
	}
	return hbox;
}

/* define the standart dropdown list for the setup page */
static Ihandle *xui_list_setting(Ihandle **xlst, char *label)
{
	Ihandle	*list, *hbox;

	list = IupList(NULL);
	IupSetAttribute(list, "SIZE", EGPS_SETUP_DROPDOWN);
	IupSetAttribute(list, "DROPDOWN", "YES");

	hbox = IupHbox(xui_label(label, EGPS_SETUP_DESCR, NULL), list, NULL);
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
	IupSetAttribute(hbox, "NGAP", "4");

	if (xlst) {
		*xlst = list;
	}
	return hbox;
}

static int xui_list_get_idx(Ihandle *ih)
{
	int	val;

	val = (int) strtol(IupGetAttribute(ih, "VALUE"), NULL, 10);
	return val - 1;
}

static int xui_text_get_number(Ihandle *ih)
{
	return (int) strtol(IupGetAttribute(ih, "VALUE"), NULL, 0);
}

static int xui_get_size(Ihandle *ih, char *attr, int *height)
{
	char	*ssize;
	int	width;

	if ((ssize = IupGetAttribute(ih, attr)) == NULL) {
		return -1;
	}
	//printf("xui_get_size: %s = %s\n", attr, ssize);

	width = (int) strtol(ssize, NULL, 10);

	if ((ssize = strchr(ssize, 'x')) != NULL) {
		if (height) {
			*height = (int) strtol(++ssize, NULL, 10);
		}
	}
	return width;
}

static Ihandle *xui_text_setting(Ihandle **xtxt, char *label, char *ext)
{
	Ihandle	*hbox, *text;

	text = IupText(NULL);
	IupSetAttribute(text, "SIZE", EGPS_SETUP_SHORT_TEXT);

	hbox = IupHbox(IupLabel(label), text,
			ext ? IupLabel(ext) : NULL, NULL);
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
	IupSetAttribute(hbox, "NGAP", "4");

	if (xtxt) {
		*xtxt = text;
	}
	return hbox;
}

static Ihandle *xui_text_grid(char *label, 
		Ihandle **xcol, Ihandle **xrow, char *ext)
{
	Ihandle	*hbox, *text1, *text2, *title;

	if ((xcol == NULL) && (xrow == NULL)) {
		return NULL;
	}

	title = IupZbox(NULL);
	IupAppend(title, xui_label(label, NULL, NULL));
	IupAppend(title, IupLabel("Column"));	/* dummy for fixing widths */

	/* Just one text control */
	if ((xcol == NULL) || (xrow == NULL)) {
		text1 = IupText(NULL);
		IupSetAttribute(text1, "SIZE", EGPS_GRID_FST_TEXT);
		if (xcol) {
			*xcol = text1;
		} else {
			*xrow = text1;
		}
		hbox = IupHbox(title, text1, ext ? IupLabel(ext) : NULL,NULL);
		IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
		//IupSetAttribute(hbox, "NGAP", "4");
		return hbox;
	}

	text1 = IupText(NULL);
	*xcol = text1;
	IupSetAttribute(text1, "SIZE", EGPS_GRID_SND_TEXT);
	text2 = IupText(NULL);
	*xrow = text2;
	IupSetAttribute(text2, "SIZE", EGPS_GRID_SND_TEXT);
	hbox = IupHbox(title, text1, IupLabel("x"), text2, 
			ext ? IupLabel(ext) : NULL, NULL);
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
	//IupSetAttribute(hbox, "NGAP", "4");
	return hbox;
}


static Ihandle *xui_button(char *prompt, Icallback ntf)
{
	Ihandle	*button;

	button = IupButton(prompt, NULL);
	IupSetAttribute(button, "SIZE", EGPS_SETUP_BUTTON);
	if (ntf) {
		IupSetCallback(button, "ACTION", ntf);
	}
	return button;
}

static int xui_config_status(void *config, char *prompt)
{
	char	*path;
	int	item;

	path = csc_cfg_status(config, &item);
	CDB_SHOW(("%s: %d items in %s\n", prompt, item, path));
	return 0;
}

/* expand the short form extension list to the full length */
static char *xui_make_filters(char *slist)
{
	char	*flt, token[32];
	int	n;

	/* Assuming the worst case of the short list is like "a;b;c;d",
	 * it will then be expanded to "*.a;*.b;*.c;*.d". The biggest length 
	 * is N + (N/2+1) * 2 */
	if ((flt = smm_alloc(strlen(slist) * 2 + 64)) == NULL) {
		return NULL;
	}

	strcpy(flt, "Video Files|");
	n = strlen(flt);
	while ((slist = csc_gettoken(slist, token, ",;:")) != NULL) {
		n += sprintf(flt + n, "*.%s;", token);
	}
	flt[strlen(flt)-1] = 0;		/* cut out the tailing ';' */
	strcat(flt, "|All Files|*.*|");
	return flt;
}


/* translate the font face style to the GD accepted one:
 *   Nimbus Sans Bold Italic 10  --> Nimbus Sans:Bold:Italic
 *   Nimbus Sans Italic 10  --> Nimbus Sans:Italic
 *   Nimbus Sans 10  --> Nimbus Sans:Regular  */
static char *xui_make_font(char *face, int *size)
{
	char	*p, *s = csc_strcpy_alloc(face, 16);

	/* isolate the number of font size */
	if (((p = strrchr(s, ' ')) != NULL) && isdigit(p[1])) {
		*p++ = 0;
		if (size) {
			*size = (int) strtol(p, NULL, 0);
		}
	}
	
	/* Sans Bold Italic 10 */
	if ((p = strstr(s, "Bold")) != NULL) {
		*--p = ':';
	}
	if ((p = strstr(s, "Italic")) != NULL) {
		*--p = ':';
	}
	if (strchr(s, ':') == NULL) {
		if ((p = strstr(s, "Regular")) != NULL) {
			*--p = ':';
		} else {
			strcat(s, ":Regular");
		}
	}

	//printf("xui_make_font: %s\n", s);
	return s;
}


