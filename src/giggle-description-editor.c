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

#include "giggle-description-editor.h"
#include "libgiggle/giggle-git.h"

typedef struct GiggleDescriptionEditorPriv GiggleDescriptionEditorPriv;

struct GiggleDescriptionEditorPriv {
	GtkWidget *textview;
	GtkWidget *save;
	GtkWidget *restore;

	GiggleGit *git;
};

static void description_editor_finalize                (GObject *object);


G_DEFINE_TYPE (GiggleDescriptionEditor, giggle_description_editor, GTK_TYPE_VBOX)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_DESCRIPTION_EDITOR, GiggleDescriptionEditorPriv))


static void
giggle_description_editor_class_init (GiggleDescriptionEditorClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = description_editor_finalize;

	g_type_class_add_private (object_class, sizeof (GiggleDescriptionEditorPriv));
}

static void
description_editor_update (GiggleDescriptionEditor *editor)
{
	GiggleDescriptionEditorPriv *priv;
	GtkTextBuffer               *buffer;
	const gchar                 *description;

	priv = GET_PRIV (editor);

	description = giggle_git_get_description (priv->git);
	if (!description) {
		description = "";
	}
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview));
	gtk_text_buffer_set_text (buffer, description, -1);
	gtk_text_buffer_set_modified (buffer, FALSE);
}

static void
description_editor_save (GiggleDescriptionEditor *editor)
{
	GiggleDescriptionEditorPriv *priv;
	GtkTextBuffer               *buffer;
	GtkTextIter                  start, end;
	gchar                       *text;

	priv = GET_PRIV (editor);
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview));
	gtk_text_buffer_get_bounds (buffer, &start, &end);

	text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
	giggle_git_write_description (priv->git, text);
	g_free (text);

	gtk_text_buffer_set_modified (buffer, FALSE);
}

static void
description_editor_modified_changed_cb (GiggleDescriptionEditor *editor)
{
	/* called when the description within the GtkTextView changes */
	GiggleDescriptionEditorPriv *priv;
	GtkTextBuffer               *buffer;
	gboolean                     modified;

	priv = GET_PRIV (editor);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview));
	modified = gtk_text_buffer_get_modified (buffer);

	gtk_widget_set_sensitive (priv->save, modified);
	gtk_widget_set_sensitive (priv->restore, modified);
}

static void
giggle_description_editor_init (GiggleDescriptionEditor *editor)
{
	GiggleDescriptionEditorPriv *priv;
	GtkWidget                   *buttonbox, *scrolled_window;
	GtkTextBuffer               *buffer;

	priv = GET_PRIV (editor);

	gtk_box_set_spacing (GTK_BOX (editor), 6);

	priv->git = giggle_git_get ();
	g_signal_connect_swapped (priv->git, "notify::git-dir",
				  G_CALLBACK (description_editor_update), editor);
	g_signal_connect_swapped (priv->git, "notify::description",
				  G_CALLBACK (description_editor_update), editor);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);

	gtk_widget_show (scrolled_window);
	gtk_box_pack_start (GTK_BOX (editor), scrolled_window, TRUE, TRUE, 0);

	priv->textview = gtk_text_view_new ();
	gtk_widget_show (priv->textview);
	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->textview);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview));
	g_signal_connect_swapped (buffer, "modified-changed",
				  G_CALLBACK (description_editor_modified_changed_cb), editor);

	buttonbox = gtk_hbutton_box_new ();
	gtk_widget_show (buttonbox);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonbox), GTK_BUTTONBOX_START);
	gtk_box_pack_start (GTK_BOX (editor), buttonbox, FALSE, FALSE, 0);

	priv->save = gtk_button_new_from_stock (GTK_STOCK_SAVE);
	gtk_widget_show (priv->save);
	gtk_widget_set_sensitive (priv->save, FALSE);
	gtk_box_pack_start (GTK_BOX (buttonbox), priv->save, FALSE, FALSE, 0);
	g_signal_connect_swapped (priv->save, "clicked",
				  G_CALLBACK (description_editor_save), editor);

	priv->restore = gtk_button_new_from_stock (GTK_STOCK_REVERT_TO_SAVED);
	gtk_widget_show (priv->restore);
	gtk_widget_set_sensitive (priv->restore, FALSE);
	gtk_box_pack_start (GTK_BOX (buttonbox), priv->restore, FALSE, FALSE, 0);
	g_signal_connect_swapped (priv->restore, "clicked",
				  G_CALLBACK (description_editor_update), editor);

	description_editor_update (editor);
}

static void
description_editor_finalize (GObject *object)
{
	GiggleDescriptionEditorPriv *priv;

	priv = GET_PRIV (object);
	
	g_object_unref (priv->git);

	G_OBJECT_CLASS (giggle_description_editor_parent_class)->finalize (object);
}

GtkWidget *
giggle_description_editor_new (void)
{
	return g_object_new (GIGGLE_TYPE_DESCRIPTION_EDITOR, NULL);
}

