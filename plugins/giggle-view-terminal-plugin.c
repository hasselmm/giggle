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
#include "giggle-view-terminal.h"

#include <libgiggle/giggle-plugin-manager.h>
#include <libgiggle/giggle-view-shell.h>

#include <libgiggle-git/giggle-git.h>

void
show_terminal_view_activate	(GtkAction    *action,
				 GigglePlugin *plugin);

void
terminal_view_activate		(GtkAction    *action,
				 GigglePlugin *plugin);

static GtkWidget *
get_terminal_view (GigglePlugin *plugin)
{
	GigglePluginManager *manager;
	GtkWidget           *shell, *view;

	view = g_object_get_data (G_OBJECT (plugin), "giggle-view-terminal");

	if (!view) {
		manager = giggle_plugin_get_manager (plugin);
		shell = giggle_plugin_manager_get_widget (manager, "ViewShell");
		view = giggle_view_terminal_new ();

		giggle_view_shell_append_view (GIGGLE_VIEW_SHELL (shell),
					       GIGGLE_VIEW (view));

		gtk_widget_show (view);

		g_object_set_data (G_OBJECT (plugin), "giggle-view-terminal", view);
	}

	return view;
}

void
show_terminal_view_activate (GtkAction    *action,
			     GigglePlugin *plugin)
{
	GtkWidget  *view, *shell;
	const char *directory;

	view = get_terminal_view (plugin);
	shell = gtk_widget_get_parent (view);

	directory = giggle_git_get_directory (giggle_git_get ());
	giggle_view_terminal_append_tab (GIGGLE_VIEW_TERMINAL (view), directory);
	giggle_view_shell_select (GIGGLE_VIEW_SHELL (shell), GIGGLE_VIEW (view));
}

void
terminal_view_activate (GtkAction    *action,
			GigglePlugin *plugin)
{
	g_signal_handlers_disconnect_by_func (action, terminal_view_activate, plugin);
}
