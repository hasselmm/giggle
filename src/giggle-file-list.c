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
#include <string.h>

#include "giggle-file-list.h"

#include "libgiggle/giggle-git.h"
#include "libgiggle/giggle-git-ignore.h"
#include "libgiggle/giggle-git-list-files.h"
#include "libgiggle/giggle-git-add.h"
#include "libgiggle/giggle-git-diff.h"
#include "libgiggle/giggle-git-diff-tree.h"

#include "libgiggle/giggle-clipboard.h"
#include "libgiggle/giggle-revision.h"
#include "libgiggle/giggle-enums.h"

#include "giggle-diff-window.h"
#include "giggle-helpers.h"

typedef struct GiggleFileListPriv GiggleFileListPriv;

struct GiggleFileListPriv {
	GiggleGit      *git;
	GtkIconTheme   *icon_theme;

	GtkTreeStore   *store;
	GtkTreeModel   *filter_model;

	GtkWidget      *popup;
	GtkUIManager   *ui_manager;

	GiggleJob      *job;

	GtkWidget      *diff_window;

	GHashTable     *idle_jobs;

	GiggleRevision *revision_from;
	GiggleRevision *revision_to;

	guint           show_all : 1;

	char           *selected_path;
};

typedef struct IdleLoaderData IdleLoaderData;

struct IdleLoaderData {
	GiggleFileList *list;
	gchar          *directory;
	gchar          *rel_path;
	GtkTreeIter     parent_iter;
};


static void file_list_add_element		(GiggleFileList *list,
						 const gchar    *directory,
						 const gchar    *rel_path,
						 const gchar    *name,
						 GtkTreeIter    *parent_iter);

static void giggle_file_list_clipboard_init	(GiggleClipboardIface *iface);


G_DEFINE_TYPE_WITH_CODE (GiggleFileList, giggle_file_list, GTK_TYPE_TREE_VIEW,
			 G_IMPLEMENT_INTERFACE (GIGGLE_TYPE_CLIPBOARD,
						giggle_file_list_clipboard_init))

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_FILE_LIST, GiggleFileListPriv))

enum {
	COL_NAME,
	COL_REL_PATH,
	COL_FILE_STATUS,
	COL_GIT_IGNORE,
	COL_HIGHLIGHT,
	LAST_COL
};

enum {
	PROP_0,
	PROP_SHOW_ALL,
};

enum {
	PATH_SELECTED,
	PROJECT_LOADED,
	STATUS_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };


static void
file_list_finalize (GObject *object)
{
	GiggleFileListPriv *priv;

	priv = GET_PRIV (object);

	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	g_object_unref (priv->git);

	if (priv->store) {
		g_object_unref (priv->store);
	}
	if (priv->filter_model) {
		g_object_unref (priv->filter_model);
	}

	g_object_unref (priv->ui_manager);

	g_hash_table_destroy (priv->idle_jobs);

	if (priv->revision_from) {
		g_object_unref (priv->revision_from);
	}

	if (priv->revision_to) {
		g_object_unref (priv->revision_to);
	}

	g_free (priv->selected_path);

	G_OBJECT_CLASS (giggle_file_list_parent_class)->finalize (object);
}

static void
file_list_get_property (GObject    *object,
			guint       param_id,
			GValue     *value,
			GParamSpec *pspec)
{
	GiggleFileListPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_SHOW_ALL:
		g_value_set_boolean (value, priv->show_all);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
file_list_set_property (GObject      *object,
			guint         param_id,
			const GValue *value,
			GParamSpec   *pspec)
{
	GiggleFileListPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_SHOW_ALL:
		giggle_file_list_set_show_all (GIGGLE_FILE_LIST (object),
					       g_value_get_boolean (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
file_list_get_path_and_ignore_for_iter (GiggleFileList   *list,
					GtkTreeIter      *iter,
					gchar           **path,
					GiggleGitIgnore **git_ignore)
{
	GiggleFileListPriv *priv;
	GtkTreeIter         parent;

	priv = GET_PRIV (list);

	if (!gtk_tree_model_iter_parent (priv->filter_model, &parent, iter)) {
		if (path) {
			*path = NULL;
		}

		if (git_ignore) {
			*git_ignore = NULL;
		}

		return FALSE;
	}

	if (path) {
		gtk_tree_model_get (GTK_TREE_MODEL (priv->filter_model), iter,
				    COL_REL_PATH, path,
				    -1);
	}

	if (git_ignore) {
		gtk_tree_model_get (GTK_TREE_MODEL (priv->filter_model), &parent,
				    COL_GIT_IGNORE, git_ignore,
				    -1);
	}

	return TRUE;
}

static gboolean
file_list_button_press (GtkWidget      *widget,
			GdkEventButton *event)
{
	GiggleFileList           *list;
	GiggleFileListPriv       *priv;
	gboolean                  add, ignore, unignore;
	gchar                    *file_path;
	GiggleGitIgnore          *git_ignore;
	GtkAction                *action;
	GtkTreeSelection         *selection;
	GtkTreeModel             *model;
	GList                    *rows, *l;
	GtkTreeIter               iter;
	GtkTreePath              *path;
	GiggleGitListFilesStatus  status;

	list = GIGGLE_FILE_LIST (widget);
	priv = GET_PRIV (list);
	ignore = unignore = add = FALSE;

	if (event->button == 3) {
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));

		if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y,
						    &path, NULL, NULL, NULL)) {
			return TRUE;
		}

		if ((event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) != 0) {
			/* we want shift+click and ctrl+click behave as always here */
			GTK_WIDGET_CLASS (giggle_file_list_parent_class)->button_press_event (widget, event);
		} else if (!gtk_tree_selection_path_is_selected (selection, path)) {
			gtk_tree_selection_unselect_all (selection);
			gtk_tree_selection_select_path (selection, path);
		}

		gtk_tree_path_free (path);
		rows = gtk_tree_selection_get_selected_rows (selection, &model);

		for (l = rows; l; l = l->next) {
			gtk_tree_model_get_iter (model, &iter, l->data);

			gtk_tree_model_get (model, &iter,
					    COL_FILE_STATUS, &status,
					    -1);

			add = (status == GIGGLE_GIT_FILE_STATUS_OTHER);

			if (file_list_get_path_and_ignore_for_iter (list, &iter, &file_path, &git_ignore)) {
				if (giggle_git_ignore_path_matches (git_ignore, file_path)) {
					unignore = TRUE;
				} else {
					ignore = TRUE;
				}

				g_object_unref (git_ignore);
				g_free (file_path);
			}
		}

		action = gtk_ui_manager_get_action (priv->ui_manager, "/ui/PopupMenu/AddFile");
		gtk_action_set_sensitive (action, add);
		action = gtk_ui_manager_get_action (priv->ui_manager, "/ui/PopupMenu/Ignore");
		gtk_action_set_sensitive (action, ignore);
		action = gtk_ui_manager_get_action (priv->ui_manager, "/ui/PopupMenu/Unignore");
		gtk_action_set_sensitive (action, unignore);

		gtk_menu_popup (GTK_MENU (priv->popup), NULL, NULL,
				NULL, NULL, event->button, event->time);

		g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (rows);
	} else {
		GTK_WIDGET_CLASS (giggle_file_list_parent_class)->button_press_event (widget, event);

		if (event->button == 1 &&
		    (event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) == 0 &&
		    event->type == GDK_2BUTTON_PRESS) {
			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
			rows = gtk_tree_selection_get_selected_rows (selection, &model);

			if (rows) {
				gtk_tree_model_get_iter (model, &iter, rows->data);

				file_list_get_path_and_ignore_for_iter (list, &iter, &file_path, NULL);
				g_signal_emit (widget, signals[PATH_SELECTED], 0, file_path);
				g_free (file_path);
			}
		}
	}

	return TRUE;
}

