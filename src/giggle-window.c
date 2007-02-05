/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2007 Imendio AB
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcelanguagesmanager.h>
#include <string.h>

#include "giggle-window.h"
#include "giggle-error.h"
#include "giggle-git.h"
#include "giggle-git-branches.h"
#include "giggle-git-diff.h"
#include "giggle-git-revisions.h"
#include "giggle-revision.h"
#include "giggle-graph-renderer.h"

typedef struct GiggleWindowPriv GiggleWindowPriv;

struct GiggleWindowPriv {
	GtkWidget           *content_vbox;
	GtkWidget           *menubar_hbox;
	GtkWidget           *revision_treeview;
	GtkWidget           *log_textview;
	GtkWidget           *diff_textview;

	GtkCellRenderer     *graph_renderer;
	
	GtkUIManager        *ui_manager;

	GiggleGit           *git;

	/* Current job in progress. */
	GiggleJob           *current_job;
};

enum {
	REVISION_COL_OBJECT,
	REVISION_NUM_COLS
};

static void window_finalize                       (GObject           *object);
static void window_setup_revision_treeview        (GiggleWindow      *window);
static void window_setup_diff_textview            (GiggleWindow      *window,
						   GtkWidget         *scrolled);
static void window_update_revision_info           (GiggleWindow      *window,
						   GiggleRevision    *current_revision,
						   GiggleRevision    *previous_revision);
static void window_add_widget_cb                  (GtkUIManager      *merge,
						   GtkWidget         *widget,
						   GiggleWindow      *window);
static void window_revision_selection_changed_cb  (GtkTreeSelection  *selection,
						   GiggleWindow      *window);
static void window_git_diff_result_callback       (GiggleGit         *git,
						   GiggleJob         *job,
						   GError            *error,
						   gpointer           user_data);
static void window_revision_cell_data_log_func    (GtkTreeViewColumn *tree_column,
						   GtkCellRenderer   *cell,
						   GtkTreeModel      *tree_model,
						   GtkTreeIter       *iter,
						   gpointer           data);
static void window_revision_cell_data_author_func (GtkTreeViewColumn *tree_column,
						   GtkCellRenderer   *cell,
						   GtkTreeModel      *tree_model,
						   GtkTreeIter       *iter,
						   gpointer           data);
static void window_revision_cell_data_date_func   (GtkTreeViewColumn *tree_column,
						   GtkCellRenderer   *cell,
						   GtkTreeModel      *tree_model,
						   GtkTreeIter       *iter,
						   gpointer           data);
static void window_action_quit_cb                 (GtkAction         *action,
						   GiggleWindow      *window);
static void window_action_open_cb                 (GtkAction         *action,
						   GiggleWindow      *window);
static void window_action_save_patch_cb           (GtkAction         *action,
						   GiggleWindow      *window);
static void window_action_about_cb                (GtkAction         *action,
						   GiggleWindow      *window);
static void window_directory_changed_cb           (GiggleGit         *git,
						   GParamSpec        *arg,
						   GiggleWindow      *window);

static const GtkActionEntry action_entries[] = {
	{ "FileMenu", NULL,
	  N_("_File"), NULL, NULL,
	  NULL
	},
	{ "EditMenu", NULL,
	  N_("_Edit"), NULL, NULL,
	  NULL
	},
	{ "HelpMenu", NULL,
	  N_("_Help"), NULL, NULL,
	  NULL
	},
	{ "Open", GTK_STOCK_OPEN,
	  N_("_Open"), "<control>O", N_("Open a GIT repository"),
	  G_CALLBACK (window_action_open_cb)
	},
	{ "SavePatch", GTK_STOCK_OPEN,
	  N_("_Save patch"), "<control>S", N_("Save a patch"),
	  G_CALLBACK (window_action_save_patch_cb)
	},
	{ "Quit", GTK_STOCK_QUIT,
	  N_("_Quit"), "<control>Q", N_("Quit the application"),
	  G_CALLBACK (window_action_quit_cb)
	},
	{ "About", GTK_STOCK_ABOUT,
	  N_("_About"), NULL, N_("About this application"),
	  G_CALLBACK (window_action_about_cb)
	}
};

