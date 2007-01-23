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

#include <string.h>

#include "giggle-git-branches.h"

typedef struct GiggleGitBranchesPriv GiggleGitBranchesPriv;

struct GiggleGitBranchesPriv {
	GList *branches;
};

static void     git_branches_finalize            (GObject           *object);
static void     git_branches_get_property        (GObject           *object,
						  guint              param_id,
						  GValue            *value,
						  GParamSpec        *pspec);
static void     git_branches_set_property        (GObject           *object,
						  guint              param_id,
						  const GValue      *value,
						  GParamSpec        *pspec);

static gboolean git_branches_get_command_line    (GiggleJob         *job,
						  gchar            **command_line);
static void     git_branches_handle_output       (GiggleJob         *job,
						  const gchar       *output_str,
						  gsize              output_len);


G_DEFINE_TYPE (GiggleGitBranches, giggle_git_branches, GIGGLE_TYPE_JOB)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_BRANCHES, GiggleGitBranchesPriv))

static void
giggle_git_branches_class_init (GiggleGitBranchesClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GiggleJobClass *job_class    = GIGGLE_JOB_CLASS (class);

	object_class->finalize     = git_branches_finalize;
	object_class->get_property = git_branches_get_property;
	object_class->set_property = git_branches_set_property;

	job_class->get_command_line = git_branches_get_command_line;
	job_class->handle_output    = git_branches_handle_output;

	g_type_class_add_private (object_class, sizeof (GiggleGitBranchesPriv));
}

static void
giggle_git_branches_init (GiggleGitBranches *branches)
{
}

static void
git_branches_finalize (GObject *object)
{
	GiggleGitBranchesPriv *priv;

	priv = GET_PRIV (object);

	g_list_foreach (priv->branches, (GFunc) g_object_unref, NULL);
	g_list_free (priv->branches);

	G_OBJECT_CLASS (giggle_git_branches_parent_class)->finalize (object);
}

static void
git_branches_get_property (GObject    *object,
			   guint       param_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
	GiggleGitBranchesPriv *priv;

	priv = GET_PRIV (object);
	
	switch (param_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
git_branches_set_property (GObject      *object,
			   guint         param_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
	GiggleGitBranchesPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
git_branches_get_command_line (GiggleJob *job, gchar **command_line)
{
	*command_line = g_strdup_printf ("git show-ref --heads");
	return TRUE;
}

static void
git_branches_handle_output (GiggleJob   *job,
			    const gchar *output_str,
			    gsize        output_len)
{
}

GiggleJob *
giggle_git_branches_new (void)
{
	return g_object_new (GIGGLE_TYPE_GIT_BRANCHES, NULL);
}

GList *
giggle_git_branches_get_branches (GiggleGitBranches *branches)
{
	GiggleGitBranchesPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_BRANCHES (branches), NULL);

	priv = GET_PRIV (branches);

	return priv->branches;
}
