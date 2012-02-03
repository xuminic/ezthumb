
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

#include "ezgui.h"
#include "ezicon.h"

static	char	*prof_list_grid[] = {
	CFG_PIC_AUTO, CFG_PIC_GRID_DIM, CFG_PIC_GRID_STEP, 
	CFG_PIC_DIS_NUM, CFG_PIC_DIS_STEP, CFG_PIC_DIS_KEY, NULL
};

static	char	*prof_list_zoom[] = {
	CFG_PIC_AUTO, CFG_PIC_ZOOM_RATIO, CFG_PIC_ZOOM_DEFINE,
	CFG_PIC_ZOOM_SCREEN, NULL
};

static int ezgui_option_save(EZGUI *gui);
static int ezgui_ezthumb(EZGUI *gui);
static int ezgui_commit(EZGUI *gui, GtkTreeModel *model, GtkTreeIter *iter);
static int ezgui_notificate(void *v, int eid, long param, long opt, void *b);

static int ezgui_create_window(EZGUI *gui);
static void ezgui_signal_win_state(GtkWidget *widget, GdkEvent *, EZGUI *);
static void ezgui_signal_resize(GtkWidget *parent, GdkRectangle *, EZGUI *);

static GtkWidget *ezgui_page_main_create(EZGUI *gui);
static GtkWidget *ezgui_page_main_profile(EZGUI *gui);
static int ezgui_page_main_read(EZGUI *gui);
static GtkWidget *ezgui_page_main_listview_create(EZGUI *gui);
static EZADD *ezgui_page_main_listview_open(GtkWidget *view);
static int ezgui_page_main_listview_close(GtkWidget *view, EZADD *ezadd);
static int ezgui_page_main_listview_append(EZGUI *gui, EZADD *ezadd, char *s);
static void ezgui_page_main_dialog_invalid_files(EZADD *ezadd);
static void ezgui_signal_file_choose(void *parent, EZGUI *gui);
static void ezgui_signal_file_remove(void *parent, EZGUI *gui);
static void ezgui_signal_select_change(GtkTreeSelection *tsel, EZGUI *gui);
static void ezgui_signal_select_dragdrop(GtkWidget *view, GdkDragContext *,
		int x, int y, GtkSelectionData *seldata, 
		guint info, guint time, EZGUI *gui);

static GtkWidget *ezgui_page_setup_create(EZGUI *gui);
static int ezgui_page_setup_output_format(EZGUI *gui, GtkWidget *table, int);
static int ezgui_signal_setup_sensible(void *parent, EZGUI *gui);
static int ezgui_signal_setup_insensible(void *parent, EZGUI *gui);
static int ezgui_signal_setup_reset(void *parent, EZGUI *gui);
static int ezgui_signal_setup_update(void *parent, EZGUI *gui);
static int ezgui_signal_setup_format(void *parent, EZGUI *gui);

static EZCFG *ezgui_cfg_create(void);
static int ezgui_cfg_free(EZCFG *cfg);
static char *ezgui_cfg_read(EZCFG *cfg, char *key);
static int ezgui_cfg_write(EZCFG *cfg, char *key, char *s);
static int ezgui_cfg_read_int(EZCFG *cfg, char *key, int def);
static int ezgui_cfg_write_int(EZCFG *cfg, char *key, int val);
static int ezgui_cfg_flush(EZCFG *cfg);

static GtkWidget *ezgui_pack_forward(GtkWidget *box, ...);
static GtkWidget *ezgui_pack_backward(GtkWidget *box, ...);
static GtkWidget *ezgui_pack_fill(GtkWidget *box, ...);
static GtkWidget *ezgui_entry_box(int val, int dw);
static int ezgui_entry_get_int(GtkWidget *entry);
static int ezgui_entry_set_int(GtkWidget *entry, int val);
static GtkWidget *ezgui_page_label(gchar *s);
static GtkWidget *ezgui_setup_label(gchar *s);
static GtkWidget *ezgui_setup_idname(gchar *s);
static GtkWidget *ezgui_button(EZGUI *gui, gchar *s, GCallback c_handler);
static GtkWidget *ezgui_combo(EZGUI *gui, char **sopt, GCallback c_handler);
static GtkWidget *ezgui_rabutton(EZGUI *gui, GtkWidget *prev, gchar *s, 
		GCallback c_handler);
static int ezgui_format_reset(EZGUI *gui, int rwcfg);
static void table_insert(GtkWidget *table, GtkWidget *w, int x, int y);
static void table_fill(GtkWidget *table, GtkWidget *w, int x, int y, int n);

extern int para_get_format(char *arg, char *fmt, int flen);


EZGUI *ezgui_init(EZOPT *ezopt, int *argcs, char ***argvs)
{
	EZGUI	*gui;
	char	*p;

	gtk_init(argcs, argvs);

	if ((gui = calloc(sizeof(EZGUI), 1)) == NULL) {
		return NULL;
	}
	if ((gui->config = ezgui_cfg_create()) == NULL) {
		free(gui);
		return NULL;
	}
	gui->sysopt = ezopt;
	
	/* setup the simple profile */
	if ((p = ezgui_cfg_read(gui->config, CFG_KEY_PROF_SIMPLE))) {
		ezopt_profile_setup(ezopt, p);
	} else {
		p = ezopt_profile_export(ezopt);
		ezgui_cfg_write(gui->config, CFG_KEY_PROF_SIMPLE, p);
	}
	free(p);

	/* setup the grid profile */
	if ((p = ezgui_cfg_read(gui->config, CFG_KEY_GRID)) == NULL) {
		ezgui_cfg_write(gui->config, CFG_KEY_GRID, CFG_PIC_AUTO);
	} else {
		free(p);
	}
	/* setup the zoom profile */
	if ((p = ezgui_cfg_read(gui->config, CFG_KEY_ZOOM)) == NULL) {
		ezgui_cfg_write(gui->config, CFG_KEY_ZOOM, CFG_PIC_AUTO);
	} else {
		free(p);
	}
	
	gui->w_width  = ezgui_cfg_read_int(gui->config, 
			CFG_KEY_WIN_WIDTH, 800);
	gui->w_height = ezgui_cfg_read_int(gui->config, 
			CFG_KEY_WIN_HEIGHT, 480);
	gui->gw_win_state = ezgui_cfg_read_int(gui->config,
			CFG_KEY_WINDOWSTATE, 0);

	ezopt->grid_col = ezgui_cfg_read_int(gui->config, 
			CFG_KEY_GRID_COLUMN, ezopt->grid_col);
	ezopt->grid_row = ezgui_cfg_read_int(gui->config,
			CFG_KEY_GRID_ROW, ezopt->grid_row);
	ezopt->tm_step = ezgui_cfg_read_int(gui->config,
			CFG_KEY_TIME_STEP, ezopt->tm_step / 1000) * 1000;
	ezopt->tn_facto = ezgui_cfg_read_int(gui->config,
			CFG_KEY_ZOOM_RATIO, ezopt->tn_facto);
	ezopt->tn_width = ezgui_cfg_read_int(gui->config,
			CFG_KEY_ZOOM_WIDTH, ezopt->tn_width);
	ezopt->tn_height = ezgui_cfg_read_int(gui->config,
			CFG_KEY_ZOOM_HEIGHT, ezopt->tn_height);
	ezopt->canvas_width = ezgui_cfg_read_int(gui->config,
			CFG_KEY_CANVAS_WIDTH, ezopt->canvas_width);
	ezopt->dur_mode = ezgui_cfg_read_int(gui->config,
			CFG_KEY_DURATION, ezopt->dur_mode);

	ezgui_format_reset(gui, EZUI_FMR_RDWR);
	return gui;
}

