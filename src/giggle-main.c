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
#include <gtk/gtkmain.h>

#include "giggle-window.h"

static gboolean diff_window = FALSE;

static GOptionEntry options[] = {
	{ "diff", 'd',
	  0, G_OPTION_ARG_NONE, &diff_window,
	  N_("Show the diff window"),
	  NULL },
	{ NULL }
};

int
main (int argc, char **argv)
{
	GtkWidget      *window;
	gchar          *dir;
	GOptionContext *context;
	GError         *error = NULL;
	
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);  
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);

	if (!gtk_init_with_args (&argc, &argv,
				 NULL,
				 options,
				 GETTEXT_PACKAGE,
				 &error)) {
		g_printerr ("%s\n", error->message);
		return 1;
	}

	g_set_application_name ("Giggle");
	gtk_window_set_default_icon_name (PACKAGE);

	/* parse GIT_DIR into dir and unset it; if empty use the current_wd */
	dir = g_strdup (g_getenv ("GIT_DIR"));
	if (!dir || !*dir) {
		g_free (dir);
		dir = g_get_current_dir ();
	}
	g_unsetenv ("GIT_DIR");

	window = giggle_window_new ();

	if (giggle_git_test_dir (dir)) {
		giggle_window_set_directory (GIGGLE_WINDOW (window), dir);
	}
	g_free (dir);

	/* window will show itself when it reads its initial size configuration */
	if (diff_window) {
		giggle_window_show_diff_window (GIGGLE_WINDOW (window));
	}

	gtk_main ();

	gtk_widget_destroy (window);

	return 0;
}
