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

#include "config.h"
#include "giggle-remotes-view.h"

#include "giggle-helpers.h"
#include "giggle-remote-editor.h"
#include "giggle-spaning-renderer.h"

#include "libgiggle/giggle-git.h"

#include <glib/gi18n.h>

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REMOTES_VIEW, GiggleRemotesViewPriv))

typedef struct {
	GtkListStore    *store;
	GiggleGit       *git;
} GiggleRemotesViewPriv;

enum {
	COL_REMOTE,
	N_COLUMNS
};

G_DEFINE_TYPE (GiggleRemotesView, giggle_remotes_view, GTK_TYPE_TREE_VIEW)

static void
remotes_view_dispose (GObject *object)
{
	GiggleRemotesViewPriv *priv;

	priv = GET_PRIV (object);

	if (priv->git) {
		g_object_unref (priv->git);
		priv->git = NULL;
	}

	if (priv->store) {
		g_object_unref (priv->store);
		priv->store = NULL;
	}

	G_OBJECT_CLASS (giggle_remotes_view_parent_class)->dispose (object);
}

static gboolean
remotes_view_key_press_event (GtkWidget   *widget,
			      GdkEventKey *event)
{
	if (giggle_list_view_delete_selection (widget, event)) {
		// FIXME: delete the files
		return TRUE;
	}

	if (GTK_WIDGET_CLASS (giggle_remotes_view_parent_class)->key_press_event)
		return GTK_WIDGET_CLASS (giggle_remotes_view_parent_class)->key_press_event (widget, event);

	return FALSE;
}

static void
giggle_remotes_view_class_init (GiggleRemotesViewClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

	object_class->dispose         = remotes_view_dispose;
	widget_class->key_press_event = remotes_view_key_press_event;

	g_type_class_add_private (object_class, sizeof (GiggleRemotesViewPriv));
}

static void
remotes_view_update (GiggleRemotesView *view)
{
	GiggleRemotesViewPriv *priv;
	GtkTreeIter            iter;
	GList                 *remotes;

	priv = GET_PRIV (view);

	gtk_list_store_clear (priv->store);

	for(remotes = giggle_git_get_remotes (priv->git); remotes; remotes = g_list_next (remotes)) {
		gtk_list_store_append (priv->store, &iter);
		gtk_list_store_set (priv->store, &iter,
				    COL_REMOTE, remotes->data,
				    -1);
	}

	gtk_list_store_append (priv->store, &iter);
	gtk_list_store_set (priv->store, &iter,
			    COL_REMOTE, NULL,
			    -1);
}

static void
remotes_view_icon_data_func (GtkTreeViewColumn *column,
			     GtkCellRenderer   *cell,
			     GtkTreeModel      *model,
			     GtkTreeIter       *iter,
			     gpointer           data)
{
	GiggleRemote *remote = NULL;

	gtk_tree_model_get (model, iter,
			    COL_REMOTE, &remote,
			    -1);

	if (GIGGLE_IS_REMOTE (remote)) {
		g_object_set (cell,
			      "icon-name", giggle_remote_get_icon_name (remote),
			      NULL);
		g_object_unref (remote);
	} else {
		g_object_set (cell,
			      "icon-name", "gtk-new",
			      NULL);
	}
}

static void
remotes_view_name_data_func (GtkTreeViewColumn *column,
			     GtkCellRenderer   *cell,
			     GtkTreeModel      *model,
			     GtkTreeIter       *iter,
			     gpointer           data)
{
	GiggleRemote *remote = NULL;
	const char   *name = NULL;

	gtk_tree_model_get (model, iter, COL_REMOTE, &remote, -1);

	if (GIGGLE_IS_REMOTE (remote))
		name = giggle_remote_get_name (remote);

	g_object_set (cell, "text", name, NULL);
}

static void
remotes_view_url_data_func (GtkTreeViewColumn *column,
			    GtkCellRenderer   *renderer,
			    GtkTreeModel      *model,
			    GtkTreeIter       *iter,
			    gpointer           data)
{
	GiggleRemote *remote = NULL;

	gtk_tree_model_get (model, iter,
			    COL_REMOTE, &remote,
			    -1);

	if(GIGGLE_IS_REMOTE (remote)) {
		g_object_set (renderer,
			      "text", giggle_remote_get_url (remote),
			      NULL);
		g_object_unref (remote);
	} else {
		g_object_set (renderer, "text", NULL, NULL);
	}
}