int ezgui_run(EZGUI *gui, char *flist[], int fnum)
{
	EZADD	*ezadd;
	int	i;

	ezgui_create_window(gui);

	/* append command line file names */
	ezadd = ezgui_page_main_listview_open(gui->gw_listview);
	for (i = 0; i < fnum; i++) {
		ezgui_page_main_listview_append(gui, ezadd, flist[i]);
	}
	ezgui_page_main_listview_close(gui->gw_listview, ezadd);

	/* find out if the list is empty */
	if (gui->list_count) {
		gtk_widget_set_sensitive(gui->button_run, TRUE);
	} else {
		gtk_widget_set_sensitive(gui->button_run, FALSE);
	}

	/* update the configure file because some options may be changed
	 * by command line options */
	ezgui_option_save(gui);
	
	gtk_widget_show_all(gui->gw_main);
	gtk_main();
	return 0;
}

int ezgui_close(EZGUI *gui)
{
	if (gui) {
		ezgui_option_save(gui);
		ezgui_cfg_free(gui->config);
		free(gui);
	}
	return 0;
}


static int ezgui_option_save(EZGUI *gui)
{
	EZOPT	*opt = gui->sysopt;
	char	tmp[32];

	ezgui_cfg_write_int(gui->config,
			CFG_KEY_WIN_WIDTH, gui->w_width);
	ezgui_cfg_write_int(gui->config,
			CFG_KEY_WIN_HEIGHT, gui->w_height);
	ezgui_cfg_write_int(gui->config,
			CFG_KEY_WINDOWSTATE, gui->gw_win_state);

	ezgui_cfg_write_int(gui->config, 
			CFG_KEY_GRID_COLUMN, opt->grid_col);
	ezgui_cfg_write_int(gui->config, 
			CFG_KEY_GRID_ROW, opt->grid_row);
	ezgui_cfg_write_int(gui->config,
			CFG_KEY_TIME_STEP, opt->tm_step / 1000);
	ezgui_cfg_write_int(gui->config,
			CFG_KEY_ZOOM_RATIO, opt->tn_facto);
	ezgui_cfg_write_int(gui->config,
			CFG_KEY_ZOOM_WIDTH, opt->tn_width);
	ezgui_cfg_write_int(gui->config,
			CFG_KEY_ZOOM_HEIGHT, opt->tn_height);
	ezgui_cfg_write_int(gui->config,
			CFG_KEY_CANVAS_WIDTH, opt->canvas_width);
	ezgui_cfg_write_int(gui->config,
			CFG_KEY_DURATION, opt->dur_mode);

	sprintf(tmp, "%s@%d", opt->img_format, opt->img_quality);
	ezgui_cfg_write(gui->config, CFG_KEY_FILE_FORMAT, tmp);

	ezgui_cfg_write(gui->config, CFG_KEY_TRANSPARENCY,
			opt->flags & EZOP_TRANSPARENT ? "yes" : "no");

	/* save to the configure file */
	ezgui_cfg_flush(gui->config);
	return 0;
}

static int ezgui_ezthumb(EZGUI *gui)
{
	GtkTreeSelection	*tsel;
	GtkTreeModel		*model;
	GtkTreeIter		iter;
	GList			*slist, *node;

	/* bind the notification function to GUI mode */
	gui->sysopt->notify = ezgui_notificate;
	/* read options from the GUI widgets */
	ezgui_page_main_read(gui);
	/* save these options into the configure file */
	ezgui_option_save(gui);

	tsel  = gtk_tree_view_get_selection(GTK_TREE_VIEW(gui->gw_listview));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gui->gw_listview));

	if (gtk_tree_selection_count_selected_rows(tsel) == 0) {
		/* no selection; process the whole list */
		if (gtk_tree_model_get_iter_first(model, &iter)) {
			do {
				ezgui_commit(gui, model, &iter);
			} while (gtk_tree_model_iter_next(model, &iter));
		}
		return 0;
	}

	slist = gtk_tree_selection_get_selected_rows(tsel, &model);
	for (node = slist; node; node = g_list_next(node)) {
		if (gtk_tree_model_get_iter(model, &iter, node->data)) {
			ezgui_commit(gui, model, &iter);
		}
		gtk_tree_path_free(node->data);
	}
	g_list_free(slist);
	return 0;
}

static int ezgui_commit(EZGUI *gui, GtkTreeModel *model, GtkTreeIter *iter)
{
	char	*s;

	/*
	GtkTreePath	*path;
	int		*idx;
	path = gtk_tree_model_get_path(model, iter);
	idx = gtk_tree_path_get_indices(path);
	gui->list_index = *idx;
	gtk_tree_path_free(path);
	printf("process %d\n", gui->list_index);
	*/
	gui->list_model = model;
	gui->list_iter  = iter;
	gtk_tree_model_get(model, iter, EZUI_COL_NAME, &s, -1);
	ezthumb(s, gui->sysopt);
	g_free(s);
	return 0;
}

static int ezgui_notificate(void *v, int eid, long param, long opt, void *b)
{
	EZGUI	*gui = ((EZVID*) v)->sysopt->gui;
	int	val;

	switch (eid) {
	case EN_PROC_BEGIN:
	case EN_PROC_END:
		break;

	case EN_PROC_CURRENT:
		if (param == 0) {	/* i-frame ripping */
			val = 100;
		} else {
			val = opt * 100 / param;
		}

		gtk_list_store_set(GTK_LIST_STORE(gui->list_model), 
				gui->list_iter, EZUI_COL_PROGRESS, val, -1);

		while (gtk_events_pending()) {
			gtk_main_iteration();
		}
		break;

	default:
		return EN_EVENT_PASSTHROUGH;
	}
	return eid;
}

static int ezgui_create_window(EZGUI *gui)
{
	GtkWidget	*page_main, *page_setup;
	GdkPixbuf	*icon;

	/* Create the pages of notebook */
	page_main = ezgui_page_main_create(gui);
	page_setup = ezgui_page_setup_create(gui);

	/* Create a new notebook, place the position of the tabs */
	gui->gw_page = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(gui->gw_page), GTK_POS_TOP);
	gtk_notebook_append_page(GTK_NOTEBOOK(gui->gw_page), page_main, 
			ezgui_page_label("Generate"));
	gtk_notebook_append_page(GTK_NOTEBOOK(gui->gw_page), page_setup, 
			ezgui_page_label("Setup"));

	gtk_notebook_set_current_page(GTK_NOTEBOOK(gui->gw_page), 0);

	/* create the top level window */
	gui->gw_main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(gui->gw_main), 
			gui->w_width, gui->w_height);
	gtk_container_set_border_width(GTK_CONTAINER(gui->gw_main), 10);
	if (gui->gw_win_state & GDK_WINDOW_STATE_ICONIFIED) {
		gtk_window_iconify(GTK_WINDOW(gui->gw_main));
	}
	if (gui->gw_win_state & GDK_WINDOW_STATE_MAXIMIZED) {
		gtk_window_maximize(GTK_WINDOW(gui->gw_main));
	}
	if (gui->gw_win_state & GDK_WINDOW_STATE_FULLSCREEN) {
		gtk_window_fullscreen(GTK_WINDOW(gui->gw_main));
	}
	icon = gdk_pixbuf_new_from_inline(-1, ezicon_pixbuf, FALSE, NULL);
	gtk_window_set_icon(GTK_WINDOW(gui->gw_main), icon);

	g_signal_connect(gui->gw_main, "delete_event", gtk_main_quit, NULL);
	g_signal_connect(gui->gw_main, "size-allocate",
			G_CALLBACK(ezgui_signal_resize), gui);
	g_signal_connect(gui->gw_main, "window-state-event",
			G_CALLBACK(ezgui_signal_win_state), gui);
	gtk_container_add(GTK_CONTAINER(gui->gw_main), gui->gw_page);
	return 0;
}

