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

typedef struct GiggleDiffViewPriv GiggleDiffViewPriv;

struct GiggleDiffViewPriv {
	GiggleRevision *revision1;
	GiggleRevision *revision2;

	GiggleGit      *git;

	/* last run job */
	GiggleJob      *job;
};

static void       diff_view_finalize           (GObject        *object);
static void       diff_view_get_property       (GObject        *object,
						guint           param_id,
						GValue         *value,
						GParamSpec     *pspec);
static void       diff_view_set_property       (GObject        *object,
						guint           param_id,
						const GValue   *value,
						GParamSpec     *pspec);

G_DEFINE_TYPE (GiggleDiffView, giggle_diff_view, GTK_TYPE_SOURCE_VIEW)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_DIFF_VIEW, GiggleDiffViewPriv))

enum {
	PROP_0,
	PROP_REV_1,
	PROP_REV_2,
};

static void
giggle_diff_view_class_init (GiggleDiffViewClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = diff_view_finalize;
	object_class->set_property = diff_view_set_property;
	object_class->get_property = diff_view_get_property;

	g_object_class_install_property (object_class,
					 PROP_REV_1,
					 g_param_spec_object ("revision-1",
							      "Revision 1",
							      "Revision 1 to diff",
							      GIGGLE_TYPE_REVISION,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_REV_2,
					 g_param_spec_object ("revision-2",
							      "Revision 2",
							      "Revision 2 to diff",
							      GIGGLE_TYPE_REVISION,
							      G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleDiffViewPriv));
}

static void
giggle_diff_view_init (GiggleDiffView *diff_view)
{
	GiggleDiffViewPriv        *priv;
	PangoFontDescription      *font_desc;
	GtkTextBuffer             *buffer;
	GtkSourceLanguage         *language;
	GtkSourceLanguagesManager *manager;

	priv = GET_PRIV (diff_view);

	priv->git = giggle_git_get ();

	gtk_text_view_set_editable (GTK_TEXT_VIEW (diff_view), FALSE);

	font_desc = pango_font_description_from_string ("monospace");
	gtk_widget_modify_font (GTK_WIDGET (diff_view), font_desc);
	pango_font_description_free (font_desc);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (diff_view));

	manager = gtk_source_languages_manager_new ();
	language = gtk_source_languages_manager_get_language_from_mime_type (
		manager, "text/x-patch");

	gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (buffer), language);
	gtk_source_buffer_set_highlight (GTK_SOURCE_BUFFER (buffer), TRUE);
	
	g_object_unref (manager);
}

static void
diff_view_finalize (GObject *object)
{
	GiggleDiffViewPriv *priv;

	priv = GET_PRIV (object);

	if (priv->job) {
		/* FIXME cancel */
	}

	g_object_unref (priv->git);

	if (priv->revision1) {
		g_object_unref (priv->revision1);
	}

	if (priv->revision2) {
		g_object_unref (priv->revision2);
	}

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
	case PROP_REV_1:
		g_value_set_object (value, priv->revision1);
		break;
	case PROP_REV_2:
		g_value_set_object (value, priv->revision2);
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
	case PROP_REV_1:
		if (priv->revision1) {
			g_object_unref (priv->revision1);
		}

		priv->revision1 = (GiggleRevision *) g_value_dup_object (value);
		/* FIXME: not running a new job */
		break;
	case PROP_REV_2:
		if (priv->revision2) {
			g_object_unref (priv->revision2);
		}

		priv->revision2 = (GiggleRevision *) g_value_dup_object (value);
		/* FIXME: not running a new job */
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
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
				GiggleRevision *revision2)
{
	GiggleDiffViewPriv *priv;

	g_return_if_fail (GIGGLE_IS_DIFF_VIEW (diff_view));
	g_return_if_fail (GIGGLE_IS_REVISION (revision1));
	g_return_if_fail (GIGGLE_IS_REVISION (revision2));

	priv = GET_PRIV (diff_view);

	g_object_set (G_OBJECT (diff_view),
		      "revision-1", revision1,
		      "revision-2", revision2,
		      NULL);

	priv->job = giggle_git_diff_new (revision2, revision1);

	giggle_git_run_job (priv->git,
			    priv->job,
			    diff_view_job_callback,
			    diff_view);
}
