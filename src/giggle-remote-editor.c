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

#include "giggle-remote-editor.h"

#include <glib/gi18n.h>
#include <gtk/gtkbox.h>
#include <gtk/gtkstock.h>
#include <glade/glade.h>

typedef struct GiggleRemoteEditorPriv GiggleRemoteEditorPriv;

struct GiggleRemoteEditorPriv {
	gboolean      new_remote;
	GiggleRemote *remote;
};

enum {
	PROP_0,
	PROP_REMOTE
};

static GObject *remote_editor_constructor         (GType                  type,
						   guint                  n_param,
						   GObjectConstructParam *params);
static void     remote_editor_finalize            (GObject           *object);
static void     remote_editor_get_property        (GObject           *object,
					   guint              param_id,
					   GValue            *value,
					   GParamSpec        *pspec);
static void     remote_editor_set_property        (GObject           *object,
					   guint              param_id,
					   const GValue      *value,
					   GParamSpec        *pspec);
static void     remote_editor_response    (GtkDialog         *dialog,
					   gint               reponse);

G_DEFINE_TYPE (GiggleRemoteEditor, giggle_remote_editor, GTK_TYPE_DIALOG)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REMOTE_EDITOR, GiggleRemoteEditorPriv))

static void
giggle_remote_editor_class_init (GiggleRemoteEditorClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (class);

	object_class->constructor  = remote_editor_constructor;
	object_class->finalize     = remote_editor_finalize;
	object_class->get_property = remote_editor_get_property;
	object_class->set_property = remote_editor_set_property;

	g_object_class_install_property (object_class,
					 PROP_REMOTE,
					 g_param_spec_object ("remote",
							      "Remote",
							      "The remote being edited",
							      GIGGLE_TYPE_REMOTE,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	dialog_class->response = remote_editor_response;

	g_type_class_add_private (object_class, sizeof (GiggleRemoteEditorPriv));
}

static void
giggle_remote_editor_init (GiggleRemoteEditor *remote_editor)
{
	GiggleRemoteEditorPriv *priv;
	GladeXML               *xml;

	priv = GET_PRIV (remote_editor);

	xml = glade_xml_new (GLADEDIR "/main-window.glade", "remote_vbox", NULL);

	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (remote_editor)->vbox),
				     glade_xml_get_widget (xml, "remote_vbox"));

	gtk_dialog_set_has_separator (GTK_DIALOG (remote_editor), FALSE);
}

static GObject *
remote_editor_constructor (GType                  type,
			   guint                  n_params,
			   GObjectConstructParam *params)
{
	GiggleRemoteEditorPriv *priv;
	GObject                *object;

	object = G_OBJECT_CLASS(giggle_remote_editor_parent_class)->constructor (
			type, n_params, params);

	priv = GET_PRIV (object);
	priv->new_remote = G_OBJECT(priv->remote)->ref_count == 1;

	gtk_dialog_add_buttons (GTK_DIALOG (object),
				GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
				priv->new_remote ? GTK_STOCK_ADD : GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				NULL);

	return object;
}

static void
remote_editor_finalize (GObject *object)
{
	GiggleRemoteEditorPriv *priv;

	priv = GET_PRIV (object);
	
	g_object_unref (priv->remote);

	G_OBJECT_CLASS (giggle_remote_editor_parent_class)->finalize (object);
}

static void
remote_editor_get_property (GObject    *object,
		    guint       param_id,
		    GValue     *value,
		    GParamSpec *pspec)
{
	GiggleRemoteEditorPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_REMOTE:
		g_value_set_object (value, priv->remote);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
remote_editor_set_property (GObject      *object,
		    guint         param_id,
		    const GValue *value,
		    GParamSpec   *pspec)
{
	GiggleRemoteEditorPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_REMOTE:
#if GLIB_MAJOR_VERSION <= 2 && GLIB_MINOR_VERSION <= 12
		priv->remote = (gpointer)g_value_dup_object (value);
#else
		priv->remote = g_value_dup_object (value);
#endif
		if(!priv->remote) {
			priv->remote = giggle_remote_new (_("Unnamed"));
		}
		// FIXME: connect to signals
		g_object_notify (object, "remote");
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
remote_editor_response (GtkDialog *dialog,
			gint       response)
{
	/* FIXME: save the remote */

	if(GTK_DIALOG_CLASS(giggle_remote_editor_parent_class)->response) {
		GTK_DIALOG_CLASS(giggle_remote_editor_parent_class)->response (dialog, response);
	}
}

GtkWidget *
giggle_remote_editor_new (GiggleRemote *remote)
{
	return g_object_new (GIGGLE_TYPE_REMOTE_EDITOR, "remote", remote, NULL);
}

