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

#include "giggle-authors-view.h"
#include "giggle-branches-view.h"
#include "giggle-description-editor.h"
#include "libgiggle/giggle-git.h"
#include "giggle-remotes-view.h"
#include "giggle-short-list.h"
#include "giggle-view-summary.h"

typedef struct GiggleViewSummaryPriv GiggleViewSummaryPriv;

struct GiggleViewSummaryPriv {
	GtkWidget *name_label;
	GtkWidget *path_label;

	GtkWidget *description_editor;

	GtkWidget *branches_view;
	GtkWidget *authors_view;
	GtkWidget *remotes_view;

	GiggleGit *git;
};

static void    view_summary_finalize              (GObject           *object);

static void    view_summary_project_changed_cb    (GObject           *object,
						   GParamSpec        *arg,
						   GiggleViewSummary *view);

G_DEFINE_TYPE (GiggleViewSummary, giggle_view_summary, GIGGLE_TYPE_VIEW)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_VIEW_SUMMARY, GiggleViewSummaryPriv))

static void
giggle_view_summary_class_init (GiggleViewSummaryClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = view_summary_finalize;

	g_type_class_add_private (object_class, sizeof (GiggleViewSummaryPriv));
}

static void
view_summary_update_data (GiggleViewSummary *view)
{
	GiggleViewSummaryPriv *priv;
	gchar                 *markup;
	const gchar           *name;

	priv = GET_PRIV (view);

	/* we could skip the markup by using PangoAttrList */
	name = giggle_git_get_project_name (priv->git);
	if (!name) {
		name = "";
	}
	
	markup = g_strdup_printf ("<span weight='bold' size='xx-large'>%s</span>",
				  name);
	gtk_label_set_markup (GTK_LABEL (priv->name_label), markup);
	g_free (markup);

	gtk_label_set_text (GTK_LABEL (priv->path_label),
			    giggle_git_get_project_dir (priv->git));
}

static void
giggle_view_summary_init (GiggleViewSummary *view)
{
	GiggleViewSummaryPriv *priv;
	GtkWidget             *vpaned, *table;
	GtkWidget             *label, *scrolled_window, *box;

	priv = GET_PRIV (view);

	gtk_container_set_border_width (GTK_CONTAINER (view), 6);
	gtk_box_set_spacing (GTK_BOX (view), 12);

	priv->name_label = gtk_label_new (NULL);
	g_object_ref_sink (priv->name_label);
	gtk_widget_show (priv->name_label);
	gtk_box_pack_start (GTK_BOX (view), priv->name_label, FALSE, FALSE, 0);

	priv->path_label = gtk_label_new (NULL);
	g_object_ref_sink (priv->path_label);
	gtk_widget_show (priv->path_label);
	gtk_box_pack_start (GTK_BOX (view), priv->path_label, FALSE, FALSE, 0);

	vpaned = gtk_vpaned_new ();
	gtk_widget_show (vpaned);
	gtk_box_pack_start (GTK_BOX (view), vpaned, TRUE, TRUE, 0);

	/* add decription editor */
	box = gtk_vbox_new (FALSE, 6);
	gtk_paned_pack1 (GTK_PANED (vpaned), box, FALSE, FALSE);

	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Description:</b>"));
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

	priv->description_editor = giggle_description_editor_new ();
	gtk_box_pack_start (GTK_BOX (box), priv->description_editor, TRUE, TRUE, 0);
	gtk_widget_show_all (box);

	table = gtk_table_new (0, 0, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_table_set_row_spacings (GTK_TABLE (table), 12);
	gtk_widget_show (table);
	gtk_paned_pack2 (GTK_PANED (vpaned), table, FALSE, FALSE);

	/* add branches view */
	priv->branches_view = giggle_branches_view_new ();

	gtk_widget_show_all (priv->branches_view);
	gtk_table_attach (GTK_TABLE (table), priv->branches_view,
			  0, 1, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);

	/* add authors view */
	priv->authors_view = giggle_authors_view_new ();

	gtk_widget_show_all (priv->authors_view);
	gtk_table_attach (GTK_TABLE (table), priv->authors_view,
			  1, 2, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);

	/* add remotes view */
	priv->remotes_view = giggle_remotes_view_new ();
	box = gtk_vbox_new (FALSE, 6);

	/* FIXME: string should not contain markup */
	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Remotes:</b>"));
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->remotes_view);
	gtk_box_pack_start (GTK_BOX (box), scrolled_window, TRUE, TRUE, 0);

	gtk_widget_show_all (box);
	gtk_table_attach (GTK_TABLE (table), box,
			  0, 2, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);

	priv->git = giggle_git_get ();
	g_signal_connect (priv->git, "notify::directory",
			  G_CALLBACK (view_summary_project_changed_cb), view);

	/* initialize for the first time */
	view_summary_update_data (view);
}

static void
view_summary_finalize (GObject *object)
{
	GiggleViewSummaryPriv *priv;

	priv = GET_PRIV (object);

	g_object_unref (priv->name_label);
	g_object_unref (priv->path_label);

	priv = GET_PRIV (object);
}

static void
view_summary_project_changed_cb (GObject           *object,
				 GParamSpec        *arg,
				 GiggleViewSummary *view)
{
	view_summary_update_data (view);
}

GtkWidget *
giggle_view_summary_new (void)
{
	return g_object_new (GIGGLE_TYPE_VIEW_SUMMARY, NULL);
}
