
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

#include "ezgui.h"


static GtkWidget *ezgui_notebook_main(EZGUI *gui);
static GtkWidget *ezgui_notebook_setup(EZGUI *gui);
static GtkWidget *ezgui_profile_box(EZGUI *gui);
static int ezgui_profile_read(EZGUI *gui);
static int ezgui_profile_write(EZGUI *gui);
static GtkWidget *ezgui_profile_pack(GtkWidget *box, 
		GtkWidget *w1, GtkWidget *w2, GtkWidget *w3, GtkWidget *w4);
static GtkWidget *ezgui_profile_entry(EZGUI *gui, char *key, int def, 
		int digitw, int boxw);

static GtkWidget *ezgui_create_view_and_model(EZGUI *gui);
static void ezgui_selection_change(GtkTreeSelection *tsel, EZGUI *gui);
static void ezgui_selection_undo(GtkWidget *view, GdkEvent *event, EZGUI *gui);
static void ezgui_selection_enter(GtkWidget *view, GtkTreePath *path,
		GtkTreeViewColumn *column, EZGUI *gui);
static void ezgui_selection_dragdrop(GtkWidget *view, GdkDragContext *context,
		int x, int y, GtkSelectionData *seldata, 
		guint info, guint time, EZGUI *gui);
static void ezgui_files_choose(void *parent, EZGUI *gui);
static void ezgui_files_remove(void *parent, EZGUI *gui);
static void ezgui_files_generate(void *parent, EZGUI *gui);
static int ezgui_option_commit(EZGUI *gui);
static int ezgui_notificate(void *v, int eid, long param, long opt, void *b);

static EZADD *ezgui_list_append_begin(GtkWidget *view);
static int ezgui_list_append_end(GtkWidget *view, EZADD *ezadd);
static int ezgui_list_append(EZGUI *gui, EZADD *ezadd, char *s);
static void ezgui_dialog_invalid_files(EZADD *ezadd);

static EZCFG *ezgui_cfg_init(void);
static int ezgui_cfg_free(EZCFG *cfg);
static char *ezgui_cfg_read(EZCFG *cfg, char *key);
static int ezgui_cfg_write(EZCFG *cfg, char *key, char *s);
static int ezgui_cfg_read_int(EZCFG *cfg, char *key, int def);
static int ezgui_cfg_write_int(EZCFG *cfg, char *key, int val);
static int ezgui_cfg_flush(EZCFG *cfg);

static int ezgui_entry_get_int(GtkWidget *entry);
static int ezgui_entry_set_int(GtkWidget *entry, int val);



EZGUI *ezgui_init(EZOPT *ezopt, int *argcs, char ***argvs)
{
	EZGUI	*gui;
	char	*p;

	gtk_init(argcs, argvs);

	if ((gui = calloc(sizeof(EZGUI), 1)) == NULL) {
		return NULL;
	}
	if ((gui->config = ezgui_cfg_init()) == NULL) {
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
	return gui;
}

int ezgui_run(EZGUI *gui)
{
	if (gui == NULL) {
		return -1;
	}

	/* update the configure file because some options may be changed
	 * by command line options */
	ezgui_profile_write(gui);
	
	gtk_widget_show_all(gui->gw_main);
	gtk_main();
	return 0;
}

int ezgui_close(EZGUI *gui)
{
	if (gui == NULL) {
		return -1;
	}

	ezgui_cfg_free(gui->config);
	free(gui);
	return 0;
}

int ezgui_create(EZGUI *gui)
{
	GtkWidget	*label1, *label2;
	GtkWidget	*page_main, *page_setup;
	int		w_wid, w_hei;

	/* Create the pages of notebook */
	page_main = ezgui_notebook_main(gui);
	page_setup = ezgui_notebook_setup(gui);

	/* Create a new notebook, place the position of the tabs */
	gui->gw_page = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(gui->gw_page), GTK_POS_TOP);
	label1 = gtk_label_new("Generate");
	gtk_notebook_append_page(GTK_NOTEBOOK(gui->gw_page), page_main, label1);
	label2 = gtk_label_new("Setup");
	gtk_notebook_append_page(GTK_NOTEBOOK(gui->gw_page), page_setup, label2);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(gui->gw_page), 0);

	/* create the top level window */
	gui->gw_main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	w_wid = ezgui_cfg_read_int(gui->config, CFG_KEY_WIN_WIDTH, 640);
	w_hei = ezgui_cfg_read_int(gui->config, CFG_KEY_WIN_HEIGHT, 480);
	gtk_window_set_default_size(GTK_WINDOW(gui->gw_main), w_wid, w_hei);
	gtk_container_set_border_width(GTK_CONTAINER(gui->gw_main), 10);
	g_signal_connect(gui->gw_main, "delete_event", gtk_main_quit, NULL);
	gtk_container_add(GTK_CONTAINER(gui->gw_main), gui->gw_page);
	return 0;
}

