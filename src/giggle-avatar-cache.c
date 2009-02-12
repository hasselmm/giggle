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
#include "giggle-avatar-cache.h"

#include <gio/gio.h>
#include <string.h>

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_AVATAR_CACHE, GiggleAvatarCachePriv))

typedef struct {
	char               *uri;
	GiggleAvatarCache  *cache;
	GCancellable       *cancellable;
	char                buffer[8192];
	GdkPixbufLoader    *pixbuf_loader;
	GdkPixbuf          *pixbuf;
	GSimpleAsyncResult *result;
} GiggleAvatarCacheLoader;

typedef struct {
	GChecksum  *checksum;
	GHashTable *pixbuf_table;
	GList      *active_loaders;
} GiggleAvatarCachePriv;

G_DEFINE_TYPE (GiggleAvatarCache, giggle_avatar_cache, G_TYPE_OBJECT)

static void
avatar_cache_loader_finish (GiggleAvatarCacheLoader *loader,
			    GError                  *error)
{
	GiggleAvatarCachePriv *priv;

	if (loader->cache) {
		priv = GET_PRIV (loader->cache);

		g_object_remove_weak_pointer
			(G_OBJECT (loader->cache),
			 (gpointer) &loader->cache);
	}

	if (loader->cancellable) {
		g_cancellable_cancel (loader->cancellable);
		g_object_unref (loader->cancellable);
	}

	if (loader->pixbuf_loader) {
		gdk_pixbuf_loader_close (loader->pixbuf_loader, NULL);
		g_object_unref (loader->pixbuf_loader);
	}

	if (loader->pixbuf) {
		g_simple_async_result_set_op_res_gpointer
			(loader->result, g_object_ref (loader->pixbuf),
			 g_object_unref);
	} else {
		g_simple_async_result_set_from_error (loader->result, error);
		g_error_free (error);
	}

	g_simple_async_result_complete_in_idle (loader->result);
	g_object_unref (loader->result);

	g_slice_free (GiggleAvatarCacheLoader, loader);
}

static void
giggle_avatar_cache_init (GiggleAvatarCache *cache)
{
}

static void
avatar_cache_insert (GiggleAvatarCache *cache,
		     const char        *uri,
		     GdkPixbuf         *pixbuf)
{
	GiggleAvatarCachePriv *priv = GET_PRIV (cache);

	if (!priv->pixbuf_table) {
		priv->pixbuf_table = g_hash_table_new_full
			(g_str_hash, g_str_equal, g_free,
			 g_object_unref);
	}

	g_hash_table_insert
		(priv->pixbuf_table, g_strdup (uri),
		 g_object_ref (pixbuf));
}

