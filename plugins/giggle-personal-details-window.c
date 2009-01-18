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
#include "giggle-personal-details-window.h"

#include "libgiggle/giggle-git.h"
#include "libgiggle/giggle-configuration.h"

#include <glade/glade.h>
#include <glib/gi18n.h>
#include <libebook/e-book.h>
#include <string.h>

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_PERSONAL_DETAILS_WINDOW, GigglePersonalDetailsWindowPriv))

typedef struct GigglePersonalDetailsWindowPriv GigglePersonalDetailsWindowPriv;

struct GigglePersonalDetailsWindowPriv {
	GtkWidget           *name_entry;
	GtkWidget           *email_entry;

	GiggleConfiguration *configuration;
};

G_DEFINE_TYPE (GigglePersonalDetailsWindow, giggle_personal_details_window, GTK_TYPE_DIALOG)

static void
personal_details_window_dispose (GObject *object)
{
	GigglePersonalDetailsWindowPriv *priv;

	priv = GET_PRIV (object);

	if (priv->configuration) {
		g_object_unref (priv->configuration);
		priv->configuration = NULL;
	}

	G_OBJECT_CLASS (giggle_personal_details_window_parent_class)->dispose (object);
}

static void
personal_details_configuration_changed_cb (GiggleConfiguration *configuration,
					   gboolean             success,
					   gpointer             user_data)
{
	GigglePersonalDetailsWindow *window;
	GtkWidget                   *dialog;
	GtkWindow                   *parent;

	window = GIGGLE_PERSONAL_DETAILS_WINDOW (user_data);
	parent = gtk_window_get_transient_for (GTK_WINDOW (window));

	gtk_widget_destroy (GTK_WIDGET (window));

	if (success)
		return;

	dialog = gtk_message_dialog_new (parent,
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_CLOSE,
					 _("There was an error "
					   "setting the configuration"));

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	g_object_unref (parent);
}

static void
personal_details_window_response (GtkDialog *dialog,
				  gint       response)
{
	GigglePersonalDetailsWindowPriv *priv;

	priv = GET_PRIV (dialog);

	giggle_configuration_set_field (priv->configuration,
					CONFIG_FIELD_NAME,
					gtk_entry_get_text (GTK_ENTRY (priv->name_entry)));

	giggle_configuration_set_field (priv->configuration,
					CONFIG_FIELD_EMAIL,
					gtk_entry_get_text (GTK_ENTRY (priv->email_entry)));

	giggle_configuration_commit (priv->configuration,
				     personal_details_configuration_changed_cb,
				     g_object_ref (dialog));
}

static void
giggle_personal_details_window_class_init (GigglePersonalDetailsWindowClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (class);

	object_class->dispose = personal_details_window_dispose;
	dialog_class->response = personal_details_window_response;

	g_type_class_add_private (object_class,
				  sizeof (GigglePersonalDetailsWindowPriv));
}

static void
personal_details_configuration_updated_cb (GiggleConfiguration *configuration,
					   gboolean             success,
					   gpointer             user_data)
{
	GigglePersonalDetailsWindow     *window = user_data;
	GigglePersonalDetailsWindowPriv *priv = GET_PRIV (window);
	EContact 			*contact = NULL;
	EBook				*book = NULL;
	GError   			*error = NULL;
	const char			*value;

	gtk_widget_set_sensitive (GTK_WIDGET (window), TRUE);

	if (!success) {
		GtkWindow *parent;
		GtkWidget *dialog;

		parent = gtk_window_get_transient_for (GTK_WINDOW (window));
		gtk_widget_hide (GTK_WIDGET (window));

		dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("There was an error "
						   "getting the configuration"));

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_object_unref (parent);
		return;
	}

	if (!e_book_get_self (&contact, &book, &error)) {
		g_warning ("%s: Cannot open retreive self-contact: %s",
			   G_STRFUNC, error->message);
	}

	value = giggle_configuration_get_field (configuration, CONFIG_FIELD_NAME);

	if ((!value || !*value) && contact)
		value = e_contact_get_const (contact, E_CONTACT_FULL_NAME);
	if (value)
		gtk_entry_set_text (GTK_ENTRY (priv->name_entry), value);

	value = giggle_configuration_get_field (configuration, CONFIG_FIELD_EMAIL);

	if ((!value || !*value) && contact)
		value = e_contact_get_const (contact, E_CONTACT_EMAIL_1);
	if (value)
		gtk_entry_set_text (GTK_ENTRY (priv->email_entry), value);

	if (contact)
		g_object_unref (contact);
	if (book)
		g_object_unref (book);
}

static void
giggle_personal_details_window_init (GigglePersonalDetailsWindow *window)
{
	GigglePersonalDetailsWindowPriv *priv;
	GladeXML                        *xml;
	GtkWidget                       *table;

	priv = GET_PRIV (window);

	xml = glade_xml_new (GLADEDIR "/main-window.glade",
			     "personal_details_table", NULL);

	table = glade_xml_get_widget (xml, "personal_details_table");
	priv->name_entry = glade_xml_get_widget (xml, "name_entry");
	priv->email_entry = glade_xml_get_widget (xml, "email_entry");

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->vbox), table);

	gtk_window_set_title (GTK_WINDOW (window), _("Personal Details"));
	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);

	gtk_dialog_add_button (GTK_DIALOG (window), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	gtk_widget_set_sensitive (GTK_WIDGET (window), FALSE);

	priv->configuration = giggle_configuration_new ();

	giggle_configuration_update (priv->configuration,
				     personal_details_configuration_updated_cb,
				     window);
}

GtkWidget*
giggle_personal_details_window_new (void)
{
	return g_object_new (GIGGLE_TYPE_PERSONAL_DETAILS_WINDOW,
			     "has-separator", FALSE,
			     NULL);
}
