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
#include <string.h>

#include "giggle-git.h"
#include "giggle-file-list.h"

typedef struct GiggleFileListPriv GiggleFileListPriv;

struct GiggleFileListPriv {
	GiggleGit    *git;
	GtkIconTheme *icon_theme;

	GtkTreeStore *store;
	GtkTreeModel *filter_model;

	gboolean      show_all;
};

static void       file_list_finalize           (GObject        *object);
static void       file_list_get_property       (GObject           *object,
						guint              param_id,
						GValue            *value,
						GParamSpec        *pspec);
static void       file_list_set_property       (GObject           *object,
						guint              param_id,
						const GValue      *value,
						GParamSpec        *pspec);

static void       file_list_directory_changed  (GObject        *object,
						GParamSpec     *pspec,
						gpointer        user_data);
static void       file_list_populate           (GiggleFileList *list);
static void       file_list_populate_dir       (GiggleFileList *list,
						const gchar    *directory,
						GtkTreeIter    *parent_iter);
static gboolean   file_list_filter_func        (GtkTreeModel   *model,
						GtkTreeIter    *iter,
						gpointer        user_data);


G_DEFINE_TYPE (GiggleFileList, giggle_file_list, GTK_TYPE_TREE_VIEW);

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_FILE_LIST, GiggleFileListPriv))

enum {
	COL_NAME,
	COL_PIXBUF,
	LAST_COL
};

enum {
	PROP_0,
	PROP_SHOW_ALL,
};


static void
giggle_file_list_class_init (GiggleFileListClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = file_list_finalize;
	object_class->get_property = file_list_get_property;
	object_class->set_property = file_list_set_property;

	g_object_class_install_property (object_class,
					 PROP_SHOW_ALL,
					 g_param_spec_boolean ("show-all",
							       "Show all",
							       "Whether to show all elements",
							       FALSE,
							       G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleFileListPriv));
}

static void
giggle_file_list_init (GiggleFileList *list)
{
	GiggleFileListPriv *priv;
	GtkCellRenderer    *renderer;
	GtkTreeViewColumn  *column;

	priv = GET_PRIV (list);

	priv->git = giggle_git_get ();
	g_signal_connect (G_OBJECT (priv->git), "notify::git-dir",
			  G_CALLBACK (file_list_directory_changed), list);

	priv->icon_theme = gtk_icon_theme_get_default ();

	priv->store = gtk_tree_store_new (LAST_COL, G_TYPE_STRING, GDK_TYPE_PIXBUF);
	priv->filter_model = gtk_tree_model_filter_new (GTK_TREE_MODEL (priv->store), NULL);

	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (priv->filter_model),
						file_list_filter_func, list, NULL);
						
	gtk_tree_view_set_model (GTK_TREE_VIEW (list), priv->filter_model);

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Name"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column), renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (column), renderer,
					"pixbuf", COL_PIXBUF,
					NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column), renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (column), renderer,
					"text", COL_NAME,
					NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
	file_list_populate (list);

}

static void
file_list_finalize (GObject *object)
{
	GiggleFileListPriv *priv;

	priv = GET_PRIV (object);

	g_object_unref (priv->git);
	g_object_unref (priv->store);
	g_object_unref (priv->filter_model);

	G_OBJECT_CLASS (giggle_file_list_parent_class)->finalize (object);
}

static void
file_list_get_property (GObject    *object,
			guint       param_id,
			GValue     *value,
			GParamSpec *pspec)
{
	GiggleFileListPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_SHOW_ALL:
		g_value_set_boolean (value, priv->show_all);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
file_list_set_property (GObject      *object,
			guint         param_id,
			const GValue *value,
			GParamSpec   *pspec)
{
	GiggleFileListPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_SHOW_ALL:
		giggle_file_list_set_show_all (GIGGLE_FILE_LIST (object),
					       g_value_get_boolean (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
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
file_list_add_element (GiggleFileList *list,
		       const gchar    *path,
		       const gchar    *name,
		       GtkTreeIter    *parent_iter)
{
	GiggleFileListPriv *priv;
	GdkPixbuf          *pixbuf;
	GtkTreeIter         iter;
	gboolean            is_dir;

	priv = GET_PRIV (list);
	is_dir = g_file_test (path, G_FILE_TEST_IS_DIR);

	gtk_tree_store_append (priv->store,
			       &iter,
			       parent_iter);

	if (is_dir) {
		file_list_populate_dir (list, path, &iter);
		pixbuf = gtk_icon_theme_load_icon (priv->icon_theme,
						   "folder", 16, 0, NULL);;
	} else {
		pixbuf = gtk_icon_theme_load_icon (priv->icon_theme,
						   "gnome-mime-text", 16, 0, NULL);;
	}

	gtk_tree_store_set (priv->store, &iter,
			    COL_PIXBUF, pixbuf,
			    COL_NAME, (name) ? name : path,
			    -1);

	if (pixbuf) {
		g_object_unref (pixbuf);
	}
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

	priv = GET_PRIV (list);
	dir = g_dir_open (directory, 0, NULL);

	g_return_if_fail (dir != NULL);

	while ((name = g_dir_read_name (dir))) {
		path = g_strconcat (directory, G_DIR_SEPARATOR_S, name, NULL);
		file_list_add_element (list, path, name, parent_iter);
		g_free (path);
	}

	g_dir_close (dir);
}

static void
file_list_populate (GiggleFileList *list)
{
	GiggleFileListPriv *priv;
	const gchar        *directory;
	GtkTreePath        *path;

	priv = GET_PRIV (list);
	directory = giggle_git_get_directory (priv->git);

	file_list_add_element (list, directory, NULL, NULL);

	/* expand the project folder */
	path = gtk_tree_path_new_first ();
	gtk_tree_view_expand_row (GTK_TREE_VIEW (list), path, FALSE);
	gtk_tree_path_free (path);
}

static gboolean
file_list_filter_func (GtkTreeModel   *model,
		       GtkTreeIter    *iter,
		       gpointer        user_data)
{
	GiggleFileList     *list;
	GiggleFileListPriv *priv;
	gchar              *name;

	list = GIGGLE_FILE_LIST (user_data);
	priv = GET_PRIV (list);

	gtk_tree_model_get (model, iter,
			    COL_NAME, &name,
			    -1);
	if (!name) {
		return FALSE;
	}

	/* we never want to show these files */
	if (strcmp (name, ".git") == 0 ||
	    strcmp (name, ".gitignore") == 0) {
		return FALSE;
	}

	/* FIXME: missing .gitignore matching */
	if (priv->show_all) {
		return TRUE;
	}

	return TRUE;
}

GtkWidget *
giggle_file_list_new (void)
{
	return g_object_new (GIGGLE_TYPE_FILE_LIST, NULL);
}

gboolean
giggle_file_list_get_show_all (GiggleFileList *list)
{
	GiggleFileListPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_FILE_LIST (list), FALSE);

	priv = GET_PRIV (list);
	return priv->show_all;
}

void
giggle_file_list_set_show_all (GiggleFileList *list,
			       gboolean        show_all)
{
	GiggleFileListPriv *priv;

	g_return_if_fail (GIGGLE_IS_FILE_LIST (list));

	priv = GET_PRIV (list);

	priv->show_all = (show_all == TRUE);
	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter_model));
	g_object_notify (G_OBJECT (list), "show-all");
}
