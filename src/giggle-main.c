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
#include "giggle-window.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdlib.h>

static gboolean diff_window = FALSE;
static gboolean version = FALSE;

static GOptionEntry options[] = {
	{ "diff", 'd',
	  0, G_OPTION_ARG_NONE, &diff_window,
	  N_("Show the diff window"),
	  NULL },
	{ "version", 'v',
	  0, G_OPTION_ARG_NONE, &version, N_("Show version"), NULL },
	{ NULL }
};

int
main (int    argc,
      char **argv)
{
	GtkWidget      *window = NULL;
	GError         *error = NULL;
	GOptionContext *context;
	char           *dir, *description;
	int             result = EXIT_SUCCESS;
	
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);  
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	g_thread_init (NULL);

	gdk_threads_init ();
	gdk_threads_enter ();

 	context = g_option_context_new (_("[DIRECTORY]"));

	description = g_strdup_printf (_("Report errors (in English, with LC_ALL=C) to <%s>."), PACKAGE_BUGREPORT);
	g_option_context_set_summary (context, _("Giggle is a graphical frontend for the git directory tracker."));
	g_option_context_set_description (context, description);
	g_free (description);

	g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		char *basename = g_filename_display_basename (argv[0]);

		g_printerr ("%s: %s\n", basename, error->message);
		g_printerr (_("Try `%s --help' for more information.\n"), basename);

		result = EXIT_FAILURE;
		g_free (basename);

		goto end;
	}

	if (g_file_test ("pixmaps", G_FILE_TEST_IS_DIR))
		gtk_icon_theme_prepend_search_path (gtk_icon_theme_get_default (), "pixmaps");

	if (version) {
		g_print ("%s %s, "
			 "Copyright (C) 2007-2008 Imendio AB, "
			 "Copyright (C) 2008 Mathias Hasselmann\n",
			 PACKAGE_NAME, PACKAGE_VERSION);

		goto end;
	}

	gtk_window_set_default_icon_name (PACKAGE);
	g_set_application_name ("Giggle");
	window = giggle_window_new ();

	/* parse GIT_DIR into dir and unset it; if empty use the current_wd */
	dir = g_strdup (g_getenv ("GIT_DIR"));

	if (!dir || !*dir) {
		g_free (dir);

		if (argc > 1 && *argv[1]) {
			dir = g_strdup (argv[1]);
		} else {
			dir = g_get_current_dir ();
		}
	}

	g_unsetenv ("GIT_DIR");

	if (giggle_git_test_dir (dir)) {
		giggle_window_set_directory (GIGGLE_WINDOW (window), dir);
	} else {
		gtk_widget_show (window);
	}

	g_free (dir);

	/* window will show itself when it reads its initial size configuration */
	if (diff_window)
		giggle_window_show_diff_window (GIGGLE_WINDOW (window));

	gtk_main ();

end:
	if (window)
		gtk_widget_destroy (window);

	gdk_threads_leave ();

	return result;
}
