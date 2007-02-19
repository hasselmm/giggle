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
#include "giggle-author.h"
#include "giggle-branch.h"
#include "giggle-error.h"
#include "giggle-git.h"
#include "giggle-git-authors.h"
#include "giggle-git-branches.h"
#include "giggle-git-diff.h"
#include "giggle-git-diff-tree.h"
#include "giggle-git-revisions.h"
#include "giggle-revision.h"
#include "giggle-graph-renderer.h"
#include "giggle-personal-details-window.h"
#include "giggle-file-list.h"
#include "eggfindbar.h"

typedef struct GiggleWindowPriv GiggleWindowPriv;

struct GiggleWindowPriv {
	GtkWidget           *content_vbox;
	GtkWidget           *menubar_hbox;
	/* Summary Tab */
	GtkWidget           *label_summary;
	GtkWidget           *label_project_path;
	GtkWidget           *textview_description;
	GtkWidget           *save_description;
	GtkWidget           *restore_description;
	GtkWidget           *treeview_branches;
	GtkWidget           *treeview_authors;
	/* History Tab */
	GtkWidget           *revision_treeview;
	GtkWidget           *log_textview;
	GtkWidget           *diff_textview;

	GtkCellRenderer     *graph_renderer;
	
	GtkUIManager        *ui_manager;

	GtkRecentManager    *recent_manager;
	GtkActionGroup      *recent_action_group;

	GtkWidget           *find_bar;

	GiggleGit           *git;

	GtkWidget           *file_list;
	GtkWidget           *personal_details_window;

	/* Current jobs in progress. */
	GiggleJob           *current_diff_job;
	GiggleJob           *current_diff_tree_job;
};

enum {
	REVISION_COL_OBJECT,
	REVISION_NUM_COLS
};

enum {
	AUTHORS_COL_AUTHOR,
	AUTHORS_N_COLUMNS
};

enum {
	BRANCHES_COL_BRANCH,
	BRANCHES_N_COLUMNS
};

enum {
	SEARCH_NEXT,
	SEARCH_PREV
};


static void window_finalize                       (GObject           *object);
static void window_setup_branches_treeview        (GiggleWindow      *window);
static void window_setup_authors_treeview         (GiggleWindow      *window);
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
static void window_git_diff_tree_result_callback  (GiggleGit         *git,
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
static void window_action_find_cb                 (GtkAction         *action,
						   GiggleWindow      *window);
static void window_action_personal_details_cb     (GtkAction         *action,
						   GiggleWindow      *window);
static void window_action_about_cb                (GtkAction         *action,
						   GiggleWindow      *window);
static void window_description_changed_cb         (GiggleGit         *git,
						   GParamSpec        *arg,
						   GiggleWindow      *window);
static void window_description_modified_cb        (GiggleWindow      *window);
static void window_save_description_cb            (GiggleWindow      *window);
static void window_restore_description_cb         (GiggleWindow      *window);
static void window_directory_changed_cb           (GiggleGit         *git,
						   GParamSpec        *arg,
						   GiggleWindow      *window);
static void window_git_dir_changed_cb             (GiggleGit         *git,
						   GParamSpec        *arg,
						   GiggleWindow      *window);
static void window_notify_project_dir_cb          (GiggleWindow      *window);
static void window_notify_project_name_cb         (GiggleWindow      *window);
static void window_recent_repositories_update     (GiggleWindow      *window);

static void window_find_next                      (GtkWidget         *widget,
						   GiggleWindow      *window);
static void window_find_previous                  (GtkWidget         *widget,
						   GiggleWindow      *window);

static const GtkActionEntry action_entries[] = {
	{ "ProjectMenu", NULL,
	  N_("_Project"), NULL, NULL,
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
	{ "SavePatch", GTK_STOCK_SAVE,
	  N_("_Save patch"), "<control>S", N_("Save a patch"),
	  G_CALLBACK (window_action_save_patch_cb)
	},
	{ "Quit", GTK_STOCK_QUIT,
	  N_("_Quit"), "<control>Q", N_("Quit the application"),
	  G_CALLBACK (window_action_quit_cb)
	},
	{ "PersonalDetails", GTK_STOCK_PREFERENCES,
	  N_("Edit Personal _Details"), NULL, N_("Edit Personal details"),
	  G_CALLBACK (window_action_personal_details_cb)
	},
	{ "Find", GTK_STOCK_FIND,
	  N_("Find..."), NULL, N_("Find..."),
	  G_CALLBACK (window_action_find_cb)
	},
	{ "About", GTK_STOCK_ABOUT,
	  N_("_About"), NULL, N_("About this application"),
	  G_CALLBACK (window_action_about_cb)
	}
};

static const gchar *ui_layout =
	"<ui>"
	"  <menubar name='MainMenubar'>"
	"    <menu action='ProjectMenu'>"
	"      <menuitem action='Open'/>"
	"      <menuitem action='SavePatch'/>"
	"      <separator/>"
	"      <placeholder name='RecentRepositories'/>"
	"      <separator/>"
	"      <menuitem action='Quit'/>"
	"    </menu>"
	"    <menu action='EditMenu'>"
	"      <menuitem action='PersonalDetails'/>"
	"      <separator/>"
	"      <menuitem action='Find'/>"
	"    </menu>"
	"    <menu action='HelpMenu'>"
	"      <menuitem action='About'/>"
	"    </menu>"
	"  </menubar>"
	"</ui>";


G_DEFINE_TYPE (GiggleWindow, giggle_window, GTK_TYPE_WINDOW)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_WINDOW, GiggleWindowPriv))

