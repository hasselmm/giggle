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

#include "giggle-diff-tree-view.h"
#include "giggle-revision.h"
#include "giggle-git-diff-tree.h"
#include "giggle-job.h"
#include "giggle-git.h"

typedef struct GiggleDiffTreeViewPriv GiggleDiffTreeViewPriv;

struct GiggleDiffTreeViewPriv {
	GtkListStore *store;

	GiggleGit    *git;
	GiggleJob    *job;
};

enum {
	COL_PATH,
	N_COLUMNS
};

static void diff_tree_view_finalize                (GObject *object);


G_DEFINE_TYPE (GiggleDiffTreeView, giggle_diff_tree_view, GTK_TYPE_TREE_VIEW)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_DIFF_TREE_VIEW, GiggleDiffTreeViewPriv))


static void
giggle_diff_tree_view_class_init (GiggleDiffTreeViewClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = diff_tree_view_finalize;

	g_type_class_add_private (object_class, sizeof (GiggleDiffTreeViewPriv));
}

static void
diff_tree_view_job_callback (GiggleGit *git,
			     GiggleJob *job,
			     GError    *error,
			     gpointer   user_data)
{
	GiggleDiffTreeView     *view;
	GiggleDiffTreeViewPriv *priv;
	GtkTreeIter             iter;
	GList                  *files;

	view = GIGGLE_DIFF_TREE_VIEW (user_data);
	priv = GET_PRIV (view);

	if (error) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view))),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("An error ocurred when retrieving different files list:\n%s"),
						 error->message);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_error_free (error);
	} else {
		files = giggle_git_diff_tree_get_files (GIGGLE_GIT_DIFF_TREE (priv->job));

		while (files) {
			gtk_list_store_append (priv->store, &iter);
			gtk_list_store_set (priv->store, &iter,
					    COL_PATH, files->data,
					    -1);
			files = files->next;
		}
	}

	g_object_unref (priv->job);
	priv->job = NULL;
}

static void
giggle_diff_tree_view_init (GiggleDiffTreeView *view)
{
	GiggleDiffTreeViewPriv *priv;
	GtkCellRenderer        *renderer;

	priv = GET_PRIV (view);

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);

	renderer = gtk_cell_renderer_text_new ();

	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
						     -1, _("Changed files"),
						     renderer,
						     "text", COL_PATH,
						     NULL);

	priv->store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (view),
				 GTK_TREE_MODEL (priv->store));

	priv->git = giggle_git_get ();
}

static void
diff_tree_view_finalize (GObject *object)
{
	GiggleDiffTreeViewPriv *priv;

	priv = GET_PRIV (object);
	
	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	g_object_unref (priv->git);
	g_object_unref (priv->store);

	G_OBJECT_CLASS (giggle_diff_tree_view_parent_class)->finalize (object);
}

GtkWidget *
giggle_diff_tree_view_new (void)
{
	return g_object_new (GIGGLE_TYPE_DIFF_TREE_VIEW, NULL);
}

void
giggle_diff_tree_view_set_revisions (GiggleDiffTreeView *view,
				     GiggleRevision     *from,
				     GiggleRevision     *to)
{
	GiggleDiffTreeViewPriv *priv;

	g_return_if_fail (GIGGLE_IS_DIFF_TREE_VIEW (view));
	g_return_if_fail (GIGGLE_IS_REVISION (from));
	g_return_if_fail (GIGGLE_IS_REVISION (to));

	priv = GET_PRIV (view);

	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	gtk_list_store_clear (priv->store);
	priv->job = giggle_git_diff_tree_new (from, to);

	giggle_git_run_job (priv->git,
			    priv->job,
			    diff_tree_view_job_callback,
			    view);
}

