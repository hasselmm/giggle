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

#include "giggle-branches-view.h"
#include "giggle-git-refs.h"
#include "giggle-ref.h"
#include "giggle-job.h"
#include "giggle-git.h"

typedef struct GiggleBranchesViewPriv GiggleBranchesViewPriv;

struct GiggleBranchesViewPriv {
	GtkListStore *store;

	GiggleGit    *git;
	GiggleJob    *job;
};

enum {
	COL_BRANCH,
	N_COLUMNS
};

static void branches_view_finalize                (GObject *object);


G_DEFINE_TYPE (GiggleBranchesView, giggle_branches_view, GTK_TYPE_TREE_VIEW)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_BRANCHES_VIEW, GiggleBranchesViewPriv))


static void
giggle_branches_view_class_init (GiggleBranchesViewClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = branches_view_finalize;

	g_type_class_add_private (object_class, sizeof (GiggleBranchesViewPriv));
}

static void
branches_view_job_callback (GiggleGit *git,
			    GiggleJob *job,
			    GError    *error,
			    gpointer   user_data)
{
	GiggleBranchesView     *view;
	GiggleBranchesViewPriv *priv;
	GtkTreeIter             iter;
	GList                  *branches;

	view = GIGGLE_BRANCHES_VIEW (user_data);
	priv = GET_PRIV (view);

	if (error) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view))),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("An error ocurred when retrieving branches list:\n%s"),
						 error->message);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	} else {
		branches = giggle_git_refs_get_refs (GIGGLE_GIT_REFS (job));

		for(; branches; branches = g_list_next (branches)) {
			gtk_list_store_append (priv->store, &iter);
			gtk_list_store_set (priv->store, &iter,
					    COL_BRANCH, g_object_ref (branches->data),
					    -1);
		}
	}

	g_object_unref (priv->job);
	priv->job = NULL;
}

static void
branches_view_update (GiggleBranchesView *view)
{
	GiggleBranchesViewPriv *priv;

	priv = GET_PRIV (view);

	gtk_list_store_clear (priv->store);

	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	priv->job = giggle_git_refs_new (GIGGLE_GIT_REF_TYPE_BRANCH);

	giggle_git_run_job (priv->git,
			    priv->job,
			    branches_view_job_callback,
			    view);
}

static void
branches_view_cell_data_func (GtkTreeViewColumn *column,
			      GtkCellRenderer   *cell,
			      GtkTreeModel      *model,
			      GtkTreeIter       *iter,
			      gpointer           data)
{
	GiggleRef *ref = NULL;

	gtk_tree_model_get (model, iter,
			    COL_BRANCH, &ref,
			    -1);
	g_object_set (cell, "text", giggle_ref_get_name (ref), NULL);
	g_object_unref (ref);
}

static void
giggle_branches_view_init (GiggleBranchesView *view)
{
	GiggleBranchesViewPriv *priv;
	GtkCellRenderer        *renderer;

	priv = GET_PRIV (view);

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (view), -1,
						    _("Branch"), renderer,
						    branches_view_cell_data_func,
						    NULL, NULL);

	priv->store = gtk_list_store_new (N_COLUMNS, G_TYPE_OBJECT);
	gtk_tree_view_set_model (GTK_TREE_VIEW (view),
				 GTK_TREE_MODEL (priv->store));

	priv->git = giggle_git_get ();
	g_signal_connect_swapped (G_OBJECT (priv->git), "notify::git-dir",
				  G_CALLBACK (branches_view_update), view);

	branches_view_update (view);
}

static void
branches_view_finalize (GObject *object)
{
	GiggleBranchesViewPriv *priv;

	priv = GET_PRIV (object);
	
	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	g_object_unref (priv->git);
	g_object_unref (priv->store);

	G_OBJECT_CLASS (giggle_branches_view_parent_class)->finalize (object);
}

GtkWidget *
giggle_branches_view_new (void)
{
	return g_object_new (GIGGLE_TYPE_BRANCHES_VIEW, NULL);
}

