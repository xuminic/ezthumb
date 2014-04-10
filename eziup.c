
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

#define	EZGSINI		-999	/* initialization status */

static	char	*list_grid[] = {
	CFG_PIC_AUTO, CFG_PIC_GRID_DIM, CFG_PIC_GRID_STEP,
	CFG_PIC_DIS_NUM, CFG_PIC_DIS_STEP, CFG_PIC_DIS_KEY, NULL
};

static	char	*list_zoom[] = {
	CFG_PIC_AUTO, CFG_PIC_ZOOM_RATIO, CFG_PIC_ZOOM_DEFINE,
	CFG_PIC_ZOOM_SCREEN, NULL
};

static	char	*list_duration[] = {
	CFG_PIC_AUTO, CFG_PIC_DFM_HEAD, CFG_PIC_DFM_SCAN, 
	CFG_PIC_DFM_FAST, NULL
};

static	char	*list_format[] = {
	CFG_PIC_FMT_JPEG, CFG_PIC_FMT_PNG, CFG_PIC_FMT_GIFA,
	CFG_PIC_FMT_GIF, NULL
};


static int ezgui_create_window(EZGUI *gui);
static EZGUI *ezgui_get_global(Ihandle *any);
static Ihandle *ezgui_page_generate(EZGUI *gui);
static Ihandle *ezgui_page_generate_workarea(EZGUI *gui);
static Ihandle *ezgui_page_generate_button(EZGUI *gui);
static Ihandle *ezgui_page_setup(EZGUI *gui);
static int ezgui_page_setup_reset(EZGUI *gui);
static Ihandle *ezgui_page_setup_profile(EZGUI *gui);
static Ihandle *ezgui_page_setup_grid_zbox(EZGUI *gui);
static Ihandle *ezgui_page_setup_zoom_zbox(EZGUI *gui);
static Ihandle *ezgui_page_setup_duration(EZGUI *gui);
static Ihandle *ezgui_page_setup_output(EZGUI *gui);
static Ihandle *ezgui_page_setup_button(EZGUI *gui);
static int ezgui_event_setup_format(Ihandle *ih, char *text, int item, int);
static int ezgui_event_setup_grid(Ihandle *ih, char *text, int i, int s);
static int ezgui_event_setup_zoom(Ihandle *ih, char *text, int i, int s);
static int ezgui_event_setup_ok(Ihandle *ih);
static int ezgui_event_setup_cancel(Ihandle *ih);

static Ihandle *xui_text(char *label, char *size);
static Ihandle *xui_label(char *label, char *size, char *font);
static Ihandle *xui_list_setting(Ihandle **xlst, char *label);
static int xui_list_get_idx(Ihandle *ih);
static Ihandle *xui_text_setting(Ihandle **xtxt, char *label, char *ext);
static Ihandle *xui_button(char *prompt, Icallback ntf);


EZGUI *ezgui_init(EZOPT *ezopt, int *argcs, char ***argvs)
{
	EZGUI	*gui;

	IupOpen(argcs, argvs);

	IupSetGlobal("SINGLEINSTANCE", "ezthumb");
	if (!IupGetGlobal("SINGLEINSTANCE")) {
		IupClose();
		return NULL;
	}

	if ((gui = calloc(sizeof(EZGUI), 1)) == NULL) {
		return NULL;
	}

	/* initialize GUI structure with parameters from command line */
	gui->sysopt = ezopt;

	/* the index of profile of grid and zoom parameters */
	gui->grid_idx = ezm_strarr_index(list_grid, CFG_PIC_GRID_STEP);
	gui->zoom_idx = ezm_strarr_index(list_zoom, CFG_PIC_ZOOM_DEFINE);

	gui->dfm_idx = ezm_strarr_index(list_duration, CFG_PIC_AUTO);
	gui->fmt_idx = ezm_strarr_index(list_format, CFG_PIC_FMT_JPEG);

	/* seperate the image quality and frame rate */
	gui->tmp_jpg_qf  = 85;
	gui->tmp_gifa_fr = 1000;
	if (!strcmp(gui->sysopt->img_format, "jpg") || 
			!strcmp(gui->sysopt->img_format, "jpeg")) {
		gui->tmp_jpg_qf  = gui->sysopt->img_quality;
	} else if (!strcmp(gui->sysopt->img_format, "gif") && 
			gui->sysopt->img_quality) {
		gui->tmp_gifa_fr = gui->sysopt->img_quality;
	}
	return gui;
}