int ezgui_list_add_file(EZGUI *gui, char *flist[], int fnum)
{
	EZADD	*ezadd;
	int	i;

	ezadd = ezgui_list_append_begin(gui->gw_listview);
	for (i = 0; i < fnum; i++) {
		ezgui_list_append(gui, ezadd, flist[i]);
	}
	ezgui_list_append_end(gui->gw_listview, ezadd);
	return i;
}



static GtkWidget *ezgui_notebook_main(EZGUI *gui)
{
	GtkWidget	*scroll;
	GtkWidget	*button_add, *button_run, *profile;
	GtkWidget	*hbox, *vbox;

	/* create the listview */
	gui->gw_listview = ezgui_create_view_and_model(gui);

	/* create the scrollbars and stuffed with the listview */
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_container_add(GTK_CONTAINER(scroll), gui->gw_listview);

	/* create the buttons */
	button_add = gtk_button_new_with_label("Add");
	gtk_widget_set_size_request(button_add, 80, 30);
	g_signal_connect(button_add, "clicked", 
			G_CALLBACK(ezgui_files_choose), gui);

	gui->button_del = gtk_button_new_with_label("Remove");
	gtk_widget_set_size_request(gui->button_del, 80, 30);
	gtk_widget_set_sensitive(gui->button_del, FALSE);
	g_signal_connect(gui->button_del, "clicked", 
			G_CALLBACK(ezgui_files_remove), gui);

	button_run = gtk_button_new_with_label("Start");
	gtk_widget_set_size_request(button_run, 80, 30);
	g_signal_connect(button_run, "clicked", 
			G_CALLBACK(ezgui_files_generate), gui);

	/* create left side */
	profile = ezgui_profile_box(gui);

	/* create the horizontal box and stuffed with the buttons */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), button_run, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), gui->button_del, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), button_add, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), profile, FALSE, FALSE, 0);

	/* create the vertical box and stuffed with all above */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	return vbox;
}

static GtkWidget *ezgui_notebook_setup(EZGUI *gui)
{
	GtkWidget	*hbox_prof, *vbox;
	GtkWidget	*label1, *label2;
	char		*pic;
	int		i;
	
	static	char	*list_grid[] = {
		CFG_PIC_AUTO, CFG_PIC_GRID_DIM, CFG_PIC_GRID_STEP, 
		CFG_PIC_DIS_NUM, CFG_PIC_DIS_STEP, CFG_PIC_DIS_KEY, NULL
	};
	static	char	*list_zoom[] = {
		CFG_PIC_AUTO, CFG_PIC_ZOOM_RATIO, CFG_PIC_ZOOM_DEFINE,
		CFG_PIC_ZOOM_SCREEN, NULL
	};

	gui->prof_grid = gtk_combo_box_new_text();
	for (i = 0; list_grid[i]; i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(gui->prof_grid), list_grid[i]);
	}
	pic = ezgui_cfg_read(gui->config, CFG_KEY_GRID);
	for (i = 0; list_grid[i]; i++) {
		if (!strcmp(pic, list_grid[i])) {
			break;
		}
	}
	if (list_grid[i]) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(gui->prof_grid), i);
	}

	gui->prof_zoom = gtk_combo_box_new_text();
	for (i = 0; list_zoom[i]; i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(gui->prof_zoom), list_zoom[i]);
	}
	pic = ezgui_cfg_read(gui->config, CFG_KEY_ZOOM);
	for (i = 0; list_zoom[i]; i++) {
		if (!strcmp(pic, list_zoom[i])) {
			break;
		}
	}
	if (list_zoom[i]) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(gui->prof_zoom), i);
	}

	label1 = gtk_label_new("Profile Grid ");
	label2 = gtk_label_new("Profile Zoom ");

	hbox_prof = gtk_hbox_new(FALSE, 0);
	gtk_widget_set_size_request(hbox_prof, -1, 30);
	gtk_box_pack_start(GTK_BOX(hbox_prof), label1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_prof), gui->prof_grid, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox_prof), gui->prof_zoom, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox_prof), label2, FALSE, FALSE, 0);

	return hbox_prof;
}

/* Grid: (Grid Auto)(Grid 4x4)(Grid 4 Step 15)(DC No. 20)(DC Step 15)(DC I-Frame)
 * Zoom: (Zoom Auto)(Zoom 50%)(Zoom 320x240)(Res 1024) */