static void
file_list_update_files_status (GiggleFileList     *file_list,
			       GtkTreeIter        *parent,
			       GiggleGitListFiles *list_files)
{
	GiggleFileListPriv        *priv;
	GtkTreeIter               iter;
	gboolean                  valid;
	gchar                    *rel_path;
	GiggleGitListFilesStatus  status;

	priv = GET_PRIV (file_list);

	if (parent) {
		valid = gtk_tree_model_iter_children (GTK_TREE_MODEL (priv->store),
						      &iter, parent);
	} else {
		valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store), &iter);
	}

	while (valid) {
		gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter,
				    COL_REL_PATH, &rel_path,
				    -1);

		if (rel_path) {
			status = giggle_git_list_files_get_file_status (list_files, rel_path);
		} else {
			status = GIGGLE_GIT_FILE_STATUS_CACHED;
		}

		if (gtk_tree_model_iter_has_child (GTK_TREE_MODEL (priv->store), &iter)) {
			/* it's a directory */
			file_list_update_files_status (file_list, &iter, list_files);
			status = GIGGLE_GIT_FILE_STATUS_CACHED;
		}

		gtk_tree_store_set (priv->store, &iter,
				    COL_FILE_STATUS, status,
				    -1);

		g_free (rel_path);
		valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->store), &iter);
	}
}

static void
file_list_files_callback (GiggleGit *git,
			  GiggleJob *job,
			  GError    *error,
			  gpointer   user_data)
{
	GiggleFileList     *list;
	GiggleFileListPriv *priv;

	list = GIGGLE_FILE_LIST (user_data);
	priv = GET_PRIV (list);

	if (error) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (list))),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("An error ocurred when retrieving the file list:\n%s"),
						 error->message);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	} else {
		file_list_update_files_status (list, NULL, GIGGLE_GIT_LIST_FILES (priv->job));
		g_signal_emit (list, signals[STATUS_CHANGED], 0);
	}

	g_object_unref (priv->job);
	priv->job = NULL;
}

static void
file_list_files_status_changed (GiggleFileList *list)
{
	GiggleFileListPriv *priv;

	priv = GET_PRIV (list);

	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	priv->job = giggle_git_list_files_new ();

	giggle_git_run_job (priv->git,
			    priv->job,
			    file_list_files_callback,
			    list);
}

static void
file_list_project_loaded (GiggleFileList *list)
{
	/* update files status */
	file_list_files_status_changed (list);
}

static void
file_list_status_changed (GiggleFileList *list)
{
	GiggleFileListPriv *priv;
	GtkTreePath        *path;

	if (gtk_tree_view_get_model (GTK_TREE_VIEW (list))) {
		return;
	}

	priv = GET_PRIV (list);
	gtk_tree_view_set_model (GTK_TREE_VIEW (list), priv->filter_model);

	/* expand the project folder */
	path = gtk_tree_path_new_first ();
	gtk_tree_view_expand_row (GTK_TREE_VIEW (list), path, FALSE);
	gtk_tree_path_free (path);

	if (priv->selected_path)
		giggle_tree_view_select_row_by_string (GTK_WIDGET (list),
						       COL_REL_PATH, priv->selected_path);
}

static void
file_list_row_activated (GtkTreeView       *tree_view,
                         GtkTreePath       *path,
                         GtkTreeViewColumn *column)
{
	GiggleFileListPriv *priv = GET_PRIV (tree_view);
	GtkAction          *action;

	action = gtk_ui_manager_get_action (priv->ui_manager, "/ui/PopupMenu/Edit");
	gtk_action_activate (action);

	if (GTK_TREE_VIEW_CLASS (giggle_file_list_parent_class)->row_activated)
		GTK_TREE_VIEW_CLASS (giggle_file_list_parent_class)->row_activated (tree_view, path, column);
}