#define RECENT_FILES_GROUP "giggle"
#define SAVE_PATCH_UI_PATH "/ui/MainMenubar/ProjectMenu/SavePatch"
#define RECENT_REPOS_PLACEHOLDER_PATH "/ui/MainMenubar/ProjectMenu/RecentRepositories"

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

	action = gtk_ui_manager_get_action (priv->ui_manager, SAVE_PATCH_UI_PATH);
	gtk_action_set_sensitive (action, FALSE);

	/* create recent repositories resources */
	priv->recent_action_group = gtk_action_group_new ("RecentRepositories");
	gtk_ui_manager_insert_action_group (priv->ui_manager, priv->recent_action_group, 0);

	priv->recent_manager = gtk_recent_manager_get_default ();
	g_signal_connect_swapped (priv->recent_manager, "changed",
				  G_CALLBACK (window_recent_repositories_update), window);

	window_recent_repositories_update (window);
}

static void
giggle_window_init (GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	gchar            *dir;
	GladeXML         *xml;
	GError           *error = NULL;

	priv = GET_PRIV (window);

	priv->git = giggle_git_get ();
	g_signal_connect (priv->git,
			  "notify::description",
			  G_CALLBACK (window_description_changed_cb),
			  window);
	g_signal_connect (priv->git,
			  "notify::directory",
			  G_CALLBACK (window_directory_changed_cb),
			  window);
	g_signal_connect (priv->git,
			  "notify::git-dir",
			  G_CALLBACK (window_git_dir_changed_cb),
			  window);
	g_signal_connect_swapped (priv->git,
				  "notify::project-dir",
				  G_CALLBACK (window_notify_project_dir_cb),
				  window);
	g_signal_connect_swapped (priv->git,
				  "notify::project-name",
				  G_CALLBACK (window_notify_project_name_cb),
				  window);

	xml = glade_xml_new (GLADEDIR "/main-window.glade",
			     "content_vbox", NULL);
	if (!xml) {
		g_error ("Couldn't find glade file, did you install?");
	}

	/* Summary Tab */
	priv->label_summary = glade_xml_get_widget (xml, "label_project_summary");
	priv->label_project_path = glade_xml_get_widget (xml, "label_project_path");

	priv->textview_description = glade_xml_get_widget (xml, "textview_project_description");
	g_signal_connect_swapped (gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview_description)),
				  "modified-changed",
				  G_CALLBACK (window_description_modified_cb),
				  window);

	priv->save_description = glade_xml_get_widget (xml, "save_description");
	g_signal_connect_swapped (priv->save_description,
				  "clicked",
				  G_CALLBACK (window_save_description_cb),
				  window);
	priv->restore_description = glade_xml_get_widget (xml, "restore_description");
	g_signal_connect_swapped (priv->restore_description,
				  "clicked",
				  G_CALLBACK (window_restore_description_cb),
				  window);

	priv->treeview_branches = glade_xml_get_widget (xml, "treeview_branches");
	window_setup_branches_treeview (window);

	priv->treeview_authors = glade_xml_get_widget (xml, "treeview_authors");
	window_setup_authors_treeview (window);

	/* History Tab */
	priv->content_vbox = glade_xml_get_widget (xml, "content_vbox");
	gtk_container_add (GTK_CONTAINER (window), priv->content_vbox);

	priv->menubar_hbox = glade_xml_get_widget (xml, "menubar_hbox");
	priv->revision_treeview = glade_xml_get_widget (xml, "revision_treeview");
	window_setup_revision_treeview (window);

	priv->log_textview = glade_xml_get_widget (xml, "log_textview");

	window_setup_diff_textview (
		window,
		glade_xml_get_widget (xml, "diff_scrolledwindow"));

	priv->file_list = giggle_file_list_new ();
	gtk_widget_show (priv->file_list);
	gtk_container_add (GTK_CONTAINER (glade_xml_get_widget (xml, "file_view_scrolledwindow")), priv->file_list);

	g_object_unref (xml);

	window_create_menu (window);

	/* parse GIT_DIR into dir and unset it; if empty use the current_wd */
	dir = g_strdup (g_getenv ("GIT_DIR"));
	if (!dir || !*dir) {
		g_free (dir);
		dir = g_get_current_dir ();
	}
	g_unsetenv ("GIT_DIR");

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

	/* setup find bar */
	priv->find_bar = egg_find_bar_new ();
	gtk_box_pack_end (GTK_BOX (priv->content_vbox), priv->find_bar, FALSE, FALSE, 0);

	g_signal_connect (G_OBJECT (priv->find_bar), "close",
			  G_CALLBACK (gtk_widget_hide), NULL);
	g_signal_connect (G_OBJECT (priv->find_bar), "next",
			  G_CALLBACK (window_find_next), window);
	g_signal_connect (G_OBJECT (priv->find_bar), "previous",
			  G_CALLBACK (window_find_previous), window);

	/* personal details window */
	priv->personal_details_window = giggle_personal_details_window_new ();

	gtk_window_set_transient_for (GTK_WINDOW (priv->personal_details_window),
				      GTK_WINDOW (window));
	g_signal_connect_after (G_OBJECT (priv->personal_details_window), "response",
				G_CALLBACK (gtk_widget_hide), NULL);
}

