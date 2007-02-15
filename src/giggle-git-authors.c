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

#include "giggle-git-authors.h"

typedef struct GiggleGitAuthorsPriv GiggleGitAuthorsPriv;

struct GiggleGitAuthorsPriv {
	guint i;
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

G_DEFINE_TYPE (GiggleGitAuthors, giggle_git_authors, G_TYPE_OBJECT)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_AUTHORS, GiggleGitAuthorsPriv))

static void
giggle_git_authors_class_init (GiggleGitAuthorsClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize     = git_authors_finalize;
	object_class->get_property = git_authors_get_property;
	object_class->set_property = git_authors_set_property;

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
	
	/* FIXME: Free object data */

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

