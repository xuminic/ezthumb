
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
static Ihandle *ezgui_page_generate_gridzoom(EZGUI *gui);
static Ihandle *ezgui_page_setup(EZGUI *gui);
static int ezgui_page_setup_reset(EZGUI *gui);
static Ihandle *ezgui_page_setup_profile(EZGUI *gui);
static Ihandle *ezgui_page_setup_duration(EZGUI *gui);
static Ihandle *ezgui_page_setup_output(EZGUI *gui);
static Ihandle *ezgui_page_setup_button(EZGUI *gui);
static int ezgui_page_setup_pic_format(EZGUI *gui, char *item);
static int ezgui_event_setup_format(Ihandle *ih, char *text, int item, int);
static int ezgui_event_setup_ok(Ihandle *ih);
static int ezgui_event_setup_cancel(Ihandle *ih);

static Ihandle *xui_text(char *label, char *size);
static Ihandle *xui_label(char *label, char *size, char *font);




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
	Ihandle	*vbox, *hbox, *sbox, *hgrid, *hbtn;

	hgrid = ezgui_page_generate_gridzoom(gui);
	gui->prog_bar = IupProgressBar();
	IupSetAttribute(gui->prog_bar, "VALUE", "0.5");
	IupSetAttribute(gui->prog_bar, "EXPAND", "HORIZONTAL");
	IupSetAttribute(gui->prog_bar, "DASHED", "YES");
	IupSetAttribute(gui->prog_bar, "SIZE", "x10");
	//IupSetAttribute(gui->prog_bar, "VISIBLE", "NO");
	hbtn = ezgui_page_generate_button(gui);
	hbox = IupHbox(hgrid, gui->prog_bar, hbtn, NULL);
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
	printf("Action %s: %d %d\n", text, item, state);
	IupSetAttribute(ih, "VALUE", NULL);
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

	gui->list_size = IupList(NULL);
	IupSetAttribute(gui->list_size, "SIZE", "50");
	IupSetAttribute(gui->list_size, "EXPAND", "VERTICAL");
	IupSetAttribute(gui->list_size, "CANFOCUS", "NO");
	IupSetAttribute(gui->list_size, "SCROLLBAR", "NO");
	vb_size = IupVbox(xui_text("Size", "50"), gui->list_size, NULL);

	gui->list_length = IupList(NULL);
	IupSetAttribute(gui->list_length, "SIZE", "48");
	IupSetAttribute(gui->list_length, "EXPAND", "VERTICAL");
	IupSetAttribute(gui->list_length, "CANFOCUS", "NO");
	IupSetAttribute(gui->list_length, "SCROLLBAR", "NO");
	vb_len = IupVbox(xui_text("Length", "48"), gui->list_length, NULL);

	gui->list_resolv = IupList(NULL);
	IupSetAttribute(gui->list_resolv, "SIZE", "50");
	IupSetAttribute(gui->list_resolv, "EXPAND", "VERTICAL");
	IupSetAttribute(gui->list_resolv, "CANFOCUS", "NO");
	IupSetAttribute(gui->list_resolv, "SCROLLBAR", "NO");
	vb_res = IupVbox(xui_text("Resolution", "50"), gui->list_resolv, NULL);
	
	gui->list_prog = IupList(NULL);
	IupSetAttribute(gui->list_prog, "SIZE", "40");
	IupSetAttribute(gui->list_prog, "SCROLLBAR", "NO");
	IupSetAttribute(gui->list_prog, "EXPAND", "VERTICAL");
	//IupSetAttribute(gui->list_prog, "CANFOCUS", "NO");
	IupSetCallback(gui->list_prog, "ACTION", (Icallback)ezgui_workarea_scroll);
	vb_prog = IupVbox(xui_text("Progress", "40"), gui->list_prog, NULL);

	hbox = IupHbox(vb_main, vb_size, vb_len, vb_res, vb_prog, NULL);
	return hbox;
}