static void
avatar_cache_close_cb (GObject      *object,
                       GAsyncResult *result,
                       gpointer      user_data)
{
	GiggleAvatarCacheLoader *loader = user_data;
	GInputStream            *stream = G_INPUT_STREAM (object);
	GError                  *error = NULL;

	if (g_input_stream_close_finish (stream, result, &error) &&
	    gdk_pixbuf_loader_close (loader->pixbuf_loader, &error)) {
		loader->pixbuf = gdk_pixbuf_loader_get_pixbuf (loader->pixbuf_loader);
		avatar_cache_insert (loader->cache, loader->uri, loader->pixbuf);
		g_object_unref (loader->pixbuf_loader);
		loader->pixbuf_loader = NULL;
	}

	if (error && !g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
		g_warning ("%s: %s", G_STRFUNC, error->message);

	avatar_cache_loader_finish (loader, error);
}

static void
avatar_cache_read_cb (GObject      *object,
                      GAsyncResult *result,
                      gpointer      user_data)
{
	GiggleAvatarCacheLoader *loader = user_data;
	GInputStream            *stream = G_INPUT_STREAM (object);
	GError                  *error = NULL;
	gssize                   len;

	len = g_input_stream_read_finish (stream, result, &error);

	if (len > 0) {
	       if (gdk_pixbuf_loader_write (loader->pixbuf_loader, (gpointer)
					    loader->buffer, len, &error)) {
			g_input_stream_read_async
				(stream, loader->buffer, sizeof loader->buffer,
				 G_PRIORITY_DEFAULT, loader->cancellable,
				 avatar_cache_read_cb, loader);
		} else {
			len = -2;
		}
	}

	if (0 >= len) {
		g_input_stream_close_async
			(stream, G_PRIORITY_DEFAULT, loader->cancellable,
			 avatar_cache_close_cb, loader);
	}

	if (error) {
		if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			g_warning ("%s: %s", G_STRFUNC, error->message);

		g_error_free (error);
	}
}

static void
avatar_cache_open_cb (GObject      *object,
                      GAsyncResult *result,
                      gpointer      user_data)
{
	GiggleAvatarCacheLoader *loader = user_data;
	GError                  *error = NULL;
	GFileInputStream        *stream;

	stream = g_file_read_finish (G_FILE (object), result, &error);

	if (stream) {
		g_input_stream_read_async
			(G_INPUT_STREAM (stream),
			 loader->buffer, sizeof loader->buffer,
			 G_PRIORITY_DEFAULT, loader->cancellable,
			 avatar_cache_read_cb, loader);
	} else {
		if (error && !g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			g_warning ("%s: %s", G_STRFUNC, error->message);

		avatar_cache_loader_finish (loader, error);
	}
}

static void
avatar_cache_finalize (GObject *object)
{
	GiggleAvatarCachePriv   *priv = GET_PRIV (object);
	GiggleAvatarCacheLoader *loader;
	GList                   *l;

	if (priv->pixbuf_table)
		g_hash_table_unref (priv->pixbuf_table);
	if (priv->checksum)
		g_checksum_free (priv->checksum);

	for (l = priv->active_loaders; l; l = g_list_delete_link (l, l)) {
		loader = priv->active_loaders->data;
		g_cancellable_cancel (loader->cancellable);
	}

	G_OBJECT_CLASS (giggle_avatar_cache_parent_class)->finalize (object);
}

static void
giggle_avatar_cache_class_init (GiggleAvatarCacheClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize     = avatar_cache_finalize;

	g_type_class_add_private (class, sizeof (GiggleAvatarCachePriv));
}

GiggleAvatarCache *
giggle_avatar_cache_new (void)
{
	return g_object_new (GIGGLE_TYPE_AVATAR_CACHE, NULL);
}

void
giggle_avatar_cache_load_uri_async (GiggleAvatarCache   *cache,
				    const char          *uri,
				    int                  io_priority,
				    GCancellable        *cancellable,
				    GAsyncReadyCallback  callback,
				    gpointer             user_data)
{
	GiggleAvatarCachePriv   *priv;
	GdkPixbuf               *pixbuf = NULL;
	GSimpleAsyncResult      *result;
	GiggleAvatarCacheLoader *loader;
	GFile                   *file;

	g_return_if_fail (GIGGLE_IS_AVATAR_CACHE (cache));
	g_return_if_fail (NULL != callback);
	g_return_if_fail (NULL != uri);

	priv = GET_PRIV (cache);

	result = g_simple_async_result_new
		(G_OBJECT (cache), callback, user_data,
		 giggle_avatar_cache_load_uri_async);

	if (priv->pixbuf_table)
		pixbuf = g_hash_table_lookup (priv->pixbuf_table, uri);

	if (pixbuf) {
		g_simple_async_result_set_op_res_gpointer
			(result, g_object_ref (pixbuf), g_object_unref);
		g_simple_async_result_complete_in_idle (result);
	} else {
		if (cancellable) {
			g_object_ref (cancellable);
		} else {
			cancellable = g_cancellable_new ();
		}

		loader = g_slice_new0 (GiggleAvatarCacheLoader);
		loader->pixbuf_loader = gdk_pixbuf_loader_new ();
		loader->result = g_object_ref (result);
		loader->cancellable = cancellable;
		loader->uri = g_strdup (uri);
		loader->cache = cache;

		g_object_add_weak_pointer
			(G_OBJECT (loader->cache),
			 (gpointer) &loader->cache);

		file = g_file_new_for_uri (uri);

		g_file_read_async
			(file, G_PRIORITY_DEFAULT, loader->cancellable,
			 avatar_cache_open_cb, loader);

		g_object_unref (file);
	}

	g_object_unref (result);
}

GdkPixbuf *
giggle_avatar_cache_load_finish (GiggleAvatarCache  *cache,
				 GAsyncResult       *result,
				 GError            **error)
{
	GSimpleAsyncResult *simple;
	gpointer            source_tag;

	g_return_val_if_fail (GIGGLE_IS_AVATAR_CACHE (cache), NULL);
	g_return_val_if_fail (G_IS_SIMPLE_ASYNC_RESULT (result), NULL);

	simple = G_SIMPLE_ASYNC_RESULT (result);
	source_tag = g_simple_async_result_get_source_tag (simple);

	g_return_val_if_fail (source_tag == giggle_avatar_cache_load_uri_async, NULL);

	if (g_simple_async_result_propagate_error (simple, error))
		return NULL;

	return g_simple_async_result_get_op_res_gpointer (simple);
}

char *
giggle_avatar_cache_get_gravatar_uri (GiggleAvatarCache   *cache,
				      const char          *gravatar_id,
				      unsigned             avatar_size)
{
	GiggleAvatarCachePriv *priv;

	g_return_val_if_fail (GIGGLE_IS_AVATAR_CACHE (cache), NULL);
	g_return_val_if_fail (NULL != gravatar_id, NULL);

	priv = GET_PRIV (cache);

	if (priv->checksum) {
		g_checksum_reset (priv->checksum);
	} else {
		priv->checksum = g_checksum_new (G_CHECKSUM_MD5);
	}

	g_checksum_update
		(priv->checksum, (gpointer) gravatar_id,
		 strlen (gravatar_id));

	return g_strdup_printf
		("http://www.gravatar.com/avatar/%s?s=%d",
		 g_checksum_get_string (priv->checksum),
		 avatar_size);
}


