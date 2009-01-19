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

#include "giggle-helpers.h"

#include <gdk/gdkkeysyms.h>
#include <gtk/gtktreeselection.h>

static void
remote_editor_tree_selection_get_branches (GtkTreeModel *model,
					   GtkTreePath  *path,
					   GtkTreeIter  *iter,
					   GList       **branches)
{
	*branches = g_list_prepend (*branches, gtk_tree_row_reference_new (model, path));
}

static void
remote_editor_remove_branch (GtkTreeRowReference *ref)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;

	model = gtk_tree_row_reference_get_model (ref);
	gtk_tree_model_get_iter (model, &iter,
				 gtk_tree_row_reference_get_path (ref));
	gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

	gtk_tree_row_reference_free (ref);
}

/* returns TRUE if the key press was delete and at least one row has been
 * deleted */
gboolean
giggle_list_view_delete_selection (GtkWidget   *treeview,
				   GdkEventKey *event)
{
	if (event->keyval == GDK_Delete) {
		GtkTreeSelection* sel;
		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

		if (gtk_tree_selection_count_selected_rows (sel) > 0) {
			GList* branches = NULL;
			gtk_tree_selection_selected_foreach (sel,
							     (GtkTreeSelectionForeachFunc)
								remote_editor_tree_selection_get_branches,
							     &branches);
			g_list_foreach (branches, (GFunc)remote_editor_remove_branch, NULL);
			g_list_free (branches);
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean
tree_model_find_string (GtkTreeModel *model,
			GtkTreeIter  *iter,
			GtkTreeIter  *parent,
			int           column,
			const char   *pattern)
{
	GtkTreeIter  child;
	char        *text;

	if (gtk_tree_model_iter_children (model, iter, parent)) {
		do {
			gtk_tree_model_get (model, iter, column, &text, -1);

			if (!g_strcmp0 (text, pattern)) {
				g_free (text);
				return TRUE;
			}

			g_free (text);

			if (tree_model_find_string (model, &child, iter, column, pattern)) {
				*iter = child;
				return TRUE;
			}
		} while (gtk_tree_model_iter_next (model, iter));
	}

	return FALSE;
}

gboolean
giggle_tree_view_select_row_by_string (GtkWidget  *treeview,
				       int         column,
				       const char *pattern)
{
	GtkTreeSelection   *selection;
	GtkTreeModel	   *model;
	GtkTreeIter	    iter;
	GtkTreePath        *path;
	char               *text;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, column, &text, -1);
	} else {
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
		text = NULL;
	}

	if (!g_strcmp0 (text, pattern)) {
		g_free (text);
		return TRUE;
	}

	if (tree_model_find_string (model, &iter, NULL, column, pattern)) {
		path = gtk_tree_model_get_path (model, &iter);
		gtk_tree_view_expand_to_path (GTK_TREE_VIEW (treeview), path);
	 	gtk_tree_path_free (path);

		gtk_tree_selection_select_iter (selection, &iter);

		return TRUE;
	}

	return FALSE;
}

GtkActionGroup *
giggle_ui_manager_get_action_group (GtkUIManager *manager,
				    const char   *group_name)
{
	GList *groups;

	groups = gtk_ui_manager_get_action_groups (manager);

	while (groups) {
		if (!g_strcmp0 (group_name,
				gtk_action_group_get_name (groups->data)))
			return groups->data;

		groups = groups->next;
	}

	return NULL;
}

GAppLaunchContext *
giggle_create_app_launch_context (GtkWidget *widget)
{
#if GTK_CHECK_VERSION(2,14,0)
	GdkAppLaunchContext *context;
	GdkScreen           *screen = NULL;

	context = gdk_app_launch_context_new ();

	if (widget) {
		screen = gtk_widget_get_screen (widget);
		gdk_app_launch_context_set_screen (context, screen);
	}

	return G_APP_LAUNCH_CONTEXT (context);
#else
	return g_app_launch_context_new ();
#endif
}
