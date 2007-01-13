#include <gtk/gtk.h>
#include "giggle-revision-info.h"

GdkColor colors[] = {
	{ 0x0, 0x0000, 0x0000, 0x0000 }, /* black */
	{ 0x0, 0xffff, 0x0000, 0x0000 }, /* red   */
	{ 0x0, 0x0000, 0xffff, 0x0000 }, /* green */
	{ 0x0, 0x0000, 0x0000, 0xffff }, /* blue  */
};

static void
copy_branch_status (gpointer    key,
		    gpointer    value,
		    GHashTable *table)
{
	g_hash_table_insert (table, key, value);
}

static void
copy_status (GigRevisionInfo *info,
	     GHashTable      *branches_info)
{
	if (info->branches)
		g_hash_table_destroy (info->branches);

	info->branches = g_hash_table_new (g_direct_hash, g_direct_equal);
	g_hash_table_foreach (branches_info, (GHFunc) copy_branch_status, info->branches);
}

static void
calculate_status (GigRevisionInfo *info,
		  GHashTable      *branches_info,
		  gint            *color)
{
	GdkColor *c;

	switch (info->type) {
	case GIG_REVISION_BRANCH:
		/* insert in the new branch the color from the first one */
		c = g_hash_table_lookup (branches_info, info->branch1);

		if (!c)
			g_critical ("model inconsistent, branches from an unexisting branch?");

		g_hash_table_insert (branches_info, info->branch2, c);
		copy_status (info, branches_info);
		break;
	case GIG_REVISION_MERGE:
		copy_status (info, branches_info);
		/* remove merged branch */
		g_hash_table_remove (branches_info, info->branch2);
		break;
	case GIG_REVISION_COMMIT:
		/* increment color */
		*color = (*color + 1) % G_N_ELEMENTS (colors);

		/* change color for the changed branch */
		g_hash_table_insert (branches_info, info->branch1, &colors[*color]);

		copy_status (info, branches_info);
		break;
	}
}

GigBranchInfo*
gig_branch_info_new (const gchar *name)
{
	GigBranchInfo *info;

	info = g_new0 (GigBranchInfo, 1);
	info->name = g_strdup (name);

	return info;
}

void
gig_branch_info_free (GigBranchInfo *info)
{
	if (info->name)
		g_free (info->name);

	g_free (info);
}

GigRevisionInfo*
gig_revision_info_new_commit (GigBranchInfo *branch)
{
	GigRevisionInfo *info;

	info = g_new0 (GigRevisionInfo, 1);
	info->type = GIG_REVISION_COMMIT;
	info->branch1 = branch;

	return info;
}

GigRevisionInfo*
gig_revision_info_new_branch (GigBranchInfo *old, GigBranchInfo *new)
{
	GigRevisionInfo *info;

	info = g_new0 (GigRevisionInfo, 1);
	info->type = GIG_REVISION_BRANCH;
	info->branch1 = old;
	info->branch2 = new;

	return info;
}

GigRevisionInfo*
gig_revision_info_new_merge (GigBranchInfo *to, GigBranchInfo *from)
{
	GigRevisionInfo *info;

	info = g_new0 (GigRevisionInfo, 1);
	info->type = GIG_REVISION_MERGE;
	info->branch1 = to;
	info->branch2 = from;

	return info;
}

void
gig_revision_info_free (GigRevisionInfo *info)
{
	if (info->branch1)
		gig_branch_info_free (info->branch1);

	if (info->branch2)
		gig_branch_info_free (info->branch2);

	if (info->branches)
		g_hash_table_destroy (info->branches);

	g_free (info);
}

void
gig_revision_info_validate (GtkTreeModel *model,
			    gint          n_column)
{
	GtkTreeIter iter;
	gint n_children;
	gint color = 0;
	GHashTable *branches_info;
	GigRevisionInfo *info;

	g_return_if_fail (GTK_IS_TREE_MODEL (model));

	branches_info = g_hash_table_new (g_direct_hash, g_direct_equal);
	n_children = gtk_tree_model_iter_n_children (model, NULL);

	/* need to calculate backwards from the end of the model */
	while (n_children) {
		n_children--;
		gtk_tree_model_iter_nth_child (model, &iter, NULL, n_children);
		gtk_tree_model_get (model, &iter, n_column, &info, -1);

		calculate_status (info, branches_info, &color);
	}
}