static Ihandle *ezgui_page_generate_button(EZGUI *gui)
{
	gui->button_add = IupButton("Add", NULL);
	IupSetAttribute(gui->button_add, "SIZE", "42");
	gui->button_del  = IupButton("Remove", NULL);
	IupSetAttribute(gui->button_del, "SIZE", "42");
	gui->button_run = IupButton("Run", NULL);
	IupSetAttribute(gui->button_run, "SIZE", "42");
	return IupHbox(gui->button_add, gui->button_del, gui->button_run, NULL);
}

static Ihandle *ezgui_page_generate_gridzoom(EZGUI *gui)
{
	Ihandle	*hbox1, *hbox2, *hbox3, *hbox4, *hbox5, *hbox6, *hbox7;

	gui->entry_col1 = IupText(NULL);
	IupSetAttribute(gui->entry_col1, "SIZE", "12x10");
	gui->entry_row = IupText(NULL);
	IupSetAttribute(gui->entry_row, "SIZE", "12x10");
	hbox1 = IupHbox(IupLabel("Grid"), gui->entry_col1, 
			IupLabel("x"), gui->entry_row, NULL);
	IupSetAttribute(hbox1, "ALIGNMENT", "ACENTER");
	
	gui->entry_col2 = IupText(NULL);
	IupSetAttribute(gui->entry_col2, "SIZE", "12x10");
	gui->entry_step = IupText(NULL);
	IupSetAttribute(gui->entry_step, "SIZE", "18x10");
	hbox2 = IupHbox(IupLabel("Col"), gui->entry_col2, IupLabel("Step"),
			gui->entry_step, IupLabel("(s)"), NULL);
	IupSetAttribute(hbox2, "ALIGNMENT", "ACENTER");

	gui->entry_dss_no = IupText(NULL);
	IupSetAttribute(gui->entry_dss_no, "SIZE", "24x10");
	hbox3 = IupHbox(IupLabel("Dss No. "), gui->entry_dss_no, NULL);
	IupSetAttribute(hbox3, "ALIGNMENT", "ACENTER");

	gui->entry_dss_step = IupText(NULL);
	IupSetAttribute(gui->entry_dss_step, "SIZE", "24x10");
	hbox4 = IupHbox(IupLabel("Dss Step "), gui->entry_dss_step, 
			IupLabel("(s) "), NULL);
	IupSetAttribute(hbox4, "ALIGNMENT", "ACENTER");

	gui->entry_zbox_grid = IupZbox(IupLabel("Grid Auto"), hbox1, hbox2, 
			hbox3, hbox4, IupLabel("DSS I-Frame "), NULL);
	IupSetAttribute(gui->entry_zbox_grid, "ALIGNMENT", "ACENTER");

	gui->entry_zoom_ratio = IupText(NULL);
	IupSetAttribute(gui->entry_zoom_ratio, "SIZE", "24x11");
	IupSetAttribute(gui->entry_zoom_ratio, "SPIN", "YES");
	IupSetAttribute(gui->entry_zoom_ratio, "SPINMIN", "5");
	IupSetAttribute(gui->entry_zoom_ratio, "SPINMAX", "200");
	IupSetAttribute(gui->entry_zoom_ratio, "SPININC", "5");
	IupSetAttribute(gui->entry_zoom_ratio, "SPINALIGN", "LEFT");
	IupSetAttribute(gui->entry_zoom_ratio, "SPINVALUE", "50");
	hbox5 = IupHbox(IupLabel("Zoom"), gui->entry_zoom_ratio,
			IupLabel("%"), NULL);
	IupSetAttribute(hbox5, "ALIGNMENT", "ACENTER");

	gui->entry_zoom_wid = IupText(NULL);
	IupSetAttribute(gui->entry_zoom_wid, "SIZE", "18x10");
	gui->entry_zoom_hei = IupText(NULL);
	IupSetAttribute(gui->entry_zoom_hei, "SIZE", "18x10");
	hbox6 = IupHbox(IupLabel("Zoom"), gui->entry_zoom_wid, IupLabel("x"),
			gui->entry_zoom_hei, NULL);
	IupSetAttribute(hbox6, "ALIGNMENT", "ACENTER");
	
	gui->entry_width = IupText(NULL);
	IupSetAttribute(gui->entry_width, "SIZE", "24x10");
	hbox7 = IupHbox(IupLabel("Canvas "), gui->entry_width, NULL);
	IupSetAttribute(hbox7, "ALIGNMENT", "ACENTER");

	gui->entry_zbox_zoom = IupZbox(IupLabel("Zoom Auto"), hbox5, hbox6, 
			hbox7, NULL);
	IupSetAttribute(gui->entry_zbox_zoom, "ALIGNMENT", "ACENTER");

	IupSetInt(gui->entry_zbox_grid, "VALUEPOS", gui->grid_idx);
	IupSetInt(gui->entry_zbox_zoom, "VALUEPOS", gui->zoom_idx);
	return IupHbox(gui->entry_zbox_grid, gui->entry_zbox_zoom, NULL);
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
	if (gui->sysopt->flags & EZOP_TRANSPARENT) {
		IupSetAttribute(gui->fmt_transp, "VALUE", "ON");
	} else {
		IupSetAttribute(gui->fmt_transp, "VALUE", "OFF");
	}
	IupSetInt(gui->fmt_gif_fr, "VALUE", gui->tmp_gifa_fr);
	IupSetInt(gui->fmt_jpg_qf, "VALUE", gui->tmp_jpg_qf);
	ezgui_page_setup_pic_format(gui, list_format[gui->fmt_idx]);
	return 0;
}

