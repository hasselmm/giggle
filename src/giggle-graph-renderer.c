#include <math.h>
#include <gtk/gtk.h>
#include "giggle-graph-renderer.h"
#include "giggle-revision.h"

#define GIGGLE_CELL_RENDERER_GRAPH_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), GIGGLE_TYPE_CELL_RENDERER_GRAPH, GiggleCellRendererGraphPrivate))

/* included padding */
#define DOT_SPACE 15
#define DOT_RADIUS 3
#define GROSS_HACK_TO_PAINT_OUTSIDE_RENDERER 5

typedef struct GiggleCellRendererGraphPrivate GiggleCellRendererGraphPrivate;

struct GiggleCellRendererGraphPrivate {
	GList          *branches;
	GiggleRevision *revision;
};

enum {
	PROP_0,
	PROP_BRANCHES_INFO,
	PROP_REVISION,
};

static void giggle_cell_renderer_graph_finalize     (GObject                 *object);
static void giggle_cell_renderer_graph_get_property (GObject                 *object,
						     guint                    param_id,
						     GValue                  *value,
						     GParamSpec              *pspec);
static void giggle_cell_renderer_graph_set_property (GObject                 *object,
						     guint                    param_id,
						     const GValue            *value,
						     GParamSpec              *pspec);

static void giggle_cell_renderer_graph_get_size     (GtkCellRenderer         *cell,
						     GtkWidget               *widget,
						     GdkRectangle            *cell_area,
						     gint                    *x_offset,
						     gint                    *y_offset,
						     gint                    *width,
						     gint                    *height);
static void giggle_cell_renderer_graph_render       (GtkCellRenderer         *cell,
						     GdkWindow               *window,
						     GtkWidget               *widget,
						     GdkRectangle            *background_area,
						     GdkRectangle            *cell_area,
						     GdkRectangle            *expose_area,
						     guint                    flags);

G_DEFINE_TYPE (GiggleCellRendererGraph, giggle_cell_renderer_graph, GTK_TYPE_CELL_RENDERER)