static void ezgui_signal_win_state(GtkWidget *widget, 
		GdkEvent *event, EZGUI *gui)
{
	//printf("Envent %d\n", event->type);
	if (event->type == GDK_WINDOW_STATE) {
		gui->gw_win_state = event->window_state.new_window_state;
	}
}

static void ezgui_signal_resize(GtkWidget *parent, 
		GdkRectangle *rect, EZGUI *gui)
{
	if (gui->gw_win_state & (GDK_WINDOW_STATE_ICONIFIED | 
				GDK_WINDOW_STATE_MAXIMIZED | 
				GDK_WINDOW_STATE_FULLSCREEN)) {
		return;
	}

	//printf("X=%d Y=%d Width=%d Height=%d\n",
	//		rect->x, rect->y, rect->width, rect->height);
	gui->w_width  = rect->width;
	gui->w_height = rect->height;
}

static GtkWidget *ezgui_page_main_create(EZGUI *gui)
{
	GtkWidget	*scroll;
	GtkWidget	*button_add;
	GtkWidget	*vbox;

	/* create the listview */
	gui->gw_listview = ezgui_page_main_listview_create(gui);

	/* create the scrollbars and stuffed with the listview */
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_container_add(GTK_CONTAINER(scroll), gui->gw_listview);

	/* create the buttons */
	button_add = ezgui_button(gui, "Add", 
			G_CALLBACK(ezgui_signal_file_choose));
	gui->button_del = ezgui_button(gui, "Remove",
			G_CALLBACK(ezgui_signal_file_remove));
	gui->button_run = ezgui_button(gui, "Start", NULL);
	g_signal_connect_swapped(gui->button_run, "clicked",
			G_CALLBACK(ezgui_ezthumb), gui);

	gtk_widget_set_sensitive(gui->button_del, FALSE);
	gtk_widget_set_sensitive(gui->button_run, FALSE);

	/* create left side */
	gui->prof_group = ezgui_page_main_profile(gui);

	/* create the horizontal box and stuffed with the buttons */
	gui->button_box = gtk_hbox_new(FALSE, 0);
	ezgui_pack_backward(gui->button_box, gui->button_run, gui->button_del,
			button_add, NULL);
	ezgui_pack_forward(gui->button_box, gui->prof_group, NULL);

	/* create the vertical box and stuffed with all above */
	vbox = gtk_vbox_new(FALSE, 0);
	ezgui_pack_fill(vbox, scroll, NULL);
	ezgui_pack_forward(vbox, gui->button_box, NULL);
	return vbox;
}

/* Grid: (Grid Auto)(Grid 4x4)(Grid 4 Step 15)(DC No. 20)(DC Step 15)(DC I-Frame)
 * Zoom: (Zoom Auto)(Zoom 50%)(Zoom 320x240)(Res 1024) */
static GtkWidget *ezgui_page_main_profile(EZGUI *gui)
{
	GtkWidget	*hbox;
	GtkAdjustment	*adjust;
	int		val;
	char		*pic;

	hbox = gtk_hbox_new(FALSE, 0);

	pic = ezgui_cfg_read(gui->config, CFG_KEY_GRID);
	if (!strcmp(pic, CFG_PIC_GRID_DIM)) {
		gui->entry_col = ezgui_entry_box(gui->sysopt->grid_col, 3);
		gui->entry_row = ezgui_entry_box(gui->sysopt->grid_row, 3);
		ezgui_pack_forward(hbox, 
				gtk_label_new("Grid"), gui->entry_col, 
				gtk_label_new("x"), gui->entry_row, NULL);
	} else if (!strcmp(pic, CFG_PIC_GRID_STEP)) {
		gui->entry_col = ezgui_entry_box(gui->sysopt->grid_col, 3);
		gui->entry_step = 
			ezgui_entry_box(gui->sysopt->tm_step / 1000, 5);
		ezgui_pack_forward(hbox, 
				gtk_label_new("Grid"), gui->entry_col,
				gtk_label_new("Step"), gui->entry_step, NULL);
	} else if (!strcmp(pic, CFG_PIC_DIS_NUM)) {
		gui->entry_row = ezgui_entry_box(gui->sysopt->grid_row, 3);
		ezgui_pack_forward(hbox, 
				gtk_label_new("DSS No "), gui->entry_row,
				NULL);
	} else if (!strcmp(pic, CFG_PIC_DIS_STEP)) {
		gui->entry_step = 
			ezgui_entry_box(gui->sysopt->tm_step / 1000, 5);
		ezgui_pack_forward(hbox, 
				gtk_label_new("DSS Step "), gui->entry_step, 
				NULL);
	} else if (!strcmp(pic, CFG_PIC_DIS_KEY)) {
		ezgui_pack_forward(hbox, gtk_label_new("DSS I-Frame"), NULL);
	} else {
		ezgui_cfg_write(gui->config, CFG_KEY_GRID, CFG_PIC_AUTO);
		ezgui_pack_forward(hbox, gtk_label_new("Grid Auto"), NULL);
	}
	g_free(pic);

	pic = ezgui_cfg_read(gui->config, CFG_KEY_ZOOM);
	if (!strcmp(pic, CFG_PIC_ZOOM_RATIO)) {
		val = ezgui_cfg_read_int(gui->config, CFG_KEY_ZOOM_RATIO, 
				gui->sysopt->tn_facto);
		adjust = (GtkAdjustment *) gtk_adjustment_new(val, 5.0, 
				200.0, 5.0, 25.0, 0.0);
		gui->entry_zoom_ratio = gtk_spin_button_new(adjust, 0, 0);
		gtk_widget_set_size_request(gui->entry_zoom_ratio, 50, -1);

		ezgui_pack_forward(hbox, 
				gtk_label_new("  Zoom"), 
				gui->entry_zoom_ratio, 
				gtk_label_new("%"), NULL);
	} else if (!strcmp(pic, CFG_PIC_ZOOM_DEFINE)) {
		gui->entry_zoom_wid = 
			ezgui_entry_box(gui->sysopt->tn_width, 5);
		gui->entry_zoom_hei = 
			ezgui_entry_box(gui->sysopt->tn_height, 5);
		ezgui_pack_forward(hbox, 
				gtk_label_new("  Zoom"), gui->entry_zoom_wid,
				gtk_label_new("x"), gui->entry_zoom_hei, NULL);
	} else if (!strcmp(pic, CFG_PIC_ZOOM_SCREEN)) {
		gui->entry_width = 
			ezgui_entry_box(gui->sysopt->canvas_width, 5);
		ezgui_pack_forward(hbox, gtk_label_new("  Res"), 
				gui->entry_width, NULL);
	} else {
		ezgui_cfg_write(gui->config, CFG_KEY_ZOOM, CFG_PIC_AUTO);
		ezgui_pack_forward(hbox, gtk_label_new("  Zoom Auto"), NULL);
	}
	g_free(pic);
	return hbox;
}

