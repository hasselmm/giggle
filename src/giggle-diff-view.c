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
typedef struct GiggleDiffViewFile GiggleDiffViewFile;
typedef struct GiggleDiffViewHunk GiggleDiffViewHunk;

struct GiggleDiffViewPriv {
	GiggleGit   *git;

	GtkTextMark *search_mark;
	gchar       *search_term;

	int	     current_hunk;
	GArray      *files, *hunks;

	/* last run job */
	GiggleJob   *job;
};

struct GiggleDiffViewFile {
	char        *filename;
	char        *header;
};

struct GiggleDiffViewHunk {
	unsigned     file;
	char        *text;
};

static void       giggle_diff_view_searchable_init (GiggleSearchableIface *iface);

G_DEFINE_TYPE_WITH_CODE (GiggleDiffView, giggle_diff_view, GTK_TYPE_SOURCE_VIEW,
			 G_IMPLEMENT_INTERFACE (GIGGLE_TYPE_SEARCHABLE,
						giggle_diff_view_searchable_init))

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_DIFF_VIEW, GiggleDiffViewPriv))

enum {
	PROP_0,
	PROP_CURRENT_FILE,
	PROP_CURRENT_HUNK,
	PROP_N_HUNKS,
};

inline static GiggleDiffViewFile *
diff_view_get_file (GiggleDiffViewPriv *priv,
		    unsigned            i)
{
	return &g_array_index (priv->files, GiggleDiffViewFile, i);
}

inline static GiggleDiffViewHunk *
diff_view_get_hunk (GiggleDiffViewPriv *priv,
		    unsigned            i)
{
	return &g_array_index (priv->hunks, GiggleDiffViewHunk, i);
}

static void
diff_view_reset_hunks (GiggleDiffViewPriv *priv)
{
	unsigned i;

	for (i = 0; i < priv->files->len; ++i) {
		g_free (diff_view_get_file (priv, i)->filename);
		g_free (diff_view_get_file (priv, i)->header);
	}

	for (i = 0; i < priv->hunks->len; ++i) {
		g_free (diff_view_get_hunk (priv, i)->text);
	}

	g_array_set_size (priv->files, 0);
	g_array_set_size (priv->hunks, 0);
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
	diff_view_reset_hunks (priv);

	g_array_free (priv->files, TRUE);
	g_array_free (priv->hunks, TRUE);

	G_OBJECT_CLASS (giggle_diff_view_parent_class)->finalize (object);
}