int ezgui_run(EZGUI *gui, char *flist[], int fnum)
{
	EZVID	vobj;
	EZMEDIA	*minfo;
	int	i;

	ezgui_create_window(gui);

	/* filling the work area with file names from command line */
	for (i = 0; i < fnum; i++) {
		/* 20120903 Bugfix: set the codepage to utf-8 before calling
		 * ezthumb core. In Win32 version, the ezthumb core uses the 
		 * default codepage to process file name. However the GTK 
		 * converted the file name to UTF-8 so the Windows version 
		 * could not find the file. 
		 * There's no such problem in linux.*/
		smm_codepage_set(65001);
		if (ezinfo(flist[i], gui->sysopt, &vobj) != EZ_ERR_NONE) {
			smm_codepage_reset();
			continue;
		}
		smm_codepage_reset();

		/* FIXME: memory leak */
		minfo = malloc(strlen(flist[i]) + sizeof(EZMEDIA) + 4);
		if (minfo == NULL) {
			continue;
		}

		strcpy(minfo->fname, flist[i]);
		meta_timestamp(vobj.duration, 0, minfo->vidlen);
		meta_filesize(vobj.filesize, minfo->fsize);
		sprintf(minfo->resolv, "%dx%d", vobj.width, vobj.height);
		sprintf(minfo->progr, "0%%");

		IupSetAttributeId(gui->list_fname,  "", i+1, minfo->fname);
		IupSetAttributeId(gui->list_size,   "", i+1, minfo->fsize);
		IupSetAttributeId(gui->list_length, "", i+1, minfo->vidlen);
		IupSetAttributeId(gui->list_resolv, "", i+1, minfo->resolv);
		IupSetAttributeId(gui->list_prog,   "", i+1, minfo->progr);
	}

	for ( ; i <= 20; i++) {
		minfo = malloc(sizeof(EZMEDIA) + 64);
		sprintf(minfo->fname, "Mytestfile%03d.txt", i);
		strcpy(minfo->vidlen, "10:31:97");
		strcpy(minfo->fsize, "10.997GB");
		strcpy(minfo->resolv, "1920x1024");
		strcpy(minfo->progr, "100%");
		IupSetAttributeId(gui->list_fname,  "", i+1, minfo->fname);
		IupSetAttributeId(gui->list_size,   "", i+1, minfo->fsize);
		IupSetAttributeId(gui->list_length, "", i+1, minfo->vidlen);
		IupSetAttributeId(gui->list_resolv, "", i+1, minfo->resolv);
		IupSetAttributeId(gui->list_prog,   "", i+1, minfo->progr);
	}

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
}


static int ezgui_create_window(EZGUI *gui)
{
	Ihandle		*tabs, *dlg;

	tabs = IupTabs(ezgui_page_generate(gui), 
			ezgui_page_setup(gui), IupVbox(IupFill(), NULL), NULL);
	IupSetAttribute(tabs, "TABTITLE0", "Generate");
	IupSetAttribute(tabs, "TABTITLE1", " Setup  ");
	IupSetAttribute(tabs, "TABTITLE2", "Advanced");
	IupSetAttribute(tabs, "PADDING", "6x2");

	dlg = IupDialog(tabs);
	IupSetAttribute(dlg, "TITLE", "Ezthumb");
	IupSetAttribute(dlg, "SIZE", "HALFx");

	/* bind the GUI structure into the current dialog so it can be accessed
	 * in its sub-controls */
	IupSetAttribute(dlg, "GUIEXT", (char*) gui);
	IupShow(dlg);
	return 0;
}

/* retrieve the GUI structure from the top leve dialog wedget */
static EZGUI *ezgui_get_global(Ihandle *any)
{
	Ihandle	*dlg = IupGetDialog(any);

	if (dlg) {
		return (EZGUI*) IupGetAttribute(dlg, "GUIEXT");
	}
	return NULL;
}


/****************************************************************************
 * Page Main 
 ****************************************************************************/
static Ihandle *ezgui_page_generate(EZGUI *gui)
{
	Ihandle	*vbox, *hbox, *sbox;

	gui->prog_bar = IupProgressBar();
	IupSetAttribute(gui->prog_bar, "VALUE", "0.5");
	IupSetAttribute(gui->prog_bar, "EXPAND", "HORIZONTAL");
	IupSetAttribute(gui->prog_bar, "DASHED", "YES");
	IupSetAttribute(gui->prog_bar, "SIZE", "x10");
	//IupSetAttribute(gui->prog_bar, "VISIBLE", "NO");

	hbox = IupHbox(gui->prog_bar, ezgui_page_generate_button(gui), NULL);
	IupSetAttribute(hbox, "NGAP", "10");
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");

	sbox = IupScrollBox(ezgui_page_generate_workarea(gui));

	vbox = IupVbox(sbox, hbox, NULL);
	IupSetAttribute(vbox, "NGAP", "4");
	IupSetAttribute(vbox, "NMARGIN", "4x4");
	return vbox;
}

