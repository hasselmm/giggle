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
#include "giggle-remote-editor.h"
#include "giggle-revision.h"
#include "giggle-graph-renderer.h"
#include "giggle-personal-details-window.h"
#include "giggle-file-list.h"
#include "giggle-revision-list.h"
#include "giggle-revision-view.h"
#include "giggle-diff-view.h"
#include "giggle-view-summary.h"
#include "giggle-view-history.h"
#include "giggle-view-file.h"
#include "eggfindbar.h"

typedef struct GiggleWindowPriv GiggleWindowPriv;

struct GiggleWindowPriv {
	GtkWidget           *content_vbox;
	GtkWidget           *menubar_hbox;
	GtkWidget           *main_notebook;

	/* Views */
	GtkWidget           *summary_view;
	GtkWidget           *history_view;
	GtkWidget           *file_view;

	/* Menu */
	GtkUIManager        *ui_manager;
	GtkRecentManager    *recent_manager;
	GtkActionGroup      *recent_action_group;

	GtkWidget           *find_bar;

	GiggleGit           *git;

	GtkWidget           *personal_details_window;
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
	REMOTES_COL_REMOTE,
	REMOTES_N_COLUMNS
};

enum {
	SEARCH_NEXT,
	SEARCH_PREV
};


static void window_finalize                       (GObject           *object);

static void window_add_widget_cb                  (GtkUIManager      *merge,
						   GtkWidget         *widget,
						   GiggleWindow      *window);

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
static void window_directory_changed_cb           (GiggleGit         *git,
						   GParamSpec        *arg,
						   GiggleWindow      *window);
static void window_recent_repositories_add        (GiggleWindow      *window);
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
			  "notify::directory",
			  G_CALLBACK (window_directory_changed_cb),
			  window);
	g_signal_connect_swapped (priv->git,
				  "notify::project-dir",
				  G_CALLBACK (window_recent_repositories_add),
				  window);

	xml = glade_xml_new (GLADEDIR "/main-window.glade",
			     "content_vbox", NULL);
	if (!xml) {
		g_error ("Couldn't find glade file, did you install?");
	}

	priv->main_notebook = glade_xml_get_widget (xml, "main_notebook");

	priv->content_vbox = glade_xml_get_widget (xml, "content_vbox");
	gtk_container_add (GTK_CONTAINER (window), priv->content_vbox);

	priv->menubar_hbox = glade_xml_get_widget (xml, "menubar_hbox");

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

	/* append summary view */
	priv->summary_view = giggle_view_summary_new ();
	gtk_widget_show (priv->summary_view);

	gtk_notebook_append_page (GTK_NOTEBOOK (priv->main_notebook),
				  priv->summary_view,
				  gtk_label_new ("Summary"));

	/* append history view */
	priv->history_view = giggle_view_history_new ();
	gtk_widget_show (priv->history_view);

	gtk_notebook_append_page (GTK_NOTEBOOK (priv->main_notebook),
				  priv->history_view,
				  gtk_label_new ("History"));

	/* append file view */
	priv->file_view = giggle_view_file_new ();
	gtk_widget_show (priv->file_view);

	gtk_notebook_append_page (GTK_NOTEBOOK (priv->main_notebook),
				  priv->file_view,
				  gtk_label_new ("Files"));
}

static void
window_finalize (GObject *object)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (object);
	
	g_object_unref (priv->ui_manager);
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

		giggle_view_history_set_model (GIGGLE_VIEW_HISTORY (priv->history_view), GTK_TREE_MODEL (store));
		g_object_unref (store);
	}

	g_object_unref (job);
}

/* Update revision info. If previous_revision is not NULL, a diff between it and
 * the current revision will be shown.
 */

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
/* FIXME: implement this again with GiggleView */
#if 0
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
#endif
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

/* FIXME: implement these again with GiggleView */
#if 0
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
#endif

static void
window_find (GiggleWindow *window,
	     const gchar  *search_string,
	     gint          direction)
{
/* FIXME: implement this again with GiggleView */
#if 0
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

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->revision_list));
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
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (priv->revision_list),
					      path, NULL, FALSE, 0., 0.);
	}

	gtk_tree_path_free (path);
	g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (list);
#endif
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

	job = giggle_git_revisions_new ();
	giggle_git_run_job (priv->git, job,
			    window_git_get_revisions_cb,
			    window);
}

GtkWidget *
giggle_window_new (void)
{
	GtkWidget *window;

	window = g_object_new (GIGGLE_TYPE_WINDOW, NULL);

	return window;
}

GiggleGit *
giggle_window_get_git (GiggleWindow *self)
{
	g_return_val_if_fail (GIGGLE_IS_WINDOW (self), NULL);

	return GET_PRIV (self)->git;
}