static void
giggle_cell_renderer_graph_class_init (GiggleCellRendererGraphClass *class)
{
	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	cell_class->get_size = giggle_cell_renderer_graph_get_size;
	cell_class->render = giggle_cell_renderer_graph_render;

	object_class->finalize = giggle_cell_renderer_graph_finalize;
	object_class->set_property = giggle_cell_renderer_graph_set_property;
	object_class->get_property = giggle_cell_renderer_graph_get_property;

	g_object_class_install_property (object_class,
					 PROP_BRANCHES_INFO,
					 g_param_spec_pointer ("branches-info",
							       "branches-info",
							       "branches-info",
							       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_REVISION,
					 g_param_spec_object ("revision-info",
							      "revision-info",
							      "revision-info",
							      GIGGLE_TYPE_REVISION,
							      G_PARAM_READWRITE));
	g_type_class_add_private (object_class,
				  sizeof (GiggleCellRendererGraphPrivate));
}



static void
giggle_cell_renderer_graph_init (GiggleCellRendererGraph *instance)
{
	instance->_priv = GIGGLE_CELL_RENDERER_GRAPH_GET_PRIVATE (instance);
}

static void
giggle_cell_renderer_graph_finalize (GObject *object)
{
	/* FIXME: free stuff */

	G_OBJECT_CLASS (giggle_cell_renderer_graph_parent_class)->finalize (object);
}

static void
giggle_cell_renderer_graph_get_property (GObject *object,
					 guint param_id,
					 GValue *value,
					 GParamSpec *pspec)
{
	GiggleCellRendererGraphPrivate *priv = GIGGLE_CELL_RENDERER_GRAPH (object)->_priv;

	switch (param_id) {
	case PROP_BRANCHES_INFO:
		g_value_set_pointer (value, priv->branches);
		break;
	case PROP_REVISION:
		g_value_set_object (value, priv->revision);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	}
}

static void
giggle_cell_renderer_graph_set_property (GObject      *object,
					 guint         param_id,
					 const GValue *value,
					 GParamSpec   *pspec)
{
	GiggleCellRendererGraphPrivate *priv = GIGGLE_CELL_RENDERER_GRAPH (object)->_priv;

	switch (param_id) {
	case PROP_BRANCHES_INFO:
		priv->branches = g_value_get_pointer (value);
		break;
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
giggle_cell_renderer_graph_get_size (GtkCellRenderer         *cell,
				     GtkWidget               *widget,
				     GdkRectangle            *cell_area,
				     gint                    *x_offset,
				     gint                    *y_offset,
				     gint                    *width,
				     gint                    *height)
{
	GiggleCellRendererGraphPrivate *priv;
	GList *elem;

	priv = GIGGLE_CELL_RENDERER_GRAPH (cell)->_priv;

	if (height)
		*height = DOT_SPACE;

	if (width) {
		elem = priv->branches;
		*width = 0;

		while (elem) {
			*width += DOT_SPACE;
			elem = elem->next;
		}
	}

	if (x_offset)
		x_offset = 0;

	if (y_offset)
		y_offset = 0;
}

static void
giggle_cell_renderer_graph_render (GtkCellRenderer         *cell,
				   GdkWindow               *window,
				   GtkWidget               *widget,
				   GdkRectangle            *background_area,
				   GdkRectangle            *cell_area,
				   GdkRectangle            *expose_area,
				   guint                    flags)
{
	GiggleCellRendererGraphPrivate *priv;
	gint x, y, h;
	cairo_t *cr;
	GList *list;
	GdkColor *color;
	GiggleRevision *revision;
	gint x1, y1, x2, y2;

	priv = GIGGLE_CELL_RENDERER_GRAPH (cell)->_priv;

	g_return_if_fail (priv->revision != NULL);

	x1 = y1 = x2 = y2 = 0;
	cr = gdk_cairo_create (window);
	x = cell_area->x + cell->xpad;
	y = cell_area->y + cell->ypad;
	h = cell_area->height - cell->ypad * 2;
	list = priv->branches;
	revision = priv->revision;

	while (list) {
		color = g_hash_table_lookup (revision->branches, list->data);

		if (color) {
			/* paint something only if there's info about it */

			gdk_cairo_set_source_color (cr, color);

			if ((revision->type == GIGGLE_REVISION_BRANCH && revision->branch1 == list->data) ||
			    (revision->type == GIGGLE_REVISION_MERGE && revision->branch1 == list->data) ||
			    revision->type == GIGGLE_REVISION_COMMIT ||
			    (revision->branch1 == list->data && revision->branch2 == list->data)) {
				/* draw full line */

				/* evil hack to paint continously across rows, paint
				 * outside the cell renderer area */
				cairo_move_to (cr, x + (DOT_SPACE / 2), y - GROSS_HACK_TO_PAINT_OUTSIDE_RENDERER);
				cairo_line_to (cr, x + (DOT_SPACE / 2), y + h + GROSS_HACK_TO_PAINT_OUTSIDE_RENDERER);
				cairo_stroke  (cr);

				if (revision->type == GIGGLE_REVISION_COMMIT && revision->branch1 == list->data) {
					/* paint circle */
					cairo_arc (cr,
						   x + (DOT_SPACE / 2),
						   y + (DOT_SPACE / 2),
						   DOT_RADIUS, 0, 2 * G_PI);
					cairo_fill (cr);
					cairo_stroke (cr);
				} else if (revision->branch1 == list->data ||
					   revision->type == GIGGLE_REVISION_BRANCH || revision->type == GIGGLE_REVISION_MERGE) {
					x1 = x + (DOT_SPACE / 2);
					y1 = y + (DOT_SPACE / 2);
				}
			} else if (revision->type == GIGGLE_REVISION_BRANCH && revision->branch2 == list->data) {
				/* paint line going to the row above */
				cairo_move_to (cr, x + (DOT_SPACE / 2), y + (DOT_SPACE / 2));
				cairo_line_to (cr, x + (DOT_SPACE / 2), y - GROSS_HACK_TO_PAINT_OUTSIDE_RENDERER);
				cairo_stroke  (cr);
				
				x2 = x + (DOT_SPACE / 2);
				y2 = y + (DOT_SPACE / 2);
			} else if (revision->type == GIGGLE_REVISION_MERGE && revision->branch2 == list->data) {
				/* paint line coming from the row below */
				cairo_move_to (cr, x + (DOT_SPACE / 2), y + (DOT_SPACE / 2));
				cairo_line_to (cr, x + (DOT_SPACE / 2), y + h + GROSS_HACK_TO_PAINT_OUTSIDE_RENDERER);
				cairo_stroke  (cr);
				
				x2 = x + (DOT_SPACE / 2);
				y2 = y + (DOT_SPACE / 2);
				
			}
		}

		x += DOT_SPACE;
		list = list->next;
	}

	/* paint line between branches/merges */
	cairo_move_to (cr, x1, y1);
	cairo_line_to (cr, x2, y2);
	cairo_stroke  (cr);

	cairo_destroy (cr);
}

GtkCellRenderer*
giggle_cell_renderer_graph_new (GList *branches)
{
	return g_object_new (GIGGLE_TYPE_CELL_RENDERER_GRAPH,
			     "branches-info", branches,
			     NULL);
}
