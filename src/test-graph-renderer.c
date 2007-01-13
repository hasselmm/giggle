#include <gtk/gtk.h>
#include "giggle-graph-renderer.h"

GigBranchInfo *branch1;
GigBranchInfo *branch2;

static GList*
get_branches (void)
{
	GList *list = NULL;

	branch1 = g_new0 (GigBranchInfo, 1);
	branch1->name = "master";
	list = g_list_prepend (list, branch1);

	branch2 = g_new0 (GigBranchInfo, 1);
	branch2->name = "foo";
	list = g_list_prepend (list, branch2);

	return list;
}

static GtkTreeModel*
create_model (void)
{
	GtkListStore *store;
	GtkTreeIter iter;
	GigRevisionInfo *info;

	store = gtk_list_store_new (2, G_TYPE_POINTER, G_TYPE_STRING);

	gtk_list_store_append (store, &iter);
	info = g_new0 (GigRevisionInfo, 1);
	info->type = GIG_REVISION_FORK;
	info->branch1 = branch1;
	info->branch2 = branch2;
	gtk_list_store_set (store, &iter, 0, info, 1, "Boooh", -1);

	gtk_list_store_append (store, &iter);
	info = g_new0 (GigRevisionInfo, 1);
	info->type = GIG_REVISION_COMMIT;
	info->branch1 = branch2;
	info->branch2 = NULL;
	gtk_list_store_set (store, &iter, 0, info, 1, "Fixety fix", -1);

	gtk_list_store_append (store, &iter);
	info = g_new0 (GigRevisionInfo, 1);
	info->type = GIG_REVISION_JOIN;
	info->branch1 = branch1;
	info->branch2 = branch2;
	gtk_list_store_set (store, &iter, 0, info, 1, "Blah", -1);

	return GTK_TREE_MODEL (store);
}

static GtkWidget*
create_main_window (void)
{
	GtkWidget *window, *treeview;
	GtkCellRenderer *renderer;
	GtkTreeModel *model;
	GList *branches;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	treeview = gtk_tree_view_new ();
	branches = get_branches ();
	model = create_model ();

	gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), model);
	g_object_unref (model);

	renderer = gig_cell_renderer_graph_new (branches);
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1,
						     "Graph",
						     renderer,
						     "revision-info", 0,
						     NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1,
						     "Comment",
						     renderer,
						     "text", 1,
						     NULL);

	gtk_container_add (GTK_CONTAINER (window), treeview);

	return window;
}

int
main (int argc, char *argv[])
{
	GtkWidget *window;

	gtk_init (&argc, &argv);

	window = create_main_window ();
	gtk_widget_show_all (window);

	g_signal_connect (G_OBJECT (window), "delete-event",
			  G_CALLBACK (gtk_main_quit), NULL);
	gtk_main ();
	return 0;
}
