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
#include <glade/glade.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcelanguagesmanager.h>

#include "giggle-window.h"
#include "giggle-graph-renderer.h"

typedef struct GiggleWindowPriv GiggleWindowPriv;

struct GiggleWindowPriv {
	GtkWidget    *content_vbox;
	GtkWidget    *menubar_hbox;
	GtkWidget    *revision_treeview;
	GtkWidget    *log_textview;
	GtkWidget    *diff_textview;

	GtkUIManager *ui_manager;
};

enum {
	REVISION_COL_SHORT_LOG,
	REVISION_COL_LONG_LOG,
	REVISION_COL_AUTHOR,
	REVISION_COL_DATE,
	REVISION_COL_SHA,
	REVISION_NUM_COLS
};

static void window_finalize                      (GObject          *object);
static void window_setup_revision_treeview       (GiggleWindow     *window);
static void window_setup_diff_textview           (GiggleWindow     *window,
						  GtkWidget        *scrolled);
static void window_update_revision_info          (GiggleWindow     *window,
						  const gchar      *log,
						  const gchar      *sha);
static void window_add_widget_cb                 (GtkUIManager     *merge,
						  GtkWidget        *widget,
						  GiggleWindow     *window);
static void window_revision_selection_changed_cb (GtkTreeSelection *selection,
						  GiggleWindow     *window);
static void window_action_quit_cb                (GtkAction        *action,
						  GiggleWindow     *window);
static void window_action_about_cb               (GtkAction        *action,
						  GiggleWindow     *window);
static void window_action_foo_cb                 (GtkAction        *action,
						  GiggleWindow     *window);

static const GtkActionEntry action_entries[] = {
	{ "FileMenu", NULL,
	  N_("_File"), NULL, NULL,
	  NULL
	},
	{ "EditMenu", NULL,
	  N_("_Edit"), NULL, NULL,
	  NULL
	},
	{ "HelpMenu", NULL,
	  N_("_Help"), NULL, NULL,
	  NULL
	},
	{ "Foo", NULL,
	  N_("_Foo"), NULL, N_("Foo bar baz"),
	  G_CALLBACK (window_action_foo_cb)
	},
	{ "Quit", GTK_STOCK_QUIT,
	  N_("_Quit"), "<control>Q", N_("Quit the application"),
	  G_CALLBACK (window_action_quit_cb)
	},
	{ "About", NULL,
	  N_("_About"), NULL, N_("About this application"),
	  G_CALLBACK (window_action_about_cb)
	}
};

static const gchar *ui_layout =
	"<ui>"
	"  <menubar name='MainMenubar'>"
	"    <menu action='FileMenu'>"
	"      <menuitem action='Foo'/>"
	"      <separator/>"
	"      <menuitem action='Quit'/>"
	"    </menu>"
	"    <menu action='EditMenu'>"
	"      <menuitem action='Foo'/>"
	"    </menu>"
	"    <menu action='HelpMenu'>"
	"      <menuitem action='About'/>"
	"    </menu>"
	"  </menubar>"
	"</ui>";


G_DEFINE_TYPE (GiggleWindow, giggle_window, GTK_TYPE_WINDOW);

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_WINDOW, GiggleWindowPriv))

static void
giggle_window_class_init (GiggleWindowClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = window_finalize;

	g_type_class_add_private (object_class, sizeof (GiggleWindowPriv));
}

static void
giggle_window_init (GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GladeXML         *xml;
	GtkActionGroup   *action_group;
	GError           *error = NULL;

	priv = GET_PRIV (window);

	gtk_window_set_title (GTK_WINDOW (window), "Giggle");
	
	xml = glade_xml_new (GLADEDIR "/main-window.glade",
			     "content_vbox", NULL);
	if (!xml) {
		g_error ("Couldn't find glade file, did you install?");
	}

	priv->content_vbox = glade_xml_get_widget (xml, "content_vbox");
	gtk_container_add (GTK_CONTAINER (window), priv->content_vbox);

	priv->menubar_hbox = glade_xml_get_widget (xml, "menubar_hbox");
	priv->revision_treeview = glade_xml_get_widget (xml, "revision_treeview");
	window_setup_revision_treeview (window);

	priv->log_textview = glade_xml_get_widget (xml, "log_textview");

	window_setup_diff_textview (
		window,
		glade_xml_get_widget (xml, "diff_scrolledwindow"));

	g_object_unref (xml);

	priv->ui_manager = gtk_ui_manager_new ();
	g_signal_connect (priv->ui_manager,
			  "add_widget",
			  G_CALLBACK (window_add_widget_cb),
			  window);

	action_group = gtk_action_group_new ("MainActions");
	/*gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);*/
	gtk_action_group_add_actions (action_group,
				      action_entries,
				      G_N_ELEMENTS (action_entries),
				      window);
	gtk_ui_manager_insert_action_group (priv->ui_manager, action_group, 0);

	gtk_window_add_accel_group (GTK_WINDOW (window),
				    gtk_ui_manager_get_accel_group (priv->ui_manager));

	g_object_unref (action_group);

	gtk_ui_manager_add_ui_from_string (priv->ui_manager, ui_layout, -1, &error);
	if (error) {
		g_error ("Couldn't create UI: %s}\n", error->message);
	}

	gtk_ui_manager_ensure_update (priv->ui_manager);
}

static void
window_finalize (GObject *object)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (object);
	
	g_object_unref (priv->ui_manager);

	G_OBJECT_CLASS (giggle_window_parent_class)->finalize (object);
}

