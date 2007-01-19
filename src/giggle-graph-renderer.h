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

#ifndef __GIGGLE_GRAPH_RENDERER_H__
#define __GIGGLE_GRAPH_RENDERER_H__

G_BEGIN_DECLS

#include <gtk/gtk.h>

#define GIGGLE_TYPE_GRAPH_RENDERER                 (giggle_graph_renderer_get_type ())
#define GIGGLE_GRAPH_RENDERER(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_GRAPH_RENDERER, GiggleGraphRenderer))
#define GIGGLE_GRAPH_RENDERER_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_GRAPH_RENDERER, GiggleGraphRendererClass))
#define GIGGLE_IS_GRAPH_RENDERER(obj)	   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_GRAPH_RENDERER))
#define GIGGLE_IS_GRAPH_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_GRAPH_RENDERER))
#define GIGGLE_GRAPH_RENDERER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_GRAPH_RENDERER, GiggleGraphRendererClass))

typedef struct GiggleGraphRenderer      GiggleGraphRenderer;
typedef struct GiggleGraphRendererClass GiggleGraphRendererClass;

struct GiggleGraphRenderer {
	GtkCellRenderer parent_instance;

	/*<private>*/
	gpointer _priv;
};

struct GiggleGraphRendererClass {
	GtkCellRendererClass parent_class;
};

GType		 giggle_graph_renderer_get_type (void);
GtkCellRenderer *giggle_graph_renderer_new      (void);

void             giggle_graph_renderer_validate_model (GiggleGraphRenderer *renderer,
						       GtkTreeModel        *model,
						       gint                 column);

G_END_DECLS

#endif /* __GIGGLE_GRAPH_RENDERER_H__ */