static void
giggle_file_list_class_init (GiggleFileListClass *class)
{
	GObjectClass     *object_class    = G_OBJECT_CLASS      (class);
	GtkWidgetClass   *widget_class    = GTK_WIDGET_CLASS    (class);
	GtkTreeViewClass *tree_view_class = GTK_TREE_VIEW_CLASS (class);

	object_class->finalize           = file_list_finalize;
	object_class->get_property       = file_list_get_property;
	object_class->set_property       = file_list_set_property;

	widget_class->button_press_event = file_list_button_press;

	tree_view_class->row_activated   = file_list_row_activated;

	class->project_loaded            = file_list_project_loaded;
	class->status_changed            = file_list_status_changed;

	g_object_class_install_property (object_class,
					 PROP_SHOW_ALL,
					 g_param_spec_boolean ("show-all",
							       "Show all",
							       "Whether to show all elements",
							       FALSE,
							       G_PARAM_READWRITE));
	signals[PATH_SELECTED] =
		g_signal_new ("path-selected",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GiggleFileListClass, path_selected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE,
			      1, G_TYPE_STRING);

	signals[PROJECT_LOADED] =
		g_signal_new ("project-loaded",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GiggleFileListClass, project_loaded),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	signals[STATUS_CHANGED] =
		g_signal_new ("status-changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GiggleFileListClass, status_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	g_type_class_add_private (object_class, sizeof (GiggleFileListPriv));
}

static gboolean
file_list_can_copy (GiggleClipboard *clipboard)
{
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (clipboard));
	return gtk_tree_selection_count_selected_rows (selection) > 0;
}

static void
file_list_do_copy (GiggleClipboard *clipboard)
{
	GiggleFileList *list = GIGGLE_FILE_LIST (clipboard);
	GtkClipboard   *widget_clipboard;
	GList          *selection;
	GString        *text;

	text = g_string_new (NULL);
	selection = giggle_file_list_get_selection (list);

	while (selection) {
		if (text->len)
			g_string_append_c (text, ' ');

		g_string_append (text, selection->data);

		g_free (selection->data);
		selection = g_list_delete_link (selection, selection);
	}

	widget_clipboard = gtk_widget_get_clipboard (GTK_WIDGET (list),
						     GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text (widget_clipboard, text->str, text->len);

	g_string_free (text, TRUE);
}

static void
giggle_file_list_clipboard_init (GiggleClipboardIface *iface)
{
	iface->can_copy = file_list_can_copy;
	iface->do_copy  = file_list_do_copy;
}

static gboolean
file_list_get_ignore_file (GtkTreeModel *model,
			   GtkTreeIter  *file_iter,
			   const gchar  *path)
{
	GiggleGitIgnore *git_ignore;
	GtkTreeIter      iter, parent;
	gboolean         matches = FALSE;

	iter = *file_iter;

	while (!matches && gtk_tree_model_iter_parent (model, &parent, &iter)) {
		gtk_tree_model_get (model, &parent,
				    COL_GIT_IGNORE, &git_ignore,
				    -1);

		if (git_ignore) {
			matches = giggle_git_ignore_path_matches (git_ignore, path);
			g_object_unref (git_ignore);
		}

		/* scale up through the hierarchy */
		iter = parent;
	}

	return matches;
}

static gboolean
file_list_filter_func (GtkTreeModel   *model,
		       GtkTreeIter    *iter,
		       gpointer        user_data)
{
	GiggleFileList     *list;
	GiggleFileListPriv *priv;
	gchar              *path;
	gboolean            retval = TRUE;

	list = GIGGLE_FILE_LIST (user_data);
	priv = GET_PRIV (list);

	gtk_tree_model_get (model, iter,
			    COL_REL_PATH, &path,
			    -1);
	if (!path) {
		return FALSE;
	}

	/* we never want to show these files */
	if (g_str_has_suffix (path, ".git") ||
	    g_str_has_suffix (path, ".gitignore") ||
	    (!priv->show_all && file_list_get_ignore_file (model, iter, path))) {
		retval = FALSE;
	}

	g_free (path);

	return retval;
}

static gint
file_list_compare_func (GtkTreeModel *model,
			GtkTreeIter  *iter1,
			GtkTreeIter  *iter2,
			gpointer      user_data)
{
	GiggleGitIgnore *git_ignore1, *git_ignore2;
	gchar           *name1, *name2;
	gint             retval = 0;

	gtk_tree_model_get (model, iter1,
			    COL_GIT_IGNORE, &git_ignore1,
			    COL_NAME, &name1,
			    -1);
	gtk_tree_model_get (model, iter2,
			    COL_GIT_IGNORE, &git_ignore2,
			    COL_NAME, &name2,
			    -1);

	if (git_ignore1 && !git_ignore2) {
		retval = -1;
	} else if (git_ignore2 && !git_ignore1) {
		retval = 1;
	} else {
		retval = strcmp (name1, name2);
	}

	/* free stuff */
	if (git_ignore1) {
		g_object_unref (git_ignore1);
	}

	if (git_ignore2) {
		g_object_unref (git_ignore2);
	}

	g_free (name1);
	g_free (name2);

	return retval;
}

static void
file_list_create_store (GiggleFileList *list)
{
	GiggleFileListPriv *priv;

	priv = GET_PRIV (list);

	gtk_tree_view_set_model (GTK_TREE_VIEW (list), NULL);

	if (priv->store) {
		g_object_unref (priv->store);
	}

	if (priv->filter_model) {
		g_object_unref (priv->filter_model);
	}

	priv->store = gtk_tree_store_new (LAST_COL, G_TYPE_STRING, G_TYPE_STRING, GIGGLE_TYPE_GIT_LIST_FILES_STATUS,
					  GIGGLE_TYPE_GIT_IGNORE, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
	priv->filter_model = gtk_tree_model_filter_new (GTK_TREE_MODEL (priv->store), NULL);

	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (priv->filter_model),
						file_list_filter_func, list, NULL);

	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (priv->store),
					 COL_NAME,
					 file_list_compare_func,
					 list, NULL);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (priv->store),
					      COL_NAME, GTK_SORT_ASCENDING);
}

