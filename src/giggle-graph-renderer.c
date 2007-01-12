#include <gtk/gtk.h>
#include "giggle-graph-renderer.h"

#define GIG_CELL_RENDERER_GRAPH_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), GIG_TYPE_CELL_RENDERER_GRAPH, GigCellRendererGraphPrivate))
#define DOT_SIZE 20
#define DOTS_SEPARATION 6

GdkColor colors[] = {
	{ 0xff, 0x00, 0x00, 0x00 }, /* black */
	{ 0xff, 0xff, 0x00, 0x00 }, /* red   */
	{ 0xff, 0x00, 0xff, 0x00 }, /* green */
	{ 0xff, 0x00, 0x00, 0xff }, /* blue  */
};

typedef struct GigCellRendererGraphPrivate GigCellRendererGraphPrivate;

struct GigCellRendererGraphPrivate {
	GigBranchInfo **branches;
	GigRevisionInfo **info;
};

enum {
	PROP_0,
	PROP_BRANCHES_INFO,
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
	GigBranchInfo **branch;

	priv = GIG_CELL_RENDERER_GRAPH (cell)->_priv;

	if (height)
		*height = DOT_SIZE;

	if (width) {
		branch = priv->branches;
		*width = 0;

		while (*branch) {
			*width = DOT_SIZE + DOTS_SEPARATION;
			branch++;
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
	/* FIXME: funny code missing here */
}

GigCellRendererGraph*
gig_cell_renderer_graph_new (GigBranchInfo **branches)
{
	return g_object_new (GIG_TYPE_CELL_RENDERER_GRAPH,
			     "branches-info", branches,
			     NULL);
}
