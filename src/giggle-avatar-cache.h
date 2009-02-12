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

#ifndef __GIGGLE_AVATAR_CACHE_H__
#define __GIGGLE_AVATAR_CACHE_H__

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define GIGGLE_TYPE_AVATAR_CACHE            (giggle_avatar_cache_get_type ())
#define GIGGLE_AVATAR_CACHE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_AVATAR_CACHE, GiggleAvatarCache))
#define GIGGLE_AVATAR_CACHE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_AVATAR_CACHE, GiggleAvatarCacheClass))
#define GIGGLE_IS_AVATAR_CACHE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_AVATAR_CACHE))
#define GIGGLE_IS_AVATAR_CACHE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_AVATAR_CACHE))
#define GIGGLE_AVATAR_CACHE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_AVATAR_CACHE, GiggleAvatarCacheClass))

typedef struct GiggleAvatarCache      GiggleAvatarCache;
typedef struct GiggleAvatarCacheClass GiggleAvatarCacheClass;

struct GiggleAvatarCache {
	GObject parent_instance;
};

struct GiggleAvatarCacheClass {
	GObjectClass parent_class;
};

GType               giggle_avatar_cache_get_type         (void) G_GNUC_CONST;

GiggleAvatarCache * giggle_avatar_cache_new              (void);

void                giggle_avatar_cache_load_uri_async   (GiggleAvatarCache   *cache,
							  const char          *uri,
							  int                  io_priority,
							  GCancellable        *cancellable,
							  GAsyncReadyCallback  callback,
							  gpointer             user_data);

GdkPixbuf *         giggle_avatar_cache_load_finish      (GiggleAvatarCache   *cache,
							  GAsyncResult        *result,
							  GError             **error);

char *              giggle_avatar_cache_get_gravatar_uri (GiggleAvatarCache   *cache,
							  const char          *gravatar_id,
							  unsigned             avatar_size);

G_END_DECLS

#endif /* __GIGGLE_AVATAR_CACHE_H__ */

