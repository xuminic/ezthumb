
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

static int ezgui_create_window(EZGUI *gui);
static Ihandle *ezgui_page_generate(EZGUI *gui);
static Ihandle *ezgui_page_generate_workarea(EZGUI *gui);
static Ihandle *ezgui_page_generate_button(EZGUI *gui);
static Ihandle *ezgui_page_generate_gridzoom(EZGUI *gui);
static Ihandle *ezgui_page_setup(EZGUI *gui);
static Ihandle *ezgui_page_setup_profile(EZGUI *gui);
static Ihandle *ezgui_page_setup_duration(EZGUI *gui);
static Ihandle *ezgui_page_setup_output(EZGUI *gui);
static Ihandle *ezgui_page_setup_button(EZGUI *gui);


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
	ezgui_create_window(gui);

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


static int ezgui_create_window(EZGUI *gui)
{
	Ihandle		*tabs, *vb, *dlg;

	tabs = IupTabs(ezgui_page_generate(gui), 
			ezgui_page_setup(gui), IupVbox(IupFill(), NULL), NULL);
	IupSetAttribute(tabs, "TABTITLE0", "Generate");
	IupSetAttribute(tabs, "TABTITLE1", " Setup  ");
	IupSetAttribute(tabs, "TABTITLE2", "Advanced");
	IupSetAttribute(tabs, "PADDING", "4");

	dlg = IupDialog(tabs);
	IupSetAttribute(dlg, "TITLE", "Ezthumb");
	IupShow(dlg);
	return 0;
}


static Ihandle *ezgui_page_generate(EZGUI *gui)
{
	Ihandle	*vbox, *hbox, *hgrid, *hprog, *hbtn;

	hgrid = ezgui_page_generate_gridzoom(gui);
	hprog = IupProgressBar();
	IupSetAttribute(hprog, "VALUE", "0.5");
	IupSetAttribute(hprog, "EXPAND", "HORIZONTAL");
	IupSetAttribute(hprog, "DASHED", "YES");
	IupSetAttribute(hprog, "RASTERSIZE", "x10");
	hbtn = ezgui_page_generate_button(gui);
	hbox = IupHbox(hgrid, hprog, hbtn, NULL);
	IupSetAttribute(hbox, "NGAP", "10");
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");

	vbox = IupVbox(ezgui_page_generate_workarea(gui), hbox, NULL);
	IupSetAttribute(vbox, "NGAP", "4");
	IupSetAttribute(vbox, "NMARGIN", "4x4");
	return vbox;
}

static Ihandle *ezgui_page_generate_workarea(EZGUI *gui)
{
	Ihandle	*lb_main, *vb_main, *lst_main, *lb_size, *vb_size, *lst_size;
	Ihandle *lb_len, *vb_len, *lst_len, *lb_res, *vb_res, *lst_res;
	Ihandle	*lb_prog, *vb_prog, *lst_prog, *hbox;

	lb_main = IupLabel("Files");
	IupSetAttribute(lb_main, "RASTERSIZE", "200");
	lst_main = IupList(NULL);
	IupSetAttribute(lst_main, "EXPAND", "YES");
	vb_main = IupVbox(lb_main, lst_main, NULL);

	lb_size = IupLabel("Size");
	IupSetAttribute(lb_size, "RASTERSIZE", "100");
	lst_size = IupList(NULL);
	IupSetAttribute(lst_size, "RASTERSIZE", "100");
	IupSetAttribute(lst_size, "EXPAND", "VERTICAL");
	vb_size = IupVbox(lb_size, lst_size, NULL);

	lb_len = IupLabel("Length");
	IupSetAttribute(lb_len, "RASTERSIZE", "100");
	lst_len = IupList(NULL);
	IupSetAttribute(lst_len, "RASTERSIZE", "100");
	IupSetAttribute(lst_len, "EXPAND", "VERTICAL");
	vb_len = IupVbox(lb_len, lst_len, NULL);

	lb_res = IupLabel("Resolution");
	IupSetAttribute(lb_res, "RASTERSIZE", "100");
	lst_res = IupList(NULL);
	IupSetAttribute(lst_res, "RASTERSIZE", "100");
	IupSetAttribute(lst_res, "EXPAND", "VERTICAL");
	vb_res = IupVbox(lb_res, lst_res, NULL);
	
	lb_prog = IupLabel("Progress");
	IupSetAttribute(lb_prog, "RASTERSIZE", "100");
	lst_prog = IupList(NULL);
	IupSetAttribute(lst_prog, "RASTERSIZE", "100");
	IupSetAttribute(lst_prog, "SCROLLBAR", "YES");
	IupSetAttribute(lst_prog, "EXPAND", "VERTICAL");
	vb_prog = IupVbox(lb_prog, lst_prog, NULL);

	hbox = IupHbox(vb_main, vb_size, vb_len, vb_res, vb_prog, NULL);
	return hbox;
}

