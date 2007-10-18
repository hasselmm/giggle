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

#include "giggle-diff-view.h"
#include "giggle-revision.h"
#include "giggle-job.h"
#include "giggle-git.h"
#include "giggle-git-diff.h"
#include "giggle-searchable.h"

typedef struct GiggleDiffViewPriv GiggleDiffViewPriv;

struct GiggleDiffViewPriv {
	gboolean        compact_mode;
	GiggleGit      *git;

	GtkTextMark    *search_mark;
	gchar          *search_term;

	/* last run job */
	GiggleJob      *job;
};

static void       giggle_diff_view_searchable_init (GiggleSearchableIface *iface);

static void       diff_view_finalize           (GObject        *object);
static void       diff_view_get_property       (GObject        *object,
						guint           param_id,
						GValue         *value,
						GParamSpec     *pspec);
static void       diff_view_set_property       (GObject        *object,
						guint           param_id,
						const GValue   *value,
						GParamSpec     *pspec);

static gboolean   diff_view_search             (GiggleSearchable      *searchable,
						const gchar           *search_term,
						GiggleSearchDirection  direction,
						gboolean               full_search);

G_DEFINE_TYPE_WITH_CODE (GiggleDiffView, giggle_diff_view, GTK_TYPE_SOURCE_VIEW,
			 G_IMPLEMENT_INTERFACE (GIGGLE_TYPE_SEARCHABLE,
						giggle_diff_view_searchable_init))
			 

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_DIFF_VIEW, GiggleDiffViewPriv))

enum {
	PROP_0,
	PROP_COMPACT_MODE
};

static void
giggle_diff_view_class_init (GiggleDiffViewClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = diff_view_finalize;
	object_class->set_property = diff_view_set_property;
	object_class->get_property = diff_view_get_property;

	g_object_class_install_property (
		object_class,
		PROP_COMPACT_MODE,
		g_param_spec_boolean ("compact-mode",
				      "Compact mode",
				      "Whether to show the diff in compact mode or not",
				      FALSE,
				      G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleDiffViewPriv));
}

static void
giggle_diff_view_searchable_init (GiggleSearchableIface *iface)
{
	iface->search = diff_view_search;
}

static void
giggle_diff_view_init (GiggleDiffView *diff_view)
{
	GiggleDiffViewPriv        *priv;
	PangoFontDescription      *font_desc;
	GtkTextBuffer             *buffer;
	GtkSourceLanguage         *language;
	GtkSourceLanguagesManager *manager;
	GtkTextIter                iter;

	priv = GET_PRIV (diff_view);

	priv->git = giggle_git_get ();

	gtk_text_view_set_editable (GTK_TEXT_VIEW (diff_view), FALSE);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (diff_view), FALSE);

	font_desc = pango_font_description_from_string ("monospace");
	gtk_widget_modify_font (GTK_WIDGET (diff_view), font_desc);
	pango_font_description_free (font_desc);

	manager = gtk_source_languages_manager_new ();
	language = gtk_source_languages_manager_get_language_from_mime_type (
		manager, "text/x-patch");

	if (language) {
		buffer = GTK_TEXT_BUFFER (gtk_source_buffer_new_with_language (language));
		gtk_source_buffer_set_highlight (GTK_SOURCE_BUFFER (buffer), TRUE);
		gtk_text_view_set_buffer (GTK_TEXT_VIEW (diff_view), buffer);

		gtk_text_buffer_get_start_iter (buffer, &iter);
		priv->search_mark = gtk_text_buffer_create_mark (buffer,
								 "search-mark",
								 &iter, FALSE);
		g_object_unref (buffer);
	}

	g_object_unref (manager);
}

static void
diff_view_finalize (GObject *object)
{
	GiggleDiffViewPriv *priv;

	priv = GET_PRIV (object);

	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	g_free (priv->search_term);
	g_object_unref (priv->git);

	G_OBJECT_CLASS (giggle_diff_view_parent_class)->finalize (object);
}

static void
diff_view_get_property (GObject    *object,
			guint       param_id,
			GValue     *value,
			GParamSpec *pspec)
{
	GiggleDiffViewPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_COMPACT_MODE:
		g_value_set_boolean (value, priv->compact_mode);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
diff_view_set_property (GObject      *object,
			guint         param_id,
			const GValue *value,
			GParamSpec   *pspec)
{
	GiggleDiffViewPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_COMPACT_MODE:
		giggle_diff_view_set_compact_mode (GIGGLE_DIFF_VIEW (object),
						   g_value_get_boolean (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
diff_view_do_search (GiggleDiffView *view,
		     const gchar    *search_term)
{
	/* FIXME: there is similar code in GiggleRevisionView */
	GiggleDiffViewPriv *priv;
	GtkTextBuffer      *buffer;
	GtkTextIter         start_iter, end_iter;
	gchar              *diff;
	const gchar        *p;
	glong               offset, len;
	gboolean            match = FALSE;

	priv = GET_PRIV (view);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	gtk_text_buffer_get_bounds (buffer, &start_iter, &end_iter);
	diff = gtk_text_buffer_get_text (buffer, &start_iter, &end_iter, FALSE);

	if ((p = strstr (diff, search_term)) != NULL) {
		match = TRUE;
		offset = g_utf8_pointer_to_offset (diff, p);
		len = g_utf8_strlen (search_term, -1);

		gtk_text_buffer_get_iter_at_offset (buffer, &start_iter, (gint) offset);
		gtk_text_buffer_get_iter_at_offset (buffer, &end_iter, (gint) offset + len);

		gtk_text_buffer_select_range (buffer, &start_iter, &end_iter);

		gtk_text_buffer_move_mark (buffer, priv->search_mark, &start_iter);
		gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view), priv->search_mark,
					      0.0, FALSE, 0.5, 0.5);
	}

	g_free (diff);
	return match;
}

static void
diff_view_job_callback (GiggleGit *git,
			GiggleJob *job,
			GError    *error,
			gpointer   user_data)
{
	GiggleDiffView     *view;
	GiggleDiffViewPriv *priv;

	view = GIGGLE_DIFF_VIEW (user_data);
	priv = GET_PRIV (view);

	if (error) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view))),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("An error ocurred when retrieving a diff:\n%s"),
						 error->message);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	} else {
		gtk_text_buffer_set_text (
			gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)),
			giggle_git_diff_get_result (GIGGLE_GIT_DIFF (job)),
			-1);

		if (priv->search_term) {
			diff_view_do_search (view, priv->search_term);
			g_free (priv->search_term);
			priv->search_term = NULL;
		}
	}

	g_object_unref (priv->job);
	priv->job = NULL;
}