static int ezgui_page_main_read(EZGUI *gui)
{
	EZOPT	*opt = gui->sysopt;
	char	*pic;

	/* update the grid parameters */
	if ((pic = ezgui_cfg_read(gui->config, CFG_KEY_GRID)) == NULL) {
		/* do nothing as default setting */
	} else if (!strcmp(pic, CFG_PIC_GRID_DIM)) {
		opt->grid_col = ezgui_entry_get_int(gui->entry_col);
		opt->grid_row = ezgui_entry_get_int(gui->entry_row);
		opt->pro_grid = NULL;	/* disable the automatic profile */
	} else if (!strcmp(pic, CFG_PIC_GRID_STEP)) {
		opt->grid_col = ezgui_entry_get_int(gui->entry_col);
		opt->tm_step  = ezgui_entry_get_int(gui->entry_step) * 1000;
		opt->pro_grid = NULL;	/* disable the automatic profile */
	} else if (!strcmp(pic, CFG_PIC_DIS_NUM)) {
		opt->grid_col = 0;
		opt->grid_row = ezgui_entry_get_int(gui->entry_row);
		opt->pro_grid = NULL;	/* disable the automatic profile */
	} else if (!strcmp(pic, CFG_PIC_DIS_STEP)) {
		opt->grid_col = 0;
		opt->grid_row = 0;
		opt->tm_step  = ezgui_entry_get_int(gui->entry_step) * 1000;
		opt->pro_grid = NULL; /* disable the automatic profile */
	} else if (!strcmp(pic, CFG_PIC_DIS_KEY)) {
		opt->grid_col = 0;
		opt->grid_row = 0;
		opt->tm_step  = 0;
		opt->pro_grid = NULL;	/* disable the automatic profile */
	}
	if (pic) {
		g_free(pic);
	}

	/* update the zoom parameters */
	if ((pic = ezgui_cfg_read(gui->config, CFG_KEY_ZOOM)) == NULL) {
		/* do nothing as default setting */
	} else if (!strcmp(pic, CFG_PIC_ZOOM_RATIO)) {
		opt->tn_facto  = ezgui_entry_get_int(gui->entry_zoom_ratio);
		opt->pro_size  = NULL;  /* disable the automatic profile */
	} else if (!strcmp(pic, CFG_PIC_ZOOM_DEFINE)) {
		opt->tn_width  = ezgui_entry_get_int(gui->entry_zoom_wid);
		opt->tn_height = ezgui_entry_get_int(gui->entry_zoom_hei);
		opt->pro_size  = NULL;  /* disable the automatic profile */
	} else if (!strcmp(pic, CFG_PIC_ZOOM_SCREEN)) {
		opt->canvas_width = ezgui_entry_get_int(gui->entry_width);
		opt->pro_size = NULL;	/* disable the automatic profile */
	}
	if (pic) {
		g_free(pic);
	}
	return 0;
}

static GtkWidget *ezgui_page_main_listview_create(EZGUI *gui)
{
	static	GtkTargetEntry	targets[] = { { "STRING", 0, 0 } };
	GtkWidget		*view;
	GtkTreeViewColumn	*col;
	GtkTreeSelection	*tsel;
	GtkCellRenderer		*renderer;
	GtkListStore		*liststore;

	view = gtk_tree_view_new();

	/* --- Column #1 --- */
	renderer = gtk_cell_renderer_text_new();
	//g_object_set(renderer, "width", 300, NULL);
	col = gtk_tree_view_column_new_with_attributes("Name", 
			renderer, "text", EZUI_COL_NAME, NULL);
	gtk_tree_view_column_set_resizable(col, TRUE);
	//g_object_set(col, "width", 300, NULL);
	gtk_tree_view_column_set_min_width(col, 300);
	/* Nope, we autosize it later after file names been read */
	//gtk_tree_view_column_set_min_width(col, -1);
	gtk_tree_view_insert_column(GTK_TREE_VIEW(view), col, -1);

	/* --- Column #2 --- */
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("Size",
			renderer, "text", EZUI_COL_SIZE, NULL);
	gtk_tree_view_column_set_resizable(col, TRUE);	
	gtk_tree_view_insert_column(GTK_TREE_VIEW(view), col, -1);

	/* --- Column #3 --- */
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("Length",
			renderer, "text", EZUI_COL_LENGTH, NULL);
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_insert_column(GTK_TREE_VIEW(view), col, -1);

	/* --- Column #4 --- */
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("Resolution",
			renderer, "text", EZUI_COL_SCREEN, NULL);
	gtk_tree_view_column_set_resizable(col, TRUE);          
	gtk_tree_view_insert_column(GTK_TREE_VIEW(view), col, -1);

	/* --- Column #5 --- */
	renderer = gtk_cell_renderer_progress_new();
	//gtk_object_set(renderer, "text", "", "value", 100, NULL);
	col = gtk_tree_view_column_new_with_attributes("Progress",
			renderer, "value", EZUI_COL_PROGRESS, NULL);
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_column_set_min_width(col, 120);
	gtk_tree_view_insert_column(GTK_TREE_VIEW(view), col, -1);

	/* setup the model */
	liststore = gtk_list_store_new(EZUI_COL_MAX, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(liststore));

	/* setup the tree selection */
	tsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_set_mode(tsel, GTK_SELECTION_MULTIPLE);
	g_signal_connect(tsel, "changed", 
			G_CALLBACK(ezgui_signal_select_change), gui);

	/* bind the signal */
	//g_signal_connect(view, "button-release-event",
	//		G_CALLBACK(ezgui_signal_select_undo), gui);
	g_signal_connect_swapped(view, "row-activated",
			G_CALLBACK(ezgui_ezthumb), gui);

	/* Make tree view a destination for Drag'n'Drop */
	gtk_drag_dest_set(view, GTK_DEST_DEFAULT_ALL, 
			targets, 1, GDK_ACTION_COPY);
	g_signal_connect(view, "drag_data_received",
			G_CALLBACK(ezgui_signal_select_dragdrop), gui);

	return view;
}

static EZADD *ezgui_page_main_listview_open(GtkWidget *view)
{
	EZADD	*ezadd;

	if ((ezadd = calloc(sizeof(EZADD), 1)) == NULL) {
		return NULL;
	}

	/* get the point to the model */
	ezadd->app_model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
	/* Make sure the model stays with us after the tree view unrefs it */
	g_object_ref(ezadd->app_model); 
	/* Detach model from view */
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), NULL);
	
	/* create the text frame for discarded files */
	ezadd->discarded = gtk_text_buffer_new(NULL);
	return ezadd;
}

static int ezgui_page_main_listview_close(GtkWidget *view, EZADD *ezadd)
{
	int	rc;

	if (ezadd == NULL) {
		return 0;
	}

	/* Re-attach model to view */
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), ezadd->app_model);
	/* unref it becuase it has been reference by the tree view */
	g_object_unref(ezadd->app_model);

	rc = ezadd->add_count;
	if (ezadd->dis_count) {
		ezgui_page_main_dialog_invalid_files(ezadd);
	}
	g_object_unref(ezadd->discarded);
	free(ezadd);
	return rc;
}

