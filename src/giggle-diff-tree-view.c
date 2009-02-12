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

#include "libgiggle/giggle-git.h"
#include "libgiggle/giggle-git-diff-tree.h"

#include "libgiggle/giggle-clipboard.h"
#include "libgiggle/giggle-job.h"
#include "libgiggle/giggle-revision.h"

typedef struct GiggleDiffTreeViewPriv GiggleDiffTreeViewPriv;

struct GiggleDiffTreeViewPriv {
	GtkListStore *store;

	GiggleGit    *git;
	GiggleJob    *job;
};

enum {
	COL_PATH,
	COL_ICON,
	N_COLUMNS
};

enum {
	PATH_SELECTED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void      giggle_diff_tree_view_clipboard_init (GiggleClipboardIface *iface);
static void      diff_tree_view_finalize              (GObject              *object);
static gboolean  diff_tree_view_button_press          (GtkWidget            *widget,
						       GdkEventButton       *event);


G_DEFINE_TYPE_WITH_CODE (GiggleDiffTreeView, giggle_diff_tree_view, GTK_TYPE_TREE_VIEW,
			 G_IMPLEMENT_INTERFACE (GIGGLE_TYPE_CLIPBOARD,
						giggle_diff_tree_view_clipboard_init))

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_DIFF_TREE_VIEW, GiggleDiffTreeViewPriv))


static void
giggle_diff_tree_view_class_init (GiggleDiffTreeViewClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

	object_class->finalize = diff_tree_view_finalize;
	widget_class->button_press_event = diff_tree_view_button_press;

	signals[PATH_SELECTED] =
		g_signal_new ("path-selected",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GiggleDiffTreeViewClass, path_selected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE,
			      1, G_TYPE_STRING);

	g_type_class_add_private (object_class, sizeof (GiggleDiffTreeViewPriv));
}

static gboolean
diff_tree_view_can_copy (GiggleClipboard *clipboard)
{
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (clipboard));
	return gtk_tree_selection_count_selected_rows (selection) > 0;
}

static void
diff_tree_view_do_copy (GiggleClipboard *clipboard)
{
	GiggleDiffTreeView *view = GIGGLE_DIFF_TREE_VIEW (clipboard);
	GtkClipboard       *widget_clipboard;
	char               *text;

	widget_clipboard = gtk_widget_get_clipboard (GTK_WIDGET (view),
						     GDK_SELECTION_CLIPBOARD);

	text = giggle_diff_tree_view_get_selection (view);
	gtk_clipboard_set_text (widget_clipboard, text, -1);
	g_free (text);
}

static void
giggle_diff_tree_view_clipboard_init (GiggleClipboardIface *iface)
{
	iface->can_copy = diff_tree_view_can_copy;
	iface->do_copy  = diff_tree_view_do_copy;
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
	const char	       *icon;

	view = GIGGLE_DIFF_TREE_VIEW (user_data);
	priv = GET_PRIV (view);

	if (error) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view))),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("An error occurred when retrieving different files list:\n%s"),
						 error->message);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	} else {
		files = giggle_git_diff_tree_get_files (GIGGLE_GIT_DIFF_TREE (priv->job));

		while (files) {
			switch (giggle_git_diff_tree_get_action
					(GIGGLE_GIT_DIFF_TREE (priv->job),
					 files->data)) {
			case 'A':
				icon = GTK_STOCK_NEW;
				break;

			case 'D':
				icon = GTK_STOCK_DELETE;
				break;

			case 'M':
				icon = GTK_STOCK_EDIT;
				break;

			default:
				icon = NULL;
				break;
			}

			gtk_list_store_append (priv->store, &iter);
			gtk_list_store_set (priv->store, &iter,
					    COL_PATH, files->data,
					    COL_ICON, icon,
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
	GtkTreeSelection       *selection;

	priv = GET_PRIV (view);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);

	renderer = gtk_cell_renderer_pixbuf_new ();

	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
						     -1, NULL,
						     renderer,
						     "stock-id", COL_ICON,
						     NULL);

	renderer = gtk_cell_renderer_text_new ();

	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
						     -1, _("Changed files"),
						     renderer,
						     "text", COL_PATH,
						     NULL);

	priv->store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
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

static gboolean
diff_tree_view_button_press (GtkWidget      *widget,
			     GdkEventButton *event)
{
	gchar *path;

	GTK_WIDGET_CLASS (giggle_diff_tree_view_parent_class)->button_press_event (widget, event);

	if (event->button == 1 &&
	    event->type == GDK_2BUTTON_PRESS) {
		path = giggle_diff_tree_view_get_selection (GIGGLE_DIFF_TREE_VIEW (widget));

		if (path) {
			g_signal_emit (widget, signals[PATH_SELECTED], 0, path);
		}

		g_free (path);
	}

	return FALSE;
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
	g_return_if_fail (!from || GIGGLE_IS_REVISION (from));
	g_return_if_fail (!to || GIGGLE_IS_REVISION (to));

	priv = GET_PRIV (view);

	gtk_list_store_clear (priv->store);

	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	priv->job = giggle_git_diff_tree_new (from, to);

	giggle_git_run_job (priv->git,
			    priv->job,
			    diff_tree_view_job_callback,
			    view);
}

gchar *
giggle_diff_tree_view_get_selection (GiggleDiffTreeView *view)
{
	GtkTreeSelection       *selection;
	GtkTreeModel           *model;
	GtkTreeIter             iter;
	gchar                  *file = NULL;

	g_return_val_if_fail (GIGGLE_IS_DIFF_TREE_VIEW (view), NULL);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter,
				    COL_PATH, &file,
				    -1);
	}

	return file;
}