static void
window_finalize (GObject *object)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (object);
	
	g_object_unref (priv->ui_manager);

	if (priv->current_diff_job) {
		giggle_git_cancel_job (priv->git, priv->current_diff_job);
		g_object_unref (priv->current_diff_job);
	}

	if (priv->current_diff_tree_job) {
		giggle_git_cancel_job (priv->git, priv->current_diff_tree_job);
		g_object_unref (priv->current_diff_tree_job);
	}

	g_object_unref (priv->git);
	g_object_unref (priv->recent_manager);
	g_object_unref (priv->recent_action_group);

	G_OBJECT_CLASS (giggle_window_parent_class)->finalize (object);
}

static void
window_recent_repositories_add (GiggleWindow *window)
{
	static gchar     *groups[] = { RECENT_FILES_GROUP, NULL };
	GiggleWindowPriv *priv;
	GtkRecentData    *data;
	const gchar      *repository;
	gchar            *tmp_string;

	priv = GET_PRIV (window);

	repository = giggle_git_get_project_dir (priv->git);
	if(!repository) {
		repository = giggle_git_get_git_dir (priv->git);
	}

	g_return_if_fail (repository != NULL);

	data = g_slice_new0 (GtkRecentData);
	data->display_name = g_strdup (giggle_git_get_project_name (priv->git));
	data->groups = groups;
	data->mime_type = g_strdup ("x-directory/normal");
	data->app_name = (gchar *) g_get_application_name ();
	data->app_exec = g_strjoin (" ", g_get_prgname (), "%u", NULL);

	tmp_string = g_filename_to_uri (repository, NULL, NULL);
	gtk_recent_manager_add_full (priv->recent_manager,
                                     tmp_string, data);
	g_free (tmp_string);
}

static void
window_recent_repository_activate (GtkAction    *action,
				   GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	const gchar      *directory;

	priv = GET_PRIV (window);

	directory = g_object_get_data (G_OBJECT (action), "recent-action-path");
	giggle_git_set_directory (priv->git, directory, NULL);
}

