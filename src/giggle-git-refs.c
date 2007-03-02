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

#include "giggle-git-refs.h"
#include "giggle-ref.h"
#include "giggle-enums.h"

typedef struct GiggleGitRefsPriv GiggleGitRefsPriv;

struct GiggleGitRefsPriv {
	GiggleGitRefType  type;
	GList            *refs;
};

static void     git_refs_finalize            (GObject           *object);
static void     git_refs_get_property        (GObject           *object,
					      guint              param_id,
					      GValue            *value,
					      GParamSpec        *pspec);
static void     git_refs_set_property        (GObject           *object,
					      guint              param_id,
					      const GValue      *value,
					      GParamSpec        *pspec);

static gboolean git_refs_get_command_line    (GiggleJob         *job,
					      gchar            **command_line);
static void     git_refs_handle_output       (GiggleJob         *job,
					      const gchar       *output_str,
					      gsize              output_len);


G_DEFINE_TYPE (GiggleGitRefs, giggle_git_refs, GIGGLE_TYPE_JOB)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_REFS, GiggleGitRefsPriv))

enum {
	PROP_0,
	PROP_TYPE
};

static void
giggle_git_refs_class_init (GiggleGitRefsClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GiggleJobClass *job_class    = GIGGLE_JOB_CLASS (class);

	object_class->finalize     = git_refs_finalize;
	object_class->get_property = git_refs_get_property;
	object_class->set_property = git_refs_set_property;

	job_class->get_command_line = git_refs_get_command_line;
	job_class->handle_output    = git_refs_handle_output;

	g_object_class_install_property (
		object_class,
		PROP_TYPE,
		g_param_spec_enum ("type",
				   "Type",
				   "Type",
				   GIGGLE_TYPE_GIT_REF_TYPE,
				   GIGGLE_GIT_REF_TYPE_BRANCH,
				   G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private (object_class, sizeof (GiggleGitRefsPriv));
}

static void
giggle_git_refs_init (GiggleGitRefs *refs)
{
}

static void
git_refs_finalize (GObject *object)
{
	GiggleGitRefsPriv *priv;

	priv = GET_PRIV (object);

	g_list_foreach (priv->refs, (GFunc) g_object_unref, NULL);
	g_list_free (priv->refs);

	G_OBJECT_CLASS (giggle_git_refs_parent_class)->finalize (object);
}

static void
git_refs_get_property (GObject    *object,
		       guint       param_id,
		       GValue     *value,
		       GParamSpec *pspec)
{
	GiggleGitRefsPriv *priv;

	priv = GET_PRIV (object);
	
	switch (param_id) {
	case PROP_TYPE:
		g_value_set_enum (value, priv->type);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
git_refs_set_property (GObject      *object,
		       guint         param_id,
		       const GValue *value,
		       GParamSpec   *pspec)
{
	GiggleGitRefsPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_TYPE:
		priv->type = g_value_get_enum (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
git_refs_get_command_line (GiggleJob *job, gchar **command_line)
{
	GiggleGitRefsPriv *priv;

	priv = GET_PRIV (job);

	*command_line = g_strdup_printf ("git show-ref %s",
					 (priv->type == GIGGLE_GIT_REF_TYPE_BRANCH) ?
					 "--heads" : "--tags");
	return TRUE;
}

static GiggleRef *
git_refs_get_ref (GiggleJob   *job,
		  const gchar *str)
{
	GiggleGitRefsPriv  *priv;
	GiggleRef          *ref;
	gchar             **data;
	gint                len;

	priv = GET_PRIV (job);

	len = (priv->type == GIGGLE_GIT_REF_TYPE_BRANCH) ?
		strlen ("refs/heads/") :
		strlen ("refs/tags/");

	data = g_strsplit (str, " ", 2);
	/* FIXME: use sha to set head revision */
	ref = giggle_ref_new (data[1] + len);
	g_strfreev (data);

	return ref;
}

static void
git_refs_handle_output (GiggleJob   *job,
			const gchar *output_str,
			gsize        output_len)
{
	GiggleGitRefsPriv  *priv;
	gchar             **lines;
	gint                n_line = 0;

	priv = GET_PRIV (job);
	lines = g_strsplit (output_str, "\n", -1);

	while (lines[n_line] && *lines[n_line]) {
		priv->refs =
			g_list_prepend (priv->refs,
					git_refs_get_ref (job, lines[n_line]));
		n_line++;
	}

	priv->refs = g_list_reverse (priv->refs);
	g_strfreev (lines);
}

GiggleJob *
giggle_git_refs_new (GiggleGitRefType type)
{
	return g_object_new (GIGGLE_TYPE_GIT_REFS,
			     "type", type,
			     NULL);
}

GList *
giggle_git_refs_get_refs (GiggleGitRefs *refs)
{
	GiggleGitRefsPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_REFS (refs), NULL);

	priv = GET_PRIV (refs);

	return priv->refs;
}