static int ezgui_page_main_listview_append(EZGUI *gui, EZADD *ezadd, char *s)
{
	GtkTreeIter	row;
	EZVID		*vidx;
	char		*fname, tmark[32], res[32], tsize[32];

	if (!s || !*s || !ezadd) {
		return 0;
	}

	/* check if there is the same file in the list */
	if (gtk_tree_model_get_iter_first(ezadd->app_model, &row)) {
		do {
			gtk_tree_model_get(ezadd->app_model, &row, 
					EZUI_COL_NAME, &fname, -1);
			if (!strcmp(fname, s)) {
				g_free(fname);
				return 0;
			}
			g_free(fname);
		} while (gtk_tree_model_iter_next(ezadd->app_model, &row));
	}

	if ((vidx = video_allocate(s, gui->sysopt, NULL)) == NULL) {
		ezadd->dis_count++;
		gtk_text_buffer_insert_at_cursor(ezadd->discarded, s, -1);
		gtk_text_buffer_insert_at_cursor(ezadd->discarded, "\n", -1);
		return 0;

	}
	meta_timestamp(vidx->duration, 0, tmark);
	meta_filesize(vidx->filesize, tsize);
	sprintf(res, "%dx%d", 
			vidx->formatx->streams[vidx->vsidx]->codec->width, 
			vidx->formatx->streams[vidx->vsidx]->codec->height);
	video_free(vidx);

	gtk_list_store_append(GTK_LIST_STORE(ezadd->app_model), &row);
	gtk_list_store_set(GTK_LIST_STORE(ezadd->app_model), &row, 
			EZUI_COL_NAME, s, EZUI_COL_SIZE, tsize, 
			EZUI_COL_LENGTH, tmark, EZUI_COL_SCREEN, res, -1);
	gui->list_count++;
	ezadd->add_count++;
	return ezadd->add_count;
}

static void ezgui_page_main_dialog_invalid_files(EZADD *ezadd)
{
	GtkWidget	*dialog, *vbox, *text, *scroll;

	dialog = gtk_message_dialog_new(NULL,	//GTK_WINDOW(), 
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, 
			GTK_BUTTONS_CLOSE, "Failed to load following files");
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
			"The following files could not be loaded because of "
			"unsupported format or file corrupted");
	gtk_window_set_title(GTK_WINDOW(dialog), "Loading files");
	//gtk_widget_set_size_request(dialog, 500, 300);

	text = gtk_text_view_new_with_buffer(ezadd->discarded);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(scroll, 300, 150);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), text);

	vbox = gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(dialog));
	ezgui_pack_fill(vbox, scroll, NULL);
	gtk_widget_show_all(vbox);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void ezgui_signal_file_choose(void *parent, EZGUI *gui)
{
	GtkWidget 	*dialog;
	GtkFileFilter	*filter;
	GtkFileChooser	*chooser;
	GSList		*flist, *p;
	EZADD		*ezadd;
	char		*dir;

	dialog = gtk_file_chooser_dialog_new ("Choose File", 
			GTK_WINDOW(gui->gw_main),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL);
	chooser = GTK_FILE_CHOOSER(dialog);

	gtk_file_chooser_set_select_multiple(chooser, TRUE);
	if ((dir = ezgui_cfg_read(gui->config, CFG_KEY_DIRECTORY)) != NULL) {
		gtk_file_chooser_set_current_folder(chooser, dir);
		g_free(dir);
	}

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "All Videos");
	gtk_file_filter_add_mime_type(filter, "video/*");
	gtk_file_chooser_add_filter(chooser, filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "*.avi");
	gtk_file_filter_add_pattern(filter, "*.avi");
	gtk_file_chooser_add_filter(chooser, filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "All Files");
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(chooser, filter);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		ezadd = ezgui_page_main_listview_open(gui->gw_listview);

		flist = gtk_file_chooser_get_filenames(chooser);
		for (p = flist; p != NULL; p = p->next) {
			ezgui_page_main_listview_append(gui, ezadd, p->data);
			//puts(p->data);
			g_free(p->data);
		}
		g_slist_free(flist);

		ezgui_page_main_listview_close(gui->gw_listview, ezadd);

		/* find out if the list is empty */
		if (gui->list_count) {
			gtk_widget_set_sensitive(gui->button_run, TRUE);
		} else {
			gtk_widget_set_sensitive(gui->button_run, FALSE);
		}

		/* save the current working directory */
		if ((dir = gtk_file_chooser_get_current_folder(chooser))) {
			ezgui_cfg_write(gui->config, CFG_KEY_DIRECTORY, dir);
			g_free(dir);
		}
	}
	gtk_widget_destroy(dialog);
}

static void ezgui_signal_file_remove(void *parent, EZGUI *gui)
{
	GtkTreeSelection	*tsel;
	GtkTreeModel	*model;
	GtkTreeIter 	iter;
	GList		*slist, *node;

	tsel  = gtk_tree_view_get_selection(GTK_TREE_VIEW(gui->gw_listview));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gui->gw_listview));

	while ((slist = gtk_tree_selection_get_selected_rows(tsel, &model)) 
			!= NULL) {
		if (gtk_tree_model_get_iter(model, &iter, slist->data)) {
			gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
			gui->list_count--;
		}
		for (node = slist; node; node = g_list_next(node)) {
			gtk_tree_path_free(node->data);
		}
		g_list_free(slist);
	}

	/* find out if the list is empty */
	if (gui->list_count) {
		gtk_widget_set_sensitive(gui->button_run, TRUE);
	} else {
		gtk_widget_set_sensitive(gui->button_run, FALSE);
	}
}

static void ezgui_signal_select_change(GtkTreeSelection *tsel, EZGUI *gui)
{
	if (gtk_tree_selection_count_selected_rows(tsel) > 0) {
		gtk_widget_set_sensitive(gui->button_del, TRUE);
	} else {
		gtk_widget_set_sensitive(gui->button_del, FALSE);
	}
}

#if 0
static void ezgui_signal_select_undo(GtkWidget *view, GdkEvent *event, EZGUI *gui)
{
	GtkTreeSelection	*tsel;

	tsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(gui->gw_listview));
	if (((GdkEventButton*)event)->button == 3) {	/* right button */
		gtk_widget_set_sensitive(gui->button_del, FALSE);
		gtk_tree_selection_unselect_all(tsel);
	}
}
#endif

static void ezgui_signal_select_dragdrop(GtkWidget *view, GdkDragContext *context,
		int x, int y, GtkSelectionData *seldata, 
		guint info, guint time, EZGUI *gui)
{
	EZADD	*ezadd;
	char	*dndl, *head, *tail, *tmp;

	if ((dndl = (char*)gtk_selection_data_get_text(seldata)) == NULL) {
		return;
	}

	ezadd = ezgui_page_main_listview_open(gui->gw_listview);

	for (head = dndl; head; head = tail + 1) {
		if ((tail = strchr(head, '\n')) == NULL) {
			tail = (char*) -1;
		} else {
			*tail = 0;
		}
		head += 7;	/* skip the leading 'file://' */
		if ((tmp = strchr(head, '\r')) != NULL) {
			*tmp = 0;
		}
		ezgui_page_main_listview_append(gui, ezadd, head);
	}

	ezgui_page_main_listview_close(gui->gw_listview, ezadd);

	/* find out if the list is empty */
	if (gui->list_count) {
		gtk_widget_set_sensitive(gui->button_run, TRUE);
	} else {
		gtk_widget_set_sensitive(gui->button_run, FALSE);
	}

	g_free(dndl);
}

