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
#include "giggle-view-summary.h"

#include "giggle-authors-view.h"
#include "giggle-branches-view.h"
#include "giggle-description-editor.h"
#include "giggle-remotes-view.h"
#include "giggle-short-list.h"

#include <libgiggle-git/giggle-git.h>

#include <glib/gi18n.h>

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

G_DEFINE_TYPE (GiggleViewSummary, giggle_view_summary, GTK_TYPE_NOTEBOOK)

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

static GtkWidget *
create_summary_page (GiggleViewSummaryPriv *priv)
{
	GtkWidget *page;
	GtkWidget *label;

	page = gtk_vbox_new (FALSE, 6);

	/* create header */

	priv->name_label = gtk_label_new (NULL);
	g_object_ref_sink (priv->name_label);
	gtk_widget_show (priv->name_label);
	gtk_box_pack_start (GTK_BOX (page), priv->name_label, FALSE, FALSE, 0);

	priv->path_label = gtk_label_new (NULL);
	g_object_ref_sink (priv->path_label);
	gtk_widget_show (priv->path_label);
	gtk_box_pack_start (GTK_BOX (page), priv->path_label, FALSE, FALSE, 0);

	/* add decription editor */

	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Description:</b>"));
	gtk_box_pack_start (GTK_BOX (page), label, FALSE, FALSE, 0);

	priv->description_editor = giggle_description_editor_new ();
	gtk_box_pack_start (GTK_BOX (page), priv->description_editor, TRUE, TRUE, 0);

	gtk_container_set_border_width (GTK_CONTAINER (page), 6);
	gtk_widget_show_all (page);

	return page;
}

static GtkWidget *
create_branches_page (GiggleViewSummaryPriv *priv)
{
	priv->branches_view = giggle_branches_view_new ();

	gtk_container_set_border_width (GTK_CONTAINER (priv->branches_view), 6);
	gtk_widget_show_all (priv->branches_view);

	return priv->branches_view;
}

static GtkWidget *
create_authors_page (GiggleViewSummaryPriv *priv)
{
	priv->authors_view = giggle_authors_view_new ();

	gtk_container_set_border_width (GTK_CONTAINER (priv->authors_view), 6);
	gtk_widget_show_all (priv->authors_view);

	return priv->authors_view;
}

static GtkWidget *
create_remotes_page (GiggleViewSummaryPriv *priv)
{
	GtkWidget *page, *label, *scrolled_window;

	/* add remotes view */
	priv->remotes_view = giggle_remotes_view_new ();
	page = gtk_vbox_new (FALSE, 6);

	/* FIXME: string should not contain markup */
	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Remotes:</b>"));
	gtk_box_pack_start (GTK_BOX (page), label, FALSE, FALSE, 0);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->remotes_view);
	gtk_box_pack_start (GTK_BOX (page), scrolled_window, TRUE, TRUE, 0);

	gtk_container_set_border_width (GTK_CONTAINER (page), 6);
	gtk_widget_show_all (page);

	return page;
}

static void
giggle_view_summary_init (GiggleViewSummary *view)
{
	GiggleViewSummaryPriv *priv;

	priv = GET_PRIV (view);
	priv->git = giggle_git_get ();

	gtk_notebook_append_page
		(GTK_NOTEBOOK (view), create_summary_page (priv),
		 gtk_label_new_with_mnemonic (_("_Summary")));
	gtk_notebook_append_page
		(GTK_NOTEBOOK (view), create_branches_page (priv),
		 gtk_label_new_with_mnemonic (_("_Branches")));
	gtk_notebook_append_page
		(GTK_NOTEBOOK (view), create_authors_page (priv),
		 gtk_label_new_with_mnemonic (_("_Authors")));
	gtk_notebook_append_page
		(GTK_NOTEBOOK (view), create_remotes_page (priv),
		 gtk_label_new_with_mnemonic (_("_Remotes")));

	gtk_container_set_border_width (GTK_CONTAINER (view), 6);

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