static Ihandle *ezgui_page_setup_profile(EZGUI *gui)
{
	Ihandle	*hbox1, *hbox2, *vbox;
	int	i;

	gui->prof_grid = IupList(NULL);
	IupSetAttribute(gui->prof_grid, "SIZE", "100x12");
	IupSetAttribute(gui->prof_grid, "DROPDOWN", "YES");
	for (i = 0; list_grid[i]; i++) {
		IupSetAttributeId(gui->prof_grid, "", i+1, list_grid[i]);
	}

	gui->prof_zoom = IupList(NULL);
	IupSetAttribute(gui->prof_zoom, "SIZE", "100x12");
	IupSetAttribute(gui->prof_zoom, "DROPDOWN", "YES");
	for (i = 0; list_zoom[i]; i++) {
		IupSetAttributeId(gui->prof_zoom, "", i+1, list_zoom[i]);
	}

	hbox1 = IupHbox(xui_text("Grid Setting", "120"), gui->prof_grid, NULL);
	IupSetAttribute(hbox1, "ALIGNMENT", "ACENTER");
	hbox2 = IupHbox(xui_text("Zoom Setting", "120"), gui->prof_zoom, NULL);
	IupSetAttribute(hbox2, "ALIGNMENT", "ACENTER");
	vbox = IupVbox(hbox1, hbox2, NULL);
	IupSetAttribute(vbox, "NMARGIN", "16x4");
	return vbox;
}

static Ihandle *ezgui_page_setup_duration(EZGUI *gui)
{
	Ihandle	*hbox;
	int	i;


	gui->dfm_list = IupList(NULL);
	IupSetAttribute(gui->dfm_list, "SIZE", "100x12");
	IupSetAttribute(gui->dfm_list, "DROPDOWN", "YES");
	for (i = 0; list_duration[i]; i++) {
		IupSetAttributeId(gui->dfm_list, "", i+1, list_duration[i]);
	}

	hbox = IupHbox(xui_text("Find Media Duration By", "120"), 
			gui->dfm_list, NULL);
	IupSetAttribute(hbox, "NMARGIN", "16x4");
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
	return hbox;
}

