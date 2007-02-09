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
#include <glib.h>
#include <fnmatch.h>

#include "giggle-git-ignore.h"

typedef struct GiggleGitIgnorePriv GiggleGitIgnorePriv;

struct GiggleGitIgnorePriv {
	gchar  *directory_path;
	gchar **globs;
};

static void       git_ignore_finalize           (GObject               *object);
static void       git_ignore_get_property       (GObject               *object,
						 guint                  param_id,
						 GValue                *value,
						 GParamSpec            *pspec);
static void       git_ignore_set_property       (GObject               *object,
						 guint                  param_id,
						 const GValue          *value,
						 GParamSpec            *pspec);
static GObject*   git_ignore_constructor        (GType                  type,
						 guint                  n_construct_properties,
						 GObjectConstructParam *construct_params);


G_DEFINE_TYPE (GiggleGitIgnore, giggle_git_ignore, G_TYPE_OBJECT);

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_IGNORE, GiggleGitIgnorePriv))

enum {
	PROP_0,
	PROP_DIRECTORY,
};

static void
giggle_git_ignore_class_init (GiggleGitIgnoreClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = git_ignore_finalize;
	object_class->get_property = git_ignore_get_property;
	object_class->set_property = git_ignore_set_property;
	object_class->constructor = git_ignore_constructor;

	g_object_class_install_property (object_class,
					 PROP_DIRECTORY,
					 g_param_spec_string ("directory",
							      "Directory",
							      "Path to the Directory containing the .gitignore file",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private (object_class, sizeof (GiggleGitIgnorePriv));
}

static void
giggle_git_ignore_init (GiggleGitIgnore *git_ignore)
{
}

static void
git_ignore_finalize (GObject *object)
{
	GiggleGitIgnorePriv *priv;

	priv = GET_PRIV (object);

	g_free (priv->directory_path);

	if (priv->globs) {
		g_strfreev (priv->globs);
	}

	G_OBJECT_CLASS (giggle_git_ignore_parent_class)->finalize (object);
}

static void
git_ignore_get_property (GObject    *object,
			 guint       param_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	GiggleGitIgnorePriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_DIRECTORY:
		g_value_set_string (value, priv->directory_path);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
git_ignore_set_property (GObject      *object,
			 guint         param_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	GiggleGitIgnorePriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_DIRECTORY:
		priv->directory_path = g_value_dup_string (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static GObject*
git_ignore_constructor (GType                  type,
			guint                  n_construct_properties,
			GObjectConstructParam *construct_params)
{
	GObject             *object;
	GiggleGitIgnorePriv *priv;
	gchar               *dot_git_path, *contents;

	object = (* G_OBJECT_CLASS (giggle_git_ignore_parent_class)->constructor) (type,
										   n_construct_properties,
										   construct_params);
	priv = GET_PRIV (object);
	dot_git_path = g_strconcat (priv->directory_path, G_DIR_SEPARATOR_S ".gitignore", NULL);

	if (g_file_get_contents (dot_git_path,
				 &contents, NULL, NULL)) {
		priv->globs = g_strsplit (contents, "\n", -1);
		g_free (contents);
	}

	g_free (dot_git_path);

	return object;
}

GiggleGitIgnore *
giggle_git_ignore_new (const gchar *directory_path)
{
	g_return_val_if_fail (directory_path != NULL, NULL);

	return g_object_new (GIGGLE_TYPE_GIT_IGNORE,
			     "directory", directory_path,
			     NULL);
}

gboolean
giggle_git_ignore_name_matches (GiggleGitIgnore *git_ignore,
				const gchar     *name)
{
	GiggleGitIgnorePriv *priv;
	gint                 n_glob = 0;

	g_return_val_if_fail (GIGGLE_IS_GIT_IGNORE (git_ignore), FALSE);

	priv = GET_PRIV (git_ignore);

	if (!priv->globs) {
		return FALSE;
	}

	while (priv->globs[n_glob]) {
		if (fnmatch (priv->globs[n_glob], name, 0) == 0) {
			return TRUE;
		}
		n_glob++;
	}

	return FALSE;
}