static void
window_recent_repositories_clear (GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GList            *actions, *l;

	priv = GET_PRIV (window);
	actions = l = gtk_action_group_list_actions (priv->recent_action_group);

	for (l = actions; l != NULL; l = l->next) {
		gtk_action_group_remove_action (priv->recent_action_group, l->data);
	}

	g_list_free (actions);
}

/* this should not be necessary when there's
 * GtkRecentManager/GtkUIManager integration
 */
static void
window_recent_repositories_reload (GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GList            *recent_items, *l;
	GtkRecentInfo    *info;
	GtkAction        *action;
	guint             recent_menu_id;
	gchar            *action_name, *label;
	gint              count = 0;

	priv = GET_PRIV (window);

	recent_items = l = gtk_recent_manager_get_items (priv->recent_manager);
	recent_menu_id = gtk_ui_manager_new_merge_id (priv->ui_manager);

	/* FIXME: the max count is hardcoded */
	while (l && count < 10) {
		info = l->data;

		if (gtk_recent_info_has_group (info, RECENT_FILES_GROUP)) {
			action_name = g_strdup_printf ("recent-repository-%d", count);
			label = g_strdup (gtk_recent_info_get_uri_display (info));

			/* FIXME: add accel? */

			action = gtk_action_new (action_name,
						 label,
						 NULL,
						 NULL);

			g_object_set_data_full (G_OBJECT (action), "recent-action-path",
						g_strdup (gtk_recent_info_get_uri_display (info)),
						(GDestroyNotify) g_free);

			g_signal_connect (action,
					  "activate",
					  G_CALLBACK (window_recent_repository_activate),
					  window);

			gtk_action_group_add_action (priv->recent_action_group, action);

			gtk_ui_manager_add_ui (priv->ui_manager,
					       recent_menu_id,
					       RECENT_REPOS_PLACEHOLDER_PATH,
					       action_name,
					       action_name,
					       GTK_UI_MANAGER_MENUITEM,
					       FALSE);

			g_object_unref (action);
			g_free (action_name);
			g_free (label);
			count++;
		}

		l = l->next;
	}

	g_list_foreach (recent_items, (GFunc) gtk_recent_info_unref, NULL);
	g_list_free (recent_items);
}

static void
window_recent_repositories_update (GiggleWindow *window)
{
	window_recent_repositories_clear (window);
	window_recent_repositories_reload (window);
}

static void
window_show_error (GiggleWindow *window,
		   const gchar  *message,
		   GError       *error)
{
	GtkWidget *dialog;

	g_return_if_fail (error != NULL);

	dialog = gtk_message_dialog_new (GTK_WINDOW (window),
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_OK,
					 _(message), error->message);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
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

	if (error) {
		window_show_error (window,
				   N_("An error ocurred when getting the revisions list:\n%s"),
				   error);
		g_error_free (error);
	} else {
		store = gtk_list_store_new (REVISION_NUM_COLS, GIGGLE_TYPE_REVISION);
		revisions = giggle_git_revisions_get_revisions (GIGGLE_GIT_REVISIONS (job));

		while (revisions) {
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter,
					    REVISION_COL_OBJECT, revisions->data,
					    -1);
			revisions = revisions->next;
		}

		giggle_graph_renderer_validate_model (GIGGLE_GRAPH_RENDERER (priv->graph_renderer), GTK_TREE_MODEL (store), 0);
		gtk_tree_view_set_model (GTK_TREE_VIEW (priv->revision_treeview), GTK_TREE_MODEL (store));
		g_object_unref (store);
	}

	g_object_unref (job);
}

static void
window_git_get_branches_cb (GiggleGit    *git,
			    GiggleJob    *job,
			    GError       *error,
			    gpointer      user_data)
{
	GiggleWindow     *window;
	GiggleWindowPriv *priv;
	GtkListStore     *store;
	GtkTreeIter       iter;
	GList            *branches;

	window = GIGGLE_WINDOW (user_data);
	priv = GET_PRIV (window);

	if (error) {
		window_show_error (window,
				   N_("An error ocurred when retrieving branches list:%s"),
				   error);
		g_error_free (error);
	} else {
		store = gtk_list_store_new (BRANCHES_N_COLUMNS, GIGGLE_TYPE_BRANCH);
		branches = giggle_git_branches_get_branches (GIGGLE_GIT_BRANCHES (job));

		for(; branches; branches = g_list_next (branches)) {
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter,
					    BRANCHES_COL_BRANCH, branches->data,
					    -1);
		}

		gtk_tree_view_set_model (GTK_TREE_VIEW (priv->treeview_branches), GTK_TREE_MODEL (store));
		g_object_unref (store);
	}

	g_object_unref (job);
}