static int ezgui_workarea_scroll(Ihandle *ih, char *text, int item, int state)
{
	printf("Action %s: %p %d %d\n", text, ih, item, state);
	return 0;
}

static Ihandle *ezgui_page_generate_workarea(EZGUI *gui)
{
	Ihandle	*vb_main, *vb_size, *vb_len, *vb_res, *vb_prog, *hbox;

	gui->list_fname = IupList(NULL);
	IupSetAttribute(gui->list_fname, "EXPAND", "YES");
	IupSetAttribute(gui->list_fname, "MULTIPLE", "YES");
	IupSetAttribute(gui->list_fname, "SCROLLBAR", "NO");
	vb_main = IupVbox(xui_text("Files", NULL), gui->list_fname, NULL);
	IupSetCallback(gui->list_fname, "ACTION", 
			(Icallback) ezgui_workarea_scroll);

	gui->list_size = IupList(NULL);
	IupSetAttribute(gui->list_size, "SIZE", "50");
	IupSetAttribute(gui->list_size, "EXPAND", "VERTICAL");
	IupSetAttribute(gui->list_size, "SCROLLBAR", "NO");
	IupSetAttribute(gui->list_size, "ACTIVE", "NO");
	vb_size = IupVbox(xui_text("Size", "50"), gui->list_size, NULL);

	gui->list_length = IupList(NULL);
	IupSetAttribute(gui->list_length, "SIZE", "48");
	IupSetAttribute(gui->list_length, "EXPAND", "VERTICAL");
	IupSetAttribute(gui->list_length, "SCROLLBAR", "NO");
	IupSetAttribute(gui->list_length, "ACTIVE", "NO");
	vb_len = IupVbox(xui_text("Length", "48"), gui->list_length, NULL);

	gui->list_resolv = IupList(NULL);
	IupSetAttribute(gui->list_resolv, "SIZE", "50");
	IupSetAttribute(gui->list_resolv, "EXPAND", "VERTICAL");
	IupSetAttribute(gui->list_resolv, "SCROLLBAR", "NO");
	IupSetAttribute(gui->list_resolv, "ACTIVE", "NO");
	vb_res = IupVbox(xui_text("Resolution", "50"), gui->list_resolv, NULL);
	
	gui->list_prog = IupList(NULL);
	IupSetAttribute(gui->list_prog, "SIZE", "40");
	IupSetAttribute(gui->list_prog, "SCROLLBAR", "NO");
	IupSetAttribute(gui->list_prog, "EXPAND", "VERTICAL");
	IupSetAttribute(gui->list_prog, "ACTIVE", "NO");
	vb_prog = IupVbox(xui_text("Progress", "40"), gui->list_prog, NULL);

	hbox = IupHbox(vb_main, vb_size, vb_len, vb_res, vb_prog, NULL);
	return hbox;
}

static Ihandle *ezgui_page_generate_button(EZGUI *gui)
{
	gui->button_add = xui_button("Add", NULL);
	gui->button_del = xui_button("Remove", NULL);
	gui->button_run = xui_button("Run", NULL);
	return IupHbox(gui->button_add, gui->button_del, gui->button_run, NULL);
}


/****************************************************************************
 * Page Setup 
 ****************************************************************************/
static Ihandle *ezgui_page_setup(EZGUI *gui)
{
	Ihandle	*vbox;

	vbox = IupVbox(xui_label("Profile Selection", NULL, "Bold"), 
			ezgui_page_setup_profile(gui), 
			xui_label("Duration Finding Mode", NULL, "Bold"), 
			ezgui_page_setup_duration(gui), 
			xui_label("Output File Format", NULL, "Bold"), 
			ezgui_page_setup_output(gui), 
			IupFill(), 
			ezgui_page_setup_button(gui), NULL);
	IupSetAttribute(vbox, "NGAP", "4");
	IupSetAttribute(vbox, "NMARGIN", "4x4");

	ezgui_page_setup_reset(gui);
	return vbox;
}

