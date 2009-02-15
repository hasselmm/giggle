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

#include <libgiggle-git/giggle-git-config.h>

#include <glib/gi18n.h>
#include <string.h>

#ifdef ENABLE_EDS
#include <libebook/e-book.h>
#endif /* ENABLE_EDS */

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_PERSONAL_DETAILS_WINDOW, GigglePersonalDetailsWindowPriv))

typedef struct GigglePersonalDetailsWindowPriv GigglePersonalDetailsWindowPriv;

struct GigglePersonalDetailsWindowPriv {
	GtkWidget       *name_entry;
	GtkWidget       *email_entry;

	GiggleGitConfig *configuration;
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
personal_details_configuration_changed_cb (GiggleGitConfig *config,
					   gboolean         success,
					   gpointer         user_data)
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

	giggle_git_config_set_field (priv->configuration,
				     GIGGLE_GIT_CONFIG_FIELD_NAME,
				     gtk_entry_get_text (GTK_ENTRY (priv->name_entry)));

	giggle_git_config_set_field (priv->configuration,
				     GIGGLE_GIT_CONFIG_FIELD_EMAIL,
				     gtk_entry_get_text (GTK_ENTRY (priv->email_entry)));

	giggle_git_config_commit (priv->configuration,
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

#ifdef ENABLE_EDS

static GtkEntryCompletion *
create_email_completion (EContact *contact)
{
	GList              *email_list;
	GtkListStore       *email_store;
	GtkEntryCompletion *completion;
	GtkTreeIter         iter;

	email_list = e_contact_get (contact, E_CONTACT_EMAIL);
	email_store = gtk_list_store_new (1, G_TYPE_STRING);

	while (email_list) {
		gtk_list_store_append (email_store, &iter);
		gtk_list_store_set (email_store, &iter, 0, email_list->data, -1);
		email_list = email_list->next;
	}

	completion = gtk_entry_completion_new ();

	gtk_entry_completion_set_popup_set_width (completion, FALSE);
	gtk_entry_completion_set_model (completion, GTK_TREE_MODEL (email_store));
	gtk_entry_completion_set_text_column (completion, 0);

	g_object_unref (email_store);

	return completion;
}

#endif /* ENABLE_EDS */

static void
personal_details_configuration_updated_cb (GiggleGitConfig *configuration,
					   gboolean         success,
					   gpointer         user_data)
{
	GigglePersonalDetailsWindow     *window = user_data;
	GigglePersonalDetailsWindowPriv *priv = GET_PRIV (window);
	const char			*name, *email;

#ifdef ENABLE_EDS
	EContact 			*contact = NULL;
	EBook				*book = NULL;
	GError   			*error = NULL;
#endif /* ENABLE_EDS */

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

	name = giggle_git_config_get_field (configuration, GIGGLE_GIT_CONFIG_FIELD_NAME);
	email = giggle_git_config_get_field (configuration, GIGGLE_GIT_CONFIG_FIELD_EMAIL);

#ifdef ENABLE_EDS

	if (!e_book_get_self (&contact, &book, &error)) {
		g_warning ("%s: Cannot open retreive self-contact: %s",
			   G_STRFUNC, error->message);
	}

	if ((!name || !*name) && contact)
		name = e_contact_get_const (contact, E_CONTACT_FULL_NAME);
	if ((!email || !*email) && contact)
		email = e_contact_get_const (contact, E_CONTACT_EMAIL_1);

#endif /* ENABLE_EDS */

	if (!name || !*name)
		name = g_get_real_name ();
	if (!email || !*email)
		email = g_getenv ("EMAIL");

	if (name)
		gtk_entry_set_text (GTK_ENTRY (priv->name_entry), name);
	if (email)
		gtk_entry_set_text (GTK_ENTRY (priv->email_entry), email);

#ifdef ENABLE_EDS

	if (contact) {
		GtkEntryCompletion *completion = create_email_completion (contact);
		gtk_entry_set_completion (GTK_ENTRY (priv->email_entry), completion);
		g_object_unref (completion);
	}

	if (contact)
		g_object_unref (contact);
	if (book)
		g_object_unref (book);

#endif /* ENABLE_EDS */
}

static void
giggle_personal_details_window_init (GigglePersonalDetailsWindow *window)
{
	GigglePersonalDetailsWindowPriv *priv = GET_PRIV (window);
	GtkWidget                       *label, *table;

	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 6);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_container_set_border_width (GTK_CONTAINER (table), 6);

	label = gtk_label_new_with_mnemonic (_("_Name:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

	priv->name_entry = gtk_entry_new ();
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), priv->name_entry);
	gtk_table_attach (GTK_TABLE (table), priv->name_entry, 1, 2, 0, 1, GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);

	label = gtk_label_new_with_mnemonic (_("_Email:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

	priv->email_entry = gtk_entry_new ();
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), priv->email_entry);
	gtk_table_attach (GTK_TABLE (table), priv->email_entry, 1, 2, 1, 2, GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);

	gtk_widget_show_all (table);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->vbox), table);

	gtk_window_set_title (GTK_WINDOW (window), _("Personal Details"));
	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);

	gtk_dialog_add_button (GTK_DIALOG (window), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	gtk_widget_set_sensitive (GTK_WIDGET (window), FALSE);

	priv->configuration = giggle_git_config_new ();

	giggle_git_config_update (priv->configuration,
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