static gboolean
diff_view_search (GiggleSearchable      *searchable,
		  const gchar           *search_term,
		  GiggleSearchDirection  direction,
		  gboolean               full_search)
{
	GiggleDiffViewPriv *priv;

	priv = GET_PRIV (searchable);

	if (priv->job) {
		/* There's a job running, we want it to
		 * search after the job has finished,
		 * it's not what I'd call interactive, but
		 * good enough for the searching purposes
		 * of this object.
		 */
		priv->search_term = g_strdup (search_term);
		return TRUE;
	} else {
		return diff_view_do_search (GIGGLE_DIFF_VIEW (searchable), search_term);
	}
}

GtkWidget *
giggle_diff_view_new (void)
{
	return g_object_new (GIGGLE_TYPE_DIFF_VIEW, NULL);
}

void
giggle_diff_view_set_revisions (GiggleDiffView *diff_view,
				GiggleRevision *revision1,
				GiggleRevision *revision2,
				GList          *files)
{
	GiggleDiffViewPriv *priv;

	g_return_if_fail (GIGGLE_IS_DIFF_VIEW (diff_view));
	g_return_if_fail (!revision1 || GIGGLE_IS_REVISION (revision1));
	g_return_if_fail (!revision2 || GIGGLE_IS_REVISION (revision2));

	priv = GET_PRIV (diff_view);

	/* Clear the view until we get new content. */
	gtk_text_buffer_set_text (
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (diff_view)),
		"", 0);

	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	priv->job = giggle_git_diff_new ();
	giggle_git_diff_set_revisions (GIGGLE_GIT_DIFF (priv->job),
				       revision2, revision1);
	giggle_git_diff_set_files (GIGGLE_GIT_DIFF (priv->job), files);

	giggle_git_run_job (priv->git,
			    priv->job,
			    diff_view_job_callback,
			    diff_view);
}

void
giggle_diff_view_diff_current (GiggleDiffView *diff_view,
			       GList          *files)
{
	GiggleDiffViewPriv *priv;

	g_return_if_fail (GIGGLE_IS_DIFF_VIEW (diff_view));

	priv = GET_PRIV (diff_view);

	/* Clear the view until we get new content. */
	gtk_text_buffer_set_text (
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (diff_view)),
		"", 0);

	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	priv->job = giggle_git_diff_new ();
	giggle_git_diff_set_files (GIGGLE_GIT_DIFF (priv->job), files);

	giggle_git_run_job (priv->git,
			    priv->job,
			    diff_view_job_callback,
			    diff_view);
}

gboolean
giggle_diff_view_get_compact_mode (GiggleDiffView *view)
{
	GiggleDiffViewPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_DIFF_VIEW (view), FALSE);

	priv = GET_PRIV (view);

	return priv->compact_mode;
}

void
giggle_diff_view_set_compact_mode (GiggleDiffView *view,
				   gboolean        compact_mode)
{
	GiggleDiffViewPriv   *priv;
	PangoFontDescription *font_desc;
	gint                  size;

	g_return_if_fail (GIGGLE_IS_DIFF_VIEW (view));

	priv = GET_PRIV (view);

	if (compact_mode != priv->compact_mode) {
		priv->compact_mode = (compact_mode == TRUE);

		if (!compact_mode) {
			/* Reset to default font to get the default size. */
			gtk_widget_modify_font (GTK_WIDGET (view), NULL);

			/* Then set the right font. */
			font_desc = pango_font_description_from_string ("monospace");
			gtk_widget_modify_font (GTK_WIDGET (view), font_desc);
			pango_font_description_free (font_desc);
		} else {
			/* Get the existing font_desc, and change the size. */
			font_desc = pango_font_description_copy (GTK_WIDGET (view)->style->font_desc);
			size = pango_font_description_get_size (font_desc);
			pango_font_description_set_size (font_desc, size * PANGO_SCALE_SMALL);
			gtk_widget_modify_font (GTK_WIDGET (view), font_desc);
			pango_font_description_free (font_desc);
		}

		g_object_notify (G_OBJECT (view), "compact-mode");
	}
}
