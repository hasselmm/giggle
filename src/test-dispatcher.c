#include "giggle-dispatcher.h"

static GMainLoop        *main_loop;
static GiggleDispatcher *dispatcher;

static void
execute_callback (GiggleDispatcher *dispatcher,
		  guint             id,
		  GError           *error,
		  const gchar      *output,
		  gpointer          user_data)
{
	if (error) {
		g_print ("Operation failed: %s\n", error->message);
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

static gboolean
timeout_cancel (gpointer data)
{
	guint id = GPOINTER_TO_INT (data);

	giggle_dispatcher_cancel (dispatcher, id);

	return FALSE;
}

int
main (int argc, char **argv)
{
	guint id;

	g_type_init ();

	dispatcher = giggle_dispatcher_new ();
	
	main_loop = g_main_loop_new (NULL, FALSE);
	g_print ("Running execute\n");
	id = giggle_dispatcher_execute (dispatcher,
					"/home/micke/Source/giggle",
					"git diff 9ad6c75b46aa 64456fc212e",
					execute_callback,
					NULL);
#if 0
	id = giggle_dispatcher_execute (dispatcher,
					NULL, "yes", execute_callback, 
					NULL);
#endif

	g_timeout_add (10000, timeout_cancel, GINT_TO_POINTER (id));

	g_print ("Waiting for the result\n");

	g_main_loop_run (main_loop);
	
	g_print ("Closing down\n");

	g_main_loop_unref (main_loop);
	g_object_unref (dispatcher);

	return 0;
}
