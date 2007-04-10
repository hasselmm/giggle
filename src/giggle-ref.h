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

#ifndef __GIGGLE_REF_H__
#define __GIGGLE_REF_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GIGGLE_TYPE_REF            (giggle_ref_get_type ())
#define GIGGLE_REF(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_REF, GiggleRef))
#define GIGGLE_REF_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_REF, GiggleRefClass))
#define GIGGLE_IS_REF(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_REF))
#define GIGGLE_IS_REF_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_REF))
#define GIGGLE_REF_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_REF, GiggleRefClass))

typedef struct _GiggleRef      GiggleRef;
typedef struct _GiggleRefClass GiggleRefClass;

struct _GiggleRef {
	GObject parent_instance;
};

struct _GiggleRefClass {
	GObjectClass parent_class;
};

GType                   giggle_ref_get_type          (void);
GiggleRef             * giggle_ref_new               (const gchar *name);

G_CONST_RETURN gchar  * giggle_ref_get_name          (GiggleRef   *ref);
G_CONST_RETURN gchar  * giggle_ref_get_sha           (GiggleRef   *ref);


G_END_DECLS

#endif /* __GIGGLE_REF_H__ */
