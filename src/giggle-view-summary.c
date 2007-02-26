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

#include "giggle-git.h"
#include "giggle-view-summary.h"

typedef struct GiggleViewSummaryPriv GiggleViewSummaryPriv;

struct GiggleViewSummaryPriv {
	GtkWidget *name_label;
	GtkWidget *path_label;

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

	priv = GET_PRIV (view);

	/* we could skip the markup by using PangoAttrList */
	markup = g_strdup_printf ("<span weight='bold' size='xx-large'>%s</span>",
				  giggle_git_get_project_name (priv->git));
	gtk_label_set_markup (GTK_LABEL (priv->name_label), markup);
	g_free (markup);

	gtk_label_set_text (GTK_LABEL (priv->path_label),
			    giggle_git_get_project_dir (priv->git));
}

static void
giggle_view_summary_init (GiggleViewSummary *view)
{
	GiggleViewSummaryPriv *priv;
	GtkWidget             *vpaned;

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

	priv->git = giggle_git_get ();
	g_signal_connect (G_OBJECT (priv->git), "notify::directory",
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