static void
file_list_edit_file (GtkAction      *action,
		     GiggleFileList *list)
{
	GiggleFileListPriv*priv = GET_PRIV (list);
	GAppLaunchContext *context;
	GList             *selection, *l;
	char              *filename, *uri;
	GError            *error = NULL;
	const char        *dir;

	context = giggle_create_app_launch_context (GTK_WIDGET (list));
	selection = giggle_file_list_get_selection (list);
	dir = giggle_git_get_directory (priv->git);

	for (l = selection; l; l = l->next) {
		filename = g_build_filename (dir, l->data, NULL);
		uri = g_filename_to_uri (filename, NULL, &error);
		g_free (filename);

		if (!uri) {
			g_warning ("%s: %s", G_STRFUNC, error->message);
			g_clear_error (&error);
			continue;
		}

		if (!g_app_info_launch_default_for_uri (uri, context, &error)) {
			g_warning ("%s: %s", G_STRFUNC, error->message);
			g_clear_error (&error);
		}

		g_free (uri);
	}

	g_list_foreach (selection, (GFunc) g_free, NULL);
	g_list_free (selection);

	g_object_unref (context);
}

static void
file_list_diff_file (GtkAction      *action,
		     GiggleFileList *list)
{
	GiggleFileListPriv *priv;
	GtkWidget          *toplevel;
	GList              *files;

	priv = GET_PRIV (list);

	files = giggle_file_list_get_selection (list);
	giggle_diff_window_set_files (GIGGLE_DIFF_WINDOW (priv->diff_window), files);

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (list));
	gtk_window_set_transient_for (GTK_WINDOW (priv->diff_window),
				      GTK_WINDOW (toplevel));
	gtk_widget_show (priv->diff_window);
}

static void
file_list_add_file_callback (GiggleGit *git,
			     GiggleJob *job,
			     GError    *error,
			     gpointer   user_data)
{
	GiggleFileList     *list;
	GiggleFileListPriv *priv;

	list = GIGGLE_FILE_LIST (user_data);
	priv = GET_PRIV (list);

	if (error) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (list))),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("An error ocurred when adding a file to git:\n%s"),
						 error->message);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		g_object_unref (priv->job);
		priv->job = NULL;
	} else {
		g_object_unref (priv->job);
		priv->job = NULL;

		file_list_files_status_changed (list);
	}
}

static void
file_list_add_file (GtkAction      *action,
		    GiggleFileList *list)
{
	GiggleFileListPriv *priv;

	priv = GET_PRIV (list);

	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	priv->job = giggle_git_add_new ();
	giggle_git_add_set_files (GIGGLE_GIT_ADD (priv->job),
				  giggle_file_list_get_selection (list));

	giggle_git_run_job (priv->git,
			    priv->job,
			    file_list_add_file_callback,
			    list);
}

static void
file_list_create_patch_callback (GiggleGit *git,
				 GiggleJob *job,
				 GError    *error,
				 gpointer   user_data)
{
	GiggleFileList     *list;
	GiggleFileListPriv *priv;
	GtkWidget          *dialog;
	const gchar        *filename;
	gchar              *primary_str;

	list = GIGGLE_FILE_LIST (user_data);
	priv = GET_PRIV (list);

	filename = g_object_get_data (G_OBJECT (priv->job), "create-patch-filename");

	if (error) {
		const gchar *secondary_str;
		
		primary_str = g_strdup_printf (_("Could not save the patch as %s"), filename);
		
		if (error->message) {
			secondary_str = error->message;
		} else {
			secondary_str = _("No error was given");
		}

		dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (list))),
							     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							     GTK_MESSAGE_ERROR,
							     GTK_BUTTONS_OK,
							     "<b>%s</b>",
							     primary_str);
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", secondary_str);

		g_free (primary_str);
	} else {
		const gchar *result;
		GError      *save_error = NULL;

		result = giggle_git_diff_get_result (GIGGLE_GIT_DIFF (priv->job));
		
		if (!g_file_set_contents (filename, result, -1, &save_error)) {
			const gchar *secondary_str;

			primary_str = g_strdup_printf (_("Could not save the patch as %s"), filename);

			if (save_error && save_error->message) {
				secondary_str = save_error->message;
			} else {
				secondary_str = _("No error was given");
			}

			dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (list))),
								     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
								     GTK_MESSAGE_ERROR,
								     GTK_BUTTONS_OK,
								     "<b>%s</b>",
								     primary_str);
			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", secondary_str);

			g_free (primary_str);
			g_error_free (save_error);
		} else {
			gchar *dirname;
			gchar *basename;
			gchar *secondary_str;
			
			dirname = g_path_get_dirname (filename);
			basename = g_path_get_basename (filename);

			primary_str = g_strdup_printf (_("Patch saved as %s"), basename);
			g_free (basename);
			
			if (!dirname || strcmp (dirname, ".") == 0) {
				secondary_str = g_strdup_printf (_("Created in project directory"));
				g_free (dirname);
			} else {
				secondary_str = g_strdup_printf (_("Created in directory %s"), dirname);
				g_free (dirname);
			}
		
			dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (list))),
								     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
								     GTK_MESSAGE_INFO,
								     GTK_BUTTONS_OK,
								     "<b>%s</b>",
								     primary_str);
			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
								  "%s", secondary_str);
			
			g_free (secondary_str);
			g_free (primary_str);
		}
	}
	
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	
	g_object_unref (priv->job);
	priv->job = NULL;
}