static int ezgui_page_setup_reset(EZGUI *gui)
{
	IupSetInt(gui->prof_grid, "VALUE", gui->grid_idx + 1);
	IupSetInt(gui->prof_zoom, "VALUE", gui->zoom_idx + 1);
	IupSetInt(gui->dfm_list, "VALUE", gui->dfm_idx + 1);

	IupSetInt(gui->fmt_list, "VALUE", gui->fmt_idx + 1);
	ezgui_event_setup_format((Ihandle*) gui, 
			list_format[gui->fmt_idx], gui->fmt_idx + 1, EZGSINI);

	if (gui->sysopt->flags & EZOP_TRANSPARENT) {
		IupSetAttribute(gui->fmt_transp, "VALUE", "ON");
	} else {
		IupSetAttribute(gui->fmt_transp, "VALUE", "OFF");
	}
	IupSetInt(gui->fmt_gif_fr, "VALUE", gui->tmp_gifa_fr);
	IupSetInt(gui->fmt_jpg_qf, "VALUE", gui->tmp_jpg_qf);

	IupSetInt(gui->entry_zbox_grid, "VALUEPOS", gui->grid_idx);
	IupSetInt(gui->entry_zbox_zoom, "VALUEPOS", gui->zoom_idx);
	return 0;
}

static Ihandle *ezgui_page_setup_profile(EZGUI *gui)
{
	Ihandle	*hbox1, *hbox2, *vbox;
	int	i;

	/* First line: Grid Setting */
	hbox1 = xui_list_setting(&gui->prof_grid, "Grid Setting");
	for (i = 0; list_grid[i]; i++) {
		IupSetAttributeId(gui->prof_grid, "", i + 1, list_grid[i]);
	}
	IupSetCallback(gui->prof_grid, "ACTION",
			(Icallback) ezgui_event_setup_grid);
	gui->entry_zbox_grid = ezgui_page_setup_grid_zbox(gui);
	IupAppend(hbox1, gui->entry_zbox_grid);

	/* Second line: Zoom Setting */
	hbox2 = xui_list_setting(&gui->prof_zoom, "Zoom Setting");
	for (i = 0; list_zoom[i]; i++) {
		IupSetAttributeId(gui->prof_zoom, "", i + 1, list_zoom[i]);
	}
	IupSetCallback(gui->prof_zoom, "ACTION",
			(Icallback) ezgui_event_setup_zoom);
	gui->entry_zbox_zoom = ezgui_page_setup_zoom_zbox(gui);
	IupAppend(hbox2, gui->entry_zbox_zoom);

	/* assemble the Profile Setting area */
	vbox = IupVbox(hbox1, hbox2, NULL);
	IupSetAttribute(vbox, "NMARGIN", "16x4");
	return vbox;
}

static Ihandle *ezgui_page_setup_grid_zbox(EZGUI *gui)
{
	Ihandle	*hbox, *zbox;

	zbox = IupZbox(IupFill(), NULL);

	hbox = xui_text_setting(&gui->entry_col1, "Grid", "x");
	gui->entry_row = IupText(NULL);
	IupSetAttribute(gui->entry_row, "SIZE", "12x10");
	IupAppend(hbox, gui->entry_row);

	/*gui->entry_col1 = IupText(NULL);
	IupSetAttribute(gui->entry_col1, "SIZE", "12x10");
	gui->entry_row = IupText(NULL);
	IupSetAttribute(gui->entry_row, "SIZE", "12x10");
	hbox = IupHbox(IupLabel("Grid"), gui->entry_col1, 
			IupLabel("x"), gui->entry_row, NULL);
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");*/
	IupAppend(zbox, hbox);
	
	gui->entry_col2 = IupText(NULL);
	IupSetAttribute(gui->entry_col2, "SIZE", "12x10");
	gui->entry_step = IupText(NULL);
	IupSetAttribute(gui->entry_step, "SIZE", "18x10");
	hbox = IupHbox(IupLabel("Col"), gui->entry_col2, IupLabel("Step"),
			gui->entry_step, IupLabel("(s)"), NULL);
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
	IupAppend(zbox, hbox);

	gui->entry_dss_no = IupText(NULL);
	IupSetAttribute(gui->entry_dss_no, "SIZE", "24x10");
	hbox = IupHbox(IupLabel("Dss No. "), gui->entry_dss_no, NULL);
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
	IupAppend(zbox, hbox);

	gui->entry_dss_step = IupText(NULL);
	IupSetAttribute(gui->entry_dss_step, "SIZE", "24x10");
	hbox = IupHbox(IupLabel("Dss Step "), gui->entry_dss_step, 
			IupLabel("(s) "), NULL);
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
	IupAppend(zbox, hbox);
	IupAppend(zbox, IupFill());
	return zbox;
}

