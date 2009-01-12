/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2009 Mathias Hasselmann
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

#include "hello-world.h"

void
hello_world_activate (GtkAction   *action,
		      GigglePlugin *plugin)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_INFO,
					 GTK_BUTTONS_OK, "Hello World");

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  "action: %s\nplugin: %s (%s)",
						  gtk_action_get_name (action),
						  giggle_plugin_get_name (plugin),
						  giggle_plugin_get_description (plugin));

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