static void
remotes_view_last_data_func (GtkTreeViewColumn *column,
			     GtkCellRenderer   *cell,
			     GtkTreeModel      *model,
			     GtkTreeIter       *iter,
			     gpointer           data)
{
	GiggleRemote *remote = NULL;
	gboolean      visible = TRUE;

	gtk_tree_model_get (model, iter, COL_REMOTE, &remote, -1);

	if (GIGGLE_IS_REMOTE (remote)) {
		g_object_unref (remote);
		visible = FALSE;
	}

	g_object_set (cell, "visible", visible, NULL);
}

static void
window_remotes_row_activated_cb (GiggleRemotesView *view,
				 GtkTreePath       *path,
				 GtkTreeViewColumn *column,
				 GtkTreeView       *treeview)
{
	GiggleRemotesViewPriv *priv = GET_PRIV (view);
	GiggleRemote          *remote = NULL;
	GtkTreeModel          *model;
	GtkTreeIter            iter;
	GtkWidget             *editor;
	GtkWidget             *toplevel;
	gint                   response;

	model = gtk_tree_view_get_model (treeview);
	g_return_if_fail (gtk_tree_model_get_iter (model, &iter, path));

	gtk_tree_model_get (model, &iter,
			    COL_REMOTE, &remote,
			    -1);

	editor = giggle_remote_editor_new (remote);
	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (view));
	gtk_window_set_transient_for (GTK_WINDOW (editor), GTK_WINDOW (toplevel));
	response = gtk_dialog_run (GTK_DIALOG (editor));

	if (!remote && response == GTK_RESPONSE_ACCEPT) {
		GtkListStore* store = GTK_LIST_STORE (model);
		GtkTreeIter   new;

		g_object_get (editor,
			      "remote", &remote,
			      NULL);

		gtk_list_store_insert_before (store, &new, &iter);
		gtk_list_store_set (store, &new,
				    COL_REMOTE, remote,
				    -1);
	}

	if (response == GTK_RESPONSE_ACCEPT)
		giggle_git_save_remote (priv->git, remote);

	if (remote)
		g_object_unref (remote);

	gtk_widget_destroy (editor);
}

static void
giggle_remotes_view_init (GiggleRemotesView *view)
{
	GiggleRemotesViewPriv *priv;
	GtkCellRenderer       *renderer;
	GtkTreeViewColumn     *column;

	priv = GET_PRIV (view);

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (view), -1,
						    _("Icon"), renderer,
						    remotes_view_icon_data_func,
						    NULL, NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (view), -1,
						    _("Name"), renderer,
						    remotes_view_name_data_func,
						    NULL, NULL);

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("URL"));
	gtk_tree_view_insert_column (GTK_TREE_VIEW (view), column, -1);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
						 remotes_view_url_data_func,
						 NULL, NULL);

	renderer = giggle_spaning_renderer_new ();
	g_object_set (renderer,
		      "first-column", 1, "style", PANGO_STYLE_ITALIC,
		      "text", _("Double click to add remote..."), NULL);
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
						 remotes_view_last_data_func,
						 NULL, NULL);

	g_signal_connect_swapped (view, "row-activated",
				  G_CALLBACK (window_remotes_row_activated_cb), view);

	priv->store = gtk_list_store_new (N_COLUMNS, G_TYPE_OBJECT);
	gtk_tree_view_set_model (GTK_TREE_VIEW (view),
				 GTK_TREE_MODEL (priv->store));

	priv->git = giggle_git_get ();
	g_signal_connect_swapped (priv->git, "notify::remotes",
				  G_CALLBACK (remotes_view_update), view);

	remotes_view_update (view);

	/* initialize for first time */
	remotes_view_update (view);
}

GtkWidget *
giggle_remotes_view_new (void)
{
	return g_object_new (GIGGLE_TYPE_REMOTES_VIEW, NULL);
}

