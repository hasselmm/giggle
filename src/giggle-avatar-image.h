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

#ifndef __GIGGLE_AVATAR_IMAGE_H__
#define __GIGGLE_AVATAR_IMAGE_H__

#include "giggle-avatar-cache.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GIGGLE_TYPE_AVATAR_IMAGE            (giggle_avatar_image_get_type ())
#define GIGGLE_AVATAR_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_AVATAR_IMAGE, GiggleAvatarImage))
#define GIGGLE_AVATAR_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_AVATAR_IMAGE, GiggleAvatarImageClass))
#define GIGGLE_IS_AVATAR_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_AVATAR_IMAGE))
#define GIGGLE_IS_AVATAR_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_AVATAR_IMAGE))
#define GIGGLE_AVATAR_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_AVATAR_IMAGE, GiggleAvatarImageClass))

typedef struct GiggleAvatarImage      GiggleAvatarImage;
typedef struct GiggleAvatarImageClass GiggleAvatarImageClass;

struct GiggleAvatarImage {
	GtkMisc parent_instance;
};

struct GiggleAvatarImageClass {
	GtkMiscClass parent_class;
};

GType                 giggle_avatar_image_get_type        (void) G_GNUC_CONST;

GtkWidget *           giggle_avatar_image_new             (void);

void                  giggle_avatar_image_set_cache       (GiggleAvatarImage *image,
							   GiggleAvatarCache *cache);

GiggleAvatarCache *   giggle_avatar_image_get_cache       (GiggleAvatarImage *image);

void                  giggle_avatar_image_set_pixbuf      (GiggleAvatarImage *image,
							   GdkPixbuf         *pixbuf);

GdkPixbuf *           giggle_avatar_image_get_pixbuf      (GiggleAvatarImage *image);

void                  giggle_avatar_image_set_image_size  (GiggleAvatarImage *image,
							   unsigned           size);

unsigned              giggle_avatar_image_get_image_size  (GiggleAvatarImage *image);

void                  giggle_avatar_image_set_image_uri   (GiggleAvatarImage *image,
							   const char        *uri);

G_CONST_RETURN char * giggle_avatar_image_get_image_uri   (GiggleAvatarImage *image);

void                  giggle_avatar_image_set_gravatar_id (GiggleAvatarImage *image,
							   const char        *id);

G_CONST_RETURN char * giggle_avatar_image_get_gravatar_id (GiggleAvatarImage *image);

G_END_DECLS

#endif /* __GIGGLE_AVATAR_IMAGE_H__ */
