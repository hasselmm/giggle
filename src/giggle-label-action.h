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

#ifndef __GIGGLE_LABEL_ACTION_H__
#define __GIGGLE_LABEL_ACTION_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GIGGLE_TYPE_LABEL_ACTION            (giggle_label_action_get_type ())
#define GIGGLE_LABEL_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_LABEL_ACTION, GiggleLabelAction))
#define GIGGLE_LABEL_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_LABEL_ACTION, GiggleLabelActionClass))
#define GIGGLE_IS_LABEL_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_LABEL_ACTION))
#define GIGGLE_IS_LABEL_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_LABEL_ACTION))
#define GIGGLE_LABEL_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_LABEL_ACTION, GiggleLabelActionClass))

typedef struct GiggleLabelAction      GiggleLabelAction;
typedef struct GiggleLabelActionClass GiggleLabelActionClass;

struct GiggleLabelAction {
	GtkAction parent_instance;
};

struct GiggleLabelActionClass {
	GtkActionClass parent_class;
};

GType              giggle_label_action_get_type          (void);
GtkAction *        giggle_label_action_new               (const char *name);

void		   giggle_label_action_set_text          (GiggleLabelAction *action,
							  const char        *text);
void		   giggle_label_action_set_markup        (GiggleLabelAction *action,
							  const char        *markup);

void		   giggle_label_action_set_ellipsize     (GiggleLabelAction *action,
							  PangoEllipsizeMode mode);
PangoEllipsizeMode giggle_label_action_get_ellipsize     (GiggleLabelAction *action);

void		   giggle_label_action_set_use_markup    (GiggleLabelAction *action,
							  gboolean           use_markup);
gboolean	   giggle_label_action_get_use_markup    (GiggleLabelAction *action);

void		   giggle_label_action_set_selectable    (GiggleLabelAction *action,
							  gboolean           selectable);
gboolean	   giggle_label_action_get_selectable    (GiggleLabelAction *action);

G_END_DECLS

#endif /* __GIGGLE_LABEL_ACTION_H__ */
