#include <gtk/gtk.h>

int
main (int argc, char **argv)
{
	gtk_init (&argc, &argv);

	g_print ("Cool things will happen here.\n");

	return 0;
}
