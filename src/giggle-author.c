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

#include "giggle-author.h"

typedef struct GiggleAuthorPriv GiggleAuthorPriv;

struct GiggleAuthorPriv {
	gchar* string;
};

enum {
	PROP_0,
	PROP_STRING
};

static void     author_finalize            (GObject           *object);
static void     author_get_property        (GObject           *object,
					   guint              param_id,
					   GValue            *value,
					   GParamSpec        *pspec);
static void     author_set_property        (GObject           *object,
					   guint              param_id,
					   const GValue      *value,
					   GParamSpec        *pspec);

G_DEFINE_TYPE (GiggleAuthor, giggle_author, G_TYPE_OBJECT)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_AUTHOR, GiggleAuthorPriv))

static void
giggle_author_class_init (GiggleAuthorClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize     = author_finalize;
	object_class->get_property = author_get_property;
	object_class->set_property = author_set_property;

	g_object_class_install_property (object_class,
					 PROP_STRING,
					 g_param_spec_string ("string",
							      "Author String",
							      "The string describing the author",
							      NULL,
							      G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleAuthorPriv));
}

static void
giggle_author_init (GiggleAuthor *author)
{
	GiggleAuthorPriv *priv;

	priv = GET_PRIV (author);
}

static void
author_finalize (GObject *object)
{
	GiggleAuthorPriv *priv;

	priv = GET_PRIV (object);
	
	/* FIXME: Free object data */

	G_OBJECT_CLASS (giggle_author_parent_class)->finalize (object);
}

static void
author_get_property (GObject    *object,
		    guint       param_id,
		    GValue     *value,
		    GParamSpec *pspec)
{
	GiggleAuthorPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_STRING:
		g_value_set_string (value, priv->string);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
author_set_property (GObject      *object,
		    guint         param_id,
		    const GValue *value,
		    GParamSpec   *pspec)
{
	GiggleAuthorPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_STRING:
		g_free (priv->string);
		priv->string = g_value_dup_string (value);
		g_object_notify (object, "string");
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

GiggleAuthor *
giggle_author_new (const gchar *string)
{
	return g_object_new (GIGGLE_TYPE_AUTHOR, "string", string, NULL);
}

const gchar *
giggle_author_get_string (GiggleAuthor* self)
{
	g_return_val_if_fail (GIGGLE_IS_AUTHOR (self), NULL);

	return GET_PRIV(self)->string;
}

