#ifndef __GIG_GRAPH_RENDERER_H__
#define __GIG_GRAPH_RENDERER_H__

G_BEGIN_DECLS

#include <gtk/gtk.h>

#define GIG_TYPE_CELL_RENDERER_GRAPH (gig_cell_renderer_graph_get_type ())
#define GIG_CELL_RENDERER_GRAPH(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIG_TYPE_CELL_RENDERER_GRAPH, GigCellRendererGraph))
#define GIG_CELL_RENDERER_GRAPH_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), GIG_TYPE_CELL_RENDERER_GRAPH, GigCellRendererGraphClass))
#define GIG_IS_CELL_RENDERER_GRAPH(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIG_TYPE_CELL_RENDERER_GRAPH))
#define GIG_IS_CELL_RENDERER_GRAPH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIG_TYPE_CELL_RENDERER_GRAPH))
#define GIG_CELL_RENDERER_GRAPH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIG_TYPE_CELL_RENDERER_GRAPH, GigCellRendererGraphClass))

typedef struct GigCellRendererGraph GigCellRendererGraph;
typedef struct GigCellRendererGraphClass GigCellRendererGraphClass;

/* FIXME: perhaps should be splitted out to their own files */
typedef struct GigBranchInfo GigBranchInfo;

struct GigBranchInfo {
	const gchar *name;
};

typedef struct GigRevisionInfo GigRevisionInfo;

struct GigRevisionInfo {
};

struct GigCellRendererGraphClass {
	GtkCellRendererClass parent_class;
};

struct GigCellRendererGraph {
	GtkCellRenderer parent;

	/*<private>*/
	gpointer _priv;
};

GType		      gig_cell_renderer_graph_get_type (void);
GigCellRendererGraph *gig_cell_renderer_graph_new      (GigBranchInfo **branches);

G_END_DECLS

#endif /* __GIG_GRAPH_RENDERER_H__ */
