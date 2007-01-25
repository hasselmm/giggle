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

#include <config.h>
#include <math.h>
#include <gtk/gtk.h>

#include "giggle-graph-renderer.h"
#include "giggle-revision.h"

#define GET_PRIV(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), GIGGLE_TYPE_GRAPH_RENDERER, GiggleGraphRendererPrivate))

/* included padding */
#define DOT_SPACE 15
#define DOT_RADIUS 3
#define INITIAL_PATH 1

typedef struct GiggleGraphRendererPrivate GiggleGraphRendererPrivate;

struct GiggleGraphRendererPrivate {
	gint            n_paths;
	GHashTable     *paths_info;
	GiggleRevision *revision;
};

typedef struct GiggleGraphRendererPathState GiggleGraphRendererPathState;

struct GiggleGraphRendererPathState {
	GdkColor *upper_color;
	GdkColor *lower_color;
};

enum {
	PROP_0,
	PROP_REVISION,
};

static GdkColor colors[] = {
	{ 0x0, 0xdddd, 0x0000, 0x0000 }, /* red   */
	{ 0x0, 0x0000, 0xdddd, 0x0000 }, /* green */
	{ 0x0, 0x0000, 0x0000, 0xdddd }, /* blue  */
	{ 0x0, 0xdddd, 0xdddd, 0x0000 }, /* yellow */
	{ 0x0, 0xdddd, 0x0000, 0xdddd }, /* pink?  */
	{ 0x0, 0x0000, 0xdddd, 0xdddd }, /* cyan   */
	{ 0x0, 0x6666, 0x6666, 0x6666 }, /* grey   */
};

static GQuark revision_paths_state_quark;

static void giggle_graph_renderer_finalize     (GObject         *object);
static void giggle_graph_renderer_get_property (GObject         *object,
						guint            param_id,
						GValue          *value,
						GParamSpec      *pspec);
static void giggle_graph_renderer_set_property (GObject         *object,
						guint            param_id,
						const GValue    *value,
						GParamSpec      *pspec);
static void giggle_graph_renderer_get_size     (GtkCellRenderer *cell,
						GtkWidget       *widget,
						GdkRectangle    *cell_area,
						gint            *x_offset,
						gint            *y_offset,
						gint            *width,
						gint            *height);
static void giggle_graph_renderer_render       (GtkCellRenderer *cell,
						GdkWindow       *window,
						GtkWidget       *widget,
						GdkRectangle    *background_area,
						GdkRectangle    *cell_area,
						GdkRectangle    *expose_area,
						guint            flags);

G_DEFINE_TYPE (GiggleGraphRenderer, giggle_graph_renderer, GTK_TYPE_CELL_RENDERER)

static void
giggle_graph_renderer_class_init (GiggleGraphRendererClass *class)
{
	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);
	GObjectClass         *object_class = G_OBJECT_CLASS (class);

	cell_class->get_size = giggle_graph_renderer_get_size;
	cell_class->render = giggle_graph_renderer_render;

	object_class->finalize = giggle_graph_renderer_finalize;
	object_class->set_property = giggle_graph_renderer_set_property;
	object_class->get_property = giggle_graph_renderer_get_property;

	g_object_class_install_property (
		object_class,
		PROP_REVISION,
		g_param_spec_object ("revision",
				     "revision",
				     "revision",
				     GIGGLE_TYPE_REVISION,
				     G_PARAM_READWRITE));

	g_type_class_add_private (object_class,
				  sizeof (GiggleGraphRendererPrivate));

	revision_paths_state_quark = g_quark_from_static_string ("giggle-revision-paths-state");
}

static void
giggle_graph_renderer_init (GiggleGraphRenderer *instance)
{
	instance->_priv = GET_PRIV (instance);
}

static void
giggle_graph_renderer_finalize (GObject *object)
{
	GiggleGraphRendererPrivate *priv;

	priv = GET_PRIV (object);

	if (priv->paths_info) {
		g_hash_table_destroy (priv->paths_info);
	}

	G_OBJECT_CLASS (giggle_graph_renderer_parent_class)->finalize (object);
}

static void
giggle_graph_renderer_get_property (GObject    *object,
				    guint       param_id,
				    GValue     *value,
				    GParamSpec *pspec)
{
	GiggleGraphRendererPrivate *priv = GIGGLE_GRAPH_RENDERER (object)->_priv;

	switch (param_id) {
	case PROP_REVISION:
		g_value_set_object (value, priv->revision);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	}
}

