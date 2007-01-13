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

typedef enum GigRevisionType GigRevisionType;

enum GigRevisionType {
	GIG_REVISION_FORK,
	GIG_REVISION_JOIN,
	GIG_REVISION_COMMIT
};

typedef struct GigRevisionInfo GigRevisionInfo;

struct GigRevisionInfo {
	GigRevisionType type;
	GigBranchInfo *branch1; /* only this will be used in GIG_REVISION_COMMIT */
	GigBranchInfo *branch2;
};

struct GigCellRendererGraphClass {
	GtkCellRendererClass parent_class;
};

struct GigCellRendererGraph {
	GtkCellRenderer parent;

	/*<private>*/
	gpointer _priv;
};

GType		 gig_cell_renderer_graph_get_type (void);
GtkCellRenderer *gig_cell_renderer_graph_new      (GList *branches);

G_END_DECLS

#endif /* __GIG_GRAPH_RENDERER_H__ */
