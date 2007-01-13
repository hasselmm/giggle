#include <math.h>
#include <gtk/gtk.h>
#include "giggle-graph-renderer.h"

#define GIG_CELL_RENDERER_GRAPH_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), GIG_TYPE_CELL_RENDERER_GRAPH, GigCellRendererGraphPrivate))

/* included padding */
#define DOT_SPACE 15
#define DOT_RADIUS 3
#define M_PI 3.14159265358979323846

GdkColor colors[] = {
	{ 0x0, 0x0000, 0x0000, 0x0000 }, /* black */
	{ 0x0, 0xffff, 0x0000, 0x0000 }, /* red   */
	{ 0x0, 0x0000, 0xffff, 0x0000 }, /* green */
	{ 0x0, 0x0000, 0x0000, 0xffff }, /* blue  */
};

typedef struct GigCellRendererGraphPrivate GigCellRendererGraphPrivate;

struct GigCellRendererGraphPrivate {
	GList *branches;
	GigRevisionInfo *revision;
};

enum {
	PROP_0,
	PROP_BRANCHES_INFO,
	PROP_REVISION_INFO,
};

static void gig_cell_renderer_graph_finalize     (GObject                 *object);
static void gig_cell_renderer_graph_get_property (GObject                 *object,
						  guint                    param_id,
						  GValue                  *value,
						  GParamSpec              *pspec);
static void gig_cell_renderer_graph_set_property (GObject                 *object,
						  guint                    param_id,
						  const GValue            *value,
						  GParamSpec              *pspec);

static void gig_cell_renderer_graph_get_size     (GtkCellRenderer         *cell,
						  GtkWidget               *widget,
						  GdkRectangle            *cell_area,
						  gint                    *x_offset,
						  gint                    *y_offset,
						  gint                    *width,
						  gint                    *height);
static void gig_cell_renderer_graph_render       (GtkCellRenderer         *cell,
						  GdkWindow               *window,
						  GtkWidget               *widget,
						  GdkRectangle            *background_area,
						  GdkRectangle            *cell_area,
						  GdkRectangle            *expose_area,
						  guint                    flags);

G_DEFINE_TYPE (GigCellRendererGraph, gig_cell_renderer_graph, GTK_TYPE_CELL_RENDERER)

static void
gig_cell_renderer_graph_class_init (GigCellRendererGraphClass *class)
{
	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	cell_class->get_size = gig_cell_renderer_graph_get_size;
	cell_class->render = gig_cell_renderer_graph_render;

	object_class->finalize = gig_cell_renderer_graph_finalize;
	object_class->set_property = gig_cell_renderer_graph_set_property;
	object_class->get_property = gig_cell_renderer_graph_get_property;

	g_object_class_install_property (object_class,
					 PROP_BRANCHES_INFO,
					 g_param_spec_pointer ("branches-info",
							       "branches-info",
							       "branches-info",
							       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_REVISION_INFO,
					 g_param_spec_pointer ("revision-info",
							       "revision-info",
							       "revision-info",
							       G_PARAM_READWRITE));
	g_type_class_add_private (object_class,
				  sizeof (GigCellRendererGraphPrivate));
}



static void
gig_cell_renderer_graph_init (GigCellRendererGraph *instance)
{
	instance->_priv = GIG_CELL_RENDERER_GRAPH_GET_PRIVATE (instance);
}

static void
gig_cell_renderer_graph_finalize (GObject *object)
{
	/* FIXME: free stuff */

	G_OBJECT_CLASS (gig_cell_renderer_graph_parent_class)->finalize (object);
}

static void
gig_cell_renderer_graph_get_property (GObject *object,
				      guint param_id,
				      GValue *value,
				      GParamSpec *pspec)
{
	GigCellRendererGraphPrivate *priv = GIG_CELL_RENDERER_GRAPH (object)->_priv;

	switch (param_id) {
	case PROP_BRANCHES_INFO:
		g_value_set_pointer (value, priv->branches);
		break;
	case PROP_REVISION_INFO:
		g_value_set_pointer (value, priv->revision);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	}
}

static void
gig_cell_renderer_graph_set_property (GObject      *object,
				      guint         param_id,
				      const GValue *value,
				      GParamSpec   *pspec)
{
	GigCellRendererGraphPrivate *priv = GIG_CELL_RENDERER_GRAPH (object)->_priv;

	switch (param_id) {
	case PROP_BRANCHES_INFO:
		priv->branches = g_value_get_pointer (value);
		break;
	case PROP_REVISION_INFO:
		priv->revision = g_value_get_pointer (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	}
}

static void
gig_cell_renderer_graph_get_size (GtkCellRenderer         *cell,
				  GtkWidget               *widget,
				  GdkRectangle            *cell_area,
				  gint                    *x_offset,
				  gint                    *y_offset,
				  gint                    *width,
				  gint                    *height)
{
	GigCellRendererGraphPrivate *priv;
	GList *elem;

	priv = GIG_CELL_RENDERER_GRAPH (cell)->_priv;

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
gig_cell_renderer_graph_render (GtkCellRenderer         *cell,
				GdkWindow               *window,
				GtkWidget               *widget,
				GdkRectangle            *background_area,
				GdkRectangle            *cell_area,
				GdkRectangle            *expose_area,
				guint                    flags)
{
	GigCellRendererGraphPrivate *priv;
	gint x, y, h, count = 0;
	cairo_t *cr;
	GList *list;

	cr = gdk_cairo_create (window);
	priv = GIG_CELL_RENDERER_GRAPH (cell)->_priv;
	x = cell_area->x + cell->xpad;
	y = cell_area->y + cell->ypad;
	h = cell_area->height - cell->ypad * 2;
	list = priv->branches;

	while (list) {
		count %= G_N_ELEMENTS (colors);

		gdk_cairo_set_source_color (cr, &colors[count]);

		/* evil hack to paint continously across rows, paint
		 * outside the cell renderer area */
		cairo_move_to (cr, x + (DOT_SPACE / 2), y - 5);
		cairo_line_to (cr, x + (DOT_SPACE / 2), y + h + 5);
		cairo_stroke  (cr);

		if (priv->revision &&
		    (priv->revision->branch1 == list->data ||
		     priv->revision->branch2 == list->data)) {
			cairo_arc (cr,
				   x + (DOT_SPACE / 2),
				   y + (DOT_SPACE / 2),
				   DOT_RADIUS, 0, 2 * M_PI);
			cairo_fill (cr);
			cairo_stroke (cr);
		}

		x += DOT_SPACE;
		count++;
		list = list->next;
	}

	cairo_destroy (cr);
}

GtkCellRenderer*
gig_cell_renderer_graph_new (GList *branches)
{
	return g_object_new (GIG_TYPE_CELL_RENDERER_GRAPH,
			     "branches-info", branches,
			     NULL);
}
