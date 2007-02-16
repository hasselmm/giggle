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

#include "giggle-git-authors.h"
#include "giggle-author.h"

typedef struct GiggleGitAuthorsPriv GiggleGitAuthorsPriv;

struct GiggleGitAuthorsPriv {
	GList *authors;
};

static void     git_authors_finalize            (GObject           *object);
static void     git_authors_get_property        (GObject           *object,
					   guint              param_id,
					   GValue            *value,
					   GParamSpec        *pspec);
static void     git_authors_set_property        (GObject           *object,
					   guint              param_id,
					   const GValue      *value,
					   GParamSpec        *pspec);
static gboolean authors_get_command_line  (GiggleJob         *job,
					   gchar            **command_line);
static void     authors_handle_output     (GiggleJob         *job,
					   const gchar       *output,
					   gsize              length);

G_DEFINE_TYPE (GiggleGitAuthors, giggle_git_authors, GIGGLE_TYPE_JOB)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_AUTHORS, GiggleGitAuthorsPriv))

static void
giggle_git_authors_class_init (GiggleGitAuthorsClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GiggleJobClass *job_class  = GIGGLE_JOB_CLASS (class);

	object_class->finalize     = git_authors_finalize;
	object_class->get_property = git_authors_get_property;
	object_class->set_property = git_authors_set_property;

	job_class->get_command_line = authors_get_command_line;
	job_class->handle_output    = authors_handle_output;

#if 0
	g_object_class_install_property (object_class,
					 PROP_MY_PROP,
					 g_param_spec_string ("my-prop",
							      "My Prop",
							      "Describe the property",
							      NULL,
							      G_PARAM_READABLE));
#endif

	g_type_class_add_private (object_class, sizeof (GiggleGitAuthorsPriv));
}

static void
giggle_git_authors_init (GiggleGitAuthors *git_authors)
{
	GiggleGitAuthorsPriv *priv;

	priv = GET_PRIV (git_authors);
}

static void
git_authors_finalize (GObject *object)
{
	GiggleGitAuthorsPriv *priv;

	priv = GET_PRIV (object);
	
	g_list_foreach (priv->authors, (GFunc) g_object_unref, NULL);
	g_list_free (priv->authors);
	priv->authors = NULL;

	G_OBJECT_CLASS (giggle_git_authors_parent_class)->finalize (object);
}

static void
git_authors_get_property (GObject    *object,
		    guint       param_id,
		    GValue     *value,
		    GParamSpec *pspec)
{
	GiggleGitAuthorsPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
git_authors_set_property (GObject      *object,
		    guint         param_id,
		    const GValue *value,
		    GParamSpec   *pspec)
{
	GiggleGitAuthorsPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
authors_get_command_line (GiggleJob *job,
			  gchar    **command_line)
{
	*command_line = g_strdup ("git log");
	return TRUE;
}

static void
authors_handle_output (GiggleJob   *job,
		       const gchar *output,
		       gsize        length)
{
	GiggleGitAuthorsPriv *priv;
	GList                *authors;
	gchar               **lines;
	gchar               **line;

	priv = GET_PRIV (job);

	lines = g_strsplit (output, "\n", -1);

	authors = NULL;
	for (line = lines; line && *line; line++) {
		if (g_str_has_prefix (*line, "Author: ")) {
			authors = g_list_prepend (authors, giggle_author_new (*line + strlen ("Author: ")));
		}
	}

	g_list_foreach (priv->authors, (GFunc)g_object_unref, NULL);
	g_list_free (priv->authors);

	priv->authors = g_list_reverse (authors);
	g_strfreev (lines);
}

GiggleJob *
giggle_git_authors_new (void)
{
	return g_object_new (GIGGLE_TYPE_GIT_AUTHORS, NULL);
}

GList *
giggle_git_authors_get_list (GiggleGitAuthors *authors)
{
	GiggleGitAuthorsPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_AUTHORS (authors), NULL);

	priv = GET_PRIV (authors);

	return priv->authors;
}

