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

#include "giggle-window.h"
#include "giggle-error.h"
#include "giggle-git.h"
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
	guint                git_job_id;
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
static void window_git_get_diff_callback          (GiggleGit         *git,
						   guint              id,
						   GError            *error,
						   GiggleRevision    *rev1,
						   GiggleRevision    *rev2,
						   const gchar       *diff,
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
static void window_action_about_cb                (GtkAction         *action,
						   GiggleWindow      *window);

/* Just for testing. */
static GtkTreeModel * window_create_test_model    (GiggleWindow      *window);


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
	"      <menuitem action='Quit'/>"
	"    </menu>"
	"    <menu action='EditMenu'>"
	"    </menu>"
	"    <menu action='HelpMenu'>"
	"      <menuitem action='About'/>"
	"    </menu>"
	"  </menubar>"
	"</ui>";


G_DEFINE_TYPE (GiggleWindow, giggle_window, GTK_TYPE_WINDOW);

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_WINDOW, GiggleWindowPriv))

static void
giggle_window_class_init (GiggleWindowClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = window_finalize;

	g_type_class_add_private (object_class, sizeof (GiggleWindowPriv));
}

static void
giggle_window_init (GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	gchar            *dir;
	GladeXML         *xml;
	GtkActionGroup   *action_group;
	GError           *error = NULL;

	priv = GET_PRIV (window);

	gtk_window_set_title (GTK_WINDOW (window), "Giggle");

	priv->git = giggle_git_new ();
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
}

static void
window_finalize (GObject *object)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (object);
	
	g_object_unref (priv->ui_manager);

	if (priv->git_job_id) {
		giggle_git_cancel (priv->git, priv->git_job_id);
	}

	g_object_unref (priv->git);

	G_OBJECT_CLASS (giggle_window_parent_class)->finalize (object);
}

static void
window_setup_revision_treeview (GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GtkTreeModel     *model;
	GtkCellRenderer  *cell;
	GtkTreeSelection *selection;

	priv = GET_PRIV (window);

	priv->graph_renderer = giggle_graph_renderer_new (NULL);
	gtk_tree_view_insert_column_with_attributes (
		GTK_TREE_VIEW (priv->revision_treeview),
		-1,
		_("Graph"),
		priv->graph_renderer,
		"revision", REVISION_COL_OBJECT, 
		NULL);

	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_data_func (
		GTK_TREE_VIEW (priv->revision_treeview),
		-1,
		_("Short Log"),
		cell,
		window_revision_cell_data_log_func,
		window,
		NULL);

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

	model = window_create_test_model (window);
	gtk_tree_view_set_model (GTK_TREE_VIEW (priv->revision_treeview), model);
	g_object_unref (model);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->revision_treeview));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

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

	if (priv->git_job_id) {
		giggle_git_cancel (priv->git, priv->git_job_id);
		priv->git_job_id = 0;
	}
	
	if (current_revision && previous_revision) {
		priv->git_job_id = giggle_git_get_diff (priv->git,
							previous_revision,
							current_revision,
							window_git_get_diff_callback,
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
	GtkTreeIter       iter;
	GiggleRevision   *current_revision;
	GiggleRevision   *previous_revision;

	priv = GET_PRIV (window);
	
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		return;
	}

	gtk_tree_model_get (model, &iter,
			    REVISION_COL_OBJECT, &current_revision,
			    -1);

	/* Get the revision below the current, for a diff. */
	if (gtk_tree_model_iter_next (model, &iter)) {
		gtk_tree_model_get (model, &iter,
				    REVISION_COL_OBJECT, &previous_revision,
				    -1);
	} else {
		previous_revision = NULL;
	}
	
	window_update_revision_info (window,
				     current_revision,
				     previous_revision);

	g_object_unref (current_revision);
	if (previous_revision) {
		g_object_unref (previous_revision);
	}
}

