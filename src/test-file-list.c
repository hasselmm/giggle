#include <gtk/gtk.h>
#include "giggle-file-list.h"
#include "giggle-git.h"

int
main (int argc, gchar *argv[])
{
	GtkWidget *window, *scrolled_window, *file_list;
	GiggleGit *git;

	gtk_init (&argc, &argv);

	git = giggle_git_get ();
	giggle_git_set_directory (git, g_get_current_dir (), NULL);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	file_list = giggle_file_list_new ();

	gtk_window_set_default_size (GTK_WINDOW (window), 250, 250);

	gtk_container_add (GTK_CONTAINER (window), scrolled_window);
	gtk_container_add (GTK_CONTAINER (scrolled_window), file_list);
	gtk_widget_show_all (window);

	gtk_main ();

	return 0;
}
