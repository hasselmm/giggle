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

#ifndef __GIGGLE_REVISION_VIEW_H__
#define __GIGGLE_REVISION_VIEW_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#include "giggle-revision.h"

G_BEGIN_DECLS

#define GIGGLE_TYPE_REVISION_VIEW            (giggle_revision_view_get_type ())
#define GIGGLE_REVISION_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_REVISION_VIEW, GiggleRevisionView))
#define GIGGLE_REVISION_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_REVISION_VIEW, GiggleRevisionViewClass))
#define GIGGLE_IS_REVISION_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_REVISION_VIEW))
#define GIGGLE_IS_REVISION_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_REVISION_VIEW))
#define GIGGLE_REVISION_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_REVISION_VIEW, GiggleRevisionViewClass))

typedef struct GiggleRevisionView      GiggleRevisionView;
typedef struct GiggleRevisionViewClass GiggleRevisionViewClass;

struct GiggleRevisionView {
	GtkTable parent_instance;
};

struct GiggleRevisionViewClass {
	GtkTableClass parent_class;
};

GType              giggle_revision_view_get_type          (void);
GtkWidget *        giggle_revision_view_new               (void);

void               giggle_revision_view_set_revision      (GiggleRevisionView *view,
							   GiggleRevision     *revision);
GiggleRevision *   giggle_revision_view_get_revision      (GiggleRevisionView *view);
gboolean           giggle_revision_view_get_compact_mode  (GiggleRevisionView *view);
void               giggle_revision_view_set_compact_mode (GiggleRevisionView  *view,
							  gboolean             compact_mode);

G_END_DECLS

#endif /* __GIGGLE_REVISION_VIEW_H__ */
