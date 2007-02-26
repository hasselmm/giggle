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

#include "giggle-remotes-view.h"
#include "giggle-remote.h"
#include "giggle-git.h"

typedef struct GiggleRemotesViewPriv GiggleRemotesViewPriv;

struct GiggleRemotesViewPriv {
	GtkListStore *store;

	GiggleGit    *git;
};

enum {
	COL_REMOTE,
	N_COLUMNS
};

static void remotes_view_finalize                (GObject *object);


G_DEFINE_TYPE (GiggleRemotesView, giggle_remotes_view, GTK_TYPE_TREE_VIEW)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REMOTES_VIEW, GiggleRemotesViewPriv))


static void
giggle_remotes_view_class_init (GiggleRemotesViewClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = remotes_view_finalize;

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
}

static void
remotes_view_cell_data_func (GtkTreeViewColumn *column,
			      GtkCellRenderer   *cell,
			      GtkTreeModel      *model,
			      GtkTreeIter       *iter,
			      gpointer           data)
{
	GiggleRemote *remote = NULL;

	gtk_tree_model_get (model, iter,
			    COL_REMOTE, &remote,
			    -1);
	g_object_set (cell, "text", giggle_remote_get_name (remote), NULL);
	g_object_unref (remote);
}

static void
giggle_remotes_view_init (GiggleRemotesView *view)
{
	GiggleRemotesViewPriv *priv;
	GtkCellRenderer       *renderer;

	priv = GET_PRIV (view);

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (view), -1,
						    _("Branch"), renderer,
						    remotes_view_cell_data_func,
						    NULL, NULL);

	priv->store = gtk_list_store_new (N_COLUMNS, G_TYPE_OBJECT);
	gtk_tree_view_set_model (GTK_TREE_VIEW (view),
				 GTK_TREE_MODEL (priv->store));

	priv->git = giggle_git_get ();
	g_signal_connect_swapped (G_OBJECT (priv->git), "notify::git-dir",
				  G_CALLBACK (remotes_view_update), view);

	remotes_view_update (view);
}

static void
remotes_view_finalize (GObject *object)
{
	GiggleRemotesViewPriv *priv;

	priv = GET_PRIV (object);
	
	g_object_unref (priv->git);
	g_object_unref (priv->store);

	G_OBJECT_CLASS (giggle_remotes_view_parent_class)->finalize (object);
}

GtkWidget *
giggle_remotes_view_new (void)
{
	return g_object_new (GIGGLE_TYPE_REMOTES_VIEW, NULL);
}

