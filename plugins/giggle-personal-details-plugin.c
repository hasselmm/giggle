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

#include "config.h"
#include "giggle-personal-details-window.h"
#include "libgiggle/giggle-plugin.h"

void
personal_details_activate (GtkAction    *action,
                           GigglePlugin *plugin);

static GtkWindow *
find_parent (void)
{
	GList     *toplevels = gtk_window_list_toplevels ();
	GtkWindow *parent = NULL;

	while (toplevels) {
		if (gtk_window_has_toplevel_focus (toplevels->data)) {
			parent = toplevels->data;
			break;
		}

		toplevels = g_list_delete_link (toplevels, toplevels);
	}

	g_list_free (toplevels);

	return parent;
		
}

void
personal_details_activate (GtkAction    *action,
                           GigglePlugin *plugin)
{
	GtkWidget *window;

	window = giggle_personal_details_window_new ();
	gtk_window_set_transient_for (GTK_WINDOW (window), find_parent ());
	g_signal_connect_after (window, "response", G_CALLBACK (gtk_widget_destroy), NULL);

	gtk_widget_show (window);
}
