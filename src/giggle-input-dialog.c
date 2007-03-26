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

#include "giggle-input-dialog.h"

typedef struct GiggleInputDialogPriv GiggleInputDialogPriv;

struct GiggleInputDialogPriv {
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *ok_button;
};

static void input_dialog_finalize                (GObject *object);
static void input_dialog_set_property            (GObject        *object,
						  guint           param_id,
						  const GValue   *value,
						  GParamSpec     *pspec);
static void input_dialog_get_property            (GObject        *object,
						  guint           param_id,
						  GValue         *value,
						  GParamSpec     *pspec);

G_DEFINE_TYPE (GiggleInputDialog, giggle_input_dialog, GTK_TYPE_DIALOG)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_INPUT_DIALOG, GiggleInputDialogPriv))

enum {
	PROP_0,
	PROP_LABEL,
	PROP_TEXT,
};


static void
giggle_input_dialog_class_init (GiggleInputDialogClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = input_dialog_finalize;
	object_class->set_property = input_dialog_set_property;
	object_class->get_property = input_dialog_get_property;

	g_object_class_install_property (
		object_class,
		PROP_LABEL,
		g_param_spec_string ("label",
				     "Label",
				     "Dialog label",
				     NULL,
				     G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (
		object_class,
		PROP_TEXT,
		g_param_spec_string ("text",
				     "Text",
				     "Text",
				     NULL,
				     G_PARAM_READABLE));

	g_type_class_add_private (object_class, sizeof (GiggleInputDialogPriv));
}

static void
input_dialog_entry_changed (GtkEditable       *editable,
			    GiggleInputDialog *input_dialog)
{
	GiggleInputDialogPriv *priv;
	const gchar           *text;

	text = gtk_entry_get_text (GTK_ENTRY (editable));
	priv = GET_PRIV (input_dialog);

	gtk_widget_set_sensitive (priv->ok_button, (text && *text));
}

static void
input_dialog_entry_insert_text (GtkEditable       *editable,
				gchar             *new_text,
				gint               new_text_length,
				gint              *position,
				GiggleInputDialog *input_dialog)
{
	if (strchr (new_text, ' ')) {
		g_signal_stop_emission_by_name (editable, "insert-text");
	}
}

static void
giggle_input_dialog_init (GiggleInputDialog *input_dialog)
{
	GiggleInputDialogPriv *priv;
	GtkWidget             *box;

	priv = GET_PRIV (input_dialog);

	box = gtk_vbox_new (6, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (box), 7);

	priv->label = gtk_label_new (NULL);
	g_object_ref_sink (priv->label);
	gtk_misc_set_alignment (GTK_MISC (priv->label), 0., 0.);
	gtk_box_pack_start (GTK_BOX (box), priv->label, FALSE, FALSE, 0);

	priv->entry = gtk_entry_new ();
	g_object_ref_sink (priv->entry);
	gtk_entry_set_activates_default (GTK_ENTRY (priv->entry), TRUE);
	gtk_box_pack_start (GTK_BOX (box), priv->entry, FALSE, FALSE, 0);

	g_signal_connect (priv->entry, "changed",
			  G_CALLBACK (input_dialog_entry_changed), input_dialog);
	/* FIXME: should be an option */
	g_signal_connect (priv->entry, "insert-text",
			  G_CALLBACK (input_dialog_entry_insert_text), input_dialog);

	gtk_widget_show_all (box);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (input_dialog)->vbox), box);

	gtk_dialog_add_button (GTK_DIALOG (input_dialog),
			       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	priv->ok_button = gtk_dialog_add_button (GTK_DIALOG (input_dialog),
						 GTK_STOCK_OK, GTK_RESPONSE_OK);

	gtk_widget_set_sensitive (priv->ok_button, FALSE);
	gtk_window_set_resizable (GTK_WINDOW (input_dialog), FALSE);

	gtk_dialog_set_default_response (GTK_DIALOG (input_dialog), GTK_RESPONSE_OK);
	g_object_set (input_dialog, "has-separator", FALSE, NULL);
}

static void
input_dialog_finalize (GObject *object)
{
	GiggleInputDialogPriv *priv;

	priv = GET_PRIV (object);

	g_object_unref (priv->label);
	g_object_unref (priv->entry);

	G_OBJECT_CLASS (giggle_input_dialog_parent_class)->finalize (object);
}

static void
input_dialog_set_property (GObject      *object,
			   guint         param_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
	GiggleInputDialogPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_LABEL:
		gtk_label_set_text (GTK_LABEL (priv->label),
				    g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
input_dialog_get_property (GObject    *object,
			   guint       param_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
	GiggleInputDialogPriv *priv;
	const gchar           *text;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_TEXT:
		text = giggle_input_dialog_get_text (GIGGLE_INPUT_DIALOG (object));
		g_value_set_string (value, text);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

GtkWidget*
giggle_input_dialog_new (const gchar *text)
{
	return g_object_new (GIGGLE_TYPE_INPUT_DIALOG,
			     "label", text,
			     NULL);
}

G_CONST_RETURN gchar*
giggle_input_dialog_get_text (GiggleInputDialog *input_dialog)
{
	GiggleInputDialogPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_INPUT_DIALOG (input_dialog), NULL);

	priv = GET_PRIV (input_dialog);

	return gtk_entry_get_text (GTK_ENTRY (priv->entry));
}
