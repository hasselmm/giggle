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

#include "config.h"
#include "giggle-spaning-renderer.h"

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_SPANING_RENDERER, GiggleSpaningRendererPriv))

enum {
	PROP_NONE,
	PROP_FIRST_COLUMN,
	PROP_LAST_COLUMN
};

typedef struct {
	int first;
	int last;
} GiggleSpaningRendererPriv;

G_DEFINE_TYPE (GiggleSpaningRenderer, giggle_spaning_renderer, GTK_TYPE_CELL_RENDERER_TEXT);

static void
spaning_renderer_get_property (GObject    *object,
			       guint       param_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
	GiggleSpaningRendererPriv *priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_FIRST_COLUMN:
		g_value_set_int (value, priv->first);
		break;

	case PROP_LAST_COLUMN:
		g_value_set_int (value, priv->last);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
spaning_renderer_set_property (GObject      *object,
			       guint         param_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
	GiggleSpaningRendererPriv *priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_FIRST_COLUMN:
		priv->first = g_value_get_int (value);
		break;

	case PROP_LAST_COLUMN:
		priv->last = g_value_get_int (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
spaning_renderer_get_size (GtkCellRenderer      *cell,
			   GtkWidget            *widget,
			   GdkRectangle         *cell_area,
			   gint                 *x_offset,
			   gint                 *y_offset,
			   gint                 *width,
			   gint                 *height)
{
	*width = *height = 0;
}

static void
spaning_renderer_render (GtkCellRenderer      *cell,
			 GdkDrawable          *window,
			 GtkWidget            *widget,
			 GdkRectangle         *background_area,
			 GdkRectangle         *cell_area,
			 GdkRectangle         *expose_area,
			 GtkCellRendererState  flags)
{
	GiggleSpaningRendererPriv *priv = GET_PRIV (cell);
	GtkTreeViewColumn         *first_column, *last_column;
	GdkRectangle               first_area, last_area;
	int                        first = priv->first;
	int                        last = priv->last;
	GdkRectangle               real_background_area;
	GdkRectangle               real_cell_area;
	GList                     *columns;

	columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (widget));

	if (last < 0)
		last = g_list_length (columns) - 1;

	first_column = g_list_nth_data (columns, first);
	last_column = g_list_nth_data (columns, last);

	g_return_if_fail (GTK_IS_TREE_VIEW_COLUMN (first_column));
	g_return_if_fail (GTK_IS_TREE_VIEW_COLUMN (last_column));

	gtk_tree_view_get_cell_area (GTK_TREE_VIEW (widget), NULL, first_column, &first_area);
	gtk_tree_view_get_cell_area (GTK_TREE_VIEW (widget), NULL, last_column, &last_area);

	real_cell_area.x      = first_area.x;
	real_cell_area.y      = cell_area->y;
	real_cell_area.width  = last_area.x + last_area.width - first_area.x;
	real_cell_area.height = cell_area->height;

	gtk_tree_view_get_background_area (GTK_TREE_VIEW (widget), NULL, first_column, &first_area);
	gtk_tree_view_get_background_area (GTK_TREE_VIEW (widget), NULL, last_column, &last_area);

	real_background_area.x      = first_area.x;
	real_background_area.y      = cell_area->y;
	real_background_area.width  = last_area.x + last_area.width - first_area.x;
	real_background_area.height = cell_area->height;

	GTK_CELL_RENDERER_CLASS (giggle_spaning_renderer_parent_class)->render
		(cell, window, widget, &real_background_area, &real_cell_area,
		 &real_cell_area, flags);

	g_list_free (columns);
}

static void
giggle_spaning_renderer_class_init (GiggleSpaningRendererClass *class)
{
	GObjectClass         *object_class;
	GtkCellRendererClass *renderer_class;

	object_class   = G_OBJECT_CLASS (class);
	renderer_class = GTK_CELL_RENDERER_CLASS (class);

	object_class->set_property = spaning_renderer_set_property;
	object_class->get_property = spaning_renderer_get_property;

	renderer_class->get_size = spaning_renderer_get_size;
	renderer_class->render   = spaning_renderer_render;

	g_object_class_install_property
		(object_class, PROP_FIRST_COLUMN,
		 g_param_spec_int ("first-column", "First Column",
				   "The first column this renderer shall span",
				   0, G_MAXINT, 0,
				   G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
				   G_PARAM_STATIC_STRINGS));

	g_object_class_install_property
		(object_class, PROP_LAST_COLUMN,
		 g_param_spec_int ("last-column", "Last Column",
				   "The last column this renderer shall span",
				   -1, G_MAXINT, -1,
				   G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
				   G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (class, sizeof (GiggleSpaningRendererPriv));
}

static void
giggle_spaning_renderer_init (GiggleSpaningRenderer *renderer)
{
	GiggleSpaningRendererPriv *priv = GET_PRIV (renderer);

	priv->first = 1;
	priv->last = -1;
}

GtkCellRenderer *
giggle_spaning_renderer_new (void)
{
	return g_object_new (GIGGLE_TYPE_SPANING_RENDERER, NULL);
}
