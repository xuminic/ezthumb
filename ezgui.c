
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

#include "ezthumb.h"
#include "ezgui.h"
#include "libsmm.h"


static GtkWidget *ezgui_notebook_main(EZGUI *gui);
static GtkWidget *ezgui_profile_ratio(void);
static GtkWidget *ezgui_create_view_and_model(void);
static void ezgui_files_choose(EZGUI *gui, void *parent);
static void ezgui_files_remove(EZGUI *gui, void *parent);

static int ezgui_cfg_init(EZGUI *gui);
static char *ezgui_cfg_read_string(EZGUI *gui, char *key, char *def);
static int ezgui_cfg_read_int(EZGUI *gui, char *key, int def);
static void ezgui_cfg_write_string(EZGUI *gui, char *key, char *s);
static void ezgui_cfg_write_int(EZGUI *gui, char *key, int val);
static int ezgui_cfg_flush(EZGUI *gui);



int ezgui_init(int *argcs, char ***argvs)
{
	gtk_init(argcs, argvs);
	return 0;
}

void *ezgui_create(void)
{
	EZGUI		*gui;
	GtkWidget	*label;
	int		w_wid, w_hei;

	if ((gui = malloc(sizeof(EZGUI))) == NULL) {
		return NULL;
	}
	memset(gui, 0, sizeof(EZGUI));

	ezgui_cfg_init(gui);

	gui->gw_page_main = ezgui_notebook_main(gui);

	/* Create a new notebook, place the position of the tabs */
	gui->gw_page = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(gui->gw_page), GTK_POS_TOP);
	label = gtk_label_new("Generate");
	gtk_notebook_append_page(GTK_NOTEBOOK(gui->gw_page), 
			gui->gw_page_main, label);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(gui->gw_page), 0);

	/* create the top level window */
	gui->gw_main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	w_wid = ezgui_cfg_read_int(gui, CFG_KEY_WIN_WIDTH, 640);
	w_hei = ezgui_cfg_read_int(gui, CFG_KEY_WIN_HEIGHT, 480);
	gtk_window_set_default_size(GTK_WINDOW(gui->gw_main), w_wid, w_hei);
	gtk_container_set_border_width(GTK_CONTAINER(gui->gw_main), 10);
	g_signal_connect(gui->gw_main, "delete_event", gtk_main_quit, NULL);
	gtk_container_add(GTK_CONTAINER(gui->gw_main), gui->gw_page);
	return gui;
}

int ezgui_run(EZGUI *gui)
{
	if (gui) {
		gtk_widget_show_all(gui->gw_main);
		gtk_main();
		ezgui_close(gui);
		return 0;
	}
	return -1;
}

int ezgui_close(EZGUI *gui)
{
	if (gui == NULL) {
		return -1;
	}

	ezgui_cfg_flush(gui);

	if (gui->cfg_filename) {
		g_free(gui->cfg_filename);
		gui->cfg_filename = NULL;
	}
	
	free(gui);
	return 0;
}

void *ezgui_list_append_begin(EZGUI *gui)
{
	GtkTreeModel	*model;

	/* get the point to the model */
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gui->gw_listview));
	/* Make sure the model stays with us after the tree view unrefs it */
	g_object_ref(model); 
	/* Detach model from view */
	gtk_tree_view_set_model(GTK_TREE_VIEW(gui->gw_listview), NULL);
	return model;
}

int ezgui_list_append_end(EZGUI *gui, GtkTreeModel *model)
{
	/* Re-attach model to view */
	gtk_tree_view_set_model(GTK_TREE_VIEW(gui->gw_listview), model);
	/* unref it becuase it has been reference by the tree view */
	g_object_unref(model);
	return 0;
}

int ezgui_list_append(EZGUI *gui, GtkTreeModel *model, char *s)
{
	GtkTreeIter	row;

	gtk_list_store_append(GTK_LIST_STORE(model), &row);
	gtk_list_store_set(GTK_LIST_STORE(model), &row, EZUI_COL_NAME, s, -1);
	return 0;
}




static GtkWidget *ezgui_notebook_main(EZGUI *gui)
{
	GtkWidget	*scroll;
	GtkWidget	*button_add, *button_del, *button_run, *profile;
	GtkWidget	*hbox, *vbox;

	/* create the listview */
	gui->gw_listview = ezgui_create_view_and_model();

	/* create the scrollbars and stuffed with the listview */
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_container_add(GTK_CONTAINER(scroll), gui->gw_listview);

	/* create the buttons */
	button_add = gtk_button_new_with_label("Add");
	gtk_widget_set_size_request(button_add, 80, 30);
	g_signal_connect_swapped(button_add, "clicked", 
			G_CALLBACK(ezgui_files_choose), gui);

	button_del = gtk_button_new_with_label("Remove");
	gtk_widget_set_size_request(button_del, 80, 30);
	g_signal_connect(button_del, "clicked", 
			G_CALLBACK(ezgui_files_remove), gui);
	gtk_widget_set_sensitive(button_del, FALSE);

	button_run = gtk_button_new_with_label("Start");
	gtk_widget_set_size_request(button_run, 80, 30);

	/* create left side */
	profile = ezgui_profile_ratio();

	/* create the horizontal box and stuffed with the buttons */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), button_run, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), button_del, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), button_add, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), profile, FALSE, FALSE, 0);

	/* create the vertical box and stuffed with all above */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	return vbox;
}

