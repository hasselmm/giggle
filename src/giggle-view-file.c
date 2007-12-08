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
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "libgiggle/giggle-git.h"
#include "libgiggle/giggle-git-revisions.h"
#include "giggle-view-file.h"
#include "giggle-file-list.h"
#include "giggle-rev-list-view.h"
#include "giggle-revision-view.h"
#include "giggle-diff-view.h"

typedef struct GiggleViewFilePriv GiggleViewFilePriv;

struct GiggleViewFilePriv {
	GtkWidget *file_list;
	GtkWidget *revision_list;
	GtkWidget *revision_view;
	GtkWidget *diff_view;

	GiggleGit *git;
	GiggleJob *job;
};

static void    view_file_finalize              (GObject *object);

static void    view_file_selection_changed_cb               (GtkTreeSelection   *selection,
							     GiggleViewFile     *view);
static void    view_file_revision_list_selection_changed_cb (GiggleRevListView *list,
							     GiggleRevision     *revision1,
							     GiggleRevision     *revision2,
							     GiggleViewFile     *view);


G_DEFINE_TYPE (GiggleViewFile, giggle_view_file, GIGGLE_TYPE_VIEW)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_VIEW_FILE, GiggleViewFilePriv))

static void
giggle_view_file_class_init (GiggleViewFileClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = view_file_finalize;

	g_type_class_add_private (object_class, sizeof (GiggleViewFilePriv));
}

static void
giggle_view_file_init (GiggleViewFile *view)
{
	GiggleViewFilePriv *priv;
	GtkWidget          *hpaned, *vpaned, *vbox;
	GtkWidget          *scrolled_window;
	GtkWidget          *expander;
	GtkTreeSelection   *selection;

	priv = GET_PRIV (view);

	priv->git = giggle_git_get ();

	gtk_widget_push_composite_child ();

	hpaned = gtk_hpaned_new ();
	gtk_widget_show (hpaned);
	gtk_container_add (GTK_CONTAINER (view), hpaned);

	vpaned = gtk_vpaned_new ();
	gtk_widget_show (vpaned);
	gtk_paned_pack2 (GTK_PANED (hpaned), vpaned, TRUE, FALSE);

	/* FIXME: hardcoded sizes are evil */
	gtk_paned_set_position (GTK_PANED (hpaned), 150);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);
	gtk_paned_pack2 (GTK_PANED (vpaned), vbox, FALSE, FALSE);

	/* file view */
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);

	priv->file_list = giggle_file_list_new ();
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->file_list));
	g_signal_connect (selection, "changed",
			  G_CALLBACK (view_file_selection_changed_cb), view);

	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->file_list);
	gtk_widget_show_all (scrolled_window);

	gtk_paned_pack1 (GTK_PANED (hpaned), scrolled_window, FALSE, FALSE);

	/* revisions list */
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);

	priv->revision_list = giggle_rev_list_view_new ();
	g_signal_connect (priv->revision_list, "selection-changed",
			  G_CALLBACK (view_file_revision_list_selection_changed_cb), view);

	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->revision_list);
	gtk_widget_show_all (scrolled_window);

	gtk_paned_pack1 (GTK_PANED (vpaned), scrolled_window, TRUE, FALSE);

	/* revision view */
	expander = gtk_expander_new_with_mnemonic (_("Revision _information"));
	priv->revision_view = giggle_revision_view_new ();
	gtk_container_add (GTK_CONTAINER (expander), priv->revision_view);
	gtk_widget_show_all (expander);

	gtk_box_pack_start (GTK_BOX (vbox), expander, FALSE, TRUE, 0);

	/* diff view */
	expander = gtk_expander_new_with_mnemonic (_("_Differences"));

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);

	priv->diff_view = giggle_diff_view_new ();

	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->diff_view);
	gtk_container_add (GTK_CONTAINER (expander), scrolled_window);
	gtk_widget_show_all (expander);

	gtk_box_pack_start (GTK_BOX (vbox), expander, TRUE, TRUE, 0);

	gtk_widget_pop_composite_child ();
}

static void
view_file_finalize (GObject *object)
{
	GiggleViewFilePriv *priv;

	priv = GET_PRIV (object);

	G_OBJECT_CLASS (giggle_view_file_parent_class)->finalize (object);
}

static void
view_file_select_file_job_callback (GiggleGit *git,
				    GiggleJob *job,
				    GError    *error,
				    gpointer   data)
{
	GiggleViewFile     *view;
	GiggleViewFilePriv *priv;
	GtkListStore       *store;
	GtkTreeIter         iter;
	GList              *revisions;

	view = GIGGLE_VIEW_FILE (data);
	priv = GET_PRIV (view);

	if (error) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view))),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("An error ocurred when getting the revisions list:\n%s"),
						 error->message);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	} else {
		store = gtk_list_store_new (1, GIGGLE_TYPE_REVISION);
		revisions = giggle_git_revisions_get_revisions (GIGGLE_GIT_REVISIONS (job));

		while (revisions) {
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter,
					    0, revisions->data,
					    -1);
			revisions = revisions->next;
		}

		giggle_rev_list_view_set_model (GIGGLE_REV_LIST_VIEW (priv->revision_list), GTK_TREE_MODEL (store));
		g_object_unref (store);
	}

	g_object_unref (job);
}

static void
view_file_selection_changed_cb (GtkTreeSelection *selection,
				GiggleViewFile   *view)
{
	GiggleViewFilePriv *priv;
	GList              *files;

	priv = GET_PRIV (view);
	files = giggle_file_list_get_selection (GIGGLE_FILE_LIST (priv->file_list));

	priv->job = giggle_git_revisions_new_for_files (files);

	giggle_git_run_job (priv->git,
			    priv->job,
			    view_file_select_file_job_callback,
			    view);
}

static void
view_file_revision_list_selection_changed_cb (GiggleRevListView *list,
					      GiggleRevision     *revision1,
					      GiggleRevision     *revision2,
					      GiggleViewFile     *view)
{
	GiggleViewFilePriv *priv;
	GList              *files;

	priv = GET_PRIV (view);

	giggle_revision_view_set_revision (
		GIGGLE_REVISION_VIEW (priv->revision_view), revision1);

	if (revision1 && revision2) {
		files = giggle_file_list_get_selection (GIGGLE_FILE_LIST (priv->file_list));

		giggle_diff_view_set_revisions (GIGGLE_DIFF_VIEW (priv->diff_view),
						revision1, revision2, files);
	}
}


GtkWidget *
giggle_view_file_new (void)
{
	return g_object_new (GIGGLE_TYPE_VIEW_FILE, NULL);
}

/* FIXME: this function is ugly, but we somehow want
 * the revisions list to be global to all the application */
void
giggle_view_file_set_model (GiggleViewFile *view_history,
			       GtkTreeModel   *model)
{
	GiggleViewFilePriv *priv;

	g_return_if_fail (GIGGLE_IS_VIEW_FILE (view_history));
	g_return_if_fail (GTK_IS_TREE_MODEL (model));

	priv = GET_PRIV (view_history);

	giggle_rev_list_view_set_model (GIGGLE_REV_LIST_VIEW (priv->revision_list), model);
}
