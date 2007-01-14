#include "giggle-git.h"

GMainLoop *main_loop;

static void 
diff_cb (GiggleGit      *git,
	 guint           id,
	 GError         *error,
	 GiggleRevision *rev1,
	 GiggleRevision *rev2,
	 const gchar    *diff,
	 gpointer        user_data)
{
	g_print ("Got a diff:\n");
	g_print ("---------------------------------------------------------\n");
	g_print (diff);
	g_print ("---------------------------------------------------------\n");

	g_main_loop_quit (main_loop);
}

int 
main (int argc, char **argv)
{
	GiggleGit *git;
	GiggleBranchInfo   *branch1;
	GiggleRevision *rev1, *rev2;

	g_type_init ();

	main_loop = g_main_loop_new (NULL, FALSE);

	git = giggle_git_new ();
	giggle_git_set_directory (git, "/home/micke/Source/giggle", NULL);

	branch1 = giggle_branch_info_new ("master");

	rev1 = giggle_revision_new_commit ("4c7b72b6dc089db58d25d2a2a07de6a4d15f3560", branch1);
        g_object_set (rev1,
                      "author", "Richard Hult <richard@imendio.com>",
                      "short-log", "Make the patch view use a monospace font",
                      "date", "2007-01-12",
                      NULL);

	rev2 = giggle_revision_new_commit ("883a8b6ed7f068d01e70e3e516164f497f9babd9", branch1);
	g_object_set (rev2,
                      "author", "Richard Hult <richard@imendio.com>",
                      "short-log", "Make the patch view use a monospace font",
                      "date", "2007-01-12",
                      NULL);

	giggle_git_get_diff (git, rev1, rev2, 
			     (GiggleDiffCallback)diff_cb, NULL);

	g_main_loop_run (main_loop);

	g_object_unref (git);

	return 0;
}

