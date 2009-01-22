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
#include "giggle-plugin-manager.h"
#include "giggle-plugin.h"

#include <gio/gio.h>

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_PLUGIN_MANAGER, GigglePluginManagerPriv))

enum {
	PLUGIN_ADDED,
	LAST_SIGNAL
};

typedef struct {
	GCancellable *cancellable;
	GFile        *plugin_dir;
	GList        *plugins;
} GigglePluginManagerPriv;

static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (GigglePluginManager, giggle_plugin_manager, G_TYPE_OBJECT);

static void
plugin_manager_next_files_ready (GObject      *object,
				 GAsyncResult *result,
				 gpointer      user_data)
{
	GigglePluginManager     *manager = user_data;
	GigglePluginManagerPriv *priv = GET_PRIV (manager);
	GFileEnumerator         *children = G_FILE_ENUMERATOR (object);
	GError                  *error = NULL;
	GList                   *files, *l;

	files = g_file_enumerator_next_files_finish (children,
						     result, &error);

	if (error) {
		g_warning ("%s: %s", G_STRFUNC, error->message);
		g_clear_error (&error);
	}

	for (l = files; l; l = g_list_delete_link (l, l)) {
		const char   *name = g_file_info_get_name (l->data);
		char         *filename;
		GigglePlugin *plugin;
		GFile        *file;

		if (g_str_has_suffix (name, ".xml")) {
			file = g_file_get_child (priv->plugin_dir, name);
			filename = g_file_get_path (file);

			plugin = giggle_plugin_new_from_file (filename, &error);

			g_object_unref (file);
			g_free (filename);

			if (plugin) {
				priv->plugins = g_list_prepend (priv->plugins, plugin);
				g_signal_emit (manager, signals[PLUGIN_ADDED], 0, plugin);
			} else {
				g_warning ("%s: %s", G_STRFUNC, error->message);
				g_clear_error (&error);
			}

		}

		g_object_unref (l->data);
	}

	if (files) {
		g_file_enumerator_next_files_async (children, 16,
						    G_PRIORITY_DEFAULT, priv->cancellable,
						    plugin_manager_next_files_ready, user_data);
	}
}

static void
plugin_manager_children_ready (GObject      *object,
                               GAsyncResult *result,
                               gpointer      user_data)
{
	GigglePluginManagerPriv *priv = GET_PRIV (user_data);
	GError                  *error = NULL;
	GFileEnumerator         *children;
    
	children = g_file_enumerate_children_finish (G_FILE (object), result, &error);

	if (error) {
		g_warning ("%s: %s", G_STRFUNC, error->message);
		g_clear_error (&error);
	}

	if (children) {
		g_file_enumerator_next_files_async (children, 16,
						    G_PRIORITY_DEFAULT, priv->cancellable,
						    plugin_manager_next_files_ready, user_data);

		g_object_unref (children);
	}
}

#if !GLIB_CHECK_VERSION(2,18,0)

static GFileType
g_file_query_file_type (GFile               *file,
			GFileQueryInfoFlags  flags,
			GCancellable        *cancellable)
{
	GFileType  type = G_FILE_TYPE_UNKNOWN;
	GError    *error = NULL;
	GFileInfo *info;

	info = g_file_query_info
		(file, G_FILE_ATTRIBUTE_STANDARD_TYPE,
		 flags, cancellable, &error);

	if (info) {
		type = g_file_info_get_file_type (info);
		g_object_unref (info);
	} else {
		g_warning ("%s: %s", G_STRLOC, error->message);
		g_error_free (error);
	}

	return type;
}

#endif /* GLIB 2.18 */

static void
giggle_plugin_manager_init (GigglePluginManager *manager)
{
	GigglePluginManagerPriv *priv = GET_PRIV (manager);

	priv->cancellable = g_cancellable_new ();
	priv->plugin_dir = g_file_new_for_path ("plugins");

	if (G_FILE_TYPE_DIRECTORY != g_file_query_file_type
			(priv->plugin_dir, G_FILE_QUERY_INFO_NONE, NULL)) {
		g_object_unref (priv->plugin_dir);
		priv->plugin_dir = g_file_new_for_path (PLUGINDIR);
	}

	if (!g_file_query_exists (priv->plugin_dir, priv->cancellable))
		return;

	g_file_enumerate_children_async (priv->plugin_dir,
					 G_FILE_ATTRIBUTE_STANDARD_NAME,
					 G_FILE_QUERY_INFO_NONE,
					 G_PRIORITY_DEFAULT, priv->cancellable,
					 plugin_manager_children_ready, manager);
}

static void
plugin_manager_dispose (GObject *object)
{
	GigglePluginManagerPriv *priv = GET_PRIV (object);

	if (priv->cancellable) {
		g_cancellable_cancel (priv->cancellable);
		g_object_unref (priv->cancellable);
		priv->cancellable = NULL;
	}

	if (priv->plugin_dir) {
		g_object_unref (priv->plugin_dir);
		priv->plugin_dir = NULL;
	}

	G_OBJECT_CLASS (giggle_plugin_manager_parent_class)->dispose (object);
}

static void
giggle_plugin_manager_class_init (GigglePluginManagerClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->dispose = plugin_manager_dispose;

	signals[PLUGIN_ADDED] =
		g_signal_new ("plugin-added",
			      GIGGLE_TYPE_PLUGIN_MANAGER, G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GigglePluginManagerClass, plugin_added),
			      NULL, NULL, g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1, GIGGLE_TYPE_PLUGIN);

	g_type_class_add_private (class, sizeof (GigglePluginManagerPriv));
}

GigglePluginManager *
giggle_plugin_manager_new (void)
{
	return g_object_new (GIGGLE_TYPE_PLUGIN_MANAGER, NULL);
}
