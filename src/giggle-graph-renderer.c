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
#include "libgiggle/giggle-revision.h"

#define GET_PRIV(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), GIGGLE_TYPE_GRAPH_RENDERER, GiggleGraphRendererPrivate))

/* included padding */
#define PATH_SPACE(font_size) (font_size + 3)
#define DOT_RADIUS(font_size) (font_size / 2)
#define LINE_WIDTH(font_size) ((font_size / 6) << 1) /* we want the closest even number <= size/3 */
#define NEXT_COLOR(n_color)   ((n_color % (G_N_ELEMENTS (colors) - 1)) + 1)
#define INVALID_COLOR         0

typedef struct GiggleGraphRendererPrivate GiggleGraphRendererPrivate;

struct GiggleGraphRendererPrivate {
	gint            n_paths;
	GHashTable     *paths_info;
	GiggleRevision *revision;
};

typedef struct GiggleGraphRendererPathState GiggleGraphRendererPathState;

struct GiggleGraphRendererPathState {
	gushort upper_n_color : 8;
	gushort lower_n_color : 8;
	gushort n_path;
};

enum {
	PROP_0,
	PROP_REVISION,
};

static GdkColor colors[] = {
	/* invalid color */
	{ 0x0, 0x0000, 0x0000, 0x0000 },
	/* light palette */
	{ 0x0, 0xfc00, 0xe900, 0x4f00 }, /* butter */
	{ 0x0, 0xfc00, 0xaf00, 0x3e00 }, /* orange */
	{ 0x0, 0xe900, 0xb900, 0x6e00 }, /* chocolate */
	{ 0x0, 0x8a00, 0xe200, 0x3400 }, /* chameleon */
	{ 0x0, 0x7200, 0x9f00, 0xcf00 }, /* sky blue */
	{ 0x0, 0xad00, 0x7f00, 0xa800 }, /* plum */
	{ 0x0, 0xef00, 0x2900, 0x2900 }, /* scarlet red */
#if 0
	{ 0x0, 0xee00, 0xee00, 0xec00 }, /* aluminium */
#endif
	{ 0x0, 0x8800, 0x8a00, 0x8500 }, /* no name grey */
	/* medium palette */
	{ 0x0, 0xed00, 0xd400, 0x0000 }, /* butter */
	{ 0x0, 0xf500, 0x7900, 0x0000 }, /* orange */
	{ 0x0, 0xc100, 0x7d00, 0x1100 }, /* chocolate */
	{ 0x0, 0x7300, 0xd200, 0x1600 }, /* chameleon */
	{ 0x0, 0x3400, 0x6500, 0xa400 }, /* sky blue */
	{ 0x0, 0x7500, 0x5000, 0x7b00 }, /* plum */
	{ 0x0, 0xcc00, 0x0000, 0x0000 }, /* scarlet red */
#if 0
	{ 0x0, 0xd300, 0xd700, 0xcf00 }, /* aluminium */
#endif
	{ 0x0, 0x5500, 0x5700, 0x5300 }, /* no name grey */
	/* dark palette */
	{ 0x0, 0xc400, 0xa000, 0x0000 }, /* butter */
	{ 0x0, 0xce00, 0x5c00, 0x0000 }, /* orange */
	{ 0x0, 0x8f00, 0x5900, 0x0200 }, /* chocolate */
	{ 0x0, 0x4e00, 0x9a00, 0x0600 }, /* chameleon */
	{ 0x0, 0x2000, 0x4a00, 0x8700 }, /* sky blue */
	{ 0x0, 0x5c00, 0x3500, 0x6600 }, /* plum */
	{ 0x0, 0xa400, 0x0000, 0x0000 }, /* scarlet red */
#if 0
	{ 0x0, 0xba00, 0xbd00, 0xb600 }, /* aluminium */
#endif
	{ 0x0, 0x2e00, 0x3400, 0x3600 }, /* no name grey */
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
	gint size;

	priv = GIGGLE_GRAPH_RENDERER (cell)->_priv;
	size = PANGO_PIXELS (pango_font_description_get_size (widget->style->font_desc));

	if (height) {
		*height = PATH_SPACE (size);
	}

	if (width) {
		/* the +1 is because we leave half at each side */
		*width = PATH_SPACE (size) * (priv->n_paths + 1);
	}

	if (x_offset) {
		x_offset = 0;
	}

	if (y_offset) {
		y_offset = 0;
	}
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
	GArray                       *paths_state;
	GHashTable                   *table;
	cairo_t                      *cr;
	gint                          x, y, h;
	gint                          cur_pos, pos;
	GList                        *children;
	gint                          size, i;

	priv = GIGGLE_GRAPH_RENDERER (cell)->_priv;

	if (!priv->revision) {
		return;
	}

	cr = gdk_cairo_create (window);
	x = cell_area->x;
	y = background_area->y;
	h = background_area->height;
	revision = priv->revision;
	size = PANGO_PIXELS (pango_font_description_get_size (widget->style->font_desc));

	table = g_hash_table_new (g_direct_hash, g_direct_equal);
	paths_state = g_object_get_qdata (G_OBJECT (revision), revision_paths_state_quark);
	children = giggle_revision_get_children (revision);
	cur_pos = GPOINTER_TO_INT (g_hash_table_lookup (priv->paths_info, revision));
	cairo_set_line_width (cr, LINE_WIDTH (size));
	cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

	/* paint paths */
	for (i = 0; i < paths_state->len; i++) {
		path_state = & g_array_index (paths_state, GiggleGraphRendererPathState, i);
		g_hash_table_insert (table, GINT_TO_POINTER ((gint) path_state->n_path), path_state);
		pos = path_state->n_path;

		if (path_state->lower_n_color != INVALID_COLOR &&
		    (pos != cur_pos || giggle_revision_get_parents (revision))) {
			gdk_cairo_set_source_color (cr, &colors[path_state->lower_n_color]);
			cairo_move_to (cr, x + (pos * PATH_SPACE (size)), y + (h / 2));
			cairo_line_to (cr, x + (pos * PATH_SPACE (size)), y + h);
			cairo_stroke  (cr);
		}

		if (path_state->upper_n_color != INVALID_COLOR) {
			gdk_cairo_set_source_color (cr, &colors[path_state->upper_n_color]);
			cairo_move_to (cr, x + (pos * PATH_SPACE (size)), y);
			cairo_line_to (cr, x + (pos * PATH_SPACE (size)), y + (h / 2));
			cairo_stroke  (cr);
		}
	}

	/* paint connections between paths */
	while (children) {
		pos = GPOINTER_TO_INT (g_hash_table_lookup (priv->paths_info, children->data));
		path_state = g_hash_table_lookup (table, GINT_TO_POINTER (pos));

		if (path_state->upper_n_color != INVALID_COLOR) {
			gdk_cairo_set_source_color (cr, &colors[path_state->upper_n_color]);
			cairo_move_to (cr,
				       x + (cur_pos * PATH_SPACE (size)),
				       y + (h / 2));
			cairo_line_to (cr,
				       x + (pos * PATH_SPACE (size)),
				       y + (h / 2));

			/* redraw the upper part of the path before
			 * stroking to get a rounded connection
			 */
			cairo_line_to (cr, x + (pos * PATH_SPACE (size)), y);

			cairo_stroke  (cr);
		}

		children = children->next;
	}

	/* paint circle */
	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_arc (cr,
		   x + (cur_pos * PATH_SPACE (size)),
		   y + (h / 2),
		   DOT_RADIUS (size), 0, 2 * G_PI);
	cairo_stroke (cr);

	/* paint internal circle */
	path_state = g_hash_table_lookup (table, GINT_TO_POINTER (cur_pos));
	gdk_cairo_set_source_color (cr, &colors[path_state->lower_n_color]);
	cairo_arc (cr,
		   x + (cur_pos * PATH_SPACE (size)),
		   y + (h / 2),
		   DOT_RADIUS (size) - 1, 0, 2 * G_PI);
	cairo_fill (cr);
	cairo_stroke (cr);

	cairo_destroy (cr);
	g_hash_table_destroy (table);
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
	GiggleGraphRendererPathState  path_state;
	GArray                       *array;
	gint                          n_color, n_path;

	array = (GArray *) user_data;
	n_color = GPOINTER_TO_INT (value);
	n_path = GPOINTER_TO_INT (key);

	path_state.n_path = n_path;
	path_state.lower_n_color = n_color;
	path_state.upper_n_color = n_color;

	g_array_append_val (array, path_state);
}

