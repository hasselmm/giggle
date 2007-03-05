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

#ifndef __GIGGLE_AUTHOR_H__
#define __GIGGLE_AUTHOR_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GIGGLE_TYPE_AUTHOR            (giggle_author_get_type ())
#define GIGGLE_AUTHOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_AUTHOR, GiggleAuthor))
#define GIGGLE_AUTHOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_AUTHOR, GiggleAuthorClass))
#define GIGGLE_IS_AUTHOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_AUTHOR))
#define GIGGLE_IS_AUTHOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_AUTHOR))
#define GIGGLE_AUTHOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_AUTHOR, GiggleAuthorClass))

typedef struct GiggleAuthor      GiggleAuthor;
typedef struct GiggleAuthorClass GiggleAuthorClass;

struct GiggleAuthor {
	GObject parent;
};

struct GiggleAuthorClass {
	GObjectClass parent_class;
};

GType		      giggle_author_get_type       (void);
GiggleAuthor *        giggle_author_new_from_string(const gchar  *string);
gchar const *         giggle_author_get_email      (GiggleAuthor *author);
gchar const *         giggle_author_get_name       (GiggleAuthor *author);
gchar const *         giggle_author_get_string     (GiggleAuthor *author);

G_END_DECLS

#endif /* __GIGGLE_AUTHOR_H__ */