static void
window_branches_cell_data_func (GtkTreeViewColumn *column,
				GtkCellRenderer   *cell,
				GtkTreeModel      *model,
				GtkTreeIter       *iter,
				gpointer           data)
{
	GiggleBranch *branch = NULL;

	// FIXME: check whether we're leaking references here
	gtk_tree_model_get (model, iter,
			    BRANCHES_COL_BRANCH, &branch,
			    -1);
	g_object_set (cell, "text", giggle_branch_get_name (branch), NULL);
}

static void
window_setup_branches_treeview (GiggleWindow *window)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (window);
	gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (priv->treeview_branches), -1,
						    _("Branch"), gtk_cell_renderer_text_new (),
						    window_branches_cell_data_func, NULL, NULL);
}

static void
window_git_get_authors_cb (GiggleGit    *git,
			   GiggleJob    *job,
			   GError       *error,
			   gpointer      user_data)
{
	GiggleWindowPriv *priv;
	GtkListStore     *store;
	GtkTreeIter       iter;
	GList            *authors;

	priv = GET_PRIV (user_data);

	store = gtk_list_store_new (AUTHORS_N_COLUMNS, GIGGLE_TYPE_AUTHOR);
	authors = giggle_git_authors_get_list (GIGGLE_GIT_AUTHORS (job));

	for (; authors; authors = g_list_next (authors)) {
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    AUTHORS_COL_AUTHOR, authors->data,
				    -1);
	}

	gtk_tree_view_set_model (GTK_TREE_VIEW (priv->treeview_authors), GTK_TREE_MODEL (store));
	g_object_unref (store);
	g_object_unref (job);
}

static void
window_authors_cell_data_func (GtkTreeViewColumn *column,
			       GtkCellRenderer   *cell,
			       GtkTreeModel      *model,
			       GtkTreeIter       *iter,
			       gpointer           data)
{
	GiggleAuthor *author = NULL;

	// FIXME: check whether we're leaking references here
	gtk_tree_model_get (model, iter,
			    AUTHORS_COL_AUTHOR, &author,
			    -1);
	g_object_set (cell, "text", giggle_author_get_string (author), NULL);
}

static void
window_setup_authors_treeview (GiggleWindow *window)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (window);
	gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (priv->treeview_authors), -1,
						    _("Author"), gtk_cell_renderer_text_new (),
						    window_authors_cell_data_func, NULL, NULL);
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

	if (priv->current_diff_job) {
		giggle_git_cancel_job (priv->git, priv->current_diff_job);
		g_object_unref (priv->current_diff_job);
		priv->current_diff_job = NULL;
	}

	if (priv->current_diff_tree_job) {
		giggle_git_cancel_job (priv->git, priv->current_diff_tree_job);
		g_object_unref (priv->current_diff_tree_job);
		priv->current_diff_tree_job = NULL;
	}
	
	if (current_revision && previous_revision) {
		action = gtk_ui_manager_get_action (priv->ui_manager, SAVE_PATCH_UI_PATH);
		gtk_action_set_sensitive (action, FALSE);

		priv->current_diff_job = giggle_git_diff_tree_new (previous_revision, current_revision);
		giggle_git_run_job (priv->git,
				    priv->current_diff_job,
				    window_git_diff_tree_result_callback,
				    window);

		priv->current_diff_tree_job = giggle_git_diff_new (previous_revision, current_revision);
		giggle_git_run_job (priv->git,
				    priv->current_diff_tree_job,
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
	valid = FALSE;

	/* clear file list highlights */
	giggle_file_list_set_highlight_files (GIGGLE_FILE_LIST (priv->file_list), NULL);

	if (!rows) {
		return;
	}

	/* get the first row iter */
	gtk_tree_model_get_iter (model, &first_iter,
				 (GtkTreePath *) rows->data);

	if (g_list_length (rows) > 1) {
		last_row = g_list_last (rows);
		valid = gtk_tree_model_get_iter (model, &last_iter,
						 (GtkTreePath *) last_row->data);
	} else {
		valid = FALSE;
	}

	gtk_tree_model_get (model, &first_iter,
			    REVISION_COL_OBJECT, &first_revision,
			    -1);
	if (valid) {
		gtk_tree_model_get (model, &last_iter,
				    REVISION_COL_OBJECT, &last_revision,
				    -1);
	} else {
		/* maybe select a better parent? */
		GList* parents = giggle_revision_get_parents (first_revision);
		last_revision = parents ? g_object_ref(parents->data) : NULL;
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
		window_show_error (window,
				   N_("An error ocurred when retrieving a diff:\n%s"),
				   error);
		g_error_free (error);
	} else {
		gtk_text_buffer_set_text (
			gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->diff_textview)),
			giggle_git_diff_get_result (GIGGLE_GIT_DIFF (job)),
			-1);

		action = gtk_ui_manager_get_action (
			priv->ui_manager,
			SAVE_PATCH_UI_PATH);
		gtk_action_set_sensitive (action, TRUE);
	}

	g_object_unref (priv->current_diff_job);
	priv->current_diff_job = NULL;
}

