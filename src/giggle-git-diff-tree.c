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

#include "giggle-git-diff-tree.h"

typedef struct GiggleGitDiffTreePriv GiggleGitDiffTreePriv;

struct GiggleGitDiffTreePriv {
	GiggleRevision *rev1;
	GiggleRevision *rev2;

	GList          *files;
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
giggle_git_diff_tree_init (GiggleGitDiffTree *diff_tree)
{
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

	priv = GET_PRIV (job);

	*command_line = g_strdup_printf (GIT_COMMAND " diff-tree -r %s %s",
					 giggle_revision_get_sha (priv->rev1),
					 giggle_revision_get_sha (priv->rev2));

	return TRUE;
}

static void
git_diff_tree_handle_output (GiggleJob   *job,
			const gchar *output_str,
			gsize        output_len)
{
	GiggleGitDiffTreePriv  *priv;
	gchar                 **lines, **line;
	gint                    i;

	priv = GET_PRIV (job);

	g_list_foreach (priv->files, (GFunc) g_free, NULL);
	g_list_free (priv->files);

	lines = g_strsplit (output_str, "\n", -1);

	for (i = 0; lines[i] && *lines[i]; i++) {
		line = g_strsplit (lines[i], "\t", -1);

		/* add filename */
		priv->files = g_list_prepend (priv->files, g_strdup (line[1]));
		g_strfreev (line);
	}

	priv->files = g_list_reverse (priv->files);
	g_strfreev (lines);
}

GiggleJob *
giggle_git_diff_tree_new (GiggleRevision *rev1, GiggleRevision *rev2)
{
	g_return_val_if_fail (GIGGLE_IS_REVISION (rev1), NULL);
	g_return_val_if_fail (GIGGLE_IS_REVISION (rev2), NULL);

	return g_object_new (GIGGLE_TYPE_GIT_DIFF_TREE,
			     "revision-1", rev1,
			     "revision-2", rev2,
			     NULL);
}

GList *
giggle_git_diff_tree_get_files (GiggleGitDiffTree *diff_tree)
{
	GiggleGitDiffTreePriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_DIFF_TREE (diff_tree), NULL);

	priv = GET_PRIV (diff_tree);

	return priv->files;
}
