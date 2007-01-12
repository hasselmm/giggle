#include <gtk/gtk.h>
#include "giggle-graph-renderer.h"

#define GIG_CELL_RENDERER_GRAPH_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), GIG_TYPE_CELL_RENDERER_GRAPH, GigCellRendererGraphPrivate))

typedef struct GigCellRendererGraphPrivate GigCellRendererGraphPrivate;

struct GigCellRendererGraphPrivate {
};

enum {
	PROP_0,
};



static void gig_cell_renderer_graph_finalize     (GObject                 *object);

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
	GtkCellRendererClass *cell_class;
	GObjectClass *object_class;

	cell_class->get_size = gig_cell_renderer_graph_get_size;
	cell_class->render = gig_cell_renderer_graph_render;
	object_class->finalize = gig_cell_renderer_graph_finalize;

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
gig_cell_renderer_graph_get_size (GtkCellRenderer         *cell,
				  GtkWidget               *widget,
				  GdkRectangle            *cell_area,
				  gint                    *x_offset,
				  gint                    *y_offset,
				  gint                    *width,
				  gint                    *height)
{
	/* FIXME: figure out size */
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
gig_cell_renderer_graph_new (void)
{
	return g_object_new (GIG_TYPE_CELL_RENDERER_GRAPH, NULL);
}