static void
window_git_diff_tree_result_callback (GiggleGit *git,
				      GiggleJob *job,
				      GError    *error,
				      gpointer   user_data)
{
	GiggleWindow     *window;
	GiggleWindowPriv *priv;
	GList            *list;

	window = GIGGLE_WINDOW (user_data);
	priv = GET_PRIV (window);

	if (error) {
		window_show_error (window,
				   N_("An error ocurred when retrieving different files list:\n%s"),
				   error);
		g_error_free (error);
	} else {
		list = giggle_git_diff_tree_get_files (GIGGLE_GIT_DIFF_TREE (job));
		giggle_file_list_set_highlight_files (GIGGLE_FILE_LIST (priv->file_list), list);
	}

	g_object_unref (priv->current_diff_tree_job);
	priv->current_diff_tree_job = NULL;
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
			window_show_error (window,
					   N_("An error ocurred when saving to file:\n%s"),
					   error);
			g_error_free (error);
		}
	}

	gtk_widget_destroy (file_chooser);
}

static void
window_action_find_cb (GtkAction    *action,
		       GiggleWindow *window)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (window);

	gtk_widget_show (priv->find_bar);
	gtk_widget_grab_focus (priv->find_bar);
}

static gboolean
revision_property_matches (GiggleRevision *revision,
			   const gchar    *property,
			   const gchar    *search_string)
{
	gboolean  match;
	gchar    *str;

	g_object_get (revision, property, &str, NULL);
	match = strstr (str, search_string) != NULL;
	g_free (str);

	return match;
}

static gboolean
revision_matches (GiggleRevision *revision,
		  const gchar    *search_string)
{
	return (revision_property_matches (revision, "author", search_string) ||
		revision_property_matches (revision, "long-log", search_string) ||
		revision_property_matches (revision, "sha", search_string));
}

static void
window_find (GiggleWindow *window,
	     const gchar  *search_string,
	     gint          direction)
{
	GiggleWindowPriv *priv;
	GtkTreeModel     *model;
	GtkTreeSelection *selection;
	GList            *list;
	GtkTreeIter       iter;
	gboolean          valid, found;
	GiggleRevision   *revision;
	GtkTreePath      *path;
	
	priv = GET_PRIV (window);
	found = FALSE;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->revision_treeview));
	list = gtk_tree_selection_get_selected_rows (selection, &model);

	/* Find the first/current element */
	if (list) {
		if (direction == SEARCH_NEXT) {
			path = gtk_tree_path_copy (list->data);
			gtk_tree_path_next (path);
			valid = TRUE;
		} else {
			path = gtk_tree_path_copy ((g_list_last (list))->data);
			valid = gtk_tree_path_prev (path);
		}
	} else {
		path = gtk_tree_path_new_first ();
		valid = TRUE;
	}

	while (valid && !found) {
		valid = gtk_tree_model_get_iter (model, &iter, path);

		if (!valid) {
			break;
		}

		gtk_tree_model_get (model, &iter, 0, &revision, -1);
		found = revision_matches (revision, search_string);
		g_object_unref (revision);

		if (!found) {
			if (direction == SEARCH_NEXT) {
				gtk_tree_path_next (path);
			} else {
				valid = gtk_tree_path_prev (path);
			}
		}
	}

	if (found) {
		gtk_tree_selection_unselect_all (selection);
		gtk_tree_selection_select_iter (selection, &iter);

		/* scroll to row */
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (priv->revision_treeview),
					      path, NULL, FALSE, 0., 0.);
	}

	gtk_tree_path_free (path);
	g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (list);
}

