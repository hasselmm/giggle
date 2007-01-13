/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2007 Imendio AB
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <gtk/gtk.h>

#include "giggle-revision.h"

typedef struct GiggleRevisionPriv GiggleRevisionPriv;

struct GiggleRevisionPriv {
	gchar              *sha;
	gchar              *short_log;
	gchar              *long_log;
};

static GdkColor colors[] = {
	{ 0x0, 0x0000, 0x0000, 0x0000 }, /* black */
	{ 0x0, 0xffff, 0x0000, 0x0000 }, /* red   */
	{ 0x0, 0x0000, 0xffff, 0x0000 }, /* green */
	{ 0x0, 0x0000, 0x0000, 0xffff }, /* blue  */
	{ 0x0, 0xffff, 0xffff, 0x0000 }, /* yellow */
	{ 0x0, 0xffff, 0x0000, 0xffff }, /* pink?  */
	{ 0x0, 0x0000, 0xffff, 0xffff }, /* cyan   */
	{ 0x0, 0x6666, 0x6666, 0x6666 }, /* grey   */
};


static void revision_finalize           (GObject        *object);
static void revision_copy_branch_status (gpointer        key,
					 gpointer        value,
					 GHashTable     *table);
static void revision_copy_status        (GiggleRevision *revision,
					 GHashTable     *branches_info);
static void revision_calculate_status   (GiggleRevision *revision,
					 GHashTable     *branches_info,
					 gint           *color);

G_DEFINE_TYPE (GiggleRevision, giggle_revision, G_TYPE_OBJECT);
#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REVISION, GiggleRevisionPriv))

static void
giggle_revision_class_init (GiggleRevisionClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = revision_finalize;

	g_type_class_add_private (object_class, sizeof (GiggleRevisionPriv));
}

static void
giggle_revision_init (GiggleRevision *revision)
{
}

static void
revision_finalize (GObject *object)
{
	GiggleRevision     *revision;
	GiggleRevisionPriv *priv;

	revision = GIGGLE_REVISION (object);

	priv = GET_PRIV (revision);

	if (revision->branch1) {
		giggle_branch_info_free (revision->branch1);
	}
	
	if (revision->branch2) {
		giggle_branch_info_free (revision->branch2);
	}
	
	if (revision->branches) {
		g_hash_table_destroy (revision->branches);
	}

	g_free (priv->sha);
	g_free (priv->short_log);
	g_free (priv->long_log);
	
	G_OBJECT_CLASS (giggle_revision_parent_class)->finalize (object);
}

static void
revision_copy_branch_status (gpointer    key,
			     gpointer    value,
			     GHashTable *table)
{
	g_hash_table_insert (table, key, value);
}

static void
revision_copy_status (GiggleRevision *revision,
		      GHashTable     *branches_info)
{
	if (revision->branches) {
		g_hash_table_destroy (revision->branches);
	}
	
	revision->branches = g_hash_table_new (g_direct_hash, g_direct_equal);
	g_hash_table_foreach (branches_info,
			      (GHFunc) revision_copy_branch_status,
			      revision->branches);
}

static void
revision_calculate_status (GiggleRevision *revision,
			   GHashTable     *branches_info,
			   gint           *color)
{
	GdkColor *c;

	switch (revision->type) {
	case GIGGLE_REVISION_BRANCH:
		/* insert in the new branch the color from the first one */
		c = g_hash_table_lookup (branches_info, revision->branch1);

		if (!c) {
			g_critical ("model inconsistent, branches from an unexisting branch?");
		}
		
		g_hash_table_insert (branches_info, revision->branch2, c);
		revision_copy_status (revision, branches_info);
		break;

	case GIGGLE_REVISION_MERGE:
		revision_copy_status (revision, branches_info);
		/* remove merged branch */
		g_hash_table_remove (branches_info, revision->branch2);
		break;

	case GIGGLE_REVISION_COMMIT:
		/* increment color */
		*color = (*color + 1) % G_N_ELEMENTS (colors);

		/* change color for the changed branch */
		g_hash_table_insert (branches_info, revision->branch1, &colors[*color]);

		revision_copy_status (revision, branches_info);
		break;
	}
}

GiggleBranchInfo *
giggle_branch_info_new (const gchar *name)
{
	GiggleBranchInfo *info;

	info = g_new0 (GiggleBranchInfo, 1);
	info->name = g_strdup (name);

	return info;
}

void
giggle_branch_info_free (GiggleBranchInfo *info)
{
	g_free (info->name);
	g_free (info);
}

GiggleRevision *
giggle_revision_new_commit (GiggleBranchInfo *branch)
{
	GiggleRevision *revision;

	revision = g_object_new (GIGGLE_TYPE_REVISION, NULL);
	revision->type = GIGGLE_REVISION_COMMIT;
	revision->branch1 = branch;

	return revision;
}

GiggleRevision *
giggle_revision_new_branch (GiggleBranchInfo *old,
			    GiggleBranchInfo *new)
{
	GiggleRevision *revision;

	revision = g_object_new (GIGGLE_TYPE_REVISION, NULL);
	revision->type = GIGGLE_REVISION_BRANCH;
	revision->branch1 = old;
	revision->branch2 = new;

	return revision;
}

GiggleRevision *
giggle_revision_new_merge (GiggleBranchInfo *to,
			   GiggleBranchInfo *from)
{
	GiggleRevision *revision;

	revision = g_object_new (GIGGLE_TYPE_REVISION, NULL);
	revision->type = GIGGLE_REVISION_MERGE;
	revision->branch1 = to;
	revision->branch2 = from;

	return revision;
}

void
giggle_revision_validate (GtkTreeModel *model,
			  gint          n_column)
{
	GtkTreeIter     iter;
	gint            n_children;
	gint            color = 0;
	GHashTable     *branches_info;
	GiggleRevision *revision;
	
	g_return_if_fail (GTK_IS_TREE_MODEL (model));
	
	branches_info = g_hash_table_new (g_direct_hash, g_direct_equal);
	n_children = gtk_tree_model_iter_n_children (model, NULL);

	/* Need to calculate backwards from the end of the model. */
	while (n_children) {
		n_children--;
		gtk_tree_model_iter_nth_child (model, &iter, NULL, n_children);
		gtk_tree_model_get (model, &iter, n_column, &revision, -1);

		revision_calculate_status (revision, branches_info, &color);
	}
}

const gchar *
giggle_revision_get_sha (GiggleRevision *revision)
{
	GiggleRevisionPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);

	priv = GET_PRIV (revision);
	
	return priv->sha;
}

const gchar *
giggle_revision_get_short_log (GiggleRevision *revision)
{
	GiggleRevisionPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);

	priv = GET_PRIV (revision);
	
	return priv->short_log;
}

const gchar *
giggle_revision_get_long_log  (GiggleRevision   *revision)
{
	GiggleRevisionPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);

	priv = GET_PRIV (revision);
	
	return priv->long_log;
}

const gchar *
giggle_revision_get_patch (GiggleRevision *revision)
{
	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);

	return NULL;
}