static void
file_list_create_patch (GtkAction      *action,
			GiggleFileList *list)
{
	GiggleFileListPriv *priv;
	GtkWidget          *dialog;
	gchar              *filename;

	priv = GET_PRIV (list);

	dialog = gtk_file_chooser_dialog_new (_("Create Patch"), 
					      GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (list))),
					      GTK_FILE_CHOOSER_ACTION_SAVE,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					      NULL);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), TRUE);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_ACCEPT) {
		gtk_widget_destroy (dialog);
		return;
	}
	
	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	gtk_widget_destroy (dialog);

	if (!filename || filename[0] == '\0') {
		return;
	}

	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	priv->job = giggle_git_diff_new ();
	giggle_git_diff_set_revisions (GIGGLE_GIT_DIFF (priv->job),
				       priv->revision_from,
				       priv->revision_to);
	giggle_git_diff_set_files (GIGGLE_GIT_DIFF (priv->job),
				   giggle_file_list_get_selection (list));

	/* Remember the filename */
	g_object_set_data_full (G_OBJECT (priv->job), "create-patch-filename", filename, g_free);

	giggle_git_run_job (priv->git,
			    priv->job,
			    file_list_create_patch_callback,
			    list);
}

static void
file_list_ignore_file_foreach (GtkTreeModel *model,
			       GtkTreePath  *path,
			       GtkTreeIter  *iter,
			       gpointer      data)
{
	GiggleGitIgnore *git_ignore;
	gchar           *name;

	if (!file_list_get_path_and_ignore_for_iter (GIGGLE_FILE_LIST (data),
						     iter, &name, &git_ignore)) {
		return;
	}

	if (git_ignore) {
		giggle_git_ignore_add_glob_for_path (git_ignore, name);
		g_object_unref (git_ignore);
	}

	g_free (name);
}

static void
file_list_ignore_file (GtkAction      *action,
		       GiggleFileList *list)
{
	GiggleFileListPriv *priv;
	GtkTreeSelection   *selection;

	priv = GET_PRIV (list);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	gtk_tree_selection_selected_foreach (selection, file_list_ignore_file_foreach, list);
	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter_model));
}

static void
file_list_unignore_file_foreach (GtkTreeModel *model,
				 GtkTreePath  *tree_path,
				 GtkTreeIter  *iter,
				 gpointer      data)
{
	GiggleFileList  *list;
	GiggleGitIgnore *git_ignore;
	gchar           *path;

	list = GIGGLE_FILE_LIST (data);

	if (!file_list_get_path_and_ignore_for_iter (list, iter, &path, &git_ignore)) {
		return;
	}

	if (git_ignore) {
		if (!giggle_git_ignore_remove_glob_for_path (git_ignore, path, TRUE)) {
			GtkWidget *dialog, *toplevel;

			toplevel = gtk_widget_get_toplevel (GTK_WIDGET (list));
			dialog = gtk_message_dialog_new (GTK_WINDOW (toplevel),
							 GTK_DIALOG_MODAL,
							 GTK_MESSAGE_INFO,
							 GTK_BUTTONS_YES_NO,
							 _("Delete glob pattern?"));

			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
								  _("The selected file was shadowed by a glob pattern "
								    "that may be hiding other files, delete it?"));

			if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES) {
				giggle_git_ignore_remove_glob_for_path (git_ignore, path, FALSE);
			}

			gtk_widget_destroy (dialog);
		}

		g_object_unref (git_ignore);
	}

	g_free (path);
}

static void
file_list_unignore_file (GtkAction      *action,
			 GiggleFileList *list)
{
	GiggleFileListPriv *priv;
	GtkTreeSelection   *selection;

	priv = GET_PRIV (list);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	gtk_tree_selection_selected_foreach (selection, file_list_unignore_file_foreach, list);
	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter_model));
}

static void
file_list_toggle_show_all (GtkAction      *action,
			   GiggleFileList *list)
{
	GiggleFileListPriv *priv;
	gboolean active;

	priv = GET_PRIV (list);
	active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	giggle_file_list_set_show_all (list, active);
}

static void
file_list_idle_data_free (IdleLoaderData *data)
{
	g_free (data->directory);
	g_free (data->rel_path);
	g_object_unref (data->list);
	g_free (data);
}

static void
file_list_populate_dir (GiggleFileList *list,
			const gchar    *directory,
			const gchar    *rel_path,
			GtkTreeIter    *parent_iter)
{
	GiggleFileListPriv *priv;
	GDir               *dir;
	const gchar        *name;
	gchar              *full_path, *path;

	priv = GET_PRIV (list);
	full_path = g_build_filename (directory, rel_path, NULL);
	dir = g_dir_open (full_path, 0, NULL);

	g_return_if_fail (dir != NULL);

	while ((name = g_dir_read_name (dir))) {
		path = g_build_filename (rel_path, name, NULL);
		file_list_add_element (list, directory, path, name, parent_iter);
		g_free (path);
	}

	g_free (full_path);
	g_dir_close (dir);
}

static gboolean
file_list_populate_dir_idle (gpointer user_data)
{
	GiggleFileListPriv *priv;
	GiggleFileList     *list;
	IdleLoaderData     *data;
	GiggleGitIgnore    *git_ignore;
	gchar              *full_path;

	data = (IdleLoaderData *) user_data;
	list = g_object_ref (data->list);
	priv = GET_PRIV (list);

	full_path = g_build_filename (data->directory, data->rel_path, NULL);
	git_ignore = giggle_git_ignore_new (full_path);

	gtk_tree_store_set (priv->store, &data->parent_iter,
			    COL_GIT_IGNORE, git_ignore,
			    -1);

	file_list_populate_dir (data->list, data->directory,
				data->rel_path, &data->parent_iter);

	/* remove this data from the table */
	g_hash_table_remove (priv->idle_jobs, data);

	if (g_hash_table_size (priv->idle_jobs) == 0) {
		/* no remaining jobs */
		g_signal_emit (list, signals[PROJECT_LOADED], 0);
	}

	g_object_unref (git_ignore);
	g_object_unref (list);
	g_free (full_path);

	return FALSE;
}

