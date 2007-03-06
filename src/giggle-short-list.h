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

#ifndef __GIGGLE_SHORT_LIST_H__
#define __GIGGLE_SHORT_LIST_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GIGGLE_TYPE_DUMMY          	  (giggle_short_list_get_type ())
#define GIGGLE_SHORT_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_DUMMY, GiggleShortList))
#define GIGGLE_SHORT_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_DUMMY, GiggleShortListClass))
#define GIGGLE_IS_DUMMY(obj)       	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_DUMMY))
#define GIGGLE_IS_DUMMY_CLASS(klass)	  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_DUMMY))
#define GIGGLE_SHORT_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_DUMMY, GiggleShortListClass))

typedef struct GiggleShortList      GiggleShortList;
typedef struct GiggleShortListClass GiggleShortListClass;

struct GiggleShortList {
	GObject parent;
};

struct GiggleShortListClass {
	GObjectClass parent_class;
};

GType		      giggle_short_list_get_type (void);
GiggleShortList *         giggle_short_list_new      (void);

G_END_DECLS

#endif /* __GIGGLE_SHORT_LIST_H__ */