static GtkWidget *ezgui_profile_ratio(void)
{
	GtkWidget	*label1, *label2, *label3, *label4;
	GtkWidget	*hbox, *entry1, *entry2, *entry3;
	GtkAdjustment	*adjust;

	label1 = gtk_label_new("Grid");
	label2 = gtk_label_new("x");
	label3 = gtk_label_new("  Zoom");
	label4 = gtk_label_new("%");

	entry1 = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry1), 3);
	gtk_entry_set_text(GTK_ENTRY(entry1), "4");
	gtk_widget_set_size_request(entry1, 30, -1);

	entry2 = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry2), 3);
	gtk_entry_set_text(GTK_ENTRY(entry2), "8");
	gtk_widget_set_size_request(entry2, 30, -1);

	adjust = (GtkAdjustment *) gtk_adjustment_new(50.0, 5.0, 200.0, 
			5.0, 25.0, 0.0);
	entry3 = gtk_spin_button_new (adjust, 0, 0);
	gtk_widget_set_size_request(entry3, 50, -1);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label2, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry2, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label3, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry3, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label4, FALSE, FALSE, 0);

	return hbox;
}

static GtkWidget *ezgui_create_view_and_model(void)
{
	GtkWidget		*view;
	GtkTreeViewColumn	*col;
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
	col = gtk_tree_view_column_new_with_attributes("Screen",
			renderer, "text", EZUI_COL_SCREEN, NULL);
	gtk_tree_view_column_set_resizable(col, TRUE);          
	gtk_tree_view_insert_column(GTK_TREE_VIEW(view), col, -1);

	/* --- Column #5 --- */
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("Progress",
			renderer, "text", EZUI_COL_PROGRESS, NULL);
	gtk_tree_view_column_set_resizable(col, TRUE);          
	gtk_tree_view_insert_column(GTK_TREE_VIEW(view), col, -1);


	/* FIXME: HOW TO free renderer? */

	liststore = gtk_list_store_new(EZUI_COL_MAX, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(liststore));
	return view;
}

static void ezgui_files_choose(EZGUI *gui, void *parent)
{
	GtkWidget 	*dialog;
	GtkTreeModel	*model;
	GtkFileFilter	*filter;
	GSList		*flist, *p;

	dialog = gtk_file_chooser_dialog_new ("Open File", NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "All Videos");
	gtk_file_filter_add_mime_type(filter, "video/*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "*.avi");
	gtk_file_filter_add_pattern(filter, "*.avi");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "All Files");
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		model = ezgui_list_append_begin(gui);

		flist = gtk_file_chooser_get_filenames(
				GTK_FILE_CHOOSER(dialog));
		for (p = flist; p != NULL; p = p->next) {
			ezgui_list_append(gui, model, p->data);
			//puts(p->data);
			g_free(p->data);
		}
		g_slist_free(flist);

		ezgui_list_append_end(gui, model);
	}
	gtk_widget_destroy(dialog);
}

static void ezgui_files_remove(EZGUI *gui, void *parent)
{
}


static int ezgui_cfg_init(EZGUI *gui)
{
	char		*path;

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
	gui->cfg_filename = g_build_filename(g_get_user_config_dir(), 
			CFG_SUBPATH, CFG_FILENAME, NULL);
	
	gui->cfg_keys = g_key_file_new();
	if (g_file_test(gui->cfg_filename, G_FILE_TEST_EXISTS)) {
		g_key_file_load_from_file(gui->cfg_keys, 
				gui->cfg_filename, 0, NULL);
	}
	return 0;
}

static char *ezgui_cfg_read_string(EZGUI *gui, char *key, char *def)
{
	if (!g_key_file_has_key(gui->cfg_keys, CFG_GRP_MAIN, key, NULL)) {
		ezgui_cfg_write_string(gui, key, def);
	}
	return g_key_file_get_value(gui->cfg_keys, CFG_GRP_MAIN, key, NULL);
}

static int ezgui_cfg_read_int(EZGUI *gui, char *key, int def)
{
	if (!g_key_file_has_key(gui->cfg_keys, CFG_GRP_MAIN, key, NULL)) {
		ezgui_cfg_write_int(gui, key, def);
	}
	return g_key_file_get_integer(gui->cfg_keys, CFG_GRP_MAIN, key, NULL);
}

static void ezgui_cfg_write_string(EZGUI *gui, char *key, char *s)
{
	g_key_file_set_value(gui->cfg_keys, CFG_GRP_MAIN, key, s);
	gui->mod_counter++;
}

static void ezgui_cfg_write_int(EZGUI *gui, char *key, int val)
{
	g_key_file_set_integer(gui->cfg_keys, CFG_GRP_MAIN, key, val);
	gui->mod_counter++;
}

static int ezgui_cfg_flush(EZGUI *gui)
{
	gchar	*cfgdata;
	gsize	len = 0;
	FILE	*fp;
	
	if (gui->mod_counter == 0) {
		return 0;
	}

	cfgdata = g_key_file_to_data(gui->cfg_keys, &len, NULL);

	if ((fp = fopen(gui->cfg_filename, "w")) != NULL) {
		fwrite(cfgdata, 1, len, fp);
		fclose(fp);
	}

	g_free(cfgdata);
	gui->mod_counter = 0;
	return 0;
}

/*
static int ezgui_cfg_set_monitor(EZGUI *gui)
{
	GFileMonitor	*mon_file;
	GFile 		*cfg_file;

	cfg_file = g_file_new_for_path(gui->cfg_filename);
	mon_file = g_file_monitor_file(cfg_file, 0, NULL, NULL);
	g_signal_connect(G_OBJECT(mon_file), "changed", 
			G_CALLBACK(ezgui_cfg_external_change), gui);
	return 0;
}

static int ezgui_cfg_external_change(EZGUI *gui)
{
}
*/


