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
#include "giggle-branches-view.h"

#include <libgiggle/giggle-ref.h>
#include <libgiggle/giggle-job.h>

#include <libgiggle-git/giggle-git.h>
#include <libgiggle-git/giggle-git-refs.h>

#include <glib/gi18n.h>

typedef struct GiggleBranchesViewPriv GiggleBranchesViewPriv;

struct GiggleBranchesViewPriv {
	GiggleGit    *git;
	GiggleJob    *job;
};

static void   branches_view_finalize                (GObject *object);
static gchar* branches_view_display_object          (GiggleShortList    * list,
						     GObject            * object);

G_DEFINE_TYPE (GiggleBranchesView, giggle_branches_view, GIGGLE_TYPE_SHORT_LIST)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_BRANCHES_VIEW, GiggleBranchesViewPriv))


static void
giggle_branches_view_class_init (GiggleBranchesViewClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GiggleShortListClass *short_list_class = GIGGLE_SHORT_LIST_CLASS (class);

	object_class->finalize = branches_view_finalize;

	short_list_class->display_object = branches_view_display_object;

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
	GList                  *branches;

	view = GIGGLE_BRANCHES_VIEW (user_data);
	priv = GET_PRIV (view);

	if (error) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view))),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("An error occurred when retrieving branches list:\n%s"),
						 error->message);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	} else {
		GtkListStore *store;
		GtkTreeIter   iter;

		store = gtk_list_store_new (GIGGLE_SHORT_LIST_N_COLUMNS, G_TYPE_OBJECT);
		branches = giggle_git_refs_get_branches (GIGGLE_GIT_REFS (job));

		for(; branches; branches = g_list_next (branches)) {
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter,
					    GIGGLE_SHORT_LIST_COL_OBJECT, branches->data,
					    -1);
		}

		giggle_short_list_set_model (GIGGLE_SHORT_LIST (view), GTK_TREE_MODEL (store));
		g_object_unref (store);
	}

	g_object_unref (priv->job);
	priv->job = NULL;
}

static void
branches_view_update (GiggleBranchesView *view)
{
	GiggleBranchesViewPriv *priv;

	priv = GET_PRIV (view);

	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	priv->job = giggle_git_refs_new ();

	giggle_git_run_job (priv->git,
			    priv->job,
			    branches_view_job_callback,
			    view);
}

static gchar*
branches_view_display_object (GiggleShortList    * list,
			      GObject            * object)
{
	return g_strdup (giggle_ref_get_name (GIGGLE_REF (object)));
}

static void
giggle_branches_view_init (GiggleBranchesView *view)
{
	GiggleBranchesViewPriv *priv;

	priv = GET_PRIV (view);

	priv->git = giggle_git_get ();
	g_signal_connect_swapped (priv->git, "notify::git-dir",
				  G_CALLBACK (branches_view_update), view);

	g_object_set (view, "label", _("Branches:"), NULL);

	/* initialize for first time */
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

	G_OBJECT_CLASS (giggle_branches_view_parent_class)->finalize (object);
}

GtkWidget *
giggle_branches_view_new (void)
{
	return g_object_new (GIGGLE_TYPE_BRANCHES_VIEW, NULL);
}

