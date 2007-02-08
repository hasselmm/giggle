#include <gtk/gtk.h>
#include "giggle-file-list.h"
#include "giggle-git.h"

int
main (int argc, gchar *argv[])
{
	GtkWidget *window, *file_list;
	GiggleGit *git;

	gtk_init (&argc, &argv);

	git = giggle_git_get ();
	giggle_git_set_directory (git, g_get_current_dir (), NULL);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	file_list = giggle_file_list_new ();

	gtk_container_add (GTK_CONTAINER (window), file_list);
	gtk_widget_show_all (window);

	gtk_main ();

	return 0;
}