static GtkWidget *ezgui_profile_box(EZGUI *gui)
{
	GtkWidget	*hbox, *label1, *label2, *label3, *label4;
	char		*pic;

	hbox = gtk_hbox_new(FALSE, 0);

	pic = ezgui_cfg_read(gui->config, CFG_KEY_GRID);
	if (!strcmp(pic, CFG_PIC_GRID_DIM)) {
		label1 = gtk_label_new("Grid");
		label2 = gtk_label_new("x");
		gui->entry_col = ezgui_profile_entry(gui, CFG_KEY_GRID_COLUMN,
				gui->sysopt->grid_col, 3, 30);
		gui->entry_row = ezgui_profile_entry(gui, CFG_KEY_GRID_ROW,
				gui->sysopt->grid_row, 3, 30);
		ezgui_profile_pack(hbox, label1, gui->entry_col, 
				label2, gui->entry_row);
	} else if (!strcmp(pic, CFG_PIC_GRID_STEP)) {
		label1 = gtk_label_new("Grid");
		label2 = gtk_label_new("Step");
		gui->entry_col = ezgui_profile_entry(gui, CFG_KEY_GRID_COLUMN,
				gui->sysopt->grid_col, 3, 30);
		gui->entry_step = ezgui_profile_entry(gui, CFG_KEY_TIME_STEP,
				gui->sysopt->tm_step / 1000, 3, 50);
		ezgui_profile_pack(hbox, label1, gui->entry_col,
				label2, gui->entry_step);
	} else if (!strcmp(pic, CFG_PIC_DIS_NUM)) {
		label1 = gtk_label_new("DSS No ");
		gui->entry_row = ezgui_profile_entry(gui, CFG_KEY_GRID_ROW,
				gui->sysopt->grid_row, 3, 30);
		ezgui_profile_pack(hbox, label1, gui->entry_row, 
				NULL, NULL);
	} else if (!strcmp(pic, CFG_PIC_DIS_STEP)) {
		label1 = gtk_label_new("DSS Step ");
		gui->entry_step = ezgui_profile_entry(gui, CFG_KEY_TIME_STEP,
				gui->sysopt->tm_step / 1000, 3, 50);
		ezgui_profile_pack(hbox, label1, gui->entry_step,
				NULL, NULL);
	} else if (!strcmp(pic, CFG_PIC_DIS_KEY)) {
		label1 = gtk_label_new("DSS I-Frame");
		ezgui_profile_pack(hbox, label1, NULL, NULL, NULL);
	} else {
		label1 = gtk_label_new("Grid Auto");
		ezgui_profile_pack(hbox, label1, NULL, NULL, NULL);
	}
	g_free(pic);

	pic = ezgui_cfg_read(gui->config, CFG_KEY_ZOOM);
	if (!strcmp(pic, CFG_PIC_ZOOM_RATIO)) {
		GtkAdjustment	*adjust;
		int		val;

		val = ezgui_cfg_read_int(gui->config, CFG_KEY_ZOOM_RATIO, 
				gui->sysopt->tn_facto);
		adjust = (GtkAdjustment *) gtk_adjustment_new(val, 5.0, 
				200.0, 5.0, 25.0, 0.0);
		gui->entry_zoom_ratio = gtk_spin_button_new(adjust, 0, 0);
		gtk_widget_set_size_request(gui->entry_zoom_ratio, 50, -1);

		label3 = gtk_label_new("  Zoom");
		label4 = gtk_label_new("%");
		ezgui_profile_pack(hbox, label3, gui->entry_zoom_ratio, 
				label4, NULL);
	} else if (!strcmp(pic, CFG_PIC_ZOOM_DEFINE)) {
		label3 = gtk_label_new("  Zoom");
		label4 = gtk_label_new("x");
		gui->entry_zoom_wid = ezgui_profile_entry(gui, CFG_KEY_ZOOM_WIDTH,
				gui->sysopt->tn_width, 3, 50);
		gui->entry_zoom_hei = ezgui_profile_entry(gui, CFG_KEY_ZOOM_HEIGHT,
				gui->sysopt->tn_height, 3, 50);
		ezgui_profile_pack(hbox, label3, gui->entry_zoom_wid, 
				label4, gui->entry_zoom_hei);
	} else if (!strcmp(pic, CFG_PIC_ZOOM_SCREEN)) {
		label3 = gtk_label_new("  Res");
		gui->entry_width = ezgui_profile_entry(gui, CFG_KEY_CANVAS_WIDTH,
				gui->sysopt->canvas_width, 4, 50);
		ezgui_profile_pack(hbox, label3, gui->entry_width,
				NULL, NULL);
	} else {
		label3 = gtk_label_new("  Zoom Auto");
		ezgui_profile_pack(hbox, label3, NULL, NULL, NULL);
	}
	g_free(pic);
	return hbox;
}

