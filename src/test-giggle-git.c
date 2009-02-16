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
	GiggleRevision   *rev1, *rev2;
	GiggleJob        *diff;

	g_type_init ();

	main_loop = g_main_loop_new (NULL, FALSE);

	git = giggle_git_get ();
	giggle_git_set_directory (git, "/home/micke/Source/giggle", NULL);

	rev1 = giggle_revision_new ("4c7b72b6dc089db58d25d2a2a07de6a4d15f3560");
	giggle_revision_set_author (rev1, "Richard Hult <richard@imendio.com>");
	giggle_revision_set_short_log (rev1, "Make the patch view use a monospace font");
	giggle_revision_set_date (rev1, "2007-01-12");

	rev2 = giggle_revision_new ("883a8b6ed7f068d01e70e3e516164f497f9babd9");
	giggle_revision_set_author (rev2, "Richard Hult <richard@imendio.com>");
	giggle_revision_set_short_log (rev2, "Make the patch view use a monospace font");
	giggle_revision_set_date (rev2, "2007-01-12");

	diff = giggle_git_diff_new ();
	giggle_git_diff_set_revisions (GIGGLE_GIT_DIFF (diff), rev1, rev2);
	giggle_git_run_job (git, diff, job_done_cb, NULL);
	g_object_unref (diff);

	g_main_loop_run (main_loop);

	g_object_unref (git);

	return 0;
}