static void
file_list_add_element (GiggleFileList *list,
		       const gchar    *directory,
		       const gchar    *rel_path,
		       const gchar    *name,
		       GtkTreeIter    *parent_iter)
{
	GiggleFileListPriv *priv;
	GtkTreeIter         iter;
	gboolean            is_dir;
	gchar              *full_path;

	priv = GET_PRIV (list);

	full_path = g_build_filename (directory, rel_path, NULL);
	is_dir = g_file_test (full_path, G_FILE_TEST_IS_DIR);

	gtk_tree_store_append (priv->store,
			       &iter,
			       parent_iter);

	gtk_tree_store_set (priv->store, &iter,
			    COL_NAME, (name) ? name : full_path,
			    COL_REL_PATH, rel_path,
			    -1);

	if (is_dir) {
		IdleLoaderData *data;
		guint           idle_id;

		data = g_new0 (IdleLoaderData, 1);
		data->list = g_object_ref (list);
		data->directory = g_strdup (directory);
		data->rel_path = g_strdup (rel_path);
		data->parent_iter = iter;

		idle_id = gdk_threads_add_idle_full (GDK_PRIORITY_REDRAW + 1,
						     file_list_populate_dir_idle,
						     data, NULL);

		g_hash_table_insert (priv->idle_jobs, data, GUINT_TO_POINTER (idle_id));
	}

	g_free (full_path);
}

static void
file_list_populate (GiggleFileList *list)
{
	GiggleFileListPriv *priv;
	const gchar        *directory;

	priv = GET_PRIV (list);
	directory = giggle_git_get_project_dir (priv->git);

	if (directory) {
		file_list_add_element (list, directory, "", NULL, NULL);
	}
}

static void
file_list_directory_changed (GObject    *object,
			     GParamSpec *pspec,
			     gpointer    user_data)
{
	GiggleFileListPriv *priv;
	GiggleFileList     *list;

	list = GIGGLE_FILE_LIST (user_data);
	priv = GET_PRIV (list);

	/* remove old directory idles */
	g_hash_table_remove_all (priv->idle_jobs);

	file_list_create_store (list);
	file_list_populate (list);
}

static gboolean
file_list_search_equal_func (GtkTreeModel *model,
			     gint          column,
			     const gchar  *key,
			     GtkTreeIter  *iter,
			     gpointer      search_data)
{
	gchar    *str;
	gchar    *normalized_key, *normalized_str;
	gchar    *casefold_key, *casefold_str;
	gboolean  ret;

	gtk_tree_model_get (model, iter, column, &str, -1);

	normalized_key = g_utf8_normalize (key, -1, G_NORMALIZE_ALL);
	normalized_str = g_utf8_normalize (str, -1, G_NORMALIZE_ALL);

	ret = TRUE;
	if (normalized_str && normalized_key) {
		casefold_key = g_utf8_casefold (normalized_key, -1);
		casefold_str = g_utf8_casefold (normalized_str, -1);

		if (strcmp (casefold_key, normalized_key) != 0) {
			/* Use case sensitive match when the search key has a
			 * mix of upper and lower case, mimicking vim smartcase
			 * search.
			 */
			if (strstr (normalized_str, normalized_key)) {
				ret = FALSE;
			}
		} else {
			if (strstr (casefold_str, casefold_key)) {
				ret = FALSE;
			}
		}

		g_free (casefold_key);
		g_free (casefold_str);
	}

	g_free (str);
	g_free (normalized_key);
	g_free (normalized_str);

	return ret;
}

static void
file_list_cell_data_background_func (GtkCellLayout   *cell_layout,
				     GtkCellRenderer *renderer,
				     GtkTreeModel    *tree_model,
				     GtkTreeIter     *iter,
				     gpointer         data)
{
	GdkColor            color;
	GiggleFileListPriv *priv;
	GiggleFileList     *file_list;
	gboolean            highlight;
	gchar              *rel_path;

	file_list = GIGGLE_FILE_LIST (data);
	priv = GET_PRIV (file_list);
	color = GTK_WIDGET (file_list)->style->bg[GTK_STATE_NORMAL];

	gtk_tree_model_get (tree_model, iter,
			    COL_REL_PATH, &rel_path,
			    COL_HIGHLIGHT, &highlight,
			    -1);

	g_object_set (G_OBJECT (renderer), "cell-background-gdk",
		      (rel_path && *rel_path && highlight) ? &color : NULL,
		      NULL);

	g_free (rel_path);
}

static void
file_list_cell_data_sensitive_func (GtkCellLayout   *layout,
				    GtkCellRenderer *renderer,
				    GtkTreeModel    *tree_model,
				    GtkTreeIter     *iter,
				    gpointer         data)
{
	GiggleFileList     *list;
	GiggleFileListPriv *priv;
	GiggleGitIgnore    *git_ignore = NULL;
	gchar              *path = NULL;
	gboolean            value = TRUE;
	GtkTreeIter         parent;
	GtkStateType        state;
	GdkColor            color;

	list = GIGGLE_FILE_LIST (data);
	priv = GET_PRIV (list);

	/* we want highlighted background too */
	file_list_cell_data_background_func (layout, renderer, tree_model, iter, data);

	if (priv->show_all) {
		if (file_list_get_path_and_ignore_for_iter (list, iter, &path, &git_ignore)) {
			value = ! giggle_git_ignore_path_matches (git_ignore, path);
		} else {
			/* we don't want the project root being set insensitive */
			value = ! gtk_tree_model_iter_parent (tree_model, &parent, iter);
		}
	}

	if (value) {
		GiggleGitListFilesStatus status;

		gtk_tree_model_get (tree_model, iter,
				    COL_FILE_STATUS, &status,
				    -1);

		value = !(status == GIGGLE_GIT_FILE_STATUS_OTHER);
	}

	if (GTK_IS_CELL_RENDERER_TEXT (renderer)) {
		state = (value) ? GTK_STATE_NORMAL : GTK_STATE_INSENSITIVE;
		color = GTK_WIDGET (list)->style->text [state];
		g_object_set (renderer, "foreground-gdk", &color, NULL);
	} else {
		g_object_set (renderer, "sensitive", value, NULL);
	}

	if (git_ignore) {
		g_object_unref (git_ignore);
	}
	g_free (path);
}