static Ihandle *ezgui_page_generate_button(EZGUI *gui)
{
	Ihandle	*hbox, *btn_add, *btn_rm, *btn_run;

	btn_add = IupButton("Add", NULL);
	IupSetAttribute(btn_add, "RASTERSIZE", "80");
	btn_rm  = IupButton("Remove", NULL);
	IupSetAttribute(btn_rm, "RASTERSIZE", "80");
	btn_run = IupButton("Run", NULL);
	IupSetAttribute(btn_run, "RASTERSIZE", "80");
	hbox = IupHbox(btn_add, btn_rm, btn_run, NULL);
	return hbox;
}

static Ihandle *ezgui_page_generate_gridzoom(EZGUI *gui)
{
	Ihandle	*hbox, *lb1, *lb2;

	lb1 = IupLabel("Grid Auto");
	lb2 = IupLabel("Zoom Auto");
	hbox = IupHbox(lb1, lb2, NULL);
	return hbox;
}

static Ihandle *ezgui_page_setup(EZGUI *gui)
{
	Ihandle	*lb_prof, *hb_prof, *lb_dur, *hb_dur, *lb_ext, *hb_ext;
	Ihandle	*hb_butt, *vbox;

	/* the label of Profile Selection */
	lb_prof = IupLabel("Profile Selection");
	IupSetAttribute(lb_prof, "FONTSTYLE", "Bold");

	hb_prof = ezgui_page_setup_profile(gui);

	/* the label of Duration finding */
	lb_dur = IupLabel("Duration Finding Mode");
	IupSetAttribute(lb_dur, "FONTSTYLE", "Bold");

	hb_dur = ezgui_page_setup_duration(gui);

	/* the label of output file */
	lb_ext = IupLabel("Output File Format");
	IupSetAttribute(lb_ext, "FONTSTYLE", "Bold");
	hb_ext = ezgui_page_setup_output(gui);
	
	hb_butt = ezgui_page_setup_button(gui);

	vbox = IupVbox(lb_prof, hb_prof, lb_dur, hb_dur, lb_ext, hb_ext, 
			IupFill(), hb_butt, NULL);
	IupSetAttribute(vbox, "NGAP", "4");
	IupSetAttribute(vbox, "NMARGIN", "4x4");
	return vbox;
}

