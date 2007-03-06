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

#include "giggle-revision-view.h"
#include "giggle-revision.h"
#include "giggle-searchable.h"

typedef struct GiggleRevisionViewPriv GiggleRevisionViewPriv;

struct GiggleRevisionViewPriv {
	GiggleRevision *revision;

	GtkWidget      *date;
	GtkWidget      *sha;
	GtkWidget      *log;

	GtkTextMark    *search_mark;
};

static void       giggle_revision_view_searchable_init (GiggleSearchableIface *iface);

static void       revision_view_finalize           (GObject        *object);
static void       revision_view_get_property       (GObject        *object,
						    guint           param_id,
						    GValue         *value,
						    GParamSpec     *pspec);
static void       revision_view_set_property       (GObject        *object,
						    guint           param_id,
						    const GValue   *value,
						    GParamSpec     *pspec);

static gboolean   revision_view_search             (GiggleSearchable      *searchable,
						    const gchar           *search_term,
						    GiggleSearchDirection  direction);

static void       revision_view_update             (GiggleRevisionView *view);


G_DEFINE_TYPE_WITH_CODE (GiggleRevisionView, giggle_revision_view, GTK_TYPE_TABLE,
			 G_IMPLEMENT_INTERFACE (GIGGLE_TYPE_SEARCHABLE,
						giggle_revision_view_searchable_init))

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REVISION_VIEW, GiggleRevisionViewPriv))

enum {
	PROP_0,
	PROP_REVISION,
};

static void
giggle_revision_view_class_init (GiggleRevisionViewClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = revision_view_finalize;
	object_class->set_property = revision_view_set_property;
	object_class->get_property = revision_view_get_property;

	g_object_class_install_property (object_class,
					 PROP_REVISION,
					 g_param_spec_object ("revision",
							      "Revision",
							      "Revision to show",
							      GIGGLE_TYPE_REVISION,
							      G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleRevisionViewPriv));
}

static void
giggle_revision_view_searchable_init (GiggleSearchableIface *iface)
{
	iface->search = revision_view_search;
}