static void
giggle_graph_renderer_set_property (GObject      *object,
				    guint         param_id,
				    const GValue *value,
				    GParamSpec   *pspec)
{
	GiggleGraphRendererPrivate *priv = GIGGLE_GRAPH_RENDERER (object)->_priv;

	switch (param_id) {
	case PROP_REVISION:
		if (priv->revision) {
			g_object_unref (priv->revision);
		}
		priv->revision = GIGGLE_REVISION (g_value_dup_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	}
}

static void
giggle_graph_renderer_get_size (GtkCellRenderer *cell,
				GtkWidget       *widget,
				GdkRectangle    *cell_area,
				gint            *x_offset,
				gint            *y_offset,
				gint            *width,
				gint            *height)
{
	GiggleGraphRendererPrivate *priv;

	priv = GIGGLE_GRAPH_RENDERER (cell)->_priv;

	if (height) {
		*height = DOT_SPACE;
	}

	if (width) {
		/* the +1 is because we leave half at each side */
		*width = DOT_SPACE * (priv->n_paths + 1);
	}

	if (x_offset) {
		x_offset = 0;
	}

	if (y_offset) {
		y_offset = 0;
	}
}

static void
get_list_foreach (gpointer  key,
		  gpointer  value,
		  GList    **list)
{
	*list = g_list_prepend (*list, key);
}

static GList*
get_list (GHashTable *table)
{
	GList *list = NULL;

	g_hash_table_foreach (table, (GHFunc) get_list_foreach, &list);
	return list;
}

static void
giggle_graph_renderer_render (GtkCellRenderer *cell,
			      GdkWindow       *window,
			      GtkWidget       *widget,
			      GdkRectangle    *background_area,
			      GdkRectangle    *cell_area,
			      GdkRectangle    *expose_area,
			      guint            flags)
{
	GiggleGraphRendererPrivate   *priv;
	GiggleGraphRendererPathState *path_state;
	GiggleRevision               *revision;
	GHashTable                   *paths_state;
	cairo_t                      *cr;
	gint                          x, y, h;
	gint                          cur_pos, pos;
	GList                        *children, *paths, *path;

	priv = GIGGLE_GRAPH_RENDERER (cell)->_priv;

	g_return_if_fail (priv->revision != NULL);

	cr = gdk_cairo_create (window);
	x = cell_area->x;
	y = background_area->y;
	h = background_area->height;
	revision = priv->revision;

	paths_state = g_object_get_qdata (G_OBJECT (revision), revision_paths_state_quark);
	children = giggle_revision_get_children (revision);
	cur_pos = (gint) g_hash_table_lookup (priv->paths_info, revision);
	path_state = g_hash_table_lookup (paths_state, GINT_TO_POINTER (cur_pos));
	paths = path = get_list (paths_state);

	/* paint paths */
	while (path) {
		pos = GPOINTER_TO_INT (path->data);
		path_state = g_hash_table_lookup (paths_state, GINT_TO_POINTER (pos));

		if (path_state->lower_color) {
			gdk_cairo_set_source_color (cr, path_state->lower_color);
			cairo_move_to (cr, x + (pos * DOT_SPACE), y + (h / 2));
			cairo_line_to (cr, x + (pos * DOT_SPACE), y + h);
			cairo_stroke  (cr);
		}

		if (path_state->upper_color) {
			gdk_cairo_set_source_color (cr, path_state->upper_color);
			cairo_move_to (cr, x + (pos * DOT_SPACE), y);
			cairo_line_to (cr, x + (pos * DOT_SPACE), y + (h / 2));
			cairo_stroke  (cr);
		}

		path = path->next;
	}

	/* paint connections between paths */
	while (children) {
		pos = (gint) g_hash_table_lookup (priv->paths_info, children->data);
		path_state = g_hash_table_lookup (paths_state, GINT_TO_POINTER (pos));

		if (path_state->upper_color) {
			gdk_cairo_set_source_color (cr, path_state->upper_color);
			cairo_move_to (cr,
				       x + (cur_pos * DOT_SPACE),
				       y + (h / 2));
			cairo_line_to (cr,
				       x + (pos * DOT_SPACE),
				       y + (h / 2));
			cairo_stroke  (cr);
		}

		children = children->next;
	}

	/* paint circle */
	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_arc (cr,
		   x + (cur_pos * DOT_SPACE),
		   y + (h / 2),
		   DOT_RADIUS, 0, 2 * G_PI);
	cairo_fill (cr);
	cairo_stroke (cr);

	cairo_destroy (cr);
	g_list_free (paths);
}

GtkCellRenderer *
giggle_graph_renderer_new (void)
{
	return g_object_new (GIGGLE_TYPE_GRAPH_RENDERER, NULL);
}

static void
find_free_path (GHashTable *visible_paths,
		gint       *n_paths,
		gint       *path)
{
	gint cur_path = 1;

	/* find first path not in list */
	while (g_hash_table_lookup (visible_paths, GINT_TO_POINTER (cur_path))) {
		cur_path++;
	}

	*path = cur_path;

	/* increment number of paths */
	if (*path > *n_paths) {
		*n_paths = *path;
	}
}

static void
get_initial_status_foreach (gpointer key,
			    gpointer value,
			    gpointer user_data)
{
	GiggleGraphRendererPathState *path_state;
	GHashTable *table = (GHashTable *) user_data;

	path_state = g_new0 (GiggleGraphRendererPathState, 1);
	path_state->lower_color = value;
	path_state->upper_color = value;
	g_hash_table_insert (table, key, path_state);
}

static void
get_initial_status (GHashTable *paths,
		    GHashTable *visible_paths)
{
	g_hash_table_foreach (visible_paths, (GHFunc) get_initial_status_foreach, paths);
}

static void
giggle_graph_renderer_calculate_revision_state (GiggleGraphRenderer *renderer,
						GiggleRevision      *revision,
						GHashTable          *visible_paths,
						gint                *n_color)
{
	GiggleGraphRendererPathState *path_state;
	GiggleGraphRendererPrivate   *priv;
	GHashTable                   *paths_state;
	GList                        *children;
	gboolean                      current_path_reused = FALSE;
	gboolean                      update_color;
	gint                          n_path;

	priv = renderer->_priv;
	children = giggle_revision_get_children (revision);
	update_color = (g_list_length (children) > 1);
	paths_state = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);
	get_initial_status (paths_state, visible_paths);

	while (children) {
		path_state = g_new0 (GiggleGraphRendererPathState, 1);
		n_path = GPOINTER_TO_INT (g_hash_table_lookup (priv->paths_info, children->data));

		if (!n_path) {
			/* there wasn't a path for this revision, choose one */
			if (!current_path_reused) {
				n_path = GPOINTER_TO_INT (g_hash_table_lookup (priv->paths_info, revision));
				current_path_reused = TRUE;
			} else {
				find_free_path (visible_paths, &priv->n_paths, &n_path);
			}

			g_hash_table_insert (priv->paths_info, children->data, GINT_TO_POINTER (n_path));
			path_state->lower_color = g_hash_table_lookup (visible_paths, GINT_TO_POINTER (n_path));

			if (update_color) {
				*n_color = (*n_color + 1) % G_N_ELEMENTS (colors);
				path_state->upper_color = &colors[*n_color];
			} else {
				path_state->upper_color = path_state->lower_color;
			}
		} else {
			path_state->lower_color = g_hash_table_lookup (visible_paths, GINT_TO_POINTER (n_path));
			path_state->upper_color = path_state->lower_color;
		}

		g_hash_table_insert (visible_paths, GINT_TO_POINTER (n_path), path_state->upper_color);
		g_hash_table_insert (paths_state, GINT_TO_POINTER (n_path), path_state);

		children = children->next;
	}

	if (!current_path_reused) {
		/* current path is a dead end, remove it from the visible paths table */
		n_path = GPOINTER_TO_INT (g_hash_table_lookup (priv->paths_info, revision));
		g_hash_table_remove (visible_paths, GINT_TO_POINTER (n_path));
		path_state = g_hash_table_lookup (paths_state, GINT_TO_POINTER (n_path));
		path_state->upper_color = NULL;
	}

	g_object_set_qdata_full (G_OBJECT (revision), revision_paths_state_quark,
				 paths_state, (GDestroyNotify) g_hash_table_destroy);
}

