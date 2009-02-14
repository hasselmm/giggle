/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2008 Mathias Hasselmann
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
#include "giggle-git-cat-file.h"

typedef struct GiggleGitCatFilePriv GiggleGitCatFilePriv;

struct GiggleGitCatFilePriv {
	char  *contents;
	gsize  length;
	char  *type;
	char  *sha;
};

G_DEFINE_TYPE (GiggleGitCatFile, giggle_git_cat_file, GIGGLE_TYPE_JOB)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_CAT_FILE, GiggleGitCatFilePriv))

enum {
	PROP_0,
	PROP_TYPE,
	PROP_SHA,
};


static void
git_cat_file_finalize (GObject *object)
{
	GiggleGitCatFilePriv *priv;

	priv = GET_PRIV (object);

	g_free (priv->contents);
	g_free (priv->type);
	g_free (priv->sha);

	G_OBJECT_CLASS (giggle_git_cat_file_parent_class)->finalize (object);
}

static void
git_cat_file_get_property (GObject    *object,
			    guint       param_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	GiggleGitCatFilePriv *priv;

	priv = GET_PRIV (object);
	
	switch (param_id) {
	case PROP_TYPE:
		g_value_set_string (value, priv->type);
		break;

	case PROP_SHA:
		g_value_set_string (value, priv->sha);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
git_cat_file_set_property (GObject      *object,
			    guint         param_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	GiggleGitCatFilePriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_TYPE:
		g_assert (NULL == priv->type);
		priv->type = g_value_dup_string (value);
		break;

	case PROP_SHA:
		g_assert (NULL == priv->sha);
		priv->sha = g_value_dup_string (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
git_cat_file_get_command_line (GiggleJob *job, gchar **command_line)
{
	GiggleGitCatFilePriv *priv;

	priv = GET_PRIV (job);

	*command_line = g_strconcat (GIT_COMMAND " cat-file ",
				     priv->type, " ", priv->sha, NULL);

	return TRUE;
}

static void
git_cat_file_handle_output (GiggleJob   *job,
			    const gchar *output_str,
			    gsize        output_len)
{
	GiggleGitCatFilePriv *priv;

	priv = GET_PRIV (job);

	priv->contents = g_strdup (output_str);
	priv->length = output_len;
}

static void
giggle_git_cat_file_class_init (GiggleGitCatFileClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GiggleJobClass *job_class    = GIGGLE_JOB_CLASS (class);

	object_class->get_property  = git_cat_file_get_property;
	object_class->set_property  = git_cat_file_set_property;
	object_class->finalize      = git_cat_file_finalize;

	job_class->get_command_line = git_cat_file_get_command_line;
	job_class->handle_output    = git_cat_file_handle_output;

	g_object_class_install_property (object_class,
					 PROP_TYPE,
					 g_param_spec_string ("type",
							      "type",
							      "type of the file to retrieve",
							      NULL,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
					 PROP_SHA,
					 g_param_spec_string ("sha",
							      "sha",
							      "hash of the file to retrieve",
							      NULL,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private (object_class, sizeof (GiggleGitCatFilePriv));
}

static void
giggle_git_cat_file_init (GiggleGitCatFile *cat_file)
{
}

GiggleJob *
giggle_git_cat_file_new (const char *type,
			 const char *sha)
{
	g_return_val_if_fail (NULL != type, NULL);
	g_return_val_if_fail (NULL != sha, NULL);

	return g_object_new (GIGGLE_TYPE_GIT_CAT_FILE,
			     "type", type, "sha", sha, NULL);
}

const char *
giggle_git_cat_file_get_contents (GiggleGitCatFile *job,
				  gsize            *length)
{
	GiggleGitCatFilePriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_CAT_FILE (job), NULL);

	priv = GET_PRIV (job);

	if (length)
		*length = priv->length;

	return priv->contents;
}