static void
window_find_next (GtkWidget    *widget,
		  GiggleWindow *window)
{
	const gchar *search_string;

	search_string = egg_find_bar_get_search_string (EGG_FIND_BAR (widget));
	window_find (window, search_string, SEARCH_NEXT);
}

static void
window_find_previous (GtkWidget    *widget,
		      GiggleWindow *window)
{
	const gchar *search_string;

	search_string = egg_find_bar_get_search_string (EGG_FIND_BAR (widget));
	window_find (window, search_string, SEARCH_PREV);
}

static void
window_action_personal_details_cb (GtkAction    *action,
				   GiggleWindow *window)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (window);

	gtk_widget_show (priv->personal_details_window);
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
}

static void
window_git_dir_changed_cb (GiggleGit    *git,
			   GParamSpec   *arg,
			   GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GiggleJob        *job;

	priv = GET_PRIV (window);

	/* Update Branches */
	gtk_tree_view_set_model (GTK_TREE_VIEW (priv->treeview_branches), NULL);
	job = giggle_git_branches_new ();
	giggle_git_run_job (priv->git, job,
			    window_git_get_branches_cb,
			    window);

	job = giggle_git_authors_new ();
	giggle_git_run_job (priv->git, job,
			    window_git_get_authors_cb,
			    window);
}

static void
window_notify_project_dir_cb (GiggleWindow* window)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (window);
	gtk_label_set_text (GTK_LABEL (priv->label_project_path),
			    giggle_git_get_project_dir (priv->git));

	/* add repository uri to recents */
	window_recent_repositories_add (window);
}

static void
window_notify_project_name_cb (GiggleWindow* window)
{
	GiggleWindowPriv *priv;
	gchar            *markup;

	priv = GET_PRIV (window);
	markup = g_strdup_printf ("<span weight='bold' size='xx-large'>%s</span>",
				  giggle_git_get_project_name (priv->git));

	gtk_label_set_markup (GTK_LABEL (priv->label_summary), markup);

	g_free (markup);
}

static void
window_save_description_cb (GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GtkTextBuffer    *buffer;
	GtkTextIter       start;
	GtkTextIter       end;
	gchar            *text;

	priv = GET_PRIV (window);
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview_description));
	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_end_iter (buffer, &end);

	text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
	giggle_git_write_description (priv->git, text);
	g_free (text);
}

static void
window_restore_description_cb (GiggleWindow *window)
{
	GiggleWindowPriv* priv;
	GtkTextBuffer    *buffer;

	priv = GET_PRIV (window);
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview_description));
	gtk_text_buffer_set_text (buffer, giggle_git_get_description (priv->git), -1);
	gtk_text_buffer_set_modified (buffer, FALSE);
}

static void
window_description_modified_cb (GiggleWindow *window)
{
	/* called when the description within the GtkTextView changes */
	GiggleWindowPriv* priv;
	gboolean modified;

	priv = GET_PRIV (window);

	modified = gtk_text_buffer_get_modified (gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview_description)));
	gtk_widget_set_sensitive (priv->save_description, modified);
	gtk_widget_set_sensitive (priv->restore_description, modified);
}

static void
window_description_changed_cb (GiggleGit    *git,
			       GParamSpec   *pspec,
			       GiggleWindow *window)
{
	/* called when the description of the repository changes */
	GiggleWindowPriv *priv;

	priv = GET_PRIV (window);

	window_restore_description_cb (window);
}

GtkWidget *
giggle_window_new (void)
{
	GtkWidget *window;

	window = g_object_new (GIGGLE_TYPE_WINDOW, NULL);

	return window;
}

