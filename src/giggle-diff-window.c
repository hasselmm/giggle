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
#include "giggle-diff-window.h"
#include "giggle-diff-view.h"

#include <libgiggle/giggle-job.h>

#include <libgiggle-git/giggle-git.h>
#include <libgiggle-git/giggle-git-commit.h>

#include <glib/gi18n.h>
#include <string.h>

typedef struct GiggleDiffWindowPriv GiggleDiffWindowPriv;

struct GiggleDiffWindowPriv {
	GtkWidget *diff_view;
	GtkWidget *commit_textview;

	GList     *files;

	GiggleGit *git;
	GiggleJob *job;
};

static void       diff_window_finalize           (GObject        *object);
static void       diff_window_map                (GtkWidget      *widget);
static void       diff_window_response           (GtkDialog      *dialog,
						  gint            response);


G_DEFINE_TYPE (GiggleDiffWindow, giggle_diff_window, GTK_TYPE_DIALOG)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_DIFF_WINDOW, GiggleDiffWindowPriv))

static void
giggle_diff_window_class_init (GiggleDiffWindowClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
	GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (class);

	widget_class->map = diff_window_map;
	object_class->finalize = diff_window_finalize;
	dialog_class->response = diff_window_response;

	g_type_class_add_private (object_class, sizeof (GiggleDiffWindowPriv));
}

static void
giggle_diff_window_init (GiggleDiffWindow *diff_window)
{
	GiggleDiffWindowPriv *priv;
	GtkWidget            *vbox, *scrolled_window;
	GtkWidget            *vbox2, *label;
	gchar                *str;

	priv = GET_PRIV (diff_window);

	priv->git = giggle_git_get ();

	gtk_window_set_default_size (GTK_WINDOW (diff_window), 500, 380);
	gtk_window_set_title (GTK_WINDOW (diff_window), _("Commit changes"));

	vbox = gtk_vbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 7);

	/* diff view */
	priv->diff_view = giggle_diff_view_new ();
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
					     GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->diff_view);
	gtk_widget_show_all (scrolled_window);

	gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

	/* commit log textview */
	vbox2 = gtk_vbox_new (FALSE, 6);

	label = gtk_label_new (NULL);
	str = g_strdup_printf ("<b>%s</b>", _("Revision log:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_label_set_markup (GTK_LABEL (label), str);
	g_free (str);

	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);

	priv->commit_textview = gtk_text_view_new ();
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (priv->commit_textview),
				     GTK_WRAP_WORD_CHAR);
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
					     GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->commit_textview);
	gtk_box_pack_start (GTK_BOX (vbox2), scrolled_window, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);

	gtk_widget_show_all (vbox);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (diff_window)->vbox), vbox);

	g_object_set (diff_window,
		      "has-separator", FALSE,
		      NULL);

	gtk_dialog_add_button (GTK_DIALOG (diff_window),
			       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (diff_window),
			       _("Co_mmit"), GTK_RESPONSE_OK);
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

	g_list_foreach (priv->files, (GFunc) g_free, NULL);
	g_list_free (priv->files);

	G_OBJECT_CLASS (giggle_diff_window_parent_class)->finalize (object);
}

static GList *
diff_window_copy_list (GList *list)
{
	GList *copy = NULL;

	while (list) {
		copy = g_list_prepend (copy, g_strdup ((gchar *) list->data));
		list = list->next;
	}

	return g_list_reverse (copy);
}

static void
diff_window_map (GtkWidget *widget)
{
	GiggleDiffWindowPriv *priv;
	GtkTextBuffer        *buffer;
	GList                *files;

	priv = GET_PRIV (widget);

	files = diff_window_copy_list (priv->files);
	giggle_diff_view_diff_current (GIGGLE_DIFF_VIEW (priv->diff_view), files);
	gtk_widget_grab_focus (priv->commit_textview);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->commit_textview));
	gtk_text_buffer_set_text (buffer, "", -1);

	GTK_WIDGET_CLASS (giggle_diff_window_parent_class)->map (widget);
}

static void
diff_window_job_callback (GiggleGit *git,
			  GiggleJob *job,
			  GError    *error,
			  gpointer   user_data)
{
	GiggleDiffWindowPriv *priv;

	priv = GET_PRIV (user_data);

	if (error) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (NULL,
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("An error occurred when committing:\n%s"),
						 error->message);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	} else {
		/* Tell GiggleGit listeners to update */
		giggle_git_changed (priv->git);
	}

	g_object_unref (priv->job);
	priv->job = NULL;
}

static void
diff_window_response (GtkDialog *dialog,
		      gint       response)
{
	GiggleDiffWindowPriv *priv;
	GtkTextBuffer        *buffer;
	GtkTextIter           start, end;
	gchar                *log;
	GList                *files;

	if (response != GTK_RESPONSE_OK) {
		/* do not commit */
		return;
	}

	priv = GET_PRIV (dialog);

	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->commit_textview));
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	log = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);
	files = diff_window_copy_list (priv->files);

	priv->job = giggle_git_commit_new (log);
	giggle_git_commit_set_files (GIGGLE_GIT_COMMIT (priv->job), files);

	giggle_git_run_job (priv->git,
			    priv->job,
			    diff_window_job_callback,
			    dialog);
}

GtkWidget *
giggle_diff_window_new (void)
{
	return g_object_new (GIGGLE_TYPE_DIFF_WINDOW, NULL);
}

void
giggle_diff_window_set_files (GiggleDiffWindow *window,
			      GList            *files)
{
	GiggleDiffWindowPriv *priv;

	g_return_if_fail (GIGGLE_IS_DIFF_WINDOW (window));

	priv = GET_PRIV (window);

	if (priv->files) {
		g_list_foreach (priv->files, (GFunc) g_free, NULL);
		g_list_free (priv->files);
	}

	priv->files = files;
}