static void
file_list_cell_pixbuf_func (GtkCellLayout   *layout,
			    GtkCellRenderer *renderer,
			    GtkTreeModel    *tree_model,
			    GtkTreeIter     *iter,
			    gpointer         data)
{
	GiggleFileListPriv       *priv;
	GiggleGitIgnore          *git_ignore;
	GiggleGitListFilesStatus  status;
	GdkPixbuf                *pixbuf = NULL;
	const gchar              *icon_name;

	file_list_cell_data_sensitive_func (layout, renderer, tree_model, iter, data);

	priv = GET_PRIV (data);

	gtk_tree_model_get (tree_model, iter,
			    COL_FILE_STATUS, &status,
			    COL_GIT_IGNORE, &git_ignore,
			    -1);

	if (git_ignore) {
		/* it's a folder */
		icon_name = "folder";
		g_object_unref (git_ignore);
	} else {
		switch (status) {
		case GIGGLE_GIT_FILE_STATUS_OTHER:
		case GIGGLE_GIT_FILE_STATUS_CACHED:
			icon_name = "text-x-generic";
			break;
		case GIGGLE_GIT_FILE_STATUS_CHANGED:
			icon_name = "gtk-new";
			break;
		case GIGGLE_GIT_FILE_STATUS_DELETED:
		case GIGGLE_GIT_FILE_STATUS_UNMERGED:
			icon_name = "gtk-delete";
			break;
		case GIGGLE_GIT_FILE_STATUS_KILLED:
			icon_name = "gtk-stop";
			break;
		default:
			g_assert_not_reached ();
		}
	}

	if (icon_name) {
		pixbuf = gtk_icon_theme_load_icon (priv->icon_theme,
						   icon_name, 16, 0, NULL);

		g_object_set (renderer, "pixbuf", pixbuf, NULL);

		if (pixbuf) {
			g_object_unref (pixbuf);
		}
	} else {
		g_object_set (renderer, "pixbuf", NULL, NULL);
	}
}

static void
giggle_file_list_init (GiggleFileList *list)
{
	GiggleFileListPriv *priv;
	GtkCellRenderer    *renderer;
	GtkTreeViewColumn  *column;
	GtkActionGroup     *action_group;
	GtkTreeSelection   *selection;

	const GtkActionEntry menu_items [] = {
		{ "Commit",      NULL,             N_("_Commit Changes"),         NULL, NULL, G_CALLBACK (file_list_diff_file) },
		{ "Edit",        GTK_STOCK_EDIT,   NULL,                          NULL, NULL, G_CALLBACK (file_list_edit_file) },
		{ "AddFile",     NULL,             N_("A_dd file to repository"), NULL, NULL, G_CALLBACK (file_list_add_file) },
		{ "CreatePatch", NULL,             N_("Create _Patch"),           NULL, NULL, G_CALLBACK (file_list_create_patch) },
		{ "Ignore",      GTK_STOCK_ADD,    N_("_Add to .gitignore"),      NULL, NULL, G_CALLBACK (file_list_ignore_file) },
		{ "Unignore",    GTK_STOCK_REMOVE, N_("_Remove from .gitignore"), NULL, NULL, G_CALLBACK (file_list_unignore_file) },
	};

	const GtkToggleActionEntry toggle_menu_items [] = {
		{ "ShowAll",     NULL,             N_("_Show all files"),         NULL, NULL, G_CALLBACK (file_list_toggle_show_all), FALSE },
	};

	const gchar ui_description[] =
		"<ui>"
		"  <popup name='PopupMenu'>"
		"    <menuitem action='Commit'/>"
		"    <separator/>"
		"    <menuitem action='Edit'/>"
		"    <menuitem action='AddFile'/>"
		"    <separator/>"
		"    <menuitem action='CreatePatch'/>"
		"    <separator/>"
		"    <menuitem action='Ignore'/>"
		"    <menuitem action='Unignore'/>"
		"    <separator/>"
		"    <menuitem action='ShowAll'/>"
		"  </popup>"
		"</ui>";

	priv = GET_PRIV (list);

	priv->idle_jobs = g_hash_table_new_full (g_direct_hash, g_direct_equal,
						 (GDestroyNotify) file_list_idle_data_free,
						 (GDestroyNotify) g_source_remove);

	priv->git = giggle_git_get ();
	g_signal_connect (priv->git, "notify::project-dir",
			  G_CALLBACK (file_list_directory_changed), list);
	g_signal_connect_swapped (priv->git, "changed",
				  G_CALLBACK (file_list_files_status_changed), list);

	priv->icon_theme = gtk_icon_theme_get_default ();

	gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (list),
					     file_list_search_equal_func,
					     NULL, NULL);

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Project"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column), renderer, FALSE);

	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (column), renderer,
					    file_list_cell_pixbuf_func,
					    list, NULL);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (column), renderer,
					"text", COL_NAME,
					NULL);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (column), renderer,
					    file_list_cell_data_sensitive_func,
					    list, NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

	/* create the popup menu */
	action_group = gtk_action_group_new ("PopupActions");
	gtk_action_group_set_translation_domain (action_group, NULL);
	gtk_action_group_add_actions (action_group, menu_items,
				      G_N_ELEMENTS (menu_items), list);
	gtk_action_group_add_toggle_actions (action_group, toggle_menu_items,
					     G_N_ELEMENTS (toggle_menu_items), list);

	priv->ui_manager = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (priv->ui_manager, action_group, 0);

	if (gtk_ui_manager_add_ui_from_string (priv->ui_manager, ui_description, -1, NULL)) {
		priv->popup = gtk_ui_manager_get_widget (priv->ui_manager, "/ui/PopupMenu");
	}

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	/* create diff window */
	priv->diff_window = giggle_diff_window_new ();

	g_signal_connect (priv->diff_window, "delete-event",
			  G_CALLBACK (gtk_widget_hide_on_delete), NULL);
	g_signal_connect_after (priv->diff_window, "response",
				G_CALLBACK (gtk_widget_hide), NULL);
}

