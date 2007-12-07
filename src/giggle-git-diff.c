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

#include "giggle-git-diff.h"

typedef struct GiggleGitDiffPriv GiggleGitDiffPriv;

struct GiggleGitDiffPriv {
	GiggleRevision *rev1;
	GiggleRevision *rev2;

	GList          *files;
	GiggleRevision *patch_format;

	gchar          *result;
};

static void     git_diff_finalize            (GObject           *object);
static void     git_diff_get_property        (GObject           *object,
					      guint              param_id,
					      GValue            *value,
					      GParamSpec        *pspec);
static void     git_diff_set_property        (GObject           *object,
					      guint              param_id,
					      const GValue      *value,
					      GParamSpec        *pspec);

static gboolean git_diff_get_command_line    (GiggleJob         *job,
					      gchar            **command_line);
static void     git_diff_handle_output       (GiggleJob         *job,
					      const gchar       *output_str,
					      gsize              output_len);

G_DEFINE_TYPE (GiggleGitDiff, giggle_git_diff, GIGGLE_TYPE_JOB)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_DIFF, GiggleGitDiffPriv))

enum {
	PROP_0,
	PROP_REV1,
	PROP_REV2,
	PROP_FILES,
	PROP_PATCH_FORMAT
};

static void
giggle_git_diff_class_init (GiggleGitDiffClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GiggleJobClass *job_class    = GIGGLE_JOB_CLASS (class);

	object_class->finalize     = git_diff_finalize;
	object_class->get_property = git_diff_get_property;
	object_class->set_property = git_diff_set_property;

	job_class->get_command_line = git_diff_get_command_line;
	job_class->handle_output    = git_diff_handle_output;

	g_object_class_install_property (object_class,
					 PROP_REV1,
					 g_param_spec_object ("revision1",
							      "Revision 1",
							      "Revision 1 to make diff on",
							      GIGGLE_TYPE_REVISION,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_REV2,
					 g_param_spec_object ("revision2",
							      "Revision 2",
							      "Revision 2 to make diff on",
							      GIGGLE_TYPE_REVISION,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_FILES,
					 g_param_spec_pointer ("files",
							       "Files",
							       "Files list to make diff on",
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_PATCH_FORMAT,
					 g_param_spec_object ("patch-format",
							      "Patch format",
							      "The revision to output a patch format for",
							      GIGGLE_TYPE_REVISION,
							      G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleGitDiffPriv));
}

static void
giggle_git_diff_init (GiggleGitDiff *dummy)
{
}

static void
git_diff_finalize (GObject *object)
{
	GiggleGitDiffPriv *priv;

	priv = GET_PRIV (object);

	if (priv->rev1) {
		g_object_unref (priv->rev1);
	}

	if (priv->rev2) {
		g_object_unref (priv->rev2);
	}

	g_free (priv->result);

	g_list_foreach (priv->files, (GFunc) g_free, NULL);
	g_list_free (priv->files);

	if (priv->patch_format) {
		g_object_unref (priv->patch_format);
	}

	G_OBJECT_CLASS (giggle_git_diff_parent_class)->finalize (object);
}

static void
git_diff_get_property (GObject    *object,
		       guint       param_id,
		       GValue     *value,
		       GParamSpec *pspec)
{
	GiggleGitDiffPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_REV1:
		g_value_set_object (value, priv->rev1);
		break;
	case PROP_REV2:
		g_value_set_object (value, priv->rev2);
		break;
	case PROP_FILES:
		g_value_set_pointer (value, priv->files);
		break;
	case PROP_PATCH_FORMAT:
		g_value_set_object (value, priv->patch_format);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
git_diff_set_property (GObject      *object,
		       guint         param_id,
		       const GValue *value,
		       GParamSpec   *pspec)
{
	GiggleGitDiffPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_REV1:
		if (priv->rev1) {
			g_object_unref (priv->rev1);
		}
#if GLIB_MAJOR_VERSION > 2 || GLIB_MINOR_VERSION > 12
		priv->rev1 = g_value_dup_object (value);
#else
		priv->rev1 = GIGGLE_REVISION (g_value_dup_object (value));
#endif
		break;
	case PROP_REV2:
		if (priv->rev2) {
			g_object_unref (priv->rev2);
		}
#if GLIB_MAJOR_VERSION > 2 || GLIB_MINOR_VERSION > 12
		priv->rev2 = g_value_dup_object (value);
#else
		priv->rev2 = GIGGLE_REVISION (g_value_dup_object (value));
#endif
		break;
	case PROP_FILES:
		priv->files = g_value_get_pointer (value);
		break;
	case PROP_PATCH_FORMAT:
		priv->patch_format = g_value_get_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
git_diff_get_command_line (GiggleJob *job, gchar **command_line)
{
	GiggleGitDiffPriv *priv;
	GString           *str;
	GList             *files;

	priv = GET_PRIV (job);
	files = priv->files;

	/* NOTE: 
	 * Git currently (20071009) v1.5.2.1 only supports:
	 *
	 *   $ git format-patch
	 *
	 * for COMPLETE changes sets to a branch, you can't use this
	 * command for a selection of files you have changed, also, it
	 * only works for committed work, not uncommitted work AFAICS. 
	 */
	if (priv->patch_format) {
		str = g_string_new (GIT_COMMAND " format-patch");

		g_string_append_printf (str, " %s -1", 
					giggle_revision_get_sha (priv->patch_format));
	} else {
		str = g_string_new (GIT_COMMAND " diff");

		if (priv->rev1) {
			g_string_append_printf (str, " %s", giggle_revision_get_sha (priv->rev1));
		}
		
		if (priv->rev2) {
			g_string_append_printf (str, " %s", giggle_revision_get_sha (priv->rev2));
		}

		while (files) {
			g_string_append_printf (str, " %s", (gchar *) files->data);
			files = files->next;
		}
	}

	*command_line = g_string_free (str, FALSE);
	return TRUE;
}

static void
git_diff_handle_output (GiggleJob   *job,
			const gchar *output_str,
			gsize        output_len)
{
	GiggleGitDiffPriv *priv;

	priv = GET_PRIV (job);

	priv->result = g_strdup (output_str);
}

GiggleJob *
giggle_git_diff_new (void)
{
	return g_object_new (GIGGLE_TYPE_GIT_DIFF, NULL);
}

void
giggle_git_diff_set_revisions (GiggleGitDiff  *diff,
			       GiggleRevision *rev1,
			       GiggleRevision *rev2)
{
	g_return_if_fail (GIGGLE_IS_GIT_DIFF (diff));
	g_return_if_fail (!rev1 || GIGGLE_IS_REVISION (rev1));
	g_return_if_fail (!rev2 || GIGGLE_IS_REVISION (rev2));

	g_object_set (diff,
		      "revision1", rev1,
		      "revision2", rev2,
		      NULL);
}

void
giggle_git_diff_set_files (GiggleGitDiff *diff,
			   GList         *files)
{
	GiggleGitDiffPriv *priv;

	g_return_if_fail (GIGGLE_IS_GIT_DIFF (diff));

	priv = GET_PRIV (diff);

	if (priv->files) {
		g_warning ("You have the 'patch-format' property set to TRUE. "
			   "Use of the git-format-patch command does not allow specific files. "
			   "These files will be ignored while 'patch-format' is TRUE.");
	}

	g_object_set (diff,
		      "files", files,
		      NULL);
}

void
giggle_git_diff_set_patch_format (GiggleGitDiff  *diff,
				  GiggleRevision *rev)
{
	GiggleGitDiffPriv *priv;

	g_return_if_fail (GIGGLE_IS_GIT_DIFF (diff));
	g_return_if_fail (GIGGLE_IS_REVISION (rev));

	priv = GET_PRIV (diff);

	if (priv->files) {
		g_warning ("Use of the git-format-patch command does not allow specific files. "
			   "You have files set for this GiggleGitDiff which will be ignored.");
	}

	g_object_set (diff,
		      "patch-format", rev,
		      NULL);
}

GiggleRevision *
giggle_git_diff_get_patch_format (GiggleGitDiff *diff)
{
	GiggleGitDiffPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_DIFF (diff), NULL);

	priv = GET_PRIV (diff);

	return priv->patch_format;
}

const gchar *
giggle_git_diff_get_result (GiggleGitDiff *diff)
{
	GiggleGitDiffPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_DIFF (diff), NULL);

	priv = GET_PRIV (diff);

	return priv->result;
}
