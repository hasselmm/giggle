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

G_DEFINE_TYPE (GiggleGitDiff, giggle_git_diff, GIGGLE_TYPE_JOB);

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_DIFF, GiggleGitDiffPriv))

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
#if 0
	g_object_class_install_property (object_class,
					 PROP_MY_PROP,
					 g_param_spec_string ("my-prop",
							      "My Prop",
							      "Describe the property",
							      NULL,
							      G_PARAM_READABLE));
#endif

	g_type_class_add_private (object_class, sizeof (GiggleGitDiffPriv));
}

static void
giggle_git_diff_init (GiggleGitDiff *dummy)
{
	GiggleGitDiffPriv *priv;

	priv = GET_PRIV (dummy);

	priv->rev1 = NULL;
	priv->rev2 = NULL;
	priv->result = NULL;
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
		g_object_unref (priv->rev1);
	}

	g_free (priv->result);

	/* FIXME: Free object data */

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
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
git_diff_get_command_line (GiggleJob *job, gchar **command_line)
{
	GiggleGitDiffPriv *priv;

	priv = GET_PRIV (job);

	*command_line = g_strdup_printf ("git diff %s %s",
					 giggle_revision_get_sha (priv->rev1),
					 giggle_revision_get_sha (priv->rev2));

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

GiggleGitDiff *
giggle_git_diff_new (GiggleRevision *rev1, GiggleRevision *rev2)
{
	GiggleGitDiff     *diff;
	GiggleGitDiffPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION (rev1), NULL);
	g_return_val_if_fail (GIGGLE_IS_REVISION (rev2), NULL);

	diff = g_object_new (GIGGLE_TYPE_GIT_DIFF, NULL);
	priv = GET_PRIV (diff);

	priv->rev1 = g_object_ref (rev1);
	priv->rev2 = g_object_ref (rev2);

	return diff;
}

const gchar *
giggle_git_diff_get_result (GiggleGitDiff *diff)
{
	GiggleGitDiffPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_DIFF (diff), NULL);

	priv = GET_PRIV (diff);

	return priv->result;
}