GtkWidget *
giggle_file_list_new (void)
{
	return g_object_new (GIGGLE_TYPE_FILE_LIST, NULL);
}

gboolean
giggle_file_list_get_show_all (GiggleFileList *list)
{
	GiggleFileListPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_FILE_LIST (list), FALSE);

	priv = GET_PRIV (list);
	return priv->show_all;
}

void
giggle_file_list_set_show_all (GiggleFileList *list,
			       gboolean        show_all)
{
	GiggleFileListPriv *priv;

	g_return_if_fail (GIGGLE_IS_FILE_LIST (list));

	priv = GET_PRIV (list);

	priv->show_all = (show_all == TRUE);
	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter_model));
	g_object_notify (G_OBJECT (list), "show-all");
}

GList *
giggle_file_list_get_selection (GiggleFileList *list)
{
	GiggleFileListPriv *priv;
	GtkTreeSelection   *selection;
	GtkTreeModel       *model;
	GtkTreeIter         iter;
	GList              *rows, *l, *files;
	gchar              *path;

	g_return_val_if_fail (GIGGLE_IS_FILE_LIST (list), NULL);

	priv = GET_PRIV (list);
	files = NULL;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	rows = gtk_tree_selection_get_selected_rows (selection, &model);

	for (l = rows; l; l = l->next) {
		gtk_tree_model_get_iter (model, &iter, l->data);

		gtk_tree_model_get (model, &iter,
				    COL_REL_PATH, &path,
				    -1);

		files = g_list_prepend (files, path);
	}

	g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (rows);

	return g_list_reverse (files);
}

void
giggle_file_list_select (GiggleFileList *list,
			 const char     *path)
{
	GiggleFileListPriv *priv;

	g_return_if_fail (GIGGLE_IS_FILE_LIST (list));

	priv = GET_PRIV (list);

	g_free (priv->selected_path);
	priv->selected_path = g_strdup (path);

	if (gtk_tree_view_get_model (GTK_TREE_VIEW (list)))
		giggle_tree_view_select_row_by_string (GTK_WIDGET (list),
						       COL_REL_PATH, path);
}

static gint
file_list_compare_prefix (gconstpointer a,
			  gconstpointer b)
{
	return ! g_str_has_prefix (a, b);
}

static void
file_list_update_highlight (GiggleFileList *file_list,
			    GtkTreeIter    *parent,
			    const gchar    *parent_path,
			    GList          *files)
{
	GiggleFileListPriv *priv;
	GtkTreeIter         iter;
	gboolean            valid;
	GiggleGitIgnore    *git_ignore;
	gchar              *rel_path;
	gboolean            highlight;

	priv = GET_PRIV (file_list);

	if (parent) {
		valid = gtk_tree_model_iter_children (GTK_TREE_MODEL (priv->store),
						      &iter, parent);
	} else {
		valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store), &iter);
	}

	while (valid) {
		gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter,
				    COL_REL_PATH, &rel_path,
				    COL_GIT_IGNORE, &git_ignore,
				    -1);

		highlight = (g_list_find_custom (files, rel_path, (GCompareFunc) file_list_compare_prefix) != NULL);

		gtk_tree_store_set (priv->store, &iter,
				    COL_HIGHLIGHT, highlight,
				    -1);

		if (highlight && git_ignore) {
			/* it's a directory */
			file_list_update_highlight (file_list, &iter, rel_path, files);
			g_object_unref (git_ignore);
		}

		g_free (rel_path);
		valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->store), &iter);
	}
}

static void
file_list_job_callback (GiggleGit *git,
			GiggleJob *job,
			GError    *error,
			gpointer   user_data)
{
	GiggleFileList     *list;
	GiggleFileListPriv *priv;
	GList              *files;

	list = GIGGLE_FILE_LIST (user_data);
	priv = GET_PRIV (list);

	if (error) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (list))),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("An error ocurred when retrieving different files list:\n%s"),
						 error->message);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	} else {
		files = giggle_git_diff_tree_get_files (GIGGLE_GIT_DIFF_TREE (priv->job));
		file_list_update_highlight (list, NULL, NULL, files);
	}

	g_object_unref (priv->job);
	priv->job = NULL;
}

void
giggle_file_list_highlight_revisions (GiggleFileList *list,
				      GiggleRevision *from,
				      GiggleRevision *to)
{
	GiggleFileListPriv *priv;

	g_return_if_fail (GIGGLE_IS_FILE_LIST (list));
	g_return_if_fail (!from || GIGGLE_IS_REVISION (from));
	g_return_if_fail (!to || GIGGLE_IS_REVISION (to));

	priv = GET_PRIV (list);

	if (priv->revision_from) {
		g_object_unref (priv->revision_from);
		priv->revision_from = NULL;
	}

	if (priv->revision_to) {
		g_object_unref (priv->revision_to);
		priv->revision_to = NULL;
	}

	/* clear highlights */
	file_list_update_highlight (list, NULL, NULL, NULL);

	if (from && to) {
		if (priv->job) {
			giggle_git_cancel_job (priv->git, priv->job);
			g_object_unref (priv->job);
			priv->job = NULL;
		}

		/* Remember the revisions in case we want to create a patch */
		priv->revision_from = g_object_ref (from);
		priv->revision_to = g_object_ref (to);

		priv->job = giggle_git_diff_tree_new (from, to);

		giggle_git_run_job (priv->git,
				    priv->job,
				    file_list_job_callback,
				    list);
	}
}