static Ihandle *ezgui_page_setup_zoom_zbox(EZGUI *gui)
{
	Ihandle	*hbox, *zbox;

	zbox = IupZbox(IupFill(), NULL);

	gui->entry_zoom_ratio = IupText(NULL);
	IupSetAttribute(gui->entry_zoom_ratio, "SIZE", "24x11");
	IupSetAttribute(gui->entry_zoom_ratio, "SPIN", "YES");
	IupSetAttribute(gui->entry_zoom_ratio, "SPINMIN", "5");
	IupSetAttribute(gui->entry_zoom_ratio, "SPINMAX", "200");
	IupSetAttribute(gui->entry_zoom_ratio, "SPININC", "5");
	IupSetAttribute(gui->entry_zoom_ratio, "SPINALIGN", "LEFT");
	IupSetAttribute(gui->entry_zoom_ratio, "SPINVALUE", "50");
	hbox = IupHbox(IupLabel("Zoom"), gui->entry_zoom_ratio,
			IupLabel("%"), NULL);
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
	IupAppend(zbox, hbox);

	gui->entry_zoom_wid = IupText(NULL);
	IupSetAttribute(gui->entry_zoom_wid, "SIZE", "18x10");
	gui->entry_zoom_hei = IupText(NULL);
	IupSetAttribute(gui->entry_zoom_hei, "SIZE", "18x10");
	hbox = IupHbox(IupLabel("Zoom"), gui->entry_zoom_wid, IupLabel("x"),
			gui->entry_zoom_hei, NULL);
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
	IupAppend(zbox, hbox);
	
	gui->entry_width = IupText(NULL);
	IupSetAttribute(gui->entry_width, "SIZE", "24x10");
	hbox = IupHbox(IupLabel("Canvas "), gui->entry_width, NULL);
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
	IupAppend(zbox, hbox);
	return zbox;
}


static Ihandle *ezgui_page_setup_duration(EZGUI *gui)
{
	Ihandle	*hbox;
	int	i;

	hbox = xui_list_setting(&gui->dfm_list, "Find Media Duration By");
	for (i = 0; list_duration[i]; i++) {
		IupSetAttributeId(gui->dfm_list, "", i + 1, list_duration[i]);
	}
	IupSetAttribute(hbox, "NMARGIN", "16x4");
	return hbox;
}

static Ihandle *ezgui_page_setup_output(EZGUI *gui)
{
	Ihandle	*hbox1, *hbox2, *vbox;
	int	i;

	/* first display line */
	gui->fmt_transp = IupToggle("Transparent", NULL);
	hbox1 = xui_list_setting(&gui->fmt_list, "Save Picture As");
	for (i = 0; list_format[i]; i++) {
		IupSetAttributeId(gui->fmt_list, "", i+1, list_format[i]);
	}
	IupAppend(hbox1, gui->fmt_transp);
	IupSetCallback(gui->fmt_list, "ACTION",
			(Icallback) ezgui_event_setup_format);

	/* second display line */
	gui->fmt_zbox = IupZbox(IupFill(), 
			xui_text_setting(&gui->fmt_gif_fr, "FRate:", "(ms)"),
			xui_text_setting(&gui->fmt_jpg_qf, "Quality:", NULL),
			NULL);
	hbox2 = IupHbox(xui_text("", "120"), gui->fmt_zbox, NULL);
	IupSetAttribute(hbox2, "NGAP", "4");

	/* assemble the Picture Format area */
	vbox = IupVbox(hbox1, hbox2, NULL);
	IupSetAttribute(vbox, "NMARGIN", "16x4");
	IupSetAttribute(vbox, "NGAP", "4");
	return vbox;
}

static Ihandle *ezgui_page_setup_button(EZGUI *gui)
{
	Ihandle *hbox;
	
	gui->butt_setup_apply = 
		xui_button("OK", (Icallback) ezgui_event_setup_ok);

	gui->butt_setup_cancel = 
		xui_button("Cancel", (Icallback) ezgui_event_setup_cancel);

	hbox = IupHbox(IupFill(), gui->butt_setup_cancel, 
			gui->butt_setup_apply, NULL);
	return hbox;
}