static const gchar *ui_layout =
	"<ui>"
	"  <menubar name='MainMenubar'>"
	"    <menu action='FileMenu'>"
	/*"      <separator/>"*/
	"      <menuitem action='Open'/>"
	"      <menuitem action='SavePatch'/>"
	"      <menuitem action='Quit'/>"
	"    </menu>"
	"    <menu action='EditMenu'>"
	"    </menu>"
	"    <menu action='HelpMenu'>"
	"      <menuitem action='About'/>"
	"    </menu>"
	"  </menubar>"
	"</ui>";


G_DEFINE_TYPE (GiggleWindow, giggle_window, GTK_TYPE_WINDOW)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_WINDOW, GiggleWindowPriv))

static void
giggle_window_class_init (GiggleWindowClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = window_finalize;

	g_type_class_add_private (object_class, sizeof (GiggleWindowPriv));
}

static void
window_create_menu (GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GtkActionGroup   *action_group;
	GtkAction        *action;
	GError           *error = NULL;

	priv = GET_PRIV (window);
	priv->ui_manager = gtk_ui_manager_new ();
	g_signal_connect (priv->ui_manager,
			  "add_widget",
			  G_CALLBACK (window_add_widget_cb),
			  window);

	action_group = gtk_action_group_new ("MainActions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (action_group,
				      action_entries,
				      G_N_ELEMENTS (action_entries),
				      window);
	gtk_ui_manager_insert_action_group (priv->ui_manager, action_group, 0);

	gtk_window_add_accel_group (GTK_WINDOW (window),
				    gtk_ui_manager_get_accel_group (priv->ui_manager));

	g_object_unref (action_group);

	gtk_ui_manager_add_ui_from_string (priv->ui_manager, ui_layout, -1, &error);
	if (error) {
		g_error ("Couldn't create UI: %s}\n", error->message);
	}

	gtk_ui_manager_ensure_update (priv->ui_manager);

	action = gtk_ui_manager_get_action (priv->ui_manager, "/ui/MainMenubar/FileMenu/SavePatch");
	gtk_action_set_sensitive (action, FALSE);
}

static void
giggle_window_init (GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	gchar            *dir;
	GladeXML         *xml;
	GError           *error = NULL;

	priv = GET_PRIV (window);

	priv->git = giggle_git_new ();
	g_signal_connect (priv->git,
			  "notify::directory",
			  G_CALLBACK (window_directory_changed_cb),
			  window);

	xml = glade_xml_new (GLADEDIR "/main-window.glade",
			     "content_vbox", NULL);
	if (!xml) {
		g_error ("Couldn't find glade file, did you install?");
	}

	priv->content_vbox = glade_xml_get_widget (xml, "content_vbox");
	gtk_container_add (GTK_CONTAINER (window), priv->content_vbox);

	priv->menubar_hbox = glade_xml_get_widget (xml, "menubar_hbox");
	priv->revision_treeview = glade_xml_get_widget (xml, "revision_treeview");
	window_setup_revision_treeview (window);

	priv->log_textview = glade_xml_get_widget (xml, "log_textview");

	window_setup_diff_textview (
		window,
		glade_xml_get_widget (xml, "diff_scrolledwindow"));

	g_object_unref (xml);

	window_create_menu (window);

	dir = g_get_current_dir ();

	if (!giggle_git_set_directory (priv->git, dir, &error)) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (window),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("The directory '%s' does not look like a "
						   "GIT repository."), dir);

		gtk_dialog_run (GTK_DIALOG (dialog));

		gtk_widget_destroy (dialog);
	}

	g_free (dir);
}

static void
window_finalize (GObject *object)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (object);
	
	g_object_unref (priv->ui_manager);

	if (priv->current_job) {
		giggle_git_cancel_job (priv->git, priv->current_job);
		g_object_unref (priv->current_job);
	}

	g_object_unref (priv->git);

	G_OBJECT_CLASS (giggle_window_parent_class)->finalize (object);
}

static void
window_git_get_revisions_cb (GiggleGit    *git,
			     GiggleJob    *job,
			     GError       *error,
			     gpointer      user_data)
{
	GiggleWindow *window;
	GiggleWindowPriv *priv;
	GtkListStore *store;
	GtkTreeIter iter;
	GList *revisions;

	window = GIGGLE_WINDOW (user_data);
	priv = GET_PRIV (window);
	store = gtk_list_store_new (REVISION_NUM_COLS, GIGGLE_TYPE_REVISION);
	revisions = giggle_git_revisions_get_revisions (GIGGLE_GIT_REVISIONS (job));

	while (revisions) {
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    REVISION_COL_OBJECT, g_object_ref ((GObject*) revisions->data),
				    -1);
		revisions = revisions->next;
	}

	giggle_graph_renderer_validate_model (GIGGLE_GRAPH_RENDERER (priv->graph_renderer), GTK_TREE_MODEL (store), 0);
	gtk_tree_view_set_model (GTK_TREE_VIEW (priv->revision_treeview), GTK_TREE_MODEL (store));
	g_object_unref (store);
	g_object_unref (job);
}

