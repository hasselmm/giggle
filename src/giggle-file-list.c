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
#include <gtk/gtk.h>

#include "giggle-git.h"
#include "giggle-file-list.h"

typedef struct GiggleFileListPriv GiggleFileListPriv;

struct GiggleFileListPriv {
	GiggleGit    *git;
	GtkTreeStore *store;
};

static void   file_list_finalize           (GObject        *object);
static void   file_list_directory_changed  (GObject        *object,
					    GParamSpec     *pspec,
					    gpointer        user_data);
static void   file_list_populate           (GiggleFileList *list);


G_DEFINE_TYPE (GiggleFileList, giggle_file_list, GTK_TYPE_TREE_VIEW);

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_FILE_LIST, GiggleFileListPriv))


static void
giggle_file_list_class_init (GiggleFileListClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = file_list_finalize;

	g_type_class_add_private (object_class, sizeof (GiggleFileListPriv));
}

static void
giggle_file_list_init (GiggleFileList *list)
{
	GiggleFileListPriv *priv;
	GtkCellRenderer    *renderer;

	priv = GET_PRIV (list);

	priv->git = giggle_git_get ();
	g_signal_connect (G_OBJECT (priv->git), "notify::git-dir",
			  G_CALLBACK (file_list_directory_changed), list);

	priv->store = gtk_tree_store_new (1, G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (list),
				 GTK_TREE_MODEL (priv->store));

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (list), -1, "", renderer,
						     "text", 0,
						     NULL);
	file_list_populate (list);
}

static void
file_list_finalize (GObject *object)
{
	GiggleFileListPriv *priv;

	priv = GET_PRIV (object);

	g_object_unref (priv->git);
	g_object_unref (priv->store);

	G_OBJECT_CLASS (giggle_file_list_parent_class)->finalize (object);
}

static void
file_list_directory_changed (GObject    *object,
			     GParamSpec *pspec,
			     gpointer    user_data)
{
	GiggleFileListPriv *priv;
	GiggleFileList     *list;

	list = GIGGLE_FILE_LIST (user_data);
	priv = GET_PRIV (list);

	gtk_tree_store_clear (priv->store);
	file_list_populate (list);
}

static void
file_list_populate_dir (GiggleFileList *list,
			const gchar    *directory,
			GtkTreeIter    *parent_iter)
{
	GiggleFileListPriv *priv;
	GDir               *dir;
	const gchar        *name;
	gchar              *path;
	GtkTreeIter         iter;
	

	priv = GET_PRIV (list);
	dir = g_dir_open (directory, 0, NULL);

	g_return_if_fail (dir != NULL);

	while ((name = g_dir_read_name (dir))) {
		gtk_tree_store_append (priv->store,
				       &iter,
				       parent_iter);

		gtk_tree_store_set (priv->store, &iter,
				    0, name,
				    -1);

		path = g_strconcat (directory, G_DIR_SEPARATOR_S, name, NULL);

		if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
			file_list_populate_dir (list, path, &iter);
		}

		g_free (path);
	}

	g_dir_close (dir);
}

static void
file_list_populate (GiggleFileList *list)
{
	GiggleFileListPriv *priv;
	const gchar        *directory;

	priv = GET_PRIV (list);
	directory = giggle_git_get_directory (priv->git);

	file_list_populate_dir (list, directory, NULL);
}

GtkWidget *
giggle_file_list_new (void)
{
	return g_object_new (GIGGLE_TYPE_FILE_LIST, NULL);
}
