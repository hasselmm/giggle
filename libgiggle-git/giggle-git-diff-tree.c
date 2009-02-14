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

#include "config.h"
#include "giggle-git-diff-tree.h"

#include <string.h>

typedef struct GiggleGitDiffTreePriv GiggleGitDiffTreePriv;

struct GiggleGitDiffTreePriv {
	GiggleRevision *rev1;
	GiggleRevision *rev2;

	GList          *files;
	GHashTable     *actions;
	GHashTable     *sha_table1;
	GHashTable     *sha_table2;
};

static void     git_diff_tree_finalize            (GObject           *object);
static void     git_diff_tree_get_property        (GObject           *object,
						   guint              param_id,
						   GValue            *value,
						   GParamSpec        *pspec);
static void     git_diff_tree_set_property        (GObject           *object,
						   guint              param_id,
						   const GValue      *value,
						   GParamSpec        *pspec);

static gboolean git_diff_tree_get_command_line    (GiggleJob         *job,
						   gchar            **command_line);
static void     git_diff_tree_handle_output       (GiggleJob         *job,
						   const gchar       *output_str,
						   gsize              output_len);

G_DEFINE_TYPE (GiggleGitDiffTree, giggle_git_diff_tree, GIGGLE_TYPE_JOB)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_DIFF_TREE, GiggleGitDiffTreePriv))

enum {
	PROP_0,
	PROP_REV_1,
	PROP_REV_2,
};

static void
giggle_git_diff_tree_class_init (GiggleGitDiffTreeClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GiggleJobClass *job_class    = GIGGLE_JOB_CLASS (class);

	object_class->finalize     = git_diff_tree_finalize;
	object_class->get_property = git_diff_tree_get_property;
	object_class->set_property = git_diff_tree_set_property;

	job_class->get_command_line = git_diff_tree_get_command_line;
	job_class->handle_output    = git_diff_tree_handle_output;

	g_object_class_install_property (object_class,
					 PROP_REV_1,
					 g_param_spec_object ("revision-1",
							      "Revision 1",
							      "Revision 1 to diff tree",
							      GIGGLE_TYPE_REVISION,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_REV_2,
					 g_param_spec_object ("revision-2",
							      "Revision 2",
							      "Revision 2 to diff tree",
							      GIGGLE_TYPE_REVISION,
							      G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleGitDiffTreePriv));
}

static void
giggle_git_diff_tree_init (GiggleGitDiffTree *job)
{
	GiggleGitDiffTreePriv *priv;

	priv = GET_PRIV (job);

	priv->actions    = g_hash_table_new      (g_str_hash, g_str_equal);
	priv->sha_table1 = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_free);
	priv->sha_table2 = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_free);
}

static void
git_diff_tree_finalize (GObject *object)
{
	GiggleGitDiffTreePriv *priv;

	priv = GET_PRIV (object);

	if (priv->rev1) {
		g_object_unref (priv->rev1);
	}

	if (priv->rev2) {
		g_object_unref (priv->rev2);
	}

	g_list_foreach (priv->files, (GFunc) g_free, NULL);
	g_list_free (priv->files);

	g_hash_table_destroy (priv->actions);
	g_hash_table_destroy (priv->sha_table1);
	g_hash_table_destroy (priv->sha_table2);

	G_OBJECT_CLASS (giggle_git_diff_tree_parent_class)->finalize (object);
}