static void
window_git_get_branches_cb (GiggleGit    *git,
			    GiggleJob    *job,
			    GError       *error,
			    gpointer      user_data)
{
	/* FIXME: do something with branches */
	g_object_unref (job);
}

static void
window_setup_revision_treeview (GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GtkCellRenderer  *cell;
	GtkTreeSelection *selection;
	gint              n_columns;

	priv = GET_PRIV (window);

	priv->graph_renderer = giggle_graph_renderer_new ();
	gtk_tree_view_insert_column_with_attributes (
		GTK_TREE_VIEW (priv->revision_treeview),
		-1,
		_("Graph"),
		priv->graph_renderer,
		"revision", REVISION_COL_OBJECT, 
		NULL);

	cell = gtk_cell_renderer_text_new ();
	g_object_set(cell,
		     "ellipsize", PANGO_ELLIPSIZE_END,
		     NULL);
	n_columns = gtk_tree_view_insert_column_with_data_func (
		GTK_TREE_VIEW (priv->revision_treeview),
		-1,
		_("Short Log"),
		cell,
		window_revision_cell_data_log_func,
		window,
		NULL);
	gtk_tree_view_column_set_expand (
		gtk_tree_view_get_column (GTK_TREE_VIEW (priv->revision_treeview), n_columns - 1),
		TRUE);

	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_data_func (
		GTK_TREE_VIEW (priv->revision_treeview),
		-1,
		_("Author"),
		cell,
		window_revision_cell_data_author_func,
		window,
		NULL);

	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_data_func (
		GTK_TREE_VIEW (priv->revision_treeview),
		-1,
		_("Date"),
		cell,
		window_revision_cell_data_date_func,
		window,
		NULL);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->revision_treeview));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW (priv->revision_treeview), TRUE);
	
	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (window_revision_selection_changed_cb),
			  window);
}

static void
window_setup_diff_textview (GiggleWindow *window,
			    GtkWidget    *scrolled)
{
	GiggleWindowPriv          *priv;
	PangoFontDescription      *font_desc;
	GtkTextBuffer             *buffer;
	GtkSourceLanguage         *language;
	GtkSourceLanguagesManager *manager;

	priv = GET_PRIV (window);
	
	priv->diff_textview = gtk_source_view_new ();
	gtk_text_view_set_editable (GTK_TEXT_VIEW (priv->diff_textview), FALSE);

	font_desc = pango_font_description_from_string ("monospace");
	gtk_widget_modify_font (priv->diff_textview, font_desc);
	pango_font_description_free (font_desc);
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->diff_textview));

	manager = gtk_source_languages_manager_new ();
	language = gtk_source_languages_manager_get_language_from_mime_type (
		manager, "text/x-patch");

	gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (buffer), language);
	gtk_source_buffer_set_highlight (GTK_SOURCE_BUFFER (buffer), TRUE);
	
	g_object_unref (manager);

	gtk_widget_show (priv->diff_textview);
	
	gtk_container_add (GTK_CONTAINER (scrolled), priv->diff_textview);
}

/* Update revision info. If previous_revision is not NULL, a diff between it and
 * the current revision will be shown.
 */
static void
window_update_revision_info (GiggleWindow   *window,
			     GiggleRevision *current_revision,
			     GiggleRevision *previous_revision)
{
	GiggleWindowPriv *priv;
	const gchar      *sha;
	const gchar      *log;
	gchar            *str;
	GtkAction        *action;
	
	priv = GET_PRIV (window);

	if (current_revision) {
		sha = giggle_revision_get_sha (current_revision);
		log = giggle_revision_get_long_log (current_revision);
		if (!log) {
			log = giggle_revision_get_short_log (current_revision);
		}
		if (!log) {
			log = "";
		}
	} else {
		sha = "";
		log = "";
	}
	
	str = g_strdup_printf ("%s\n%s", sha, log);
	
	gtk_text_buffer_set_text (
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->log_textview)),
		str, -1);

	g_free (str);

	/* Clear the diff view until we get new content. */
	gtk_text_buffer_set_text (
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->diff_textview)),
		"", 0);

	if (priv->current_job) {
		giggle_git_cancel_job (priv->git, priv->current_job);
		g_object_unref (priv->current_job);
		priv->current_job = NULL;
	}
	
	if (current_revision && previous_revision) {
		action = gtk_ui_manager_get_action (priv->ui_manager, "/ui/MainMenubar/FileMenu/SavePatch");
		gtk_action_set_sensitive (action, FALSE);

		priv->current_job = giggle_git_diff_new (previous_revision, current_revision);
		giggle_git_run_job (priv->git,
				    priv->current_job,
				    window_git_diff_result_callback,
				    window);
	}
}

