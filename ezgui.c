
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
#ifdef  HAVE_CONFIG_H
#include <config.h>
#else
#error "Run configure first"
#endif

#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif
#ifdef HAVE_STRING_H
# if !defined STDC_HEADERS && defined HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <stdarg.h>
#include <ctype.h>

#include "iup.h"

#include "libcsoup.h"
#include "ezthumb.h"
#include "ezgui.h"
#include "ezicon.h"
#include "id_lookup.h"

/* re-use the debug convention in libcsoup */
//#define CSOUP_DEBUG_LOCAL	SLOG_CWORD(EZTHUMB_MOD_GUI, SLOG_LVL_WARNING)
#define CSOUP_DEBUG_LOCAL	SLOG_CWORD(EZTHUMB_MOD_GUI, SLOG_LVL_MODULE)
//#define CSOUP_DEBUG_LOCAL	SLOG_CWORD(EZTHUMB_MOD_GUI, SLOG_LVL_DEBUG)
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
static int ezgui_setup_grid_update(Ihandle *gridbox, char *status);
static int ezgui_setup_grid_check(Ihandle *gridbox);
static int ezgui_setup_grid_event(Ihandle *ih, char *text, int i, int s);
static int ezgui_setup_grid_entry_event(Ihandle *ih);

static Ihandle *ezgui_setup_zoom_create(EZGUI *gui);
static Ihandle *ezgui_setup_zoom_groupbox(Ihandle *zoombox);
static int ezgui_setup_zoom_reset(Ihandle *zoombox);
static int ezgui_setup_zoom_update(Ihandle *zoombox, char *status);
static int ezgui_setup_zoom_check(Ihandle *zoombox);
static int ezgui_setup_zoom_event(Ihandle *ih, char *text, int i, int s);
static int ezgui_setup_zoom_entry_event(Ihandle *ih);

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
static int ezgui_setup_font_update_event(Ihandle* ih, int button, 
		int pressed, int x, int y, char* status);
static int ezgui_setup_font_chooser(EZGUI *gui);

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

static int ezbar_event(void *vobj, int event, long param, long opt, void *block);
static void *ezbar_main(void *vobj);
static int ezbar_cb_cancel(Ihandle *ih);

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
#ifdef	CFG_WIN32RT
static int ezgui_sview_event_ctrl_a(Ihandle *ih, int c);
#endif
static int ezgui_sview_add(SView *sview);
static int ezgui_sview_remove(SView *sview);
static int ezgui_sview_run(SView *sview);
static int ezgui_sview_file_append(SView *sview, char *fname);
static int ezgui_sview_file_remove(SView *sview, int idx);
static int ezgui_sview_active_update(SView *sview, int type, int num);

static int xui_copy_attribute(char **dst, Ihandle *ih, char *name);
static Ihandle *xui_label(char *label, char *size, char *font);
static Ihandle *xui_text(Ihandle **xlst, char *label);
static Ihandle *xui_list_setting(Ihandle **xlst, char *label);
static int xui_list_get_idx(Ihandle *ih);
static int xui_text_get_number(Ihandle *ih, int *output, 
		int v_from, int v_to, int v_hit);
static int xui_get_size(Ihandle *ih, char *attr, int *height);
static Ihandle *xui_text_setting(Ihandle **xtxt, char *label, char *ext);
static Ihandle *xui_text_single_grid(char *label, 
		Ihandle **xcol, int size, char *ext);
static Ihandle *xui_text_double_grid(char *label, 
		Ihandle **xcol, Ihandle **xrow, char *ext);
