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

#ifndef __GIGGLE_REVISION_INFO_H__
#define __GIGGLE_REVISION_INFO_H__

#include <gtk/gtk.h>

#include "libgiggle/giggle-revision.h"

G_BEGIN_DECLS

#define GIGGLE_TYPE_REVISION_INFO            (giggle_revision_info_get_type ())
#define GIGGLE_REVISION_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_REVISION_INFO, GiggleRevisionInfo))
#define GIGGLE_REVISION_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_REVISION_INFO, GiggleRevisionInfoClass))
#define GIGGLE_IS_REVISION_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_REVISION_INFO))
#define GIGGLE_IS_REVISION_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_REVISION_INFO))
#define GIGGLE_REVISION_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_REVISION_INFO, GiggleRevisionInfoClass))

typedef struct GiggleRevisionInfo      GiggleRevisionInfo;
typedef struct GiggleRevisionInfoClass GiggleRevisionInfoClass;

struct GiggleRevisionInfo {
	GtkVBox parent_instance;
};

struct GiggleRevisionInfoClass {
	GtkVBoxClass parent_class;
};

GType			giggle_revision_info_get_type     (void);
GtkWidget *		giggle_revision_info_new          (void);

void			giggle_revision_info_set_revision (GiggleRevisionInfo *info,
							   GiggleRevision     *revision);

GiggleRevision *	giggle_revision_info_get_revision (GiggleRevisionInfo *info);

GtkWidget *		giggle_revision_info_get_label    (GiggleRevisionInfo *info);

G_END_DECLS

#endif /* __GIGGLE_REVISION_INFO_H__ */