static int ezgui_event_setup_format(Ihandle *ih, char *text, int i, int s)
{
	EZGUI	*gui;

	(void) i;

	/* The normal state of 's' from IUP callback should be 0 or 1.
	 * The special state EZGSINI is applied for initializing stage
	 * while the dialog hasn't been linked with the GUI object. */
	if (s == 0) {
		return IUP_DEFAULT;	/* ignore the leaving item */
	} else if (s == EZGSINI) {
		gui = (EZGUI *) ih;
	} else {
		gui = ezgui_get_global(ih);
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
	return IUP_DEFAULT;
}

static int ezgui_event_setup_grid(Ihandle *ih, char *text, int i, int s)
{
	EZGUI	*gui = ezgui_get_global(ih);
	
	(void) text;	/* stop the gcc complaining */
	//printf("ezgui_event_setup_grid: %s %d %d\n", text, i, s);
	
	if (s) {
		IupSetInt(gui->entry_zbox_grid, "VALUEPOS", i - 1);
	}
	return IUP_DEFAULT;
}

static int ezgui_event_setup_zoom(Ihandle *ih, char *text, int i, int s)
{
	EZGUI	*gui = ezgui_get_global(ih);

	(void) text;	/* stop the gcc complaining */

	if (s) {
		IupSetInt(gui->entry_zbox_zoom, "VALUEPOS", i - 1);
	}
	return IUP_DEFAULT;
}

static int ezgui_event_setup_ok(Ihandle *ih)
{
	EZGUI	*gui = ezgui_get_global(ih);
	char	*val;

	gui->grid_idx = xui_list_get_idx(gui->prof_grid);
	gui->zoom_idx = xui_list_get_idx(gui->prof_zoom);
	gui->dfm_idx  = xui_list_get_idx(gui->dfm_list);
	gui->fmt_idx  = xui_list_get_idx(gui->fmt_list);

	gui->tmp_jpg_qf = (int) strtol(
			IupGetAttribute(gui->fmt_jpg_qf, "VALUE"), NULL, 10);
	gui->tmp_gifa_fr = (int) strtol(
			IupGetAttribute(gui->fmt_gif_fr, "VALUE"), NULL, 10);

	val = IupGetAttribute(gui->fmt_transp, "VALUE");
	if (!strcmp(val, "ON")) {
		gui->sysopt->flags |= EZOP_TRANSPARENT;
	} else {
		gui->sysopt->flags &= ~EZOP_TRANSPARENT;
	}
	
	/*
	printf("SETUP: Grid=%d Zoom=%d Dur=%d Fmt=%d JPG=%d GIFA=%d Tra=%s\n",
			gui->grid_idx, gui->zoom_idx, gui->dfm_idx, 
			gui->fmt_idx, gui->tmp_jpg_qf, gui->tmp_gifa_fr, val);
	*/
	IupSetInt(gui->entry_zbox_grid, "VALUEPOS", gui->grid_idx);
	IupSetInt(gui->entry_zbox_zoom, "VALUEPOS", gui->zoom_idx);
	return IUP_DEFAULT;
}

static int ezgui_event_setup_cancel(Ihandle *ih)
{
	ezgui_page_setup_reset(ezgui_get_global(ih));
	return IUP_DEFAULT;
}

static Ihandle *xui_text(char *label, char *size)
{
	Ihandle	*ih;

	ih = IupLabel(label);
	if (size) {
		IupSetAttribute(ih, "SIZE", size);
	}
	return ih;
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

/* define the standart dropdown list for the setup page */
static Ihandle *xui_list_setting(Ihandle **xlst, char *label)
{
	Ihandle	*list, *hbox;

	list = IupList(NULL);
	IupSetAttribute(list, "SIZE", "100x12");
	IupSetAttribute(list, "DROPDOWN", "YES");

	hbox = IupHbox(xui_text(label, "120"), list, NULL);
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

static Ihandle *xui_text_setting(Ihandle **xtxt, char *label, char *ext)
{
	Ihandle	*hbox, *text;

	text = IupText(NULL);
	IupSetAttribute(text, "SIZE", "40x10");

	hbox = IupHbox(IupLabel(label), text,
			ext ? IupLabel(ext) : NULL, NULL);
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
	IupSetAttribute(hbox, "NGAP", "4");

	if (xtxt) {
		*xtxt = text;
	}
	return hbox;
}

static Ihandle *xui_button(char *prompt, Icallback ntf)
{
	Ihandle	*button;

	button = IupButton(prompt, NULL);
	IupSetAttribute(button, "SIZE", "42");
	if (ntf) {
		IupSetCallback(button, "ACTION", ntf);
	}
	return button;
}

