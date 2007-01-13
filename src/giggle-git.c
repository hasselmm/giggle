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

#include "giggle-dispatcher.h"
#include "giggle-git.h"

typedef struct GiggleGitPriv GiggleGitPriv;

struct GiggleGitPriv {
	GiggleDispatcher *dispatcher;
	gchar            *directory;
};

static void  git_finalize            (GObject           *object);
static void  git_get_property        (GObject           *object,
				      guint              param_id,
				      GValue            *value,
				      GParamSpec        *pspec);
static void  git_set_property        (GObject           *object,
				      guint              param_id,
				      const GValue      *value,
				      GParamSpec        *pspec);
static gboolean git_verify_directory (GiggleGit         *git,
				      const gchar       *directory,
				      GError           **error);

G_DEFINE_TYPE (GiggleGit, giggle_git, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_DIRECTORY
};

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT, GiggleGitPriv))

static void
giggle_git_class_init (GiggleGitClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = git_finalize;
	object_class->get_property = git_get_property;
	object_class->set_property = git_set_property;

	g_object_class_install_property (object_class,
					 PROP_DIRECTORY,
					 g_param_spec_string ("directory",
							      "Directory",
							      "The Git repository path",
							      NULL,
							      G_PARAM_READABLE));

	g_type_class_add_private (object_class, sizeof (GiggleGitPriv));
}

static void
giggle_git_init (GiggleGit *git)
{
	GiggleGitPriv *priv;

	priv = GET_PRIV (git);

	priv->directory = NULL;
	priv->dispatcher = giggle_dispatcher_new ();
}

static void
git_finalize (GObject *object)
{
	GiggleGitPriv *priv;

	priv = GET_PRIV (object);

	g_free (priv->directory);

	g_object_unref (priv->dispatcher);

	G_OBJECT_CLASS (giggle_git_parent_class)->finalize (object);
}

static void
git_get_property (GObject    *object,
		  guint       param_id,
		  GValue     *value,
		  GParamSpec *pspec)
{
	GiggleGitPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_DIRECTORY:
		g_value_set_string (value, priv->directory);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
git_set_property (GObject      *object,
		  guint         param_id,
		  const GValue *value,
		  GParamSpec   *pspec)
{
	GiggleGitPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean 
git_verify_directory (GiggleGit    *git,
		      const gchar  *directory,
		      GError      **error)
{
	/* FIXME: Do some funky stuff to verify that it's a valid GIT repo */
	return TRUE;
}

GiggleGit *
giggle_git_new (void)
{
	return g_object_new (GIGGLE_TYPE_GIT, NULL);
}

const gchar *
giggle_git_get_directory (GiggleGit *git)
{
	GiggleGitPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT (git), NULL);

	priv = GET_PRIV (git);

	return priv->directory;
}

gboolean
giggle_git_set_directory (GiggleGit    *git, 
			  const gchar  *directory,
			  GError      **error)
{
	GiggleGitPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT (git), FALSE);
	g_return_val_if_fail (directory != NULL, FALSE);

	priv = GET_PRIV (git);

	if (!git_verify_directory (git, directory, error)) {
		return FALSE;
	}

	g_free (priv->directory);
	priv->directory = g_strdup (directory);

	g_object_notify (G_OBJECT (git), "directory");

	return TRUE;
}