static GtkWidget *ezgui_page_setup_create(EZGUI *gui)
{
	GtkWidget	*hbox_button, *vbox_page, *scroll, *table;
	
	/* create the table for all settings */
	table = gtk_table_new(5, 9, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 2);
	gtk_table_set_col_spacings(GTK_TABLE(table), 2);
	gtk_table_set_col_spacing(GTK_TABLE(table), 0, 20);

	/* create the combo box for the profile setting */
	gui->prof_grid = ezgui_combo(gui, prof_list_grid,
			G_CALLBACK(ezgui_signal_setup_sensible));
	gui->prof_zoom = ezgui_combo(gui, prof_list_zoom,
			G_CALLBACK(ezgui_signal_setup_sensible));

	table_fill(table, ezgui_setup_label("<b>Profile Selection:</b>"), 0, 0, 5);
	table_insert(table, ezgui_setup_idname("Grid Setting:"), 1, 1);
	table_insert(table, gui->prof_grid, 2, 1);
	table_insert(table, ezgui_setup_idname("Zoom Setting:"), 3, 1);
	table_insert(table, gui->prof_zoom, 4, 1);

	/* create the radio button for the Duration finding mode */
	gui->dfm_head = ezgui_rabutton(gui, NULL, "File Head  ",
			G_CALLBACK(ezgui_signal_setup_sensible));
	gui->dfm_fast = ezgui_rabutton(gui, gui->dfm_head, "Fast scan  ",
			G_CALLBACK(ezgui_signal_setup_sensible));
	gui->dfm_slow = ezgui_rabutton(gui, gui->dfm_fast, "Full scan  ",
			G_CALLBACK(ezgui_signal_setup_sensible));

	table_fill(table, ezgui_setup_label("<b>Duration Finding Mode:</b>"),
			0, 2, 5);
	table_insert(table, gui->dfm_head, 1, 3);
	table_insert(table, gui->dfm_fast, 2, 3);
	table_insert(table, gui->dfm_slow, 3, 3);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->dfm_head), TRUE);

	/* create the radio button group for the output format */
	table_fill(table, ezgui_setup_label("<b>Output File Format:</b>"),
			0, 4, 5);
	ezgui_page_setup_output_format(gui, table, 5);


	/* create a scroll container for the setup page */
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), 
			table);

	/* create the "OK" and "Cancel" buttons in the bottom of the page */
	gui->butt_setup_cancel = ezgui_button(gui, "Cancel",
			G_CALLBACK(ezgui_signal_setup_reset));
	gui->butt_setup_apply = ezgui_button(gui, "OK",
			G_CALLBACK(ezgui_signal_setup_update));

	hbox_button = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox_button), 
			gui->butt_setup_apply, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox_button), 
			gui->butt_setup_cancel, FALSE, FALSE, 0);

	/* create the vbox container for the whole page */
	vbox_page = gtk_vbox_new(FALSE, 0);
	ezgui_pack_fill(vbox_page, scroll, NULL);
	ezgui_pack_forward(vbox_page, hbox_button, NULL);

	/* reset the setup page by the options in the config file */
	ezgui_signal_setup_reset(NULL, gui);
	return vbox_page;
}

static int ezgui_page_setup_output_format(EZGUI *gui, GtkWidget *table, int row)
{
	GtkWidget	*hbox_gifa;

	/* PNG button + Tranparent tickbox */
	gui->off_png = ezgui_rabutton(gui, NULL, "PNG",
			G_CALLBACK(ezgui_signal_setup_format));
	gui->off_transp = gtk_check_button_new_with_label("Transparent");
	g_signal_connect(gui->off_transp, "toggled",
			G_CALLBACK(ezgui_signal_setup_format), gui);

	table_insert(table, gui->off_png, 1, row);
	table_insert(table, gui->off_transp, 3, row);
	row++;

	/* GIF button */
	gui->off_gif = ezgui_rabutton(gui, gui->off_png, "GIF",
			G_CALLBACK(ezgui_signal_setup_format));
	table_insert(table, gui->off_gif, 1, row);
	row++;

	/* GIF Animated button */
	gui->off_gifa = ezgui_rabutton(gui, gui->off_gif, "Animated GIF",
			G_CALLBACK(ezgui_signal_setup_format));

	gui->off_gifa_fr = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(gui->off_gifa_fr), 4);
	//ezgui_entry_set_int(gui->off_gifa_fr, gui->tmp_gifa_fr);
	gtk_widget_set_size_request(gui->off_gifa_fr, 80, -1);
	gtk_widget_set_sensitive(gui->off_gifa_fr, FALSE);

	hbox_gifa = gtk_hbox_new(FALSE, 0);
	ezgui_pack_forward(hbox_gifa, gui->off_gifa_fr, 
			gtk_label_new("(ms)"), NULL);

	table_insert(table, gui->off_gifa,  1, row);
	table_insert(table, gtk_label_new("Speed: "), 2, row);
	table_insert(table, hbox_gifa, 3, row);
	row++;

	/* JPEG button */
	gui->off_jpg = ezgui_rabutton(gui, gui->off_gifa, "JPEG",
			G_CALLBACK(ezgui_signal_setup_format));

	gui->off_jpg_qf = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(gui->off_jpg_qf), 3);
	//ezgui_entry_set_int(gui->off_jpg_qf, gui->tmp_jpg_qf);
	gtk_widget_set_size_request(gui->off_jpg_qf, 80, -1);
	gtk_widget_set_sensitive(gui->off_jpg_qf, FALSE);

	table_insert(table, gui->off_jpg,  1, row);
	table_insert(table, gtk_label_new("Quality: "), 2, row);
	table_insert(table, gui->off_jpg_qf, 3, row);
	row++;

	ezgui_format_reset(gui, EZUI_FMR_RDRSET);
	return row;
}

static int ezgui_signal_setup_sensible(void *parent, EZGUI *gui)
{
	gtk_widget_set_sensitive(gui->butt_setup_apply, TRUE);
	gtk_widget_set_sensitive(gui->butt_setup_cancel, TRUE);
	return 0;
}

static int ezgui_signal_setup_insensible(void *parent, EZGUI *gui)
{
	gtk_widget_set_sensitive(gui->butt_setup_apply, FALSE);
	gtk_widget_set_sensitive(gui->butt_setup_cancel, FALSE);
	return 0;
}

static int ezgui_signal_setup_reset(void *parent, EZGUI *gui)
{
	char	*pic;
	int	i;

	pic = ezgui_cfg_read(gui->config, CFG_KEY_GRID);
	for (i = 0; prof_list_grid[i]; i++) {
		if (!strcmp(pic, prof_list_grid[i])) {
			break;
		}
	}
	if (prof_list_grid[i]) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(gui->prof_grid), i);
	} else {
		ezgui_cfg_write(gui->config, CFG_KEY_GRID, CFG_PIC_AUTO);
	}
	g_free(pic);

	pic = ezgui_cfg_read(gui->config, CFG_KEY_ZOOM);
	for (i = 0; prof_list_zoom[i]; i++) {
		if (!strcmp(pic, prof_list_zoom[i])) {
			break;
		}
	}
	if (prof_list_zoom[i]) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(gui->prof_zoom), i);
	} else {
		ezgui_cfg_write(gui->config, CFG_KEY_ZOOM, CFG_PIC_AUTO);
	}
	g_free(pic);

	switch (ezgui_cfg_read_int(gui->config, 
				CFG_KEY_DURATION, gui->sysopt->dur_mode)) {
	case EZ_DUR_QK_SCAN:
		gtk_toggle_button_set_active
			(GTK_TOGGLE_BUTTON(gui->dfm_fast), TRUE);
		break;
	case EZ_DUR_FULLSCAN:
		gtk_toggle_button_set_active
			(GTK_TOGGLE_BUTTON(gui->dfm_slow), TRUE);
		break;
	default:
		gtk_toggle_button_set_active
			(GTK_TOGGLE_BUTTON(gui->dfm_head), TRUE);
		break;
	}
	
	/* reset the output file format control group */
	ezgui_format_reset(gui, EZUI_FMR_RDONLY);
	
	/* dummy toggling */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->off_jpg), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->off_png), TRUE);
	
	if (!strcmp(gui->sysopt->img_format, "jpg")) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->off_jpg), TRUE);
	} else if (!strcmp(gui->sysopt->img_format, "png")) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->off_png), TRUE);
	} else if (!strcmp(gui->sysopt->img_format, "gif")) {
		if (gui->sysopt->img_quality == 0) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->off_gif), TRUE);
		} else {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->off_gifa), TRUE);
		}
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->off_transp), 
			gui->sysopt->flags & EZOP_TRANSPARENT);
	ezgui_entry_set_int(gui->off_jpg_qf, gui->tmp_jpg_qf);
	ezgui_entry_set_int(gui->off_gifa_fr, gui->tmp_gifa_fr);

	ezgui_signal_setup_insensible(parent, gui);
	return 0;
}

