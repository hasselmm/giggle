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

#include "giggle-authors-view.h"
#include "giggle-git-authors.h"
#include "giggle-author.h"
#include "giggle-job.h"
#include "giggle-git.h"

typedef struct GiggleAuthorsViewPriv GiggleAuthorsViewPriv;

struct GiggleAuthorsViewPriv {
	GtkListStore *store;

	GiggleGit    *git;
	GiggleJob    *job;
};

enum {
	COL_AUTHOR,
	N_COLUMNS
};

static void authors_view_finalize                (GObject *object);


G_DEFINE_TYPE (GiggleAuthorsView, giggle_authors_view, GTK_TYPE_TREE_VIEW)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_AUTHORS_VIEW, GiggleAuthorsViewPriv))


static void
giggle_authors_view_class_init (GiggleAuthorsViewClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = authors_view_finalize;

	g_type_class_add_private (object_class, sizeof (GiggleAuthorsViewPriv));
}

static void
authors_view_job_callback (GiggleGit *git,
			   GiggleJob *job,
			   GError    *error,
			   gpointer   user_data)
{
	GiggleAuthorsView     *view;
	GiggleAuthorsViewPriv *priv;
	GtkTreeIter            iter;
	GList                 *authors;

	view = GIGGLE_AUTHORS_VIEW (user_data);
	priv = GET_PRIV (view);

	if (error) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view))),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("An error ocurred when retrieving authors list:\n%s"),
						 error->message);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_error_free (error);
	} else {
		authors = giggle_git_authors_get_list (GIGGLE_GIT_AUTHORS (job));

		for(; authors; authors = g_list_next (authors)) {
			gtk_list_store_append (priv->store, &iter);
			gtk_list_store_set (priv->store, &iter,
					    COL_AUTHOR, g_object_ref (authors->data),
					    -1);
		}
	}

	g_object_unref (priv->job);
	priv->job = NULL;
}

static void
authors_view_update (GiggleAuthorsView *view)
{
	GiggleAuthorsViewPriv *priv;

	priv = GET_PRIV (view);

	gtk_list_store_clear (priv->store);

	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	priv->job = giggle_git_authors_new ();

	giggle_git_run_job (priv->git,
			    priv->job,
			    authors_view_job_callback,
			    view);
}

static void
authors_view_cell_data_func (GtkTreeViewColumn *column,
			     GtkCellRenderer   *cell,
			     GtkTreeModel      *model,
			     GtkTreeIter       *iter,
			     gpointer           data)
{
	GiggleAuthor *author = NULL;

	gtk_tree_model_get (model, iter,
			    COL_AUTHOR, &author,
			    -1);
	g_object_set (cell, "text", giggle_author_get_string (author), NULL);
	g_object_unref (author);
}

static void
giggle_authors_view_init (GiggleAuthorsView *view)
{
	GiggleAuthorsViewPriv *priv;
	GtkCellRenderer       *renderer;

	priv = GET_PRIV (view);

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (view), -1,
						    _("Branch"), renderer,
						    authors_view_cell_data_func,
						    NULL, NULL);

	priv->store = gtk_list_store_new (N_COLUMNS, G_TYPE_OBJECT);
	gtk_tree_view_set_model (GTK_TREE_VIEW (view),
				 GTK_TREE_MODEL (priv->store));

	priv->git = giggle_git_get ();
	g_signal_connect_swapped (G_OBJECT (priv->git), "notify::git-dir",
				  G_CALLBACK (authors_view_update), view);

	authors_view_update (view);
}

static void
authors_view_finalize (GObject *object)
{
	GiggleAuthorsViewPriv *priv;

	priv = GET_PRIV (object);
	
	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	g_object_unref (priv->git);
	g_object_unref (priv->store);

	G_OBJECT_CLASS (giggle_authors_view_parent_class)->finalize (object);
}

GtkWidget *
giggle_authors_view_new (void)
{
	return g_object_new (GIGGLE_TYPE_AUTHORS_VIEW, NULL);
}

