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
#include <string.h>

#include "giggle-diff-window.h"
#include "giggle-diff-view.h"
#include "giggle-job.h"
#include "giggle-git.h"

typedef struct GiggleDiffWindowPriv GiggleDiffWindowPriv;

struct GiggleDiffWindowPriv {
	GtkWidget *diff_view;
	GiggleGit *git;
	GiggleJob *job;
};

static void       diff_window_finalize           (GObject        *object);
static void       diff_window_map                (GtkWidget      *widget);


G_DEFINE_TYPE (GiggleDiffWindow, giggle_diff_window, GTK_TYPE_DIALOG)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_DIFF_WINDOW, GiggleDiffWindowPriv))

static void
giggle_diff_window_class_init (GiggleDiffWindowClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

	widget_class->map = diff_window_map;
	object_class->finalize = diff_window_finalize;

	g_type_class_add_private (object_class, sizeof (GiggleDiffWindowPriv));
}

static void
giggle_diff_window_init (GiggleDiffWindow *diff_window)
{
	GiggleDiffWindowPriv *priv;
	GtkWidget            *scrolled_window;

	priv = GET_PRIV (diff_window);

	priv->git = giggle_git_get ();

	gtk_window_set_default_size (GTK_WINDOW (diff_window), 500, 380);

	priv->diff_view = giggle_diff_view_new ();
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 7);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
					     GTK_SHADOW_IN);

	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->diff_view);
	gtk_widget_show_all (scrolled_window);

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (diff_window)->vbox), scrolled_window);

	g_object_set (G_OBJECT (diff_window),
		      "has-separator", FALSE,
		      NULL);

	gtk_dialog_add_button (GTK_DIALOG (diff_window),
			       GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
}

static void
diff_window_finalize (GObject *object)
{
	GiggleDiffWindowPriv *priv;

	priv = GET_PRIV (object);

	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	g_object_unref (priv->git);

	G_OBJECT_CLASS (giggle_diff_window_parent_class)->finalize (object);
}

static void
diff_window_map (GtkWidget *widget)
{
	GiggleDiffWindowPriv *priv;

	priv = GET_PRIV (widget);

	giggle_diff_view_diff_current (GIGGLE_DIFF_VIEW (priv->diff_view), NULL);
	gtk_widget_grab_focus (priv->diff_view);

	GTK_WIDGET_CLASS (giggle_diff_window_parent_class)->map (widget);
}

GtkWidget *
giggle_diff_window_new (void)
{
	return g_object_new (GIGGLE_TYPE_DIFF_WINDOW, NULL);
}