static void
window_git_get_diff_callback (GiggleGit      *git,
			      guint           id,
			      GError         *error,
			      GiggleRevision *rev1,
			      GiggleRevision *rev2,
			      const gchar    *diff,
			      gpointer        user_data)
{
	GiggleWindow     *window;
	GiggleWindowPriv *priv;

	window = GIGGLE_WINDOW (user_data);
	priv = GET_PRIV (window);

	priv->git_job_id = 0;

	if (error) {
		if (error->domain == GIGGLE_ERROR &&
		    error->code != GIGGLE_ERROR_DISPATCH_CANCELLED) {
			GtkWidget *dialog;
			
			dialog = gtk_message_dialog_new (GTK_WINDOW (window),
							 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_MESSAGE_ERROR,
							 GTK_BUTTONS_OK,
							 _("An error ocurred when retrieving a diff:\n"
							   "%s"), error->message);
			
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		}
	} else {
		gtk_text_buffer_set_text (
			gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->diff_textview)),
			diff, -1);
	}
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
window_action_about_cb (GtkAction    *action,
			GiggleWindow *window)
{
	gtk_show_about_dialog (GTK_WINDOW (window),
			       "name", "Giggle",
			       "copyright", "Copyright 2007 Imendio AB",
			       NULL);
}

GtkWidget *
giggle_window_new (void)
{
	GtkWidget *window;

	window = g_object_new (GIGGLE_TYPE_WINDOW, NULL);

	return window;
}

/* Test data. */
static GtkTreeModel *
window_create_test_model (GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GtkListStore     *store;
	GtkTreeIter       iter;
	GiggleRevision   *revision;
	GiggleBranchInfo *branch1;
	GiggleBranchInfo *branch2;
	GList            *branches = NULL;

	priv = GET_PRIV (window);
	
	branch1 = giggle_branch_info_new ("master");
	branch2 = giggle_branch_info_new ("foo");

	branches = g_list_prepend (branches, branch2);
	branches = g_list_prepend (branches, branch1);

	store = gtk_list_store_new (1, GIGGLE_TYPE_REVISION);

	revision = giggle_revision_new_commit ("74228d16a271120ac56300e93cd181b63df15bbd", branch1);
	g_object_set (revision,
		      "author", "Richard Hult <richard@imendio.com>",
		      "short-log", "Make the patch view use a monospace font",
		      "date", "2007-01-12",
		      NULL);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    REVISION_COL_OBJECT, revision,
			    -1);

	revision = giggle_revision_new_merge ("da3c13dc29c5223dab2310d54fe5a55e65ada939", branch1, branch2);
	g_object_set (revision,
		      "author", "Mikael Hallendal <micke@imendio.com>",
		      "short-log", "Add cancel method to dispatcher",
		      "date", "2007-01-13",
		      NULL);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    REVISION_COL_OBJECT, revision,
			    -1);

	revision = giggle_revision_new_commit ("cef4b2fa248bf280e792d029ee33ea8214cf1d0f", branch1);
	g_object_set (revision,
		      "author", "Carlos Garnacho <carlosg@gnome.org>",
		      "short-log", "Add more colors for branches",
		      "date", "2007-01-13",
		      NULL);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    REVISION_COL_OBJECT, revision,
			    -1);

	revision = giggle_revision_new_commit ("5af23ca9a027ae0e2f7590eadb3d9ac2fee1ce65", branch2);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    REVISION_COL_OBJECT, revision,
			    -1);

	revision = giggle_revision_new_branch ("87dc870236e299d5201c5296864a0e9dbd2abfb8", branch1, branch2);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    REVISION_COL_OBJECT, revision,
			    -1);

	revision = giggle_revision_new_commit ("12077469fb225db75a4f9c03a922feb588988a95", branch1);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    REVISION_COL_OBJECT, revision,
			    -1);

	g_object_set (priv->graph_renderer,
		      "branches-info", branches,
		      NULL);

	giggle_revision_validate (GTK_TREE_MODEL (store), 0);

	return GTK_TREE_MODEL (store);
}
