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

#include "giggle-git-add-ref.h"
#include "giggle-branch.h"
#include "giggle-tag.h"
#include "giggle-revision.h"

typedef struct GiggleGitAddRefPriv GiggleGitAddRefPriv;

struct GiggleGitAddRefPriv {
	GiggleRevision *revision;
	GiggleRef      *ref;
};

static void     git_add_ref_finalize            (GObject           *object);
static void     git_add_ref_get_property        (GObject           *object,
						 guint              param_id,
						 GValue            *value,
						 GParamSpec        *pspec);
static void     git_add_ref_set_property        (GObject           *object,
						 guint              param_id,
						 const GValue      *value,
						 GParamSpec        *pspec);

static gboolean git_add_ref_get_command_line    (GiggleJob         *job,
						 gchar            **command_line);


G_DEFINE_TYPE (GiggleGitAddRef, giggle_git_add_ref, GIGGLE_TYPE_JOB)

enum {
	PROP_0,
	PROP_REF,
	PROP_REVISION,
};

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_ADD_REF, GiggleGitAddRefPriv))


static void
giggle_git_add_ref_class_init (GiggleGitAddRefClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GiggleJobClass *job_class    = GIGGLE_JOB_CLASS (class);

	object_class->finalize     = git_add_ref_finalize;
	object_class->get_property = git_add_ref_get_property;
	object_class->set_property = git_add_ref_set_property;

	job_class->get_command_line = git_add_ref_get_command_line;

	g_object_class_install_property (
		object_class,
		PROP_REF,
		g_param_spec_object ("ref",
				     "Ref",
				     "Reference to create",
				     GIGGLE_TYPE_REF,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (
		object_class,
		PROP_REVISION,
		g_param_spec_object ("revision",
				     "Revision",
				     "Base revision for the ref",
				     GIGGLE_TYPE_REVISION,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private (object_class, sizeof (GiggleGitAddRefPriv));
}

static void
giggle_git_add_ref_init (GiggleGitAddRef *add_ref)
{
}

static void
git_add_ref_finalize (GObject *object)
{
	GiggleGitAddRefPriv *priv;

	priv = GET_PRIV (object);

	g_object_unref (priv->ref);
	g_object_unref (priv->revision);

	G_OBJECT_CLASS (giggle_git_add_ref_parent_class)->finalize (object);
}

static void
git_add_ref_get_property (GObject    *object,
			  guint       param_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
	GiggleGitAddRefPriv *priv;

	priv = GET_PRIV (object);
	
	switch (param_id) {
	case PROP_REF:
		g_value_set_object (value, priv->ref);
		break;
	case PROP_REVISION:
		g_value_set_object (value, priv->revision);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
git_add_ref_set_property (GObject      *object,
			  guint         param_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
	GiggleGitAddRefPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_REF:
		if (priv->ref) {
			g_object_unref (priv->ref);
		}
#if GLIB_MAJOR_VERSION > 2 || GLIB_MINOR_VERSION > 12
		priv->ref = g_value_dup_object (value);
#else
		priv->ref = GIGGLE_REF (g_value_dup_object (value));
#endif
		break;
	case PROP_REVISION:
		if (priv->revision) {
			g_object_unref (priv->revision);
		}
#if GLIB_MAJOR_VERSION > 2 || GLIB_MINOR_VERSION > 12
		priv->revision = g_value_dup_object (value);
#else
		priv->revision = GIGGLE_REVISION (g_value_dup_object (value));
#endif
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
git_add_ref_get_command_line (GiggleJob *job, gchar **command_line)
{
	GiggleGitAddRefPriv *priv;

	priv = GET_PRIV (job);

	if (GIGGLE_IS_BRANCH (priv->ref)) {
		*command_line = g_strdup_printf (GIT_COMMAND " branch %s %s",
						 giggle_ref_get_name (priv->ref),
						 giggle_revision_get_sha (priv->revision));
	} else {
		*command_line = g_strdup_printf (GIT_COMMAND " tag -a -m \"Tagged %s\" %s %s",
						 giggle_ref_get_name (priv->ref),
						 giggle_ref_get_name (priv->ref),
						 giggle_revision_get_sha (priv->revision));
	}

	return TRUE;
}

GiggleJob *
giggle_git_add_ref_new (GiggleRef      *ref,
			GiggleRevision *revision)
{
	g_return_val_if_fail (GIGGLE_IS_REF (ref), NULL);
	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);

	return g_object_new (GIGGLE_TYPE_GIT_ADD_REF,
			     "ref", ref,
			     "revision", revision,
			     NULL);
}