static void
window_add_widget_cb (GtkUIManager *merge, 
		      GtkWidget    *widget, 
		      GiggleWindow *window)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (window);

	gtk_box_pack_start (GTK_BOX (priv->menubar_hbox), widget, TRUE, TRUE, 0);
}

static void
window_revision_selection_changed_cb (GtkTreeSelection *selection,
				      GiggleWindow     *window)
{
	GiggleWindowPriv *priv;
	GtkTreeModel     *model;
	GtkTreeIter       first_iter;
	GtkTreeIter       last_iter;
	GiggleRevision   *first_revision;
	GiggleRevision   *last_revision;
	GList            *rows;
	GList            *last_row;
	gboolean          valid;

	priv = GET_PRIV (window);
	rows = gtk_tree_selection_get_selected_rows (selection, &model);
	first_revision = last_revision = NULL;

	if (!rows) {
		return;
	}

	/* get the first row iter */
	gtk_tree_model_get_iter (model, &first_iter,
				 (GtkTreePath *) rows->data);

	if (g_list_length (rows) == 1) {
		/* if just one row is selected, get the previous revision */
		last_iter = first_iter;
		valid = gtk_tree_model_iter_next (model, &last_iter);
	} else {
		last_row = g_list_last (rows);
		valid = gtk_tree_model_get_iter (model, &last_iter,
						 (GtkTreePath *) last_row->data);
	}

	gtk_tree_model_get (model, &first_iter,
			    REVISION_COL_OBJECT, &first_revision,
			    -1);
	if (valid) {
		gtk_tree_model_get (model, &last_iter,
				    REVISION_COL_OBJECT, &last_revision,
				    -1);
	}

	window_update_revision_info (window,
				     first_revision,
				     last_revision);

	g_object_unref (first_revision);
	if (last_revision) {
		g_object_unref (last_revision);
	}

	g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (rows);
}

static void
window_git_diff_result_callback (GiggleGit *git,
				 GiggleJob *job,
				 GError    *error,
				 gpointer   user_data)
{
	GiggleWindow     *window;
	GiggleWindowPriv *priv;
	GtkAction        *action;

	window = GIGGLE_WINDOW (user_data);
	priv = GET_PRIV (window);

	if (error) {
		GtkWidget *dialog;
		
		dialog = gtk_message_dialog_new (GTK_WINDOW (window),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("An error ocurred when retrieving a diff:\n"
						   "%s"), error->message);
		
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	} else {
		gtk_text_buffer_set_text (
			gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->diff_textview)),
			giggle_git_diff_get_result (GIGGLE_GIT_DIFF (job)),
			-1);

		action = gtk_ui_manager_get_action (
			priv->ui_manager,
			"/ui/MainMenubar/FileMenu/SavePatch");
		gtk_action_set_sensitive (action, TRUE);
	}

	g_object_unref (priv->current_job);
	priv->current_job = NULL;
}

static void
window_revision_cell_data_log_func (GtkTreeViewColumn *column,
				    GtkCellRenderer   *cell,
				    GtkTreeModel      *model,
				    GtkTreeIter       *iter,
				    gpointer           data)
{
	GiggleWindowPriv *priv;
	GiggleRevision   *revision;

	priv = GET_PRIV (data);

	gtk_tree_model_get (model, iter,
			    REVISION_COL_OBJECT, &revision,
			    -1);

	g_object_set (cell,
		      "text", giggle_revision_get_short_log (revision),
		      NULL);
	
	g_object_unref (revision);
}

