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
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcelanguagesmanager.h>

static const gchar *test_diff =
	"diff --git a/src/test-patch-view.c b/src/test-patch-view.c\n"
	"index 6999aa3..2c90d9a 100644\n"
	"--- a/src/test-patch-view.c\n"
	"+++ b/src/test-patch-view.c\n"
	"@@ -1,9 +1,47 @@\n"
	" #include <gtk/gtk.h>\n"
	"+#include <gtksourceview/gtksourceview.h>\n"
	"+#include <gtksourceview/gtksourcelanguagesmanager.h>\n"
	"+\n"
	"+static GtkWidget *\n"
	"+giggle_diff_view_new (void)\n"
	"+{\n"
	"+       GtkWidget                 *view;\n"
	"+       GtkTextBuffer             *buffer;\n"
	"+       GtkSourceLanguage         *language;\n"
	"+       GtkSourceLanguagesManager *manager;\n"
	"+\n"
	"+       view = gtk_source_view_new ();\n"
	"+       gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);\n"
	"+\n"
	"+       buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));\n"
	"+\n"
	"+       manager = gtk_source_languages_manager_new ();\n"
	"+       language = gtk_source_languages_manager_get_language_from_mime_type (\n"
	"+               manager, \"text/x-patch\");\n"
	"+       g_object_unref (manager);\n"
	"+\n"
	"+       gtk_source_buffer_set_language  (GTK_SOURCE_BUFFER (buffer), language);\n"
	"+\n";

static GtkWidget *
giggle_diff_view_new (void)
{
	GtkWidget                 *view;
	GtkTextBuffer             *buffer;
	GtkSourceLanguage         *language;
	GtkSourceLanguagesManager *manager;
	
	view = gtk_source_view_new ();
	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

	manager = gtk_source_languages_manager_new ();
	language = gtk_source_languages_manager_get_language_from_mime_type (
		manager, "text/x-patch");

	gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (buffer), language);
	gtk_source_buffer_set_highlight (GTK_SOURCE_BUFFER (buffer), TRUE);
	
	g_object_unref (manager);

	return view;
}

static void
giggle_diff_view_set_text (GtkWidget   *view /* FIXME */,
			   const gchar *text)
{
	GtkTextBuffer *buffer;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

	gtk_text_buffer_set_text (buffer, text, -1);
}

int
main (int argc, char **argv)
{
	GtkWidget *window;
	GtkWidget *scrolled;
	GtkWidget *view;
	
	gtk_init (&argc, &argv);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size (GTK_WINDOW (window), 400, 300);
	g_signal_connect (window,
			  "destroy",
			  G_CALLBACK (gtk_main_quit),
			  NULL);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (window), scrolled);
	
	view = giggle_diff_view_new ();

	giggle_diff_view_set_text (view, test_diff);

	gtk_container_add (GTK_CONTAINER (scrolled), view);

	gtk_widget_show_all (window);
	
	gtk_main ();

	return 0;
}