void
giggle_graph_renderer_validate_model (GiggleGraphRenderer *renderer,
				      GtkTreeModel        *model,
				      gint                 column)
{
	GiggleGraphRendererPrivate *priv;
	GtkTreeIter                 iter;
	gint                        n_children;
	gint                        n_color = 0;
	GiggleRevision             *revision;
	GHashTable                 *visible_paths;
	gboolean                    initialized = FALSE;
	GType                       contained_type;

	g_return_if_fail (GIGGLE_IS_GRAPH_RENDERER (renderer));
	g_return_if_fail (GTK_IS_TREE_MODEL (model));

	priv = renderer->_priv;
	contained_type = gtk_tree_model_get_column_type (model, column);

	g_return_if_fail (contained_type == GIGGLE_TYPE_REVISION);

	if (priv->paths_info) {
		g_hash_table_destroy (priv->paths_info);
	}

	priv->paths_info = g_hash_table_new (g_direct_hash, g_direct_equal);
	visible_paths = g_hash_table_new (g_direct_hash, g_direct_equal);
	n_children = gtk_tree_model_iter_n_children (model, NULL);

	while (n_children) {
		/* need to calculate state backwards for proper color asignment */
		n_children--;
		gtk_tree_model_iter_nth_child (model, &iter, NULL, n_children);
		gtk_tree_model_get (model, &iter, column, &revision, -1);

		if (!initialized) {
			g_hash_table_insert (priv->paths_info, revision, GINT_TO_POINTER (INITIAL_PATH));
			g_hash_table_insert (visible_paths, GINT_TO_POINTER (INITIAL_PATH), &colors[n_color]);
			initialized = TRUE;
		}

		g_object_unref (revision);
		giggle_graph_renderer_calculate_revision_state (renderer, revision, visible_paths, &n_color);
	}

	g_hash_table_destroy (visible_paths);
}
