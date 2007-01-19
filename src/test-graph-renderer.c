#include <gtk/gtk.h>
#include "giggle-graph-renderer.h"
#include "giggle-revision.h"

GiggleBranchInfo *branch1;
GiggleBranchInfo *branch2;

static GList*
get_branches (void)
{
	GList *list = NULL;

	branch1 = giggle_branch_info_new ("master");
	branch2 = giggle_branch_info_new ("foo");

	list = g_list_prepend (list, branch2);
	list = g_list_prepend (list, branch1);
	return list;
}

static GtkTreeModel*
create_model (void)
{
	GtkListStore *store;
	GtkTreeIter iter;

	store = gtk_list_store_new (2, G_TYPE_POINTER, G_TYPE_STRING);

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    0, giggle_revision_new_commit ("1", branch1),
			    1, "another change",
			    -1);

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    0, giggle_revision_new_merge ("2", branch1, branch2),
			    1, "merge branch foo",
			    -1);

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    0, giggle_revision_new_commit ("3", branch1),
			    1, "fix something in branch master",
			    -1);

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    0, giggle_revision_new_commit ("4", branch2),
			    1, "fix something in branch foo",
			    -1);

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    0, giggle_revision_new_branch ("5", branch1, branch2),
			    1, "branched",
			    -1);

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    0, giggle_revision_new_commit ("6", branch1),
			    1, "Initial commit",
			    -1);

	giggle_revision_validate (GTK_TREE_MODEL (store), 0);

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

	renderer = giggle_graph_renderer_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1,
						     "Graph",
						     renderer,
						     "revision", 0,
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