static GArray *
get_initial_status (GHashTable *visible_paths)
{
	GArray *array;
	guint      size;

	size = g_hash_table_size (visible_paths);
	array = g_array_sized_new (FALSE, TRUE, sizeof (GiggleGraphRendererPathState), size);

	g_hash_table_foreach (visible_paths, (GHFunc) get_initial_status_foreach, array);

	return array;
}

static void
free_paths_state (GArray *array)
{
	g_array_free (array, FALSE);
}

static void
giggle_graph_renderer_calculate_revision_state (GiggleGraphRenderer *renderer,
						GiggleRevision      *revision,
						GHashTable          *visible_paths,
						gint                *n_color)
{
	GiggleGraphRendererPathState  path_state;
	GiggleGraphRendererPrivate   *priv;
	GiggleRevision               *rev;
	GArray                       *paths_state;
	GList                        *children;
	gboolean                      current_path_reused = FALSE;
	gboolean                      update_color;
	gint                          n_path, i;

	priv = renderer->_priv;
	children = giggle_revision_get_children (revision);
	update_color = (g_list_length (children) > 1);
	paths_state = get_initial_status (visible_paths);

	while (children) {
		rev = GIGGLE_REVISION (children->data);
		n_path = GPOINTER_TO_INT (g_hash_table_lookup (priv->paths_info, rev));

		if (!n_path) {
			/* there wasn't a path for this revision, choose one */
			if (!current_path_reused) {
				n_path = GPOINTER_TO_INT (g_hash_table_lookup (priv->paths_info, revision));
				current_path_reused = TRUE;
			} else {
				find_free_path (visible_paths, &priv->n_paths, &n_path);
			}

			g_hash_table_insert (priv->paths_info, rev, GINT_TO_POINTER (n_path));
			path_state.lower_n_color =
				GPOINTER_TO_INT (g_hash_table_lookup (visible_paths, GINT_TO_POINTER (n_path)));

			if (update_color) {
				path_state.upper_n_color = *n_color = NEXT_COLOR (*n_color);
			} else {
				path_state.upper_n_color = path_state.lower_n_color;
			}
		} else {
			path_state.lower_n_color =
				GPOINTER_TO_INT (g_hash_table_lookup (visible_paths, GINT_TO_POINTER (n_path)));

			path_state.upper_n_color = path_state.lower_n_color;
		}

		path_state.n_path = n_path;
		g_hash_table_insert (visible_paths, GINT_TO_POINTER (n_path), GINT_TO_POINTER ((gint) path_state.upper_n_color));
		g_array_append_val (paths_state, path_state);

		children = children->next;
	}

	if (!current_path_reused) {
		/* current path is a dead end, remove it from the visible paths table */
		n_path = GPOINTER_TO_INT (g_hash_table_lookup (priv->paths_info, revision));
		g_hash_table_remove (visible_paths, GINT_TO_POINTER (n_path));

		for (i = 0; i < paths_state->len; i++) {
			path_state = g_array_index (paths_state, GiggleGraphRendererPathState, i);

			if (path_state.n_path == n_path) {
				path_state.upper_n_color = INVALID_COLOR;
				((GiggleGraphRendererPathState *) paths_state->data)[i] = path_state;
				break;
			}
		}
	}

	g_object_set_qdata_full (G_OBJECT (revision), revision_paths_state_quark,
				 paths_state, (GDestroyNotify) free_paths_state);
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
	GType                       contained_type;
	gint                        n_path;

	g_return_if_fail (GIGGLE_IS_GRAPH_RENDERER (renderer));
	g_return_if_fail (GTK_IS_TREE_MODEL (model));

	priv = renderer->_priv;
	contained_type = gtk_tree_model_get_column_type (model, column);

	g_return_if_fail (contained_type == GIGGLE_TYPE_REVISION);

	if (priv->paths_info) {
		g_hash_table_destroy (priv->paths_info);
	}

	priv->n_paths = 0;
	priv->paths_info = g_hash_table_new (g_direct_hash, g_direct_equal);
	visible_paths = g_hash_table_new (g_direct_hash, g_direct_equal);
	n_children = gtk_tree_model_iter_n_children (model, NULL);

	while (n_children) {
		/* need to calculate state backwards for proper color asignment */
		n_children--;
		gtk_tree_model_iter_nth_child (model, &iter, NULL, n_children);
		gtk_tree_model_get (model, &iter, column, &revision, -1);

		if (revision) {
			if (!giggle_revision_get_parents (revision)) {
				n_color = NEXT_COLOR (n_color);
				find_free_path (visible_paths, &priv->n_paths, &n_path);
				g_hash_table_insert (priv->paths_info, revision, GINT_TO_POINTER (n_path));
				g_hash_table_insert (visible_paths, GINT_TO_POINTER (n_path), GINT_TO_POINTER (n_color));
			}

			giggle_graph_renderer_calculate_revision_state (renderer, revision, visible_paths, &n_color);
			g_object_unref (revision);
		}
	}

	g_hash_table_destroy (visible_paths);
}