static Ihandle *ezgui_page_setup_output(EZGUI *gui)
{
	Ihandle	*hb_speed, *hb_quality;
	Ihandle	*lb_dummy, *hbox1, *hbox2;
	int	i;

	/* first display line */
	gui->fmt_list = IupList(NULL);
	IupSetAttribute(gui->fmt_list, "SIZE", "100x12");
	IupSetAttribute(gui->fmt_list, "DROPDOWN", "YES");
	IupSetCallback(gui->fmt_list, "ACTION",
			(Icallback) ezgui_event_setup_format);
	for (i = 0; list_format[i]; i++) {
		IupSetAttributeId(gui->fmt_list, "", i+1, list_format[i]);
	}

	hbox1 = IupHbox(xui_text("Save Picture As", "120"), 
			gui->fmt_list, NULL);
	IupSetAttribute(hbox1, "NMARGIN", "16x4");
	IupSetAttribute(hbox1, "ALIGNMENT", "ACENTER");
	
	/* second display line */
	gui->fmt_gif_fr = IupText(NULL);
	IupSetAttribute(gui->fmt_gif_fr, "SIZE", "40x10");
	hb_speed = IupHbox(xui_text("FRate:", NULL), gui->fmt_gif_fr, NULL);
	IupSetAttribute(hb_speed, "ALIGNMENT", "ACENTER");

	gui->fmt_jpg_qf = IupText(NULL);
	IupSetAttribute(gui->fmt_jpg_qf, "SIZE", "40x10");
	hb_quality = IupHbox(xui_text("Quality:", NULL), 
			gui->fmt_jpg_qf, NULL);
	IupSetAttribute(hb_quality, "ALIGNMENT", "ACENTER");

	gui->fmt_transp = IupToggle("Transparent", NULL);

	gui->fmt_zbox = IupZbox(IupFill(), hb_speed, hb_quality, NULL);
	IupSetInt(gui->fmt_zbox, "VALUEPOS", 0);

	lb_dummy = xui_text("dummy", "120");
	IupSetAttribute(lb_dummy, "VISIBLE", "NO");

	hbox2 = IupHbox(lb_dummy, gui->fmt_zbox, gui->fmt_transp, NULL);
	IupSetAttribute(hbox2, "NMARGIN", "16x4");
	IupSetAttribute(hbox2, "ALIGNMENT", "ACENTER");
	IupSetAttribute(hbox2, "GAP", "4");

	return IupVbox(hbox1, hbox2, NULL);
}

static Ihandle *ezgui_page_setup_button(EZGUI *gui)
{
	Ihandle *hbox;
	
	gui->butt_setup_apply = IupButton("OK", NULL);
	IupSetAttribute(gui->butt_setup_apply, "SIZE", "42");
	IupSetCallback(gui->butt_setup_cancel, "ACTION",
			(Icallback) ezgui_event_setup_ok);

	gui->butt_setup_cancel = IupButton("Cancel", NULL);
	IupSetAttribute(gui->butt_setup_cancel, "SIZE", "42");
	IupSetCallback(gui->butt_setup_cancel, "ACTION",
			(Icallback) ezgui_event_setup_cancel);
	hbox = IupHbox(IupFill(), gui->butt_setup_cancel, 
			gui->butt_setup_apply, NULL);
	return hbox;
}

static int ezgui_page_setup_pic_format(EZGUI *gui, char *item)
{
	/* hide the transparent toggle and quality editboxes */
	IupSetAttribute(gui->fmt_transp, "VISIBLE", "NO");
	IupSetInt(gui->fmt_zbox, "VALUEPOS", 0);

	if (!strcmp(item, CFG_PIC_FMT_JPEG)) {
		IupSetInt(gui->fmt_zbox, "VALUEPOS", 2);
	} else {
		IupSetAttribute(gui->fmt_transp, "VISIBLE", "YES");
		if (!strcmp(item, CFG_PIC_FMT_GIFA)) {
			IupSetInt(gui->fmt_zbox, "VALUEPOS", 1);
		}
	}	
	return IUP_DEFAULT;
}

static int ezgui_event_setup_format(Ihandle *ih, char *text, int item, int state)
{
	return ezgui_page_setup_pic_format(ezgui_get_global(ih), text);
}

static int ezgui_event_setup_ok(Ihandle *ih)
{
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