static GtkWidget *ezgui_profile_pack(GtkWidget *box, 
		GtkWidget *w1, GtkWidget *w2, GtkWidget *w3, GtkWidget *w4)
{
	if (w1) {
		gtk_box_pack_start(GTK_BOX(box), w1, FALSE, FALSE, 0);
	}
	if (w2) {
		gtk_box_pack_start(GTK_BOX(box), w2, FALSE, FALSE, 0);
	}
	if (w3) {
		gtk_box_pack_start(GTK_BOX(box), w3, FALSE, FALSE, 0);
	}
	if (w4) {
		gtk_box_pack_start(GTK_BOX(box), w4, FALSE, FALSE, 0);
	}
	return box;
}

static int ezgui_profile_read(EZGUI *gui)
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

static int ezgui_profile_write(EZGUI *gui)
{
	EZOPT	*opt = gui->sysopt;

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

	/* save to the configure file */
	ezgui_cfg_flush(gui->config);
	return 0;
}

static GtkWidget *ezgui_profile_entry(EZGUI *gui, char *key, int def, 
		int digitw, int boxw)
{
	GtkWidget	*entry;

	entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), digitw);
	gtk_widget_set_size_request(entry, boxw, -1);

	ezgui_entry_set_int(entry, ezgui_cfg_read_int(gui->config, key, def));
	return entry;
}

static GtkWidget *ezgui_create_view_and_model(EZGUI *gui)
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
	gtk_tree_view_insert_column(GTK_TREE_VIEW(view), col, -1);

	/* setup the model */
	liststore = gtk_list_store_new(EZUI_COL_MAX, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(liststore));

	/* setup the tree selection */
	tsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_set_mode(tsel, GTK_SELECTION_MULTIPLE);
	g_signal_connect(tsel, "changed", 
			G_CALLBACK(ezgui_selection_change), gui);

	/* bind the signal */
	//g_signal_connect(view, "button-release-event",
	//		G_CALLBACK(ezgui_selection_undo), gui);
	g_signal_connect(view, "row-activated",
			G_CALLBACK(ezgui_selection_enter), gui);

	/* Make tree view a destination for Drag'n'Drop */
	gtk_drag_dest_set(view, GTK_DEST_DEFAULT_ALL, 
			targets, 1, GDK_ACTION_COPY);
	g_signal_connect(view, "drag_data_received",
			G_CALLBACK(ezgui_selection_dragdrop), gui);
	return view;
}

static void ezgui_selection_change(GtkTreeSelection *tsel, EZGUI *gui)
{
	if (gtk_tree_selection_count_selected_rows(tsel) > 0) {
		gtk_widget_set_sensitive(gui->button_del, TRUE);
	} else {
		gtk_widget_set_sensitive(gui->button_del, FALSE);
	}
}

static void ezgui_selection_undo(GtkWidget *view, GdkEvent *event, EZGUI *gui)
{
	GtkTreeSelection	*tsel;

	tsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(gui->gw_listview));
	if (((GdkEventButton*)event)->button == 3) {	/* right button */
		gtk_widget_set_sensitive(gui->button_del, FALSE);
		gtk_tree_selection_unselect_all(tsel);
	}
}

static void ezgui_selection_enter(GtkWidget *view, GtkTreePath *path,
		GtkTreeViewColumn *column, EZGUI *gui)
{
	GtkTreeModel	*model;
	GtkTreeIter	iter;
	char	*s;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
	if (gtk_tree_model_get_iter(model, &iter, path)) {
		gtk_tree_model_get(model, &iter, EZUI_COL_NAME, &s, -1);

		ezgui_option_commit(gui);
		gui->pro_model = model;
		gui->pro_iter  = &iter;
		ezthumb(s, gui->sysopt);
		//puts(s);
	}
}

static void ezgui_selection_dragdrop(GtkWidget *view, GdkDragContext *context,
		int x, int y, GtkSelectionData *seldata, 
		guint info, guint time, EZGUI *gui)
{
	EZADD	*ezadd;
	char	*dndl, *head, *tail, *tmp;

	if ((dndl = (char*)gtk_selection_data_get_text(seldata)) == NULL) {
		return;
	}

	ezadd = ezgui_list_append_begin(gui->gw_listview);

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
		ezgui_list_append(gui, ezadd, head);
	}

	ezgui_list_append_end(gui->gw_listview, ezadd);
	g_free(dndl);
}


