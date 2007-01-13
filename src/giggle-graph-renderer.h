#ifndef __GIGGLE_GRAPH_RENDERER_H__
#define __GIGGLE_GRAPH_RENDERER_H__

G_BEGIN_DECLS

#include <gtk/gtk.h>

#define GIGGLE_TYPE_CELL_RENDERER_GRAPH (giggle_cell_renderer_graph_get_type ())
#define GIGGLE_CELL_RENDERER_GRAPH(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_CELL_RENDERER_GRAPH, GiggleCellRendererGraph))
#define GIGGLE_CELL_RENDERER_GRAPH_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_CELL_RENDERER_GRAPH, GiggleCellRendererGraphClass))
#define GIGGLE_IS_CELL_RENDERER_GRAPH(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_CELL_RENDERER_GRAPH))
#define GIGGLE_IS_CELL_RENDERER_GRAPH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_CELL_RENDERER_GRAPH))
#define GIGGLE_CELL_RENDERER_GRAPH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_CELL_RENDERER_GRAPH, GiggleCellRendererGraphClass))

typedef struct GiggleCellRendererGraph GiggleCellRendererGraph;
typedef struct GiggleCellRendererGraphClass GiggleCellRendererGraphClass;

struct GiggleCellRendererGraphClass {
	GtkCellRendererClass parent_class;
};

struct GiggleCellRendererGraph {
	GtkCellRenderer parent;

	/*<private>*/
	gpointer _priv;
};

GType		 giggle_cell_renderer_graph_get_type (void);
GtkCellRenderer *giggle_cell_renderer_graph_new      (GList *branches);

G_END_DECLS

#endif /* __GIGGLE_GRAPH_RENDERER_H__ */