static Ihandle *ezgui_page_setup_profile(EZGUI *gui)
{
	Ihandle	*hbox, *lb_grid, *lb_zoom, *lst_grid, *lst_zoom;

	lb_grid = IupLabel("Grid Setting");
	IupSetAttribute(lb_grid, "RASTERSIZE", "120");
	IupSetAttribute(lb_grid, "PADDING", "10");

	lst_grid = IupList(NULL);
	IupSetAttribute(lst_grid, "DROPDOWN", "YES");
	IupSetAttribute(lst_grid, "VALUE", "1");
	IupSetAttribute(lst_grid, "1", "auto");
	IupSetAttribute(lst_grid, "2", "column and row");
	IupSetAttribute(lst_grid, "3", "column and step");
	IupSetAttribute(lst_grid, "4", "discrete by number");
	IupSetAttribute(lst_grid, "5", "discrete by step");
	IupSetAttribute(lst_grid, "6", "discrete key frames");

	lb_zoom = IupLabel("Zoom Setting");
	IupSetAttribute(lb_zoom, "RASTERSIZE", "120");
	IupSetAttribute(lb_zoom, "PADDING", "10");

	lst_zoom = IupList(NULL);
	IupSetAttribute(lst_zoom, "DROPDOWN", "YES");
	IupSetAttribute(lst_zoom, "VALUE", "1");
	IupSetAttribute(lst_zoom, "1", "auto");
	IupSetAttribute(lst_zoom, "2", "zoom by ratio");
	IupSetAttribute(lst_zoom, "3", "zoom by size");
	IupSetAttribute(lst_zoom, "4", "zoom by canvas");

	hbox = IupHbox(lb_grid, lst_grid, lb_zoom, lst_zoom, NULL);
	IupSetAttribute(hbox, "NGAP", "4");
	IupSetAttribute(hbox, "NMARGIN", "4x4");
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
	return hbox;
}

static Ihandle *ezgui_page_setup_duration(EZGUI *gui)
{
	Ihandle	*hbox, *ra_dur_auto, *ra_dur_head, *ra_dur_fast, *ra_dur_scan;

	ra_dur_auto = IupToggle("Auto", NULL);
	ra_dur_head = IupToggle("File Head", NULL);
	ra_dur_fast = IupToggle("Fast Scan", NULL);
	ra_dur_scan = IupToggle("Full Scan", NULL);

	hbox = IupHbox(ra_dur_auto, ra_dur_head, ra_dur_fast, ra_dur_scan, NULL);
	IupSetAttribute(hbox, "NGAP", "4");
	IupSetAttribute(hbox, "NMARGIN", "4x4");
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
	return IupRadio(hbox);
}

static Ihandle *ezgui_page_setup_output(EZGUI *gui)
{
	Ihandle	*ra_ext_png, *ra_ext_gif, *ra_ext_agif, *ra_ext_jpg, *vb_ext;
	Ihandle	*lb_speed, *lb_quality, *vb_lbext;
	Ihandle	*tg_transp, *txt_speed, *txt_quality, *vb_opt, *hbox;

	ra_ext_png = IupToggle("PNG", NULL);
	ra_ext_gif = IupToggle("GIF", NULL);
	ra_ext_agif = IupToggle("Animated GIF", NULL);
	ra_ext_jpg = IupToggle("JPEG", NULL);
	vb_ext = IupVbox(ra_ext_png, ra_ext_gif, ra_ext_agif, ra_ext_jpg, 
			IupLabel(""), NULL);

	lb_speed = IupLabel("Speed:");
	lb_quality = IupLabel("Quality:");
	vb_lbext = IupVbox(IupLabel(""), IupLabel(""), lb_speed, lb_quality, 
			IupLabel(""), NULL);

	tg_transp = IupToggle("Transparent", NULL);
	txt_speed = IupText(NULL);
	IupSetAttribute(txt_speed, "VALUE", "1000");
	txt_quality = IupText(NULL);
	IupSetAttribute(txt_quality, "VALUE", "85");
	vb_opt = IupVbox(IupLabel(""), IupLabel(""), txt_speed, txt_quality, 
			tg_transp, NULL);

	hbox = IupHbox(IupRadio(vb_ext), vb_lbext, vb_opt, NULL);
	IupSetAttribute(hbox, "NGAP", "4");
	IupSetAttribute(hbox, "NMARGIN", "4x4");
	IupSetAttribute(hbox, "ALIGNMENT", "ACENTER");
	return hbox;
}

static Ihandle *ezgui_page_setup_button(EZGUI *gui)
{
	Ihandle *bt_ok, *bt_cancel, *hbox;
	
	bt_ok = IupButton("OK", NULL);
	IupSetAttribute(bt_ok, "RASTERSIZE", "60");
	bt_cancel = IupButton("Cancel", NULL);
	IupSetAttribute(bt_cancel, "RASTERSIZE", "60");
	hbox = IupHbox(IupFill(), bt_cancel, bt_ok, NULL);
	return hbox;
}

