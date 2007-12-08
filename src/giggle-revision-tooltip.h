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

#ifndef __GIGGLE_REVISION_TOOLTIP_H__
#define __GIGGLE_REVISION_TOOLTIP_H__

#include <glib-object.h>
#include <gtk/gtk.h>
#include "libgiggle/giggle-revision.h"

G_BEGIN_DECLS

#define GIGGLE_TYPE_REVISION_TOOLTIP            (giggle_revision_tooltip_get_type ())
#define GIGGLE_REVISION_TOOLTIP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_REVISION_TOOLTIP, GiggleRevisionTooltip))
#define GIGGLE_REVISION_TOOLTIP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_REVISION_TOOLTIP, GiggleRevisionTooltipClass))
#define GIGGLE_IS_REVISION_TOOLTIP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_REVISION_TOOLTIP))
#define GIGGLE_IS_REVISION_TOOLTIP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_REVISION_TOOLTIP))
#define GIGGLE_REVISION_TOOLTIP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_REVISION_TOOLTIP, GiggleRevisionTooltipClass))

typedef struct _GiggleRevisionTooltip      GiggleRevisionTooltip;
typedef struct _GiggleRevisionTooltipClass GiggleRevisionTooltipClass;

struct _GiggleRevisionTooltip {
	GtkWindow parent_instance;
};

struct _GiggleRevisionTooltipClass {
	GtkWindowClass parent_class;
};

GType              giggle_revision_tooltip_get_type          (void);
GtkWidget *        giggle_revision_tooltip_new               (void);

void               giggle_revision_tooltip_set_revision      (GiggleRevisionTooltip *tooltip,
							      GiggleRevision        *revision);
GiggleRevision *   giggle_revision_tooltip_get_revision      (GiggleRevisionTooltip *tooltip);


G_END_DECLS

#endif /* __GIGGLE_REVISION_TOOLTIP_H__ */
