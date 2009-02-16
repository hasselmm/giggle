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

#include "config.h"
#include "giggle-author.h"

#include <string.h>

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_AUTHOR, GiggleAuthorPriv))

enum {
	PROP_0,
	PROP_NAME,
	PROP_EMAIL,
	PROP_STRING,
};

typedef struct {
	char* string;
	char* email;
	char* name;
} GiggleAuthorPriv;

G_DEFINE_TYPE (GiggleAuthor, giggle_author, G_TYPE_OBJECT)

static void
author_set_name (GiggleAuthorPriv *priv,
		 const char       *name)
{
	g_free (priv->name);
	g_free (priv->string);

	priv->name = g_strdup (name);
	priv->string = NULL;
}

static void
author_set_email (GiggleAuthorPriv *priv,
		  const char       *email)
{
	g_free (priv->email);
	g_free (priv->string);

	priv->email = g_strdup (email);
	priv->string = NULL;
}

static void
author_set_string (GiggleAuthorPriv *priv,
		   const char       *string)
{
	static GRegex *regex = NULL;
	GMatchInfo    *match = NULL;

	g_free (priv->name);
	g_free (priv->email);
	g_free (priv->string);

	priv->string = g_strdup (string);
	priv->email = priv->name = NULL;

	if (G_UNLIKELY (!regex)) {
		regex = g_regex_new ("^\\s*([^<]+?)?\\s*(?:<([^>]+)>)?\\s*$",
				     G_REGEX_OPTIMIZE, 0, NULL);
	}

	if (g_regex_match (regex, priv->string, 0, &match)) {
		priv->name  = g_match_info_fetch (match, 1);
		priv->email = g_match_info_fetch (match, 2);
	}

	g_match_info_free (match);
}

static void
author_set_property (GObject      *object,
		     guint         param_id,
		     const GValue *value,
		     GParamSpec   *pspec)
{
	GiggleAuthorPriv *priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_NAME:
		author_set_name (priv, g_value_get_string (value));
		break;

	case PROP_EMAIL:
		author_set_email (priv, g_value_get_string (value));
		break;

	case PROP_STRING:
		author_set_string (priv, g_value_get_string (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
author_get_property (GObject    *object,
		     guint       param_id,
		     GValue     *value,
		     GParamSpec *pspec)
{
	GiggleAuthorPriv *priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_NAME:
		g_value_set_string (value, priv->name);
		break;

	case PROP_EMAIL:
		g_value_set_string (value, priv->email);
		break;

	case PROP_STRING:
		g_value_set_string (value, priv->string);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
author_finalize (GObject *object)
{
	GiggleAuthorPriv *priv;

	priv = GET_PRIV (object);

	g_free (priv->string);
	g_free (priv->email);
	g_free (priv->name);

	G_OBJECT_CLASS (giggle_author_parent_class)->finalize (object);
}

static void
giggle_author_class_init (GiggleAuthorClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->set_property = author_set_property;
	object_class->get_property = author_get_property;
	object_class->finalize     = author_finalize;

	g_object_class_install_property (object_class, PROP_NAME,
					 g_param_spec_string ("name", "Name",
							      "The name of the author",
							      NULL, G_PARAM_READWRITE));

	g_object_class_install_property (object_class, PROP_EMAIL,
					 g_param_spec_string ("email", "Email",
							      "The email address of the author",
							      NULL, G_PARAM_READWRITE));

	g_object_class_install_property (object_class, PROP_STRING,
					 g_param_spec_string ("string", "Author String",
							      "The string describing the author",
							      NULL, G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleAuthorPriv));
}

static void
giggle_author_init (GiggleAuthor *author)
{
}

GiggleAuthor *
giggle_author_new_from_string (const char *string)
{
	g_return_val_if_fail (NULL != string, NULL);

	return g_object_new (GIGGLE_TYPE_AUTHOR,
			     "string", string, NULL);
}

GiggleAuthor *
giggle_author_new_from_name (const char *name,
			     const char *email)
{
	g_return_val_if_fail (NULL != name, NULL);

	return g_object_new (GIGGLE_TYPE_AUTHOR,
			     "name", name, "email", email, NULL);
}

void
giggle_author_set_email (GiggleAuthor *author,
			 char const   *email)
{
	g_return_if_fail (GIGGLE_IS_AUTHOR (author));
	g_object_set (author, "email", email, NULL);
}

const char *
giggle_author_get_email (GiggleAuthor* author)
{
	g_return_val_if_fail (GIGGLE_IS_AUTHOR (author), NULL);
	return GET_PRIV (author)->email;
}

void
giggle_author_set_name (GiggleAuthor *author,
			char const   *name)
{
	g_return_if_fail (GIGGLE_IS_AUTHOR (author));
	g_object_set (author, "name", name, NULL);
}

const char *
giggle_author_get_name (GiggleAuthor* author)
{
	g_return_val_if_fail (GIGGLE_IS_AUTHOR (author), NULL);
	return GET_PRIV (author)->name;
}

void
giggle_author_set_string (GiggleAuthor *author,
			  char const   *string)
{
	g_return_if_fail (GIGGLE_IS_AUTHOR (author));
	g_object_set (author, "string", string, NULL);
}

const char *
giggle_author_get_string (GiggleAuthor* author)
{
	GiggleAuthorPriv *priv = GET_PRIV (author);
	GString          *string;

	g_return_val_if_fail (GIGGLE_IS_AUTHOR (author), NULL);

	if (!priv->string) {
		string = g_string_new (NULL);

		if (priv->name)
			g_string_append (string, priv->name);

		if (priv->email) {
			if (string->len > 0)
				g_string_append_c (string, ' ');

			g_string_append_c (string, '<');
			g_string_append   (string, priv->email);
			g_string_append_c (string, '>');
		}

		if (string->len) {
			priv->string = g_string_free (string, FALSE);
		} else {
			g_string_free (string, TRUE);
		}
	}

	return priv->string;
}