static void
window_setup_revision_treeview (GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GtkListStore     *model;
	GtkCellRenderer  *cell;
	GtkTreeIter       iter;
	GtkTreeSelection *selection;

	priv = GET_PRIV (window);

	model = gtk_list_store_new (REVISION_NUM_COLS,
				    G_TYPE_STRING,
				    G_TYPE_STRING,
				    G_TYPE_STRING,
				    G_TYPE_STRING,
				    G_TYPE_STRING);

	gtk_tree_view_set_model (GTK_TREE_VIEW (priv->revision_treeview),
				 GTK_TREE_MODEL (model));

	g_object_unref (model);
	
	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (
		GTK_TREE_VIEW (priv->revision_treeview),
		-1,
		_("Graph"),
		cell,
		NULL);

	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (
		GTK_TREE_VIEW (priv->revision_treeview),
		-1,
		_("Short Log"),
		cell,
		"text", REVISION_COL_SHORT_LOG,
		NULL);

	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (
		GTK_TREE_VIEW (priv->revision_treeview),
		-1,
		_("Author"),
		cell,
		"text", REVISION_COL_AUTHOR,
		NULL);

	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (
		GTK_TREE_VIEW (priv->revision_treeview),
		-1,
		_("Date"),
		cell,
		"text", REVISION_COL_DATE,
		NULL);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->revision_treeview));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (window_revision_selection_changed_cb),
			  window);
	
	/* Fake some data for now. */
	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter,
			    REVISION_COL_SHORT_LOG, "Blah blah",
			    REVISION_COL_LONG_LOG, "Boring stuff that nobody really cares about.",
			    REVISION_COL_AUTHOR, "Richard Hult <richard@imendio.com>",
			    REVISION_COL_SHA, "12aaa3d4a567b890",
			    REVISION_COL_DATE, "2007-01-12",
			    -1);

	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter,
			    REVISION_COL_SHORT_LOG, "Oh yeah",
			    REVISION_COL_LONG_LOG, "Did some really cool nice changes that will blow yor socks off.",
			    REVISION_COL_AUTHOR, "Mikael Hallendal <micke@imendio.com>",
			    REVISION_COL_SHA, "123db456aaf7890",
			    REVISION_COL_DATE, "2007-01-13",
			    -1);

	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter,
			    REVISION_COL_SHORT_LOG, "What!?",
			    REVISION_COL_LONG_LOG, "Made the cell renderer show nice cairo rendered lines and blobs in varying colors.",
			    REVISION_COL_AUTHOR, "Carlos Garnacho <carlosg@gnome.org>",
			    REVISION_COL_SHA, "986db1fda56aaf7890",
			    REVISION_COL_DATE, "2007-01-13",
			    -1);
}

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

static void
window_setup_diff_textview (GiggleWindow *window,
			    GtkWidget    *scrolled)
{
	GiggleWindowPriv          *priv;
	GtkTextBuffer             *buffer;
	GtkSourceLanguage         *language;
	GtkSourceLanguagesManager *manager;

	priv = GET_PRIV (window);
	
	priv->diff_textview = gtk_source_view_new ();
	gtk_text_view_set_editable (GTK_TEXT_VIEW (priv->diff_textview), FALSE);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->diff_textview));

	manager = gtk_source_languages_manager_new ();
	language = gtk_source_languages_manager_get_language_from_mime_type (
		manager, "text/x-patch");

	gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (buffer), language);
	gtk_source_buffer_set_highlight (GTK_SOURCE_BUFFER (buffer), TRUE);
	
	g_object_unref (manager);

	gtk_widget_show (priv->diff_textview);
	
	gtk_container_add (GTK_CONTAINER (scrolled), priv->diff_textview);

	/* FIXME: Just testing for now... */
	gtk_text_buffer_set_text (buffer, test_diff, -1);
}

static void
window_update_revision_info (GiggleWindow *window,
			     const gchar  *log,
			     const gchar  *sha)
{
	GiggleWindowPriv *priv;
	gchar            *str;
	
	priv = GET_PRIV (window);

	str = g_strdup_printf ("%s\n%s", sha, log);
	
	gtk_text_buffer_set_text (
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->log_textview)),
		str, -1);

	g_free (str);
}

static void
window_add_widget_cb (GtkUIManager *merge, 
		      GtkWidget    *widget, 
		      GiggleWindow *window)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (window);

	gtk_box_pack_start (GTK_BOX (priv->menubar_hbox), widget, TRUE, TRUE, 0);
}

static void
window_revision_selection_changed_cb (GtkTreeSelection *selection,
				      GiggleWindow     *window)
{
	GiggleWindowPriv *priv;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	gchar            *log, *sha;

	priv = GET_PRIV (window);
	
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		return;
	}

	gtk_tree_model_get (model, &iter,
			    REVISION_COL_LONG_LOG, &log,
			    REVISION_COL_SHA, &sha,
			    -1);

	window_update_revision_info (window, log, sha);
	
	g_free (log);
	g_free (sha);
}

static void
window_action_quit_cb (GtkAction    *action,
		       GiggleWindow *window)
{
	gtk_main_quit ();
}

static void
window_action_about_cb (GtkAction    *action,
			GiggleWindow *window)
{
	gtk_show_about_dialog (GTK_WINDOW (window),
			       "name", "Giggle",
			       "copyright", "Copyright 2007 Imendio AB",
			       NULL);
}

static void
window_action_foo_cb (GtkAction    *action,
		      GiggleWindow *window)
{
	g_print ("Foo activated\n");
}

GtkWidget *
giggle_window_new (void)
{
	GtkWidget *window;

	window = g_object_new (GIGGLE_TYPE_WINDOW, NULL);

	return window;
}