static void
git_diff_tree_get_property (GObject    *object,
			    guint       param_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	GiggleGitDiffTreePriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_REV_1:
		g_value_set_object (value, priv->rev1);
		break;
	case PROP_REV_2:
		g_value_set_object (value, priv->rev2);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
git_diff_tree_set_property (GObject      *object,
			    guint         param_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	GiggleGitDiffTreePriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_REV_1:
		if (priv->rev1) {
			g_object_unref (priv->rev1);
		}
		priv->rev1 = GIGGLE_REVISION (g_value_dup_object (value));
		break;
	case PROP_REV_2:
		if (priv->rev2) {
			g_object_unref (priv->rev2);
		}
		priv->rev2 = GIGGLE_REVISION (g_value_dup_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
git_diff_tree_get_command_line (GiggleJob *job, gchar **command_line)
{
	GiggleGitDiffTreePriv *priv;
	const char *sha1 = NULL;
	const char *sha2 = NULL;

	priv = GET_PRIV (job);

	if (priv->rev1)
		sha1 = giggle_revision_get_sha (priv->rev1);
	if (priv->rev2)
		sha2 = giggle_revision_get_sha (priv->rev2);

	if (sha1 && sha2) {
		*command_line = g_strdup_printf (GIT_COMMAND " diff-tree -r %s %s", sha2, sha1);
	} else if (sha1) {
		*command_line = g_strdup_printf (GIT_COMMAND " diff-tree -r %s^ %s", sha1, sha1);
	} else if (sha2) {
		*command_line = g_strdup_printf (GIT_COMMAND " diff-files -r -R %s", sha2);
	} else {
		*command_line = g_strdup (GIT_COMMAND " diff-files -r");
	}

	return TRUE;
}

static void
git_diff_tree_handle_output (GiggleJob   *job,
			const gchar *output_str,
			gsize        output_len)
{
	GiggleGitDiffTreePriv  *priv;
	char                  **lines, *file;
	char			sha1[41], sha2[41];
	unsigned		mode1, mode2;
	char			action;
	int                     i, n;

	priv = GET_PRIV (job);

	g_list_foreach (priv->files, (GFunc) g_free, NULL);
	g_list_free (priv->files);

	lines = g_strsplit (output_str, "\n", -1);

	for (i = 0; lines[i] && *lines[i]; i++) {
		if (5 != sscanf (lines[i], ":%6d %6d %40s %40s %c\t%n",
				 &mode1, &mode2, sha1, sha2, &action, &n))
			continue;

		/* add filename */
		file = g_strdup (lines[i] + n);
		priv->files = g_list_prepend (priv->files, file);

		/* add meta information */
		if (strcmp (sha1, "0000000000000000000000000000000000000000"))
			g_hash_table_insert (priv->sha_table1, file, g_strdup (sha1));
		if (strcmp (sha2, "0000000000000000000000000000000000000000"))
			g_hash_table_insert (priv->sha_table2, file, g_strdup (sha2));

		g_hash_table_insert (priv->actions, file, GINT_TO_POINTER ((int) action));
	}

	priv->files = g_list_reverse (priv->files);
	g_strfreev (lines);
}

GiggleJob *
giggle_git_diff_tree_new (GiggleRevision *rev1, GiggleRevision *rev2)
{
	g_return_val_if_fail (GIGGLE_IS_REVISION (rev1) || !rev1, NULL);
	g_return_val_if_fail (GIGGLE_IS_REVISION (rev2) || !rev2, NULL);

	return g_object_new (GIGGLE_TYPE_GIT_DIFF_TREE,
			     "revision-1", rev1,
			     "revision-2", rev2,
			     NULL);
}

GList *
giggle_git_diff_tree_get_files (GiggleGitDiffTree *job)
{
	GiggleGitDiffTreePriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_DIFF_TREE (job), NULL);

	priv = GET_PRIV (job);

	return priv->files;
}

const char *
giggle_git_diff_tree_get_sha1 (GiggleGitDiffTree *job,
			       const char        *file)
{
	GiggleGitDiffTreePriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_DIFF_TREE (job), NULL);
	g_return_val_if_fail (NULL != file, NULL);

	priv = GET_PRIV (job);

	return g_hash_table_lookup (priv->sha_table1, file);
}

const char *
giggle_git_diff_tree_get_sha2 (GiggleGitDiffTree *job,
			       const char        *file)
{
	GiggleGitDiffTreePriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_DIFF_TREE (job), NULL);
	g_return_val_if_fail (NULL != file, NULL);

	priv = GET_PRIV (job);

	return g_hash_table_lookup (priv->sha_table2, file);
}

char
giggle_git_diff_tree_get_action (GiggleGitDiffTree *job,
				 const char        *file)
{
	GiggleGitDiffTreePriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_DIFF_TREE (job), '\0');
	g_return_val_if_fail (NULL != file, '\0');

	priv = GET_PRIV (job);

	return GPOINTER_TO_INT (g_hash_table_lookup (priv->actions, file));
}

