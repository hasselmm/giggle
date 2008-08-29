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
#include "libgiggle/giggle-revision.h"
#include "libgiggle/giggle-job.h"
#include "libgiggle/giggle-git.h"
#include "libgiggle/giggle-git-diff.h"
#include "libgiggle/giggle-searchable.h"

typedef struct GiggleDiffViewPriv GiggleDiffViewPriv;

struct GiggleDiffViewPriv {
	GiggleGit      *git;

	GtkTextMark    *search_mark;
	gchar          *search_term;

	GPtrArray      *hunk_tags;
	int		current_hunk;

	/* last run job */
	GiggleJob      *job;
};

static void       giggle_diff_view_searchable_init (GiggleSearchableIface *iface);

G_DEFINE_TYPE_WITH_CODE (GiggleDiffView, giggle_diff_view, GTK_TYPE_SOURCE_VIEW,
			 G_IMPLEMENT_INTERFACE (GIGGLE_TYPE_SEARCHABLE,
						giggle_diff_view_searchable_init))

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_DIFF_VIEW, GiggleDiffViewPriv))

enum {
	PROP_0,
	PROP_CURRENT_HUNK,
	PROP_N_HUNKS,
};

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
	g_ptr_array_free (priv->hunk_tags, TRUE);

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
	case PROP_CURRENT_HUNK:
		g_value_set_int (value, priv->current_hunk);
		break;

	case PROP_N_HUNKS:
		g_value_set_int (value, priv->hunk_tags->len / 2);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
diff_view_set_current_hunk (GiggleDiffView *view,
			    int             hunk_index)
{
	GiggleDiffViewPriv *priv;

	priv = GET_PRIV (view);

	priv->current_hunk = hunk_index;
}

