#include "giggle-git.h"
#include "giggle-git-diff.h"

GMainLoop *main_loop;

static void 
job_done_cb (GiggleGit *git,
	     GiggleJob *job,
	     GError    *error,
	     gpointer   user_data)
{
	if (!GIGGLE_IS_GIT_DIFF (job)) {
		g_warning ("Not a diff job");
		g_main_loop_quit (main_loop);
		return;
	}

	g_print ("Got a diff:\n");
	g_print ("\n-------------------------------------------------------\n");
	g_print (giggle_git_diff_get_result (GIGGLE_GIT_DIFF (job)));
	g_print ("\n-------------------------------------------------------\n");

	g_main_loop_quit (main_loop);
}

int 
main (int argc, char **argv)
{
	GiggleGit        *git;
	GiggleBranchInfo *branch1;
	GiggleRevision   *rev1, *rev2;
	GiggleJob        *diff;

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

	diff = giggle_git_diff_new (rev1, rev2);
	giggle_git_run_job (git, diff, job_done_cb, NULL);
	g_object_unref (diff);

	g_main_loop_run (main_loop);

	g_object_unref (git);

	return 0;
}