static void ezgui_files_choose(void *parent, EZGUI *gui)
{
	GtkWidget 	*dialog;
	GtkFileFilter	*filter;
	GtkFileChooser	*chooser;
	GSList		*flist, *p;
	EZADD		*ezadd;
	char		*dir;

	dialog = gtk_file_chooser_dialog_new ("Choose File", NULL,
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
		ezadd = ezgui_list_append_begin(gui->gw_listview);

		flist = gtk_file_chooser_get_filenames(chooser);
		for (p = flist; p != NULL; p = p->next) {
			ezgui_list_append(gui, ezadd, p->data);
			//puts(p->data);
			g_free(p->data);
		}
		g_slist_free(flist);

		ezgui_list_append_end(gui->gw_listview, ezadd);

		if ((dir = gtk_file_chooser_get_current_folder(chooser))) {
			ezgui_cfg_write(gui->config, CFG_KEY_DIRECTORY, dir);
			g_free(dir);
		}
	}
	gtk_widget_destroy(dialog);
}

static void ezgui_files_remove(void *parent, EZGUI *gui)
{
	GtkTreeSelection	*tsel;
	GtkTreeModel	*model;
	GtkTreeIter 	iter;
	GList		*rows, *node;

	tsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(gui->gw_listview));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gui->gw_listview));
	node = rows = gtk_tree_selection_get_selected_rows(tsel, &model);

	while (node) {
		gtk_tree_model_get_iter(model, &iter, node->data);
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
		node = g_list_next(node);
	}
	g_list_foreach(rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(rows);
}

static void ezgui_files_generate(void *parent, EZGUI *gui)
{
	GtkTreeModel	*model;
	GtkTreeIter	row;
	char	*s;

	ezgui_option_commit(gui);

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gui->gw_listview));
	if (gtk_tree_model_get_iter_first(model, &row)) {
		do {
			gtk_tree_model_get(model, &row, EZUI_COL_NAME, &s, -1);
			
			gui->pro_model = model;
			gui->pro_iter  = &row;
			//ezthumb(s, gui->sysopt);
			puts(s);

			g_free(s);
		} while (gtk_tree_model_iter_next(model, &row));
	}
}


static int ezgui_option_commit(EZGUI *gui)
{
	/* bind the notification function to GUI mode */
	gui->sysopt->notify = ezgui_notificate;
	/* read options from the GUI widgets */
	ezgui_profile_read(gui);
	/* save these options into the configure file */
	ezgui_profile_write(gui);
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
		if (param) {	/* i-frame ripping */
			val = 100;
		} else {
			val = opt * 100 / param;
		}
		gtk_list_store_set(GTK_LIST_STORE(gui->pro_model), 
				gui->pro_iter, EZUI_COL_PROGRESS, val, -1);
		break;

	default:
		return EN_EVENT_PASSTHROUGH;
	}
	return eid;
}

static EZADD *ezgui_list_append_begin(GtkWidget *view)
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

static int ezgui_list_append_end(GtkWidget *view, EZADD *ezadd)
{
	if (ezadd == NULL) {
		return -1;
	}

	/* Re-attach model to view */
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), ezadd->app_model);
	/* unref it becuase it has been reference by the tree view */
	g_object_unref(ezadd->app_model);

	if (ezadd->dis_count) {
		ezgui_dialog_invalid_files(ezadd);
	}
	g_object_unref(ezadd->discarded);
	free(ezadd);
	return 0;
}

static int ezgui_list_append(EZGUI *gui, EZADD *ezadd, char *s)
{
	GtkTreeIter	row;
	EZVID		*vidx;
	char		*fname, tmark[32], res[16], tsize[16];

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
	meta_filesize(vidx->formatx->file_size, tsize);
	sprintf(res, "%dx%d", 
			vidx->formatx->streams[vidx->vsidx]->codec->width, 
			vidx->formatx->streams[vidx->vsidx]->codec->height);
	video_free(vidx);

	gtk_list_store_append(GTK_LIST_STORE(ezadd->app_model), &row);
	gtk_list_store_set(GTK_LIST_STORE(ezadd->app_model), &row, 
			EZUI_COL_NAME, s, EZUI_COL_SIZE, tsize, 
			EZUI_COL_LENGTH, tmark, EZUI_COL_SCREEN, res, -1);
	return 1;
}

static void ezgui_dialog_invalid_files(EZADD *ezadd)
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
	gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
	gtk_widget_show_all(vbox);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}


static EZCFG *ezgui_cfg_init(void)
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