static int ezgui_signal_setup_update(void *parent, EZGUI *gui)
{
	gchar	*pic;
	int	rc = 0;

	pic = gtk_combo_box_get_active_text(GTK_COMBO_BOX(gui->prof_grid));
	if (pic) {
		rc += ezgui_cfg_write(gui->config, CFG_KEY_GRID, pic);
		g_free(pic);
	}
	//printf("ezgui_signal_setup_update: grid %d\n", rc);

	pic = gtk_combo_box_get_active_text(GTK_COMBO_BOX(gui->prof_zoom));
	if (pic) {
		rc += ezgui_cfg_write(gui->config, CFG_KEY_ZOOM, pic);
		g_free(pic);
	}
	//printf("ezgui_signal_setup_update: zoom %d\n", rc);

	if (rc) {
		gtk_widget_destroy(gui->prof_group);
		gui->prof_group = ezgui_page_main_profile(gui);
		ezgui_pack_forward(gui->button_box, gui->prof_group, NULL);
		gtk_widget_show_all(gui->button_box);
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui->dfm_head))) {
		gui->sysopt->dur_mode = EZ_DUR_CLIPHEAD;
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui->dfm_fast))) {
		gui->sysopt->dur_mode = EZ_DUR_QK_SCAN;
	} else {
		gui->sysopt->dur_mode = EZ_DUR_FULLSCAN;
	}

	/* receive the output file format setting group */
	gui->tmp_jpg_qf   = ezgui_entry_get_int(gui->off_jpg_qf);
	gui->tmp_gifa_fr  = ezgui_entry_get_int(gui->off_gifa_fr);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui->off_transp))) {
		gui->sysopt->flags |= EZOP_TRANSPARENT;
	} else {
		gui->sysopt->flags &= ~EZOP_TRANSPARENT;
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui->off_jpg))) {
		if ((gui->tmp_jpg_qf < 5) || (gui->tmp_jpg_qf > 100)) {
			gui->tmp_jpg_qf = 85;
		}
		strcpy(gui->sysopt->img_format, "jpg");
		gui->sysopt->img_quality = gui->tmp_jpg_qf;
		gui->sysopt->flags &= ~EZOP_TRANSPARENT;
		gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(gui->off_transp), FALSE);
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui->off_gif))) {
		strcpy(gui->sysopt->img_format, "gif");
		gui->sysopt->img_quality = 0;
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui->off_gifa))) {
		if (gui->tmp_gifa_fr && (gui->tmp_gifa_fr < 15)) {
			gui->tmp_gifa_fr = 1000;
		}
		strcpy(gui->sysopt->img_format, "gif");
		gui->sysopt->img_quality = gui->tmp_gifa_fr;
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui->off_png))) {
		strcpy(gui->sysopt->img_format, "png");
		gui->sysopt->img_quality = 0;
	}

	ezgui_option_save(gui);
	ezgui_signal_setup_insensible(parent, gui);
	return 0;
}

static int ezgui_signal_setup_format(void *parent, EZGUI *gui)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(parent)) == TRUE) {
		if (parent == gui->off_png) {
			gtk_widget_set_sensitive(gui->off_transp, TRUE);
		} else if (parent == gui->off_gif) {
			gtk_widget_set_sensitive(gui->off_transp, TRUE);
		} else if (parent == gui->off_gifa) {
			gtk_widget_set_sensitive(gui->off_transp, TRUE);
			gtk_widget_set_sensitive(gui->off_gifa_fr, TRUE);
		} else if (parent == gui->off_jpg) {
			gtk_widget_set_sensitive(gui->off_transp, FALSE);
			gtk_widget_set_sensitive(gui->off_jpg_qf, TRUE);
		}
	} else {
		if (parent == gui->off_gifa) {
			gtk_widget_set_sensitive(gui->off_gifa_fr, FALSE);
		} else if (parent == gui->off_jpg) {
			gtk_widget_set_sensitive(gui->off_jpg_qf, FALSE);
		}
	}

	ezgui_signal_setup_sensible(parent, gui);
	return 0;
}


static EZCFG *ezgui_cfg_create(void)
{
	EZCFG	*cfg;
	char	*path;

	if ((cfg = calloc(sizeof(EZCFG), 1)) == NULL) {
		return NULL;
	}

	/* Make sure the path to the configure file existed */
	if (!g_file_test(g_get_user_config_dir(), G_FILE_TEST_EXISTS)) {
		g_mkdir(g_get_user_config_dir(), 0755);
	}

	path = g_build_filename(g_get_user_config_dir(), CFG_SUBPATH, NULL);
	if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
		g_mkdir(path, 0755);
	}
	g_free(path);

	/* If the configure file exists, try to read it */
	cfg->fname = g_build_filename(g_get_user_config_dir(), 
			CFG_SUBPATH, CFG_FILENAME, NULL);
	
	cfg->ckey = g_key_file_new();
	if (g_file_test(cfg->fname, G_FILE_TEST_EXISTS)) {
		g_key_file_load_from_file(cfg->ckey, cfg->fname, 0, NULL);
	}
	return cfg;
}

static int  ezgui_cfg_free(EZCFG *cfg)
{
	if (cfg == NULL) {
		return -1;
	}

	ezgui_cfg_flush(cfg);

	if (cfg->fname) {
		g_free(cfg->fname);
	}
	/*if (cfg->ckey) {
		g_free(cfg->ckey);
	}*/
	free(cfg);
	return 0;
}

static char *ezgui_cfg_read(EZCFG *cfg, char *key)
{
	if (!g_key_file_has_key(cfg->ckey, CFG_GRP_MAIN, key, NULL)) {
		return NULL;
	}
	return g_key_file_get_value(cfg->ckey, CFG_GRP_MAIN, key, NULL);
}

static int ezgui_cfg_write(EZCFG *cfg, char *key, char *s)
{
	char	*p;

	if (s == NULL) {
		return 0;
	}
	if (g_key_file_has_key(cfg->ckey, CFG_GRP_MAIN, key, NULL)) {
		p = g_key_file_get_value(cfg->ckey, CFG_GRP_MAIN, key, NULL);
		if (!strcmp(s, p)) {
			g_free(p);
			return 0;
		}
		g_free(p);
	}
	g_key_file_set_value(cfg->ckey, CFG_GRP_MAIN, key, s);
	cfg->mcount++;
	return cfg->mcount;
}

static int ezgui_cfg_read_int(EZCFG *cfg, char *key, int def)
{
	if (!g_key_file_has_key(cfg->ckey, CFG_GRP_MAIN, key, NULL)) {
		g_key_file_set_integer(cfg->ckey, CFG_GRP_MAIN, key, def);
		cfg->mcount++;
		return def;
	}
	return g_key_file_get_integer(cfg->ckey, CFG_GRP_MAIN, key, NULL);
}