static void
diff_view_set_property (GObject      *object,
			guint         param_id,
			const GValue *value,
			GParamSpec   *pspec)
{
	switch (param_id) {
	case PROP_CURRENT_HUNK:
		diff_view_set_current_hunk (GIGGLE_DIFF_VIEW (object),
					    g_value_get_int (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
giggle_diff_view_class_init (GiggleDiffViewClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize     = diff_view_finalize;
	object_class->set_property = diff_view_set_property;
	object_class->get_property = diff_view_get_property;

	g_object_class_install_property (
		object_class,
		PROP_CURRENT_HUNK,
		g_param_spec_int ("current-hunk",
				  "Current Hunk",
				  "Index of the currently shown hunk",
				  -1, G_MAXINT, -1,
				  G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_N_HUNKS,
		g_param_spec_int ("n-hunks",
				  "Number of Hunks",
				  "The number of hunks in the current patch",
				  0, G_MAXINT, 0,
				  G_PARAM_READABLE));

	g_type_class_add_private (object_class, sizeof (GiggleDiffViewPriv));
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
	}

	return diff_view_do_search (GIGGLE_DIFF_VIEW (searchable), search_term);
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
	GtkSourceLanguageManager  *manager;
	GtkTextIter                iter;

	priv = GET_PRIV (diff_view);

	priv->git = giggle_git_get ();
	priv->hunk_tags = g_ptr_array_new ();
	priv->current_hunk = -1;

	gtk_text_view_set_editable (GTK_TEXT_VIEW (diff_view), FALSE);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (diff_view), FALSE);

	font_desc = pango_font_description_from_string ("monospace");
	gtk_widget_modify_font (GTK_WIDGET (diff_view), font_desc);
	pango_font_description_free (font_desc);

	manager = gtk_source_language_manager_new ();
	language = gtk_source_language_manager_get_language (manager, "diff");

	if (language) {
		buffer = GTK_TEXT_BUFFER (gtk_source_buffer_new_with_language (language));
		gtk_source_buffer_set_highlight_syntax (GTK_SOURCE_BUFFER (buffer), TRUE);
		gtk_text_view_set_buffer (GTK_TEXT_VIEW (diff_view), buffer);

		gtk_text_buffer_get_start_iter (buffer, &iter);
		priv->search_mark = gtk_text_buffer_create_mark (buffer,
								 "search-mark",
								 &iter, FALSE);

		g_object_unref (buffer);
	}

	g_object_unref (manager);

}

typedef struct {
	GtkTextBuffer 	   *buffer;
	char		   *filename;
	GPtrArray	   *tags;

	GtkTextTag         *meta_tag;
	GtkTextTag         *hunk_tag;

	GtkTextIter         meta_start, meta_end;
	GtkTextIter         hunk_start, hunk_end;
	GtkTextIter         line_start, line_end;
} GiggleDiffViewParser;

static void
diff_view_apply_meta_tag (GiggleDiffViewParser *parser)
{
	parser->meta_tag = gtk_text_buffer_create_tag
		(parser->buffer, NULL, "editable", FALSE,
		 "paragraph-background", "yellow", NULL);

	gtk_text_buffer_apply_tag (parser->buffer, parser->meta_tag,
				   &parser->meta_start, &parser->meta_end);
	gtk_text_buffer_create_mark (parser->buffer, parser->filename,
				     &parser->meta_start, TRUE);
}

static void
diff_view_apply_hunk_tag (GiggleDiffViewParser *parser)
{
	static char *hunk_colors[] = { "red", "green", "blue" };

	parser->hunk_tag = gtk_text_buffer_create_tag
		(parser->buffer, NULL, "paragraph-background",
		 hunk_colors[(parser->tags->len / 2) % 3],
		 NULL);

	gtk_text_buffer_apply_tag (parser->buffer, parser->hunk_tag,
				   &parser->hunk_start, &parser->hunk_end);

	parser->hunk_start = parser->line_start;

	g_ptr_array_add (parser->tags, parser->meta_tag);
	g_ptr_array_add (parser->tags, parser->hunk_tag);
}

static void
diff_view_parse_patch (GiggleDiffView *view)
{
	GiggleDiffViewPriv   *priv;
	GiggleDiffViewParser  parser;
	char                 *line;

	priv = GET_PRIV (view);

	memset (&parser, 0, sizeof parser);

	g_ptr_array_set_size (priv->hunk_tags, 0);
	priv->current_hunk = -1;

	parser.tags = priv->hunk_tags;
	parser.buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	gtk_text_buffer_get_start_iter (parser.buffer, &parser.line_start);

	parser.meta_end = parser.meta_start = parser.line_start;
	parser.hunk_end = parser.hunk_start = parser.line_start;
	parser.line_end = parser.line_start;


	while (gtk_text_iter_forward_to_line_end (&parser.line_end)) {
		line = gtk_text_buffer_get_text (parser.buffer,
						 &parser.line_start,
						 &parser.line_end, FALSE);

		if (g_str_has_prefix (line, "@@ ")) {
			if (!parser.meta_tag) {
				diff_view_apply_meta_tag (&parser);
			}

			diff_view_apply_hunk_tag (&parser);
		} else if (!strchr (" +-", *line)) {
			if (parser.hunk_tag) {
				diff_view_apply_hunk_tag (&parser);
				parser.hunk_tag = NULL;
			}

			if (parser.meta_tag) {
				parser.meta_start = parser.line_start;
				parser.meta_tag = NULL;
			}
		} else if (g_str_has_prefix (line, "--- a/") || g_str_has_prefix (line, "+++ b/")) {
			g_free (parser.filename);
			parser.filename = g_strdup (line + 6);
		}

		g_free (line);

		if (!gtk_text_iter_forward_line (&parser.line_end))
			break;

		if (!parser.hunk_tag)
			parser.hunk_start = parser.line_end;

		parser.line_start = parser.line_end;
		parser.meta_end   = parser.line_end;
		parser.hunk_end   = parser.line_end;
	}

	parser.hunk_end = parser.line_end;
	diff_view_apply_hunk_tag (&parser);

	g_free (parser.filename);

	g_object_notify (G_OBJECT (view), "current-hunk");
	g_object_notify (G_OBJECT (view), "n-hunks");
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

		diff_view_parse_patch (view);

		if (priv->search_term) {
			diff_view_do_search (view, priv->search_term);
			g_free (priv->search_term);
			priv->search_term = NULL;
		}
	}

	g_object_unref (priv->job);
	priv->job = NULL;
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

void
giggle_diff_view_set_current_hunk (GiggleDiffView *diff_view,
				   int             hunk_index)
{
	g_return_if_fail (GIGGLE_IS_DIFF_VIEW (diff_view));
	g_object_set (diff_view, "current-hunk", hunk_index, NULL);
}

int
giggle_diff_view_get_current_hunk (GiggleDiffView *diff_view)
{
	g_return_val_if_fail (GIGGLE_IS_DIFF_VIEW (diff_view), -1);
	return GET_PRIV (diff_view)->current_hunk;
}

int
giggle_diff_view_get_n_hunks (GiggleDiffView *diff_view)
{
	g_return_val_if_fail (GIGGLE_IS_DIFF_VIEW (diff_view), 0);
	return GET_PRIV (diff_view)->hunk_tags->len / 2;
}

void
giggle_diff_view_scroll_to_file (GiggleDiffView *diff_view,
			    	 const char     *filename)
{
	GtkTextMark *mark;

	g_return_if_fail (GIGGLE_IS_DIFF_VIEW (diff_view));
	g_return_if_fail (NULL != filename);

        mark = gtk_text_buffer_get_mark
		(gtk_text_view_get_buffer (GTK_TEXT_VIEW (diff_view)),
		 filename);

	if (mark) {
		gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (diff_view),
					      mark, 0.0, TRUE, 0.0, 0.0);
	}
}