static void
diff_view_get_property (GObject    *object,
			guint       param_id,
			GValue     *value,
			GParamSpec *pspec)
{
	GiggleDiffView     *view;
	GiggleDiffViewPriv *priv;

	view = GIGGLE_DIFF_VIEW (object);
	priv = GET_PRIV (view);

	switch (param_id) {
	case PROP_CURRENT_FILE:
		g_value_set_string (value, giggle_diff_view_get_current_file (view));
		break;

	case PROP_CURRENT_HUNK:
		g_value_set_int (value, priv->current_hunk);
		break;

	case PROP_N_HUNKS:
		g_value_set_int (value, priv->hunks->len);
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
	GiggleDiffViewPriv *priv = GET_PRIV (view);
	GiggleDiffViewHunk *hunk = NULL;
	GiggleDiffViewFile *file = NULL;
	GtkTextBuffer      *buffer;
	GtkTextIter         iter;

	g_return_if_fail (hunk_index >= -1);
	g_return_if_fail (hunk_index < (int) priv->hunks->len);

	priv->current_hunk = hunk_index;

	hunk = diff_view_get_hunk (priv, hunk_index);

	if (hunk && hunk_index >= 0)
		file = diff_view_get_file (priv, hunk->file);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	gtk_text_buffer_set_text (buffer, "", 0);

	if (file) {
		gtk_text_buffer_get_end_iter (buffer, &iter);
		gtk_text_buffer_insert (buffer, &iter, file->header, -1);

		gtk_text_buffer_get_end_iter (buffer, &iter);
		gtk_text_buffer_insert (buffer, &iter, hunk->text, -1);
	}
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
		PROP_CURRENT_FILE,
		g_param_spec_string ("current-file",
				     "Current File",
				     "Name of the currently selected file",
				     NULL, G_PARAM_READABLE));

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

	priv->files = g_array_new (FALSE, TRUE, sizeof (GiggleDiffViewFile));
	priv->hunks = g_array_new (FALSE, TRUE, sizeof (GiggleDiffViewHunk));
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

static void
diff_view_append_hunk (GiggleDiffViewPriv  *priv,
		       GtkTextBuffer       *buffer,
		       GiggleDiffViewFile  *file,
		       GtkTextIter         *start,
		       GtkTextIter         *end)
{
	GiggleDiffViewHunk *hunk;

	if (gtk_text_iter_compare (start, end) < 0) {
		g_array_set_size (priv->hunks, priv->hunks->len + 1);

		hunk = diff_view_get_hunk (priv, priv->hunks->len - 1);
		hunk->text = gtk_text_buffer_get_text (buffer, start, end, FALSE);
		hunk->file = priv->files->len - 1;
	}
}

static void
diff_view_parse_patch (GiggleDiffView *view)
{
	GiggleDiffViewPriv   *priv;
	GtkTextBuffer	     *buffer;
	GiggleDiffViewFile   *file = NULL;
	char		     *filename = NULL;
	char                 *line;

	GtkTextIter           file_start, file_end;
	GtkTextIter           hunk_start, hunk_end;
	GtkTextIter           line_start, line_end;

	priv = GET_PRIV (view);
	priv->current_hunk = -1;

	diff_view_reset_hunks (priv);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	gtk_text_buffer_get_start_iter (buffer, &line_start);

	file_end = file_start = line_start;
	hunk_end = hunk_start = line_start;
	line_end = line_start;

	while (gtk_text_iter_forward_to_line_end (&line_end)) {
		line = gtk_text_buffer_get_text (buffer, &line_start, &line_end, FALSE);

		if (g_str_has_prefix (line, "@@ ")) {
			if (file) {
				diff_view_append_hunk (priv, buffer, file,
					 	       &hunk_start, &hunk_end);
			} else {
				gtk_text_buffer_create_mark (buffer, filename,
							     &file_start, TRUE);

				g_array_set_size (priv->files, priv->files->len + 1);
				file = diff_view_get_file (priv, priv->files->len - 1);

				file->header = gtk_text_buffer_get_text (buffer, &file_start,
								 	 &file_end, FALSE);
				file->filename = g_strdup (filename);
			}

			hunk_start = line_start;
		} else if (!strchr (" +-", *line)) {
			if (file) {
				diff_view_append_hunk (priv, buffer, file,
					 	       &hunk_start, &hunk_end);

				file_start = line_start;
				file = NULL;
			}
		} else if (g_str_has_prefix (line, "--- a/") || g_str_has_prefix (line, "+++ b/")) {
			g_free (filename); filename = g_strdup (line + 6);
		}

		g_free (line);

		if (!gtk_text_iter_forward_line (&line_end))
			break;

		line_start = line_end;
		file_end   = line_end;
		hunk_end   = line_end;
	}

	hunk_end = line_end;

	/* we need at least 1 file */
	if (priv->files->len > 0)
		diff_view_append_hunk (priv, buffer, file, &hunk_start, &hunk_end);

	g_object_notify (G_OBJECT (view), "current-hunk");
	g_object_notify (G_OBJECT (view), "n-hunks");

	g_free (filename);
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
						 _("An error occurred when retrieving a diff:\n%s"),
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
	return GET_PRIV (diff_view)->hunks->len;
}

void
giggle_diff_view_scroll_to_file (GiggleDiffView *diff_view,
			    	 const char     *filename)
{
	GiggleDiffViewPriv *priv;
	GiggleDiffViewHunk *hunk;
	GiggleDiffViewFile *file;
	GtkTextMark        *mark;
	unsigned	    i;

	g_return_if_fail (GIGGLE_IS_DIFF_VIEW (diff_view));
	g_return_if_fail (NULL != filename);

	priv = GET_PRIV (diff_view);

	if (priv->current_hunk < 0) {
		mark = gtk_text_buffer_get_mark
			(gtk_text_view_get_buffer (GTK_TEXT_VIEW (diff_view)),
			 filename);

		if (mark) {
			gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (diff_view),
						      mark, 0.0, TRUE, 0.0, 0.0);
		}
	} else {
		for (i = 0; i < priv->hunks->len; ++i) {
			hunk = diff_view_get_hunk (priv, i);
			file = diff_view_get_file (priv, hunk->file);

			if (!strcmp (file->filename, filename)) {
				if (i != priv->current_hunk)
					giggle_diff_view_set_current_hunk (diff_view, i);

				break;
			}
		}
	}
}

const char *
giggle_diff_view_get_current_file (GiggleDiffView *diff_view)
{
	GiggleDiffViewPriv *priv;
	GiggleDiffViewHunk *hunk = NULL;
	GiggleDiffViewFile *file = NULL;

	g_return_val_if_fail (GIGGLE_IS_DIFF_VIEW (diff_view), NULL);

	priv = GET_PRIV (diff_view);

	if (priv->current_hunk >= 0)
		hunk = diff_view_get_hunk (priv, priv->current_hunk);
	if (hunk)
		file = diff_view_get_file (priv, hunk->file);
	if (file)
		return file->filename;

	return NULL;
}