static void
window_revision_cell_data_author_func (GtkTreeViewColumn *column,
				       GtkCellRenderer   *cell,
				       GtkTreeModel      *model,
				       GtkTreeIter       *iter,
				       gpointer           data)
{
	GiggleWindowPriv *priv;
	GiggleRevision   *revision;

	priv = GET_PRIV (data);

	gtk_tree_model_get (model, iter,
			    REVISION_COL_OBJECT, &revision,
			    -1);

	g_object_set (cell,
		      "text", giggle_revision_get_author (revision),
		      NULL);

	g_object_unref (revision);
}

static void
window_revision_cell_data_date_func (GtkTreeViewColumn *column,
				     GtkCellRenderer   *cell,
				     GtkTreeModel      *model,
				     GtkTreeIter       *iter,
				     gpointer           data)
{
	GiggleWindowPriv *priv;
	GiggleRevision   *revision;

	priv = GET_PRIV (data);

	gtk_tree_model_get (model, iter,
			    REVISION_COL_OBJECT, &revision,
			    -1);

	g_object_set (cell,
		      "text", giggle_revision_get_date (revision),
		      NULL);

	g_object_unref (revision);
}

static void
window_action_quit_cb (GtkAction    *action,
		       GiggleWindow *window)
{
	gtk_main_quit ();
}

static void
window_action_open_cb (GtkAction    *action,
		       GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GtkWidget        *file_chooser;
	const gchar      *directory;

	priv = GET_PRIV (window);

	file_chooser = gtk_file_chooser_dialog_new (
		_("Select GIT repository"),
		GTK_WINDOW (window),
		GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		NULL);

	gtk_file_chooser_set_current_folder (
		GTK_FILE_CHOOSER (file_chooser),
		giggle_git_get_directory (priv->git));

	if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_OK) {
		directory = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (file_chooser));

		/* FIXME: no error handling */
		giggle_git_set_directory (priv->git, directory, NULL);
	}

	gtk_widget_destroy (file_chooser);
}

static void
window_action_save_patch_cb (GtkAction    *action,
			     GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GtkWidget        *file_chooser;
	GtkTextBuffer    *text_buffer;
	GtkTextIter       iter_start, iter_end;
	gchar            *text, *path;
	GError           *error = NULL;

	priv = GET_PRIV (window);

	file_chooser = gtk_file_chooser_dialog_new (
		_("Save patch file"),
		GTK_WINDOW (window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		NULL);

	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (file_chooser), TRUE);

	/* FIXME: remember the last selected folder */

	if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_OK) {
		text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->diff_textview));
		gtk_text_buffer_get_bounds (text_buffer, &iter_start, &iter_end);

		text = gtk_text_buffer_get_text (text_buffer, &iter_start, &iter_end, TRUE);
		path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));

		if (!g_file_set_contents (path, text, strlen (text), &error)) {
			GtkWidget *dialog;

			dialog = gtk_message_dialog_new (
				GTK_WINDOW (window),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				_("There was an error saving to file: \n%s"),
				error->message);

			gtk_dialog_run (GTK_DIALOG (dialog));

			gtk_widget_destroy (dialog);
			g_error_free (error);
		}
	}

	gtk_widget_destroy (file_chooser);
}

static void
window_action_about_cb (GtkAction    *action,
			GiggleWindow *window)
{
	gtk_show_about_dialog (GTK_WINDOW (window),
			       "name", "Giggle",
			       "copyright", "Copyright 2007 Imendio AB",
			       "translator-credits", _("translator-credits"),
			       NULL);
}

static void
window_directory_changed_cb (GiggleGit    *git,
			     GParamSpec   *arg,
			     GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GiggleJob        *job;
	gchar            *title;
	const gchar      *directory;

	priv = GET_PRIV (window);

	directory = giggle_git_get_directory (git);
	title = g_strdup_printf ("%s - Giggle", directory);
	gtk_window_set_title (GTK_WINDOW (window), title);
	g_free (title);

	/* empty the treeview */
	gtk_tree_view_set_model (GTK_TREE_VIEW (priv->revision_treeview), NULL);

	job = giggle_git_revisions_new ();
	giggle_git_run_job (priv->git, job,
			    window_git_get_revisions_cb,
			    window);

	job = giggle_git_branches_new ();
	giggle_git_run_job (priv->git, job,
			    window_git_get_branches_cb,
			    window);
}

GtkWidget *
giggle_window_new (void)
{
	GtkWidget *window;

	window = g_object_new (GIGGLE_TYPE_WINDOW, NULL);

	return window;
}

