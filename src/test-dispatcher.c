#include "giggle-dispatcher.h"

static GMainLoop *main_loop;

static void
execute_callback (GiggleDispatcher *dispatcher,
		  guint             id,
		  GError           *error,
		  const gchar      *output,
		  gpointer          user_data)
{
	if (error) {
		g_print ("Operation failed: %s", error->message);
		g_main_loop_quit (main_loop);
		return;
	}

	g_print ("Result from operation:\n");
	g_print ("---------------------------------------------------------------------\n");
	g_print (output);
	g_print ("---------------------------------------------------------------------\n");
	g_main_loop_quit (main_loop);
	return;
}

int
main (int argc, char **argv)
{
	GiggleDispatcher *dispatcher;

	g_type_init ();

	dispatcher = giggle_dispatcher_new ();
	
	main_loop = g_main_loop_new (NULL, FALSE);
	g_print ("Running execute\n");
	giggle_dispatcher_execute (dispatcher,
				   "/home/micke/Source/giggle",
				   "git diff 9ad6c75b46aa 64456fc212e",
				   execute_callback,
				   NULL);
	g_print ("Waiting for the result\n");

	g_main_loop_run (main_loop);
	
	g_print ("Closing down\n");

	return 0;
}