static Ihandle *xui_button(char *prompt, Icallback ntf);
static int xui_config_status(void *config, char *prompt);
static char *xui_make_filters(char *slist);
static char *xui_make_fc_fontface(char *face, int *size);


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
		IupClose();
		return NULL;
	}

	/* initialize GUI structure with parameters from command line */
	gui->sysopt = ezopt;
	sprintf(gui->inst_id, "EZTHUMB_%p", gui);

	/* load configure from file, or create the file.
	 * 20170527 Note it's GUI mode so the conent of configure
	 * are stored in utf-8  */
	smm_codepage_set(65001);
	gui->config = csc_cfg_open(SMM_CFGROOT_DESKTOP,
			"ezthumb", "ezthumb.conf", CSC_CFG_RWC);
	smm_codepage_reset();
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

	/* 20160727 enable utf-8 mode for Windows.
	 * One case had been found that default setting can only accept utf-16
	 * in Windows, though utf-8 filename can be normally displayed in 
	 * File Explorer. Enabling the following attributions will let 
	 * IupFileDlg() return utf-8 filenames */
	/* 20170523 These two lines must be ahead of ezgui_create_window(),
	 * otherwise the font face entry would display cranky garbages */
	/* Note that these two lines must be kept in this sequence */
	IupSetAttribute(NULL, "UTF8MODE", "YES");
	IupSetAttribute(NULL, "UTF8MODE_FILE", "YES");

	/* 20170524 Enforce the libcsoup into the UTF-8 mode because
	 * the libiup has been set to UTF-8.
	 * Note that the ezthumb console is still stay in CP_ACP so it
	 * can display through the Windows console */
	smm_codepage_set(65001);

	ezgui_create_window(gui);

	/* filling the work area with file names from command line */
	sview = (SView *) IupGetAttribute(gui->list_view, EZOBJ_SVIEW);
	if ((fnum > 0) && sview) {
		for (i = 0; i < fnum; i++) {
#ifdef	CFG_WIN32RT
			flist[i] = ezttf_acp2utf8_alloc(flist[i]);
#endif
			ezgui_sview_file_append(sview, flist[i]);
			ezgui_show_progress(gui, i, fnum);
			/* highlight the RUN button when the list is not empty */
			ezgui_sview_active_update(sview, 
				EZGUI_SVIEW_ACTIVE_CONTENT, sview->svnum);
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

#ifdef	CFG_WIN32RT
	for (i = 0; i < fnum; smm_free(flist[i++]));
#endif
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
		if (gui->sysopt->pathout) {
			smm_free(gui->sysopt->pathout);
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
		i = lookup_index_string(uir_choose_font, 
				0, CFG_PIC_FONT_SYSTEM);
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
	IupSetAttributeId(tabs, "TABTITLE", 0, "&Generate");
	IupSetAttributeId(tabs, "TABTITLE", 1, " &Setup  ");
	IupSetAttributeId(tabs, "TABTITLE", 2, " &About  ");
	IupSetAttribute(tabs, "PADDING", "6x2");

	tabox = IupHbox(tabs, NULL);
	IupSetAttribute(tabox, "NMARGIN", "8x8");

	IupSetHandle("DLG_ICON", IupImageRGBA(128, 128, ezicon_pixbuf));
	gui->dlg_main = IupDialog(tabox);
	IupSetAttribute(gui->dlg_main, "TITLE", "Ezthumb");

	/* 20170514 The dialog has a different background color setting
	 * to the tab control, so align the dialog to the tab:
	 * 
	 * BGCOLOR: In Windows and in GTK when in Windows, the tab buttons 
	 * background it will be always defined by the system. In Windows 
	 * the default background is different from the dialog background. 
	 * Default: the global attribute DLGBGCOLOR. 
	 *
	 * The side effect is the text control would align with the tab 
	 * control too so it's necessary to explictly set the background 
	 * of text controls. */
	IupSetAttribute(gui->dlg_main, "BGCOLOR",
			IupGetAttribute(tabs, "BGCOLOR"));

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
	Ihandle	*hbox;

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

	hbox = IupHbox(gui->button_add, gui->button_del,
			gui->button_run, NULL);
	IupSetAttribute(hbox, "NORMALIZESIZE", "VERTICAL");
	return hbox;
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

	hbox = xui_text_double_grid("Grid", 
			&grid->entry_col_grid, &grid->entry_row, NULL);
	IupAppend(grid->zbox, hbox);
	IupSetCallback(grid->entry_col_grid, "VALUECHANGED_CB",
			(Icallback) ezgui_setup_grid_entry_event);
	IupSetCallback(grid->entry_row, "VALUECHANGED_CB",
			(Icallback) ezgui_setup_grid_entry_event);
	
	hbox = xui_text_double_grid("Column", 
			&grid->entry_col_step, &grid->entry_step, "(s)");
	IupAppend(grid->zbox, hbox);
	IupSetCallback(grid->entry_col_step, "VALUECHANGED_CB",
			(Icallback) ezgui_setup_grid_entry_event);
	IupSetCallback(grid->entry_step, "VALUECHANGED_CB",
			(Icallback) ezgui_setup_grid_entry_event);

	hbox = xui_text_single_grid("Total", &grid->entry_dss_amnt, 0, NULL);
	IupAppend(grid->zbox, hbox);
	IupSetCallback(grid->entry_dss_amnt, "VALUECHANGED_CB",
			(Icallback) ezgui_setup_grid_entry_event);

	hbox = xui_text_single_grid("Every", &grid->entry_dss_step, 0, "(s)");
	IupAppend(grid->zbox, hbox);
	IupSetCallback(grid->entry_dss_step, "VALUECHANGED_CB",
			(Icallback) ezgui_setup_grid_entry_event);

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
	} else if (ezopt_profile_stat(ezopt) & EZ_PROF_LENGTH) {
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
		ezopt_profile_enable(opt, EZ_PROF_LENGTH);
		break;
	case 1:
		xui_text_get_number(grid->entry_col_grid, 
				&opt->grid_col, 1, INT_MAX, 0);
		xui_text_get_number(grid->entry_row, 
				&opt->grid_row, 1, INT_MAX, 0);
		sprintf(tmp, "Grid:%dx%d ", opt->grid_col, opt->grid_row);
		ezopt_profile_disable(opt, EZ_PROF_LENGTH);
		break;
	case 2:
		xui_text_get_number(grid->entry_col_step,
				&opt->grid_col, 1, INT_MAX, 0);
		xui_text_get_number(grid->entry_step,
				&opt->tm_step, 1, INT_MAX, 0);
		sprintf(tmp, "Column:%d Step:%d(s) ", 
				opt->grid_col, opt->tm_step);
		opt->tm_step *= 1000;
		ezopt_profile_disable(opt, EZ_PROF_LENGTH);
		break;
	case 3:
		opt->grid_col = 0;
		xui_text_get_number(grid->entry_dss_amnt,
				&opt->grid_row, 1, INT_MAX, 0);
		sprintf(tmp, "Total %d snaps ", opt->grid_row);
		ezopt_profile_disable(opt, EZ_PROF_LENGTH);
		break;
	case 4:
		opt->grid_col = 0;
		opt->grid_row = 0;
		xui_text_get_number(grid->entry_dss_step,
				&opt->tm_step, 1, INT_MAX, 0);
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
	int	rc = 0;

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
		rc += xui_text_get_number(grid->entry_col_grid, 
				NULL, 1, INT_MAX, opt->grid_col);
		rc += xui_text_get_number(grid->entry_row, 
				NULL, 1, INT_MAX, opt->grid_row);
		break;
	case 2:
		rc += xui_text_get_number(grid->entry_col_step, 
				NULL, 1, INT_MAX, opt->grid_col);
		rc += xui_text_get_number(grid->entry_step, 
				NULL, 1, INT_MAX, opt->tm_step / 1000);
		break;
	case 3:
		rc += xui_text_get_number(grid->entry_dss_amnt, 
				NULL, 1, INT_MAX, opt->grid_row);
		break;
	case 4:
		rc += xui_text_get_number(grid->entry_dss_step, 
				NULL, 1, INT_MAX, opt->tm_step / 1000);
		break;
	}
	return rc;
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

static int ezgui_setup_grid_entry_event(Ihandle *ih)
{
	return ezgui_setup_suffix_event(ih);
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

	hbox = xui_text_single_grid("Ratio", 
			&zoom->entry_zoom_ratio, 60, NULL);
	IupSetAttribute(zoom->entry_zoom_ratio, "SPIN", "YES");
	IupSetAttribute(zoom->entry_zoom_ratio, "SPINMIN", "5");
	IupSetAttribute(zoom->entry_zoom_ratio, "SPINMAX", "200");
	IupSetAttribute(zoom->entry_zoom_ratio, "SPININC", "5");
	IupSetAttribute(zoom->entry_zoom_ratio, "SPINALIGN", "LEFT");
	IupSetAttribute(zoom->entry_zoom_ratio, "SPINVALUE", "50");
	IupSetCallback(zoom->entry_zoom_ratio, "VALUECHANGED_CB",
			(Icallback) ezgui_setup_zoom_entry_event);
	IupAppend(zoom->zbox, hbox);

	hbox = xui_text_double_grid("Pixel", 
			&zoom->entry_zoom_wid, &zoom->entry_zoom_hei, NULL);
	IupSetCallback(zoom->entry_zoom_wid, "VALUECHANGED_CB",
			(Icallback) ezgui_setup_zoom_entry_event);
	IupSetCallback(zoom->entry_zoom_hei, "VALUECHANGED_CB",
			(Icallback) ezgui_setup_zoom_entry_event);
	IupAppend(zoom->zbox, hbox);
	
	hbox = xui_text_single_grid("Width", &zoom->entry_width, 0, NULL);
	IupSetCallback(zoom->entry_width, "VALUECHANGED_CB",
			(Icallback) ezgui_setup_zoom_entry_event);
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
	} else if (ezopt_profile_stat(ezopt) & EZ_PROF_WIDTH) {
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
		opt->tn_width  = opt->tn_height = 0;
		ezopt_profile_enable(opt, EZ_PROF_WIDTH);
		break;
	case 1:
		xui_text_get_number(zoom->entry_zoom_ratio,
				&opt->tn_facto, 1, INT_MAX, 0);
		sprintf(tmp, "Zoom to %d%% ", opt->tn_facto);
		ezopt_profile_disable(opt, EZ_PROF_WIDTH);
		break;
	case 2:
		xui_text_get_number(zoom->entry_zoom_wid,
				&opt->tn_width, 1, INT_MAX, 0);
		xui_text_get_number(zoom->entry_zoom_hei,
				&opt->tn_height, 1, INT_MAX, 0);
		sprintf(tmp, "Zoom to %dx%d ", 
				opt->tn_width, opt->tn_height);
		ezopt_profile_disable(opt, EZ_PROF_WIDTH);
		break;
	case 3:
		xui_text_get_number(zoom->entry_width,
				&opt->canvas_width, 1, INT_MAX, 0);
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
	int	rc = 0;

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
		rc += xui_text_get_number(zoom->entry_zoom_ratio, 
				NULL, 0, 65535, opt->tn_facto);
		break;
	case 2:
		rc += xui_text_get_number(zoom->entry_zoom_wid, 
				NULL, 1,  65535, opt->tn_width);
		rc += xui_text_get_number(zoom->entry_zoom_hei, 
				NULL, 1,  65535, opt->tn_height);
		break;
	case 3:
		rc += xui_text_get_number(zoom->entry_width, 
				NULL, 1,  65535, opt->canvas_width);
		break;
	}
	return rc;
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

static int ezgui_setup_zoom_entry_event(Ihandle *ih)
{
	return ezgui_setup_suffix_event(ih);
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

	/* safe landing the sysopt->pathout with allocated memory */
	if (gui->sysopt->pathout) {
		/* so this pathout must come from the cli */
		gui->sysopt->pathout = 
			csc_strcpy_alloc(gui->sysopt->pathout, 0);
	}
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
		if (gui->sysopt->pathout) {
			smm_free(gui->sysopt->pathout);
		}
		gui->sysopt->pathout = csc_cfg_copy(gui->config, 
				EZGUI_MAINKEY, CFG_KEY_OUTPUT_PATH, 0);
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
		xui_copy_attribute(&gui->sysopt->pathout, 
				gui->dir_path, "VALUE");
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
	IupSetCallback(gui->font_face, "BUTTON_CB",
			(Icallback) ezgui_setup_font_update_event);

	vbox = IupVbox(hbox1,  hbox2, NULL);
	IupSetAttribute(vbox, "NMARGIN", "16x4");
	IupSetAttribute(vbox, "NGAP", "4");
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
		xui_copy_attribute(&gui->font_gtk_name, 
				gui->font_face, "VALUE");
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
		xui_copy_attribute(&gui->font_gtk_name, 
				gui->font_face, "VALUE");
		csc_cfg_write(gui->config, EZGUI_MAINKEY,
				CFG_KEY_FONT_FACE, gui->font_gtk_name);

		gui->sysopt->mi_font = gui->sysopt->ins_font = 
			xui_make_fc_fontface(gui->font_gtk_name, 
					&gui->sysopt->mi_size);
	}
	CDB_DEBUG(("Font Update: %s [%d]\n", 
				gui->sysopt->mi_font, gui->sysopt->mi_size));
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

	if (ezgui_setup_font_chooser(gui) < 0) {	/* cancel choosing */
		/* 20151110 can not update list control inside the event 
		 * callback, otherwise the list control will miss calculate 
		 * the change and confuse the window manager */
		gui->font_ppp_flag = 1;
	}
	return IUP_DEFAULT;
}

static int ezgui_setup_font_update_event(Ihandle* ih, int button, 
		int pressed, int x, int y, char* status)
{
	EZGUI	*gui;

	(void) x; (void) y; (void) status;

	if (button != IUP_BUTTON1) {	/*  left mouse button (button 1) */
		return IUP_DEFAULT;
	}
	if (pressed) {	/* waiting for button release event */
		return IUP_DEFAULT;
	}

	if ((gui = (EZGUI *) IupGetAttribute(ih, EZOBJ_MAIN)) == NULL) {
		return IUP_DEFAULT;
	}
	ezgui_setup_font_chooser(gui);
	return IUP_DEFAULT;
}

static int ezgui_setup_font_chooser(EZGUI *gui)
{
	char	*val;

	val = IupGetAttribute(gui->font_face, "VALUE");
	if (val) {
		IupSetAttribute(gui->dlg_font, "VALUE", val);
	}
	IupSetAttribute(gui->dlg_font, "COLOR", "128 0 255");
	IupPopup(gui->dlg_font, IUP_CENTERPARENT, IUP_CENTERPARENT);

	if (IupGetAttribute(gui->dlg_font, "STATUS") == NULL) {
		return -1;	/* cancelled */
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
	return csc_strcmp_param(val, gui->font_gtk_name) ? 1 : 0;
}

static Ihandle *ezgui_setup_suffix_create(EZGUI *gui)
{
	Ihandle *hbox;
	char	*s;

	s = csc_cfg_read(gui->config, EZGUI_MAINKEY, CFG_KEY_FILE_SUFFIX);
	if (s) {
		csc_strlcpy(gui->sysopt->suffix, s, 
				sizeof(gui->sysopt->suffix));
	}

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
		csc_strlcpy(gui->sysopt->suffix, val, 
				sizeof(gui->sysopt->suffix));
		csc_cfg_write(gui->config, EZGUI_MAINKEY,
				CFG_KEY_FILE_SUFFIX, gui->sysopt->suffix);
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
	char	*s;
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

	s = csc_cfg_read(gui->config, EZGUI_MAINKEY, CFG_KEY_TRANSPARENCY);
	if (s && !strcmp(s, "ON")) {
		meta_transparent_option(gui->sysopt, EZOP_TRANSPARENT);
		IupSetAttribute(gui->fmt_transp, "VALUE", "ON");
	} else {
		meta_transparent_option(gui->sysopt, 0);
		IupSetAttribute(gui->fmt_transp, "VALUE", "OFF");
	}

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
	s = csc_cfg_read(gui->config, EZGUI_MAINKEY, CFG_KEY_FILE_FORMAT);
	if (s) {
		gui->sysopt->img_format = meta_image_format(s);
	}
	
	/* seperate the image quality and frame rate */
	if (csc_cfg_read_int(gui->config, EZGUI_MAINKEY, CFG_KEY_JPG_QUALITY,
				&gui->tmp_jpg_qf) != SMM_ERR_NONE) {
		gui->tmp_jpg_qf  = 85;
	}
	if (csc_cfg_read_int(gui->config, EZGUI_MAINKEY, CFG_KEY_GIF_FRATE,
				&gui->tmp_gifa_fr) != SMM_ERR_NONE) {
		gui->tmp_gifa_fr = 1000;
	}

	switch (EZ_IMG_FMT_GET(gui->sysopt->img_format)) {
	case EZ_IMG_FMT_PNG:
		gui->fmt_idx = lookup_index_string(uir_format, 0, 
				CFG_PIC_FMT_PNG);
		break;
	case EZ_IMG_FMT_GIF:
		gui->fmt_idx = lookup_index_string(uir_format, 0, 
				CFG_PIC_FMT_GIF);
		break;
	case EZ_IMG_FMT_GIFA:
		gui->fmt_idx = lookup_index_string(uir_format, 0, 
				CFG_PIC_FMT_GIFA);
		EZ_IMG_PARAM_SET(gui->sysopt->img_format, gui->tmp_gifa_fr);
		break;
	default:
		gui->fmt_idx = lookup_index_string(uir_format, 0, 
				CFG_PIC_FMT_JPEG);
		EZ_IMG_PARAM_SET(gui->sysopt->img_format, gui->tmp_jpg_qf);
		break;
	}
	return vbox;
}

static int ezgui_setup_format_reset(EZGUI *gui)
{
	IupSetInt(gui->fmt_list, "VALUE", gui->fmt_idx + 1);
	
	IupSetInt(gui->fmt_gif_fr, "VALUE", gui->tmp_gifa_fr);
	IupSetInt(gui->fmt_jpg_qf, "VALUE", gui->tmp_jpg_qf);

	if (meta_transparent_option(gui->sysopt, -1)) {
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

	xui_text_get_number(gui->fmt_jpg_qf, &gui->tmp_jpg_qf, 1, 100, 1);
	csc_cfg_write_int(gui->config, EZGUI_MAINKEY,
			CFG_KEY_JPG_QUALITY, gui->tmp_jpg_qf);

	xui_text_get_number(gui->fmt_gif_fr, 
			&gui->tmp_gifa_fr, 1, INT_MAX, 0);
	csc_cfg_write_int(gui->config, EZGUI_MAINKEY,
			CFG_KEY_GIF_FRATE, gui->tmp_gifa_fr);

	/* update the runtime parameters */
	gui->sysopt->img_format = 
		meta_image_format(uir_format[gui->fmt_idx].s);
	switch (EZ_IMG_FMT_GET(gui->sysopt->img_format)) {
	case EZ_IMG_FMT_GIFA:
		EZ_IMG_PARAM_SET(gui->sysopt->img_format, gui->tmp_gifa_fr);
		break;
	case EZ_IMG_FMT_JPEG:
		EZ_IMG_PARAM_SET(gui->sysopt->img_format, gui->tmp_jpg_qf);
		break;
	}
	
	val = IupGetAttribute(gui->fmt_transp, "VALUE");
	if (!strcmp(val, "ON")) {
		meta_transparent_option(gui->sysopt, EZOP_TRANSPARENT);
	} else {
		meta_transparent_option(gui->sysopt, 0);
	}
	csc_cfg_write(gui->config, EZGUI_MAINKEY, CFG_KEY_TRANSPARENCY, val);
	
	CDB_DEBUG(("EVT_SETUP: Fmt=%d JPG=%d GIFA=%d Tra=%s\n",
			gui->fmt_idx, gui->tmp_jpg_qf, gui->tmp_gifa_fr, val));
	return 0;
}

static int ezgui_setup_format_check(EZGUI *gui)
{
	int	rc;

	if (gui->fmt_idx != xui_list_get_idx(gui->fmt_list)) {
		return 1;
	}

	rc = xui_text_get_number(gui->fmt_jpg_qf, NULL, 
			1, 100, gui->tmp_jpg_qf);
	rc += xui_text_get_number(gui->fmt_gif_fr, NULL,
			1, INT_MAX, gui->tmp_gifa_fr);
	if (rc) {
		return rc;
	}

	if (!strcmp(IupGetAttribute(gui->fmt_transp, "VALUE"), "ON")) {
		rc = meta_transparent_option(gui->sysopt, -1) ? 0 : 1;
	} else {
		rc = meta_transparent_option(gui->sysopt, -1) ? 1 : 0;
	}
	return rc;
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
	Ihandle	*hbox;

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

	hbox = IupHbox(xui_label("", "320", NULL),  
			gui->butt_setup_cancel, gui->butt_setup_apply, NULL);
	IupSetAttribute(hbox, "NORMALIZESIZE", "VERTICAL");
	return hbox;
}

static int ezgui_setup_button_check_status(EZGUI *gui)
{
	int	rc, counter = 0;

	rc = ezgui_setup_grid_check(gui->prof_grid);	/* 0x80 */
	counter = (counter << 1) | (rc ? 1 : 0);
	rc = ezgui_setup_zoom_check(gui->prof_zoom);	/* 0x40 */
	counter = (counter << 1) | (rc ? 1 : 0);
	rc = ezgui_setup_media_check(gui);		/* 0x20 */
	counter = (counter << 1) | (rc ? 1 : 0);
	rc = ezgui_setup_outputdir_check(gui);		/* 0x10 */
	counter = (counter << 1) | (rc ? 1 : 0);
	rc = ezgui_setup_font_check(gui);		/* 0x8 */
	counter = (counter << 1) | (rc ? 1 : 0);
	rc = ezgui_setup_suffix_check(gui);		/* 0x4 */
	counter = (counter << 1) | (rc ? 1 : 0);
	rc = ezgui_setup_dupname_check(gui);		/* 0x2 */
	counter = (counter << 1) | (rc ? 1 : 0);
	rc = ezgui_setup_format_check(gui);		/* 0x1 */
	counter = (counter << 1) | (rc ? 1 : 0);

	if (counter == 0) {
		IupSetAttribute(gui->butt_setup_apply, "ACTIVE", "NO");
		IupSetAttribute(gui->butt_setup_cancel, "ACTIVE", "NO");
	} else {
		IupSetAttribute(gui->butt_setup_apply, "ACTIVE", "YES");
		IupSetAttribute(gui->butt_setup_cancel, "ACTIVE", "YES");
	}
	CDB_MODL(("EVT_CHECK: 0x%x\n", counter));
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
	name = IupLabel("Ezthumb " VERSION);
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
 * A progress dialog for integrating with file managers
 ****************************************************************************/
int ezbar_init(EZOPT *ezopt)
{
	if (ezopt->flags & EZOP_PROGRESS_BAR) {
		ezopt->notiback = ezopt->notify;
		ezopt->notify = ezbar_event;
	}
	return 0;
}

static int ezbar_event(void *vobj, int event, long param, long opt, void *block)
{
	EZVID	*vidx = block;
	EZOPT   *ezopt = vobj;
	EZGUI   *gui = ezopt->gui;
	char	*tmp;
	static	char	desc_buf[1024];

	switch (event) {
	case EN_BATCH_BEGIN:
		if ((ezopt->gui = ezbar_main(vobj)) == NULL) {
			exit(-1);	/* that's extreme! */
		}
		//smm_sleep(1,0);
		break;
	case EN_BATCH_END:
		break;
	case EN_OPEN_BEGIN:
		snprintf(desc_buf, sizeof(desc_buf)-1, 
				"Opening %s ...", vidx->filename);
		tmp = ezttf_acp2utf8_alloc(desc_buf);
		csc_strlcpy(desc_buf, tmp, sizeof(desc_buf));
		smm_free(tmp);
		IupSetAttribute(gui->dlg_main, "DESCRIPTION", desc_buf);
		//smm_sleep(2,0);
		break;
	case EN_MEDIA_OPEN:
		video_media_in_buffer(vidx, desc_buf, sizeof(desc_buf)-1);
		tmp = ezttf_acp2utf8_alloc(desc_buf);
		csc_strlcpy(desc_buf, tmp, sizeof(desc_buf));
		smm_free(tmp);
		IupSetAttribute(gui->dlg_main, "DESCRIPTION", desc_buf);
		//smm_sleep(1,0);
		break;
	case EN_PROC_BEGIN:
		break;
	case EN_PROC_CURRENT:
		if (param == 0) { /* for key frame saving only */
			IupSetAttribute(gui->dlg_main, "STATE", "UNDEFINED");
			IupSetAttribute(gui->dlg_main, "INC", NULL);
		} else {
			IupSetInt(gui->dlg_main, "TOTALCOUNT", param);
			IupSetInt(gui->dlg_main, "COUNT", opt);
		}
		break;
	case EN_PROC_END:
		IupSetInt(gui->dlg_main, "TOTALCOUNT", 100);
		IupSetInt(gui->dlg_main, "COUNT", 100);
		//smm_sleep(1,0);
		break;
	}
	return ezopt->notiback(vobj, event, param, opt, block);
}

static void *ezbar_main(void *vobj)
{
	EZGUI	*gui;
	char	*s;
	int	i;

	if ((gui = ezgui_init(vobj, NULL, NULL)) == NULL) {
		return NULL;
	}

	/* loading configuration */
	s = csc_cfg_read(gui->config, EZGUI_MAINKEY, CFG_KEY_GRID);
	if (s) {
		if (lookup_index_string(uir_grid, 0, s) == 0) {
			ezopt_profile_enable(gui->sysopt, EZ_PROF_LENGTH);
			CDB_DEBUG(("EZBAR: Grid using profile\n"));
		} else {
			ezopt_profile_disable(gui->sysopt, EZ_PROF_LENGTH);
			CDB_DEBUG(("EZBAR: Grid=%dx%d\n", 
				gui->sysopt->grid_col,gui->sysopt->grid_row));
		}
	}

	s = csc_cfg_read(gui->config, EZGUI_MAINKEY, CFG_KEY_ZOOM);
	if (s) {
		if (lookup_index_string(uir_zoom, 0, s) == 0) {
			ezopt_profile_enable(gui->sysopt, EZ_PROF_WIDTH);
			CDB_DEBUG(("EZBAR: Zoom using profile\n"));
		} else {
			ezopt_profile_disable(gui->sysopt, EZ_PROF_WIDTH);
			CDB_DEBUG(("EZBAR: Zoom=%dx%dx%d\n", 
				gui->sysopt->tn_width, gui->sysopt->tn_height,
				gui->sysopt->tn_facto));
		}
	}

	s = csc_cfg_read(gui->config, EZGUI_MAINKEY, CFG_KEY_OUTPUT_METHOD);
	if (s) {
		i = lookup_index_string(uir_outdir, 0, s);
		if (!strcmp(uir_outdir[i].s, CFG_PIC_ODIR_PATH)) {
			gui->sysopt->pathout = csc_cfg_copy(gui->config,
				EZGUI_MAINKEY, CFG_KEY_OUTPUT_PATH, 0);
		}
		CDB_DEBUG(("EZBAR: OutPath=%s\n", gui->sysopt->pathout));
	}

	s = csc_cfg_read(gui->config, EZGUI_MAINKEY, CFG_KEY_FONT_FACE);
	if (s) {
		gui->sysopt->mi_font = gui->sysopt->ins_font =
			xui_make_fc_fontface(s, &gui->sysopt->mi_size);
		CDB_DEBUG(("EZBAR: Font=%s [%d]\n", gui->sysopt->mi_font,
					gui->sysopt->mi_size));
	}

	s = csc_cfg_read(gui->config, EZGUI_MAINKEY, CFG_KEY_FILE_SUFFIX);
	if (s) {
		csc_strlcpy(gui->sysopt->suffix, s,
				sizeof(gui->sysopt->suffix));
		CDB_DEBUG(("EZBAR: Suffix=%s\n", gui->sysopt->suffix));
	}

	s = csc_cfg_read(gui->config, EZGUI_MAINKEY, CFG_KEY_FILE_FORMAT);
	if (s) {
		gui->sysopt->img_format = meta_image_format(s);
	}
	s = csc_cfg_read(gui->config, EZGUI_MAINKEY, CFG_KEY_TRANSPARENCY);
	switch (EZ_IMG_FMT_GET(gui->sysopt->img_format)) {
	case EZ_IMG_FMT_GIFA:
		gui->tmp_gifa_fr = 1000;
		csc_cfg_read_int(gui->config, EZGUI_MAINKEY, 
				CFG_KEY_GIF_FRATE, &gui->tmp_gifa_fr);
		EZ_IMG_PARAM_SET(gui->sysopt->img_format, gui->tmp_gifa_fr);
		if (s && !strcmp(s, "ON")) {
			meta_transparent_option(gui->sysopt,EZOP_TRANSPARENT);
		} else {
			meta_transparent_option(gui->sysopt, 0);
		}
		break;
	case EZ_IMG_FMT_GIF:
	case EZ_IMG_FMT_PNG:
		if (s && !strcmp(s, "ON")) {
			meta_transparent_option(gui->sysopt,EZOP_TRANSPARENT);
		} else {
			meta_transparent_option(gui->sysopt, 0);
		}
		break;
	case EZ_IMG_FMT_JPEG:
		gui->tmp_jpg_qf  = 85;
		csc_cfg_read_int(gui->config, EZGUI_MAINKEY, 
				CFG_KEY_JPG_QUALITY, &gui->tmp_jpg_qf);
		EZ_IMG_PARAM_SET(gui->sysopt->img_format, gui->tmp_jpg_qf);
		break;
	}
	CDB_DEBUG(("EZBAR: Format 0x%08x [%x]\n", gui->sysopt->img_format,
				gui->sysopt->flags & EZOP_TRANSPARENT));

	/* create window and widgets */
	gui->dlg_main = IupProgressDlg();
	IupSetAttribute(gui->dlg_main, "TITLE", "Ezthumb");
	IupSetAttribute(gui->dlg_main, "SIZE", "300");
	IupSetAttribute(gui->dlg_main, "PROGRESSHEIGHT", "16");
	IupSetAttribute(gui->dlg_main, "DESCRIPTION", "\n\n\n\n");
	IupSetAttribute(gui->dlg_main, EZOBJ_MAIN, (char*) gui);
	IupSetHandle("DLG_ICON", IupImageRGBA(128, 128, ezicon_pixbuf));
	IupSetAttribute(gui->dlg_main, "ICON", "DLG_ICON");
	IupSetCallback(gui->dlg_main, "CANCEL_CB", ezbar_cb_cancel);

	IupShowXY(gui->dlg_main, IUP_CENTER, IUP_CENTER);
	return gui;
}

static int ezbar_cb_cancel(Ihandle *ih)
{
	EZGUI	*gui;

	if ((gui = (EZGUI *) IupGetAttribute(ih, EZOBJ_MAIN)) != NULL) {
		IupSetAttribute(gui->dlg_main, EZOBJ_MAIN, NULL);
	}

	gui->sysopt->notify = gui->sysopt->notiback;
	IupExitLoop();
	exit(-1);
	return IUP_DEFAULT;
}


/****************************************************************************
 * New control for workarea
 ****************************************************************************/

static Ihandle *ezgui_sview_create(EZGUI *gui, int dblck)
{
	Ihandle	*vb_main, *vb_size, *vb_len, *vb_res, *vb_prog, *vb_attr;
	Ihandle	*hbox;
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

#ifdef	CFG_WIN32RT
	/* 20160812: In Windows the list control doesn't support Ctrl-A to
	 * select all items so I put a workaround here */
	IupSetCallback(sview->filename, "K_cA",
			(Icallback) ezgui_sview_event_ctrl_a);
#endif

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
	IupSetAttribute(sview->attrib, "SIZE", "5");
	IupSetAttribute(sview->attrib, "EXPAND", "VERTICAL");
	IupSetAttribute(sview->attrib, "SCROLLBAR", "NO");
	IupSetAttribute(sview->attrib, "ACTIVE", "NO");
	IupSetAttribute(sview->attrib, "VISIBLE", "NO");
	vb_attr = IupVbox(IupFill(), sview->attrib, NULL);

	hbox = IupHbox(vb_main, vb_size, vb_len, vb_res, vb_prog, vb_attr, NULL);
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
	//smm_codepage_set(65001);
	fname = csc_strcpy_alloc(text, 0);
	ezthumb(fname, gui->sysopt);
	smm_free(fname);
	//smm_codepage_reset();

	gui->sysopt->pre_seek = 0;
	gui->sysopt->pre_br   = 0;
	gui->sysopt->pre_dura = 0;
	return IUP_DEFAULT;
}

static int ezgui_sview_event_dropfiles(Ihandle *ih, 
		const char* filename, int num, int x, int y)
{
	SView	*sview;
	char	*tmp;

	(void)num; (void)x; (void)y;

	if ((sview = (SView *) IupGetAttribute(ih, EZOBJ_SVIEW)) == NULL) {
		return IUP_DEFAULT;
	}

	CDB_DEBUG(("EVT_Dropfiles: fname=%s number=%d %dx%d\n",
				filename, num, x, y));

	if ((tmp = csc_strcpy_alloc(filename, 0)) == NULL) {
		CDB_ERROR(("EVT_Dropfiles: low memory.\n"));
		return IUP_DEFAULT;
	}

#ifdef	CFG_WIN32RT
	ezgui_sview_file_append(sview, tmp);
#else
	csc_url_decode(tmp, strlen(filename)+1, (char*) filename);
	ezgui_sview_file_append(sview, tmp);
#endif
	smm_free(tmp);
	/* highlight the RUN button when the list is not empty */
	ezgui_sview_active_update(sview, 
			EZGUI_SVIEW_ACTIVE_CONTENT, sview->svnum);
	return IUP_DEFAULT;
}

static int ezgui_sview_event_multi_select(Ihandle *ih, char *value)
{
	SView	*sview;

	if ((sview = (SView *) IupGetAttribute(ih, EZOBJ_SVIEW)) == NULL) {
		return IUP_DEFAULT;
	}

	CDB_DEBUG(("EVT_Multi_select: %s\n", value));
	CDB_DEBUG(("List value=%s\n", 
				IupGetAttribute(sview->filename, "VALUE")));
	
	/* the parameter 'value' is useless. grab list myself */
	/*value = IupGetAttribute(sview->list_fname, "VALUE");
	IupSetAttribute(gui->button_del, "ACTIVE", "NO");*/
	value = IupGetAttribute(sview->filename, "VALUE");
	ezgui_sview_active_update(sview, EZGUI_SVIEW_ACTIVE_SELECT, 
			csc_strcount_char(value, "+"));
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

	//CDB_MODL(("EVT_Motion: %d %d %d\n", x, y, line));

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

#ifdef	CFG_WIN32RT
static int ezgui_sview_event_ctrl_a(Ihandle *ih, int c)
{
	SView	*sview;
	char	*buf;

	CDB_DEBUG(("ezgui_sview_event_ctrl_a: %x\n",c));

	if ((sview = (SView *) IupGetAttribute(ih, EZOBJ_SVIEW)) == NULL) {
		return IUP_DEFAULT;
	}

	buf = csc_strcpy_alloc(IupGetAttribute(sview->filename, "VALUE"), 0);
	c = strlen(buf);
	memset(buf, '+', c);
	IupSetAttribute(sview->filename, "VALUE", buf);
	smm_free(buf);

	ezgui_sview_active_update(sview, EZGUI_SVIEW_ACTIVE_SELECT, c);
	return IUP_DEFAULT;
}
#endif

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
	int	i = 0;

	value = IupGetAttribute(sview->filename, "VALUE");
	CDB_PROG(("EVT_Remove: %s\n", value));
	while (*value) {
		if (*value == '+') {
			ezgui_sview_file_remove(sview, i+1);
		} else {
			i++;
		}
		value++;
	}

	value = IupGetAttribute(sview->filename, "VALUE");
	ezgui_sview_active_update(sview, EZGUI_SVIEW_ACTIVE_SELECT, 
			csc_strcount_char(value, "+"));
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
	//smm_codepage_set(65001);
	if (ezinfo(fname, sview->gui->sysopt, &vobj) != EZ_ERR_NONE) {
		//smm_codepage_reset();
		/* FIXME: disaster control */
		IupSetInt(sview->filename, "REMOVEITEM", lnext);
		return EZ_ERR_FORMAT;
	}
	//smm_codepage_reset();

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
/* 20170515 Just surprisingly (re)found the contents of IupGetAttribute() was 
 * volatile. For example, the
 *   gui->font_gtk_name = IupGetAttribute(gui->font_face, "VALUE")
 * After a while the contents changed from font face to the contents of
 * suffix text control. */
static int xui_copy_attribute(char **dst, Ihandle *ih, char *name)
{
	char	*s = IupGetAttribute(ih, name);

	if (dst) {
		if (*dst) {
			free(*dst);
			*dst = NULL;
		}
		if (s) {
			*dst = csc_strcpy_alloc(s, 0);
		}
	}
	return (s != NULL);
}

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
	IupSetAttribute(text, "BGCOLOR", "TXTBGCOLOR");

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

static int xui_text_get_number(Ihandle *ih, int *output, int v_from, int v_to, int v_hit)
{
	char	*val, *end;
	int	rc;

	val = IupGetAttribute(ih, "VALUE");
	if (val) {
		rc = (int) strtol(val, &end, 0);
		if ((*val == 0) || (*end != 0)) {
			return 0;	/* no value or faulty value */
		}
		if ((rc < v_from) || (rc > v_to)) {
			return 0;	/* value out of the range */
		}
		if (output) {
			*output = rc;
		}
		if (rc != v_hit) {
			return 1;
		}
	}
	return 0;	/* no value */
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
	IupSetAttribute(text, "BGCOLOR", "TXTBGCOLOR");

	hbox = IupHbox(IupLabel(label), text,
			ext ? IupLabel(ext) : NULL, NULL);
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
	IupSetAttribute(hbox, "NGAP", "4");

	if (xtxt) {
		*xtxt = text;
	}
	return hbox;
}

static Ihandle *xui_text_single_grid(char *label, 
		Ihandle **xcol, int size, char *ext)
{
	Ihandle	*hbox, *title;

	title = IupZbox(NULL);
	IupAppend(title, xui_label(label, NULL, NULL));
	IupAppend(title, IupLabel("Column"));	/* dummy for fixing widths */

	*xcol = IupText(NULL);
	if (size) {
		IupSetInt(*xcol, "SIZE", size);
	} else {
		IupSetAttribute(*xcol, "SIZE", EGPS_GRID_FST_TEXT);
	}
	IupSetAttribute(*xcol, "BGCOLOR", "TXTBGCOLOR");

	hbox = IupHbox(title, *xcol, ext ? IupLabel(ext) : NULL,NULL);
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
	//IupSetAttribute(hbox, "NGAP", "4");
	return hbox;
}


static Ihandle *xui_text_double_grid(char *label, 
		Ihandle **xcol, Ihandle **xrow, char *ext)
{
	Ihandle	*hbox, *title;

	title = IupZbox(NULL);
	IupAppend(title, xui_label(label, NULL, NULL));
	IupAppend(title, IupLabel("Column"));	/* dummy for fixing widths */

	*xcol = IupText(NULL);
	IupSetAttribute(*xcol, "SIZE", EGPS_GRID_SND_TEXT);
	IupSetAttribute(*xcol, "BGCOLOR", "TXTBGCOLOR");

	*xrow = IupText(NULL);
	IupSetAttribute(*xrow, "SIZE", EGPS_GRID_SND_TEXT);
	IupSetAttribute(*xrow, "BGCOLOR", "TXTBGCOLOR");

	hbox = IupHbox(title, *xcol, IupLabel("x"), *xrow, 
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
#ifndef	CFG_WIN32RT
	int	i;
#endif

	/* Assuming the worst case of the short list is like "a;b;c;d",
	 * it will then be expanded to "*.a;*.A;*.b;*.B;*.c;*.C;*.d;*.D". 
	 * The biggest length is N + (N/2+1) * 2 */
#ifdef	CFG_WIN32RT
	if ((flt = smm_alloc(strlen(slist) * 2 + 64)) == NULL) {
#else
	if ((flt = smm_alloc(strlen(slist) * 4 + 64)) == NULL) {
#endif
		return NULL;
	}

	strcpy(flt, "Video Files|");
	n = strlen(flt);
	while ((slist = csc_gettoken(slist, token, sizeof(token), ",;:")) 
			!= NULL) {
		n += sprintf(flt + n, "*.%s;", token);
#ifndef	CFG_WIN32RT
		for (i = 0; i < (int)strlen(token); i++) {
			token[i] = toupper(token[i]);
		}
		n += sprintf(flt + n, "*.%s;", token);
#endif
	}
	flt[strlen(flt)-1] = 0;		/* cut out the tailing ';' */
	strcat(flt, "|All Files|*.*|");
	puts(flt);
	return flt;
}


/* translate the font face style to the GD accepted one:
 *   Nimbus Sans Bold Italic 10  --> Nimbus Sans:Bold:Italic
 *   Nimbus Sans Italic 10  --> Nimbus Sans:Italic
 *   Nimbus Sans 10  --> Nimbus Sans:Regular  
 *   PT Sans Narrow Condensed 12 --> PT Sans Narrow:Regular
 */
extern int iupGetFontInfo(const char* font, char *typeface, int *size, 
	int *is_bold, int *is_italic, int *is_underline, int *is_strikeout);

static char *xui_make_fc_fontface(char *face, int *size)
{
	char	typeface[1024];
	int	is_bold, is_italic, is_underline, is_strikeout;
	
	iupGetFontInfo(face, typeface, size, &is_bold, &is_italic, 
			&is_underline, &is_strikeout);
	
	CDB_MODL(("Font Config: %s %d %d %d %d %d\n", typeface, *size, 
			is_bold, is_italic, is_underline, is_strikeout));

	if (is_bold) {
		strcat(typeface, ":bold");
	}
	if (is_italic) {
		strcat(typeface, ":italic");
	}
	if (!is_bold && !is_italic) {
		strcat(typeface, ":regular");
	}
	return meta_make_fontdir(typeface);
}