static void
giggle_revision_view_init (GiggleRevisionView *revision_view)
{
	GiggleRevisionViewPriv *priv;
	GtkWidget              *label, *scrolled_window;
	GtkTextBuffer          *buffer;
	GtkTextIter             iter;

	priv = GET_PRIV (revision_view);

	g_object_set (G_OBJECT (revision_view),
		      "column-spacing", 12,
		      "row-spacing", 6,
		      NULL);

	label = gtk_label_new (_("Date:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_widget_show (label);

	gtk_table_attach (GTK_TABLE (revision_view), label,
			  0, 1, 0, 1,
			  GTK_FILL, GTK_FILL, 0, 0);

	priv->date = gtk_label_new (NULL);
	gtk_label_set_ellipsize (GTK_LABEL (priv->date), PANGO_ELLIPSIZE_END);
	gtk_label_set_selectable (GTK_LABEL (priv->date), TRUE);
	gtk_misc_set_alignment (GTK_MISC (priv->date), 0.0, 0.5);
	gtk_widget_show (priv->date);

	gtk_table_attach (GTK_TABLE (revision_view), priv->date,
			  1, 2, 0, 1,
			  GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

	label = gtk_label_new (_("SHA:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_widget_show (label);

	gtk_table_attach (GTK_TABLE (revision_view), label,
			  0, 1, 1, 2,
			  GTK_FILL, GTK_FILL, 0, 0);

	priv->sha = gtk_label_new (NULL);
	gtk_label_set_ellipsize (GTK_LABEL (priv->sha), PANGO_ELLIPSIZE_END);
	gtk_label_set_selectable (GTK_LABEL (priv->sha), TRUE);
	gtk_misc_set_alignment (GTK_MISC (priv->sha), 0.0, 0.5);
	gtk_widget_show (priv->sha);

	gtk_table_attach (GTK_TABLE (revision_view), priv->sha,
			  1, 2, 1, 2,
			  GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

	label = gtk_label_new (_("Change Log:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_widget_show (label);

	gtk_table_attach (GTK_TABLE (revision_view), label,
			  0, 1, 2, 3,
			  GTK_FILL, GTK_FILL, 0, 0);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
					     GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request (scrolled_window, -1, 60);
	gtk_widget_show (scrolled_window);

	priv->log = gtk_text_view_new ();
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->log));
	gtk_text_view_set_editable (GTK_TEXT_VIEW (priv->log), FALSE);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (priv->log), GTK_WRAP_WORD);
	gtk_widget_show (priv->log);

	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->log);
	gtk_table_attach (GTK_TABLE (revision_view), scrolled_window,
			  1, 2, 2, 3,
			  GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

	gtk_text_buffer_get_start_iter (buffer, &iter);
	priv->search_mark = gtk_text_buffer_create_mark (buffer,
							 "search-mark",
							 &iter, FALSE);
}

static void
revision_view_finalize (GObject *object)
{
	GiggleRevisionViewPriv *priv;

	priv = GET_PRIV (object);

	if (priv->revision) {
		g_object_unref (priv->revision);
	}

	G_OBJECT_CLASS (giggle_revision_view_parent_class)->finalize (object);
}

static void
revision_view_get_property (GObject    *object,
			    guint       param_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	GiggleRevisionViewPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_REVISION:
		g_value_set_object (value, priv->revision);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
revision_view_set_property (GObject      *object,
			    guint         param_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	GiggleRevisionViewPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_REVISION:
		if (priv->revision) {
			g_object_unref (priv->revision);
		}

		priv->revision = (GiggleRevision *) g_value_dup_object (value);
		revision_view_update (GIGGLE_REVISION_VIEW (object));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
revision_view_search (GiggleSearchable      *searchable,
		      const gchar           *search_term,
		      GiggleSearchDirection  direction)
{
	GiggleRevisionViewPriv *priv;
	const gchar            *str, *p;
	gchar                  *log;
	glong                   offset, len;
	GtkTextBuffer          *buffer;
	GtkTextIter             start_iter, end_iter;

	priv = GET_PRIV (searchable);

	/* search in SHA label */
	str = gtk_label_get_text (GTK_LABEL (priv->sha));

	if ((p = strcasestr (str, search_term)) != NULL) {
		offset = g_utf8_pointer_to_offset (str, p);
		len = g_utf8_strlen (search_term, -1);

		gtk_label_select_region (GTK_LABEL (priv->sha),
					 (gint) offset, (gint) len);

		return TRUE;
	}

	/* search in log */
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->log));
	gtk_text_buffer_get_bounds (buffer, &start_iter, &end_iter);
	log = gtk_text_buffer_get_text (buffer, &start_iter, &end_iter, FALSE);

	if ((p = strcasestr (log, search_term)) != NULL) {
		offset = g_utf8_pointer_to_offset (log, p);
		len = g_utf8_strlen (search_term, -1);

		gtk_text_buffer_get_iter_at_offset (buffer, &start_iter, (gint) offset);
		gtk_text_buffer_get_iter_at_offset (buffer, &end_iter, (gint) offset + len);

		gtk_text_buffer_select_range (buffer, &start_iter, &end_iter);

		gtk_text_buffer_move_mark (buffer, priv->search_mark, &start_iter);
		gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (priv->log), priv->search_mark,
					      0.0, FALSE, 0.5, 0.5);
		g_free (log);
		return TRUE;
	}

	g_free (log);
	return FALSE;
}

static void
revision_view_update (GiggleRevisionView *view)
{
	GiggleRevisionViewPriv *priv;
	GtkTextBuffer          *buffer;
	gchar                  *sha, *log;
	struct tm              *tm;
	gchar                   str[256];

	priv = GET_PRIV (view);
	g_object_get (G_OBJECT (priv->revision),
		      "sha", &sha, 
		      "long-log", &log,
		      "date", &tm,
		      NULL);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->log));
	gtk_text_buffer_set_text (buffer, log, -1);
	g_free (log);

	gtk_label_set_text (GTK_LABEL (priv->sha), sha);
	g_free (sha);

	strftime (str, sizeof (str), "%c", tm);
	gtk_label_set_text (GTK_LABEL (priv->date), str);
}

GtkWidget *
giggle_revision_view_new (void)
{
	return g_object_new (GIGGLE_TYPE_REVISION_VIEW, NULL);
}

void
giggle_revision_view_set_revision (GiggleRevisionView *view,
				   GiggleRevision     *revision)
{
	g_return_if_fail (GIGGLE_IS_REVISION_VIEW (view));
	g_return_if_fail (GIGGLE_IS_REVISION (revision));

	g_object_set (view,
		      "revision", revision,
		      NULL);
}

GiggleRevision *
giggle_revision_view_get_revision (GiggleRevisionView *view)
{
	GiggleRevisionViewPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION_VIEW (view), NULL);

	priv = GET_PRIV (view);
	return priv->revision;
}
