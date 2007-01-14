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

#include "giggle-git-revisions.h"

typedef struct GiggleGitRevisionsPriv GiggleGitRevisionsPriv;

struct GiggleGitRevisionsPriv {
	GList *list;
};

static void     git_revisions_finalize            (GObject           *object);
static void     git_revisions_get_property        (GObject           *object,
						   guint              param_id,
						   GValue            *value,
						   GParamSpec        *pspec);
static void     git_revisions_set_property        (GObject           *object,
						   guint              param_id,
						   const GValue      *value,
						   GParamSpec        *pspec);

static gboolean git_revisions_get_command_line    (GiggleJob         *job,
						   gchar            **command_line);
static void     git_revisions_handle_output       (GiggleJob         *job,
						   const gchar       *output_str,
						   gsize              output_len);

G_DEFINE_TYPE (GiggleGitRevisions, giggle_git_revisions, GIGGLE_TYPE_JOB);

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_REVISIONS, GiggleGitRevisionsPriv))

static void
giggle_git_revisions_class_init (GiggleGitRevisionsClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GiggleJobClass *job_class    = GIGGLE_JOB_CLASS (class);

	object_class->finalize     = git_revisions_finalize;
	object_class->get_property = git_revisions_get_property;
	object_class->set_property = git_revisions_set_property;

	job_class->get_command_line = git_revisions_get_command_line;
	job_class->handle_output    = git_revisions_handle_output;
#if 0
	g_object_class_install_property (object_class,
					 PROP_MY_PROP,
					 g_param_spec_string ("my-prop",
							      "My Prop",
							      "Describe the property",
							      NULL,
							      G_PARAM_READABLE));
#endif

	g_type_class_add_private (object_class, sizeof (GiggleGitRevisionsPriv));
}

static void
giggle_git_revisions_init (GiggleGitRevisions *revisions)
{
	GiggleGitRevisionsPriv *priv;

	priv = GET_PRIV (revisions);
}

static void
git_revisions_finalize (GObject *object)
{
	GiggleGitRevisionsPriv *priv;

	priv = GET_PRIV (object);

	/* FIXME: Free object data */

	G_OBJECT_CLASS (giggle_git_revisions_parent_class)->finalize (object);
}

static void
git_revisions_get_property (GObject    *object,
			    guint       param_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	GiggleGitRevisionsPriv *priv;

	priv = GET_PRIV (object);
	
	switch (param_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
git_revisions_set_property (GObject      *object,
			    guint         param_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	GiggleGitRevisionsPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
git_revisions_get_command_line (GiggleJob *job, gchar **command_line)
{
	GiggleGitRevisionsPriv *priv;

	priv = GET_PRIV (job);

	*command_line = g_strdup_printf ("git FIXME");

	return TRUE;
}

static void
git_revisions_handle_output (GiggleJob   *job,
			     const gchar *output_str,
			     gsize        output_len)
{
	GiggleGitRevisionsPriv *priv;

	priv = GET_PRIV (job);

	/* FIXME: Implement */
}

GiggleJob *
giggle_git_revisions_new (void)
{
	GiggleGitRevisions     *revisions;
	GiggleGitRevisionsPriv *priv;

	revisions = g_object_new (GIGGLE_TYPE_GIT_REVISIONS, NULL);
	priv = GET_PRIV (revisions);

	return GIGGLE_JOB (revisions);
}

GList *
giggle_git_revisions_get_list (GiggleGitRevisions *revisions)
{
	GiggleGitRevisionsPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_REVISIONS (revisions), NULL);

	priv = GET_PRIV (revisions);

	return priv->list;
}

