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

#ifndef __GIGGLE_REVISION_INFO_ACTION_H__
#define __GIGGLE_REVISION_INFO_ACTION_H__

#include <gtk/gtk.h>

#include "libgiggle/giggle-revision.h"

G_BEGIN_DECLS

#define GIGGLE_TYPE_REVISION_INFO_ACTION            (giggle_revision_info_action_get_type ())
#define GIGGLE_REVISION_INFO_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_REVISION_INFO_ACTION, GiggleRevisionInfoAction))
#define GIGGLE_REVISION_INFO_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_REVISION_INFO_ACTION, GiggleRevisionInfoActionClass))
#define GIGGLE_IS_REVISION_INFO_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_REVISION_INFO_ACTION))
#define GIGGLE_IS_REVISION_INFO_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_REVISION_INFO_ACTION))
#define GIGGLE_REVISION_INFO_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_REVISION_INFO_ACTION, GiggleRevisionInfoActionClass))

typedef struct GiggleRevisionInfoAction      GiggleRevisionInfoAction;
typedef struct GiggleRevisionInfoActionClass GiggleRevisionInfoActionClass;

struct GiggleRevisionInfoAction {
	GtkAction parent_instance;
};

struct GiggleRevisionInfoActionClass {
	GtkActionClass parent_class;
};

GType			giggle_revision_info_action_get_type     (void);
GtkAction *		giggle_revision_info_action_new          (const char *name);

void			giggle_revision_info_action_set_markup   (GiggleRevisionInfoAction *action,
								  const char               *markup);

void			giggle_revision_info_action_set_revision (GiggleRevisionInfoAction *action,
								  GiggleRevision           *revision);

GiggleRevision *	giggle_revision_info_action_get_revision (GiggleRevisionInfoAction *action);

void			giggle_revision_info_action_set_expand   (GiggleRevisionInfoAction *action,
								  gboolean                  expand);

gboolean		giggle_revision_info_action_get_expand   (GiggleRevisionInfoAction *action);

G_END_DECLS

#endif /* __GIGGLE_REVISION_INFO_ACTION_H__ */