static int ezgui_cfg_write_int(EZCFG *cfg, char *key, int val)
{
	if (g_key_file_has_key(cfg->ckey, CFG_GRP_MAIN, key, NULL)) {
		if (g_key_file_get_integer(cfg->ckey, CFG_GRP_MAIN, key, NULL)
				== val) {
			return 0;
		}
	}
	g_key_file_set_integer(cfg->ckey, CFG_GRP_MAIN, key, val);
	cfg->mcount++;
	return cfg->mcount;
}

static int ezgui_cfg_flush(EZCFG *cfg)
{
	gchar	*cfgdata;
	gsize	len = 0;
	FILE	*fp;
	
	//printf("ezgui_cfg_flush: %d\n", cfg->mcount);

	if (cfg->mcount == 0) {
		return 0;
	}

	cfgdata = g_key_file_to_data(cfg->ckey, &len, NULL);

	if ((fp = fopen(cfg->fname, "w")) != NULL) {
		fwrite(cfgdata, 1, len, fp);
		fclose(fp);
	}

	g_free(cfgdata);
	cfg->mcount = 0;
	return 0;
}

/*
static int ezgui_cfg_set_monitor(EZGUI *gui)
{
	GFileMonitor	*mon_file;
	GFile 		*cfg_file;

	cfg_file = g_file_new_for_path(gui->cfg_fname);
	mon_file = g_file_monitor_file(cfg_file, 0, NULL, NULL);
	g_signal_connect(G_OBJECT(mon_file), "changed", 
			G_CALLBACK(ezgui_cfg_external_change), gui);
	return 0;
}

static int ezgui_cfg_external_change(EZGUI *gui)
{
}
*/

static GtkWidget *ezgui_pack_forward(GtkWidget *box, ...)
{
	va_list		ap;
	GtkWidget	*wp;

	va_start(ap, box);
	while ((wp = va_arg(ap, GtkWidget*)) != NULL) {
		gtk_box_pack_start(GTK_BOX(box), wp, FALSE, FALSE, 0);
	}
	va_end(ap);
	return box;
}

static GtkWidget *ezgui_pack_backward(GtkWidget *box, ...)
{
	va_list		ap;
	GtkWidget	*wp;

	va_start(ap, box);
	while ((wp = va_arg(ap, GtkWidget*)) != NULL) {
		gtk_box_pack_end(GTK_BOX(box), wp, FALSE, FALSE, 0);
	}
	va_end(ap);
	return box;
}

static GtkWidget *ezgui_pack_fill(GtkWidget *box, ...)
{
	va_list		ap;
	GtkWidget	*wp;

	va_start(ap, box);
	while ((wp = va_arg(ap, GtkWidget*)) != NULL) {
		gtk_box_pack_start(GTK_BOX(box), wp, TRUE, TRUE, 0);
	}
	va_end(ap);
	return box;
}

static GtkWidget *ezgui_entry_box(int val, int dw)
{
	GtkWidget	*entry;

	entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), dw);
	gtk_widget_set_size_request(entry, dw * 10, -1);

	ezgui_entry_set_int(entry, val);
	return entry;
}

static int ezgui_entry_get_int(GtkWidget *entry)
{
	const gchar	*val;

	val = gtk_entry_get_text(GTK_ENTRY(entry));
	return (int) strtol(val, NULL, 0);
}

static int ezgui_entry_set_int(GtkWidget *entry, int val)
{
	char	buf[16];

	sprintf(buf, "%d", val);
	gtk_entry_set_text(GTK_ENTRY(entry), buf);
	return 0;
}

static GtkWidget *ezgui_page_label(gchar *s)
{
	GtkWidget	*label;

	label = gtk_label_new(s);
	gtk_widget_set_size_request(label, 80, -1);
	return label;
}

static GtkWidget *ezgui_setup_label(gchar *s)
{
	GtkWidget	*label;

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), s);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 10, 10);
	return label;
}

static GtkWidget *ezgui_setup_idname(gchar *s)
{
	GtkWidget	*idname;

	idname = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(idname), s);
	gtk_widget_set_size_request(idname, 120, 30);
	return idname;
}

static GtkWidget *ezgui_button(EZGUI *gui, gchar *s, GCallback c_handler)
{
	GtkWidget	*button;

	button = gtk_button_new_with_label(s);
	gtk_widget_set_size_request(button, 80, 30);
	if (c_handler) {
		g_signal_connect(button, "clicked", c_handler, gui);
	}
	return button;
}

static GtkWidget *ezgui_combo(EZGUI *gui, char **sopt, GCallback c_handler)
{
	GtkWidget	*combo;
	int		i;

	combo = gtk_combo_box_new_text();
	for (i = 0; sopt[i]; i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), sopt[i]);
	}
	gtk_widget_set_size_request(combo, 160, -1);
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
	if (c_handler) {
		g_signal_connect(combo, "changed", c_handler, gui);
	}
	return combo;
}

static GtkWidget *ezgui_rabutton(EZGUI *gui, GtkWidget *prev, gchar *s, 
		GCallback c_handler)
{
	GtkWidget	*rab;

	if (prev == NULL) {
		rab = gtk_radio_button_new_with_label(NULL, s);
	} else {
		rab = gtk_radio_button_new_with_label_from_widget
			(GTK_RADIO_BUTTON(prev), s);
	}
	//gtk_widget_set_size_request(rab, 180, -1);
	if (c_handler) {
		g_signal_connect(rab, "toggled", c_handler, gui);
	}
	return rab;
}

static int ezgui_format_reset(EZGUI *gui, int rwcfg)
{
	EZOPT	*ezopt = gui->sysopt;
	char	*p, tmp[32];

	if ((p = ezgui_cfg_read(gui->config, CFG_KEY_FILE_FORMAT)) != NULL) {
		ezopt->img_quality = para_get_format(p, ezopt->img_format, 8);
		free(p);
	} else if (rwcfg == EZUI_FMR_RDWR) {
		sprintf(tmp, "%s@%d", ezopt->img_format, ezopt->img_quality);
		ezgui_cfg_write(gui->config, CFG_KEY_FILE_FORMAT, tmp);
	}

	if (rwcfg == EZUI_FMR_RDRSET) {
		gui->tmp_gifa_fr = 1000;
		gui->tmp_jpg_qf  = 85;
		if (!strcmp(ezopt->img_format, "jpg")) {
			gui->tmp_jpg_qf  = ezopt->img_quality;
		} else if (!strcmp(ezopt->img_format, "gif")) {
			gui->tmp_gifa_fr = ezopt->img_quality;
		}
	}

	if ((p = ezgui_cfg_read(gui->config, CFG_KEY_TRANSPARENCY)) != NULL) {
		if (!strcmp(p, "yes")) {
			ezopt->flags |= EZOP_TRANSPARENT;
		} else {
			ezopt->flags &= ~EZOP_TRANSPARENT;
		}
		free(p);
	} else if (rwcfg) {
		ezgui_cfg_write(gui->config, CFG_KEY_TRANSPARENCY,
				ezopt->flags & EZOP_TRANSPARENT ? "yes":"no");
	}
	return 0;
}

static void table_insert(GtkWidget *table, GtkWidget *w, int x, int y)
{
	gtk_table_attach(GTK_TABLE(table), w, x, x+1, y, y+1, 
			GTK_FILL, 0, 0, 0);
}

static void table_fill(GtkWidget *table, GtkWidget *w, int x, int y, int n)
{
	gtk_table_attach(GTK_TABLE(table), w, x, x+n, y, y+1, 
			GTK_FILL, 0, 0, 0);
}

