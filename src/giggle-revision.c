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
#include <gtk/gtk.h>
#include <time.h>

#include "giggle-revision.h"

typedef struct GiggleRevisionPriv GiggleRevisionPriv;

struct GiggleRevisionPriv {
	gchar              *sha;
	gchar              *author;
	struct tm          *date;
	gchar              *short_log;
	gchar              *long_log;

	GList              *parents;
	GList              *children;
};

static void revision_finalize           (GObject        *object);
static void revision_get_property       (GObject        *object,
					 guint           param_id,
					 GValue         *value,
					 GParamSpec     *pspec);
static void revision_set_property       (GObject        *object,
					 guint           param_id,
					 const GValue   *value,
					 GParamSpec     *pspec);

G_DEFINE_TYPE (GiggleRevision, giggle_revision, G_TYPE_OBJECT)

enum {
	PROP_0,
	PROP_SHA,
	PROP_AUTHOR,
	PROP_DATE,
	PROP_SHORT_LOG,
	PROP_LONG_LOG
};

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REVISION, GiggleRevisionPriv))

static void
giggle_revision_class_init (GiggleRevisionClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize     = revision_finalize;
	object_class->get_property = revision_get_property;
	object_class->set_property = revision_set_property;

	g_object_class_install_property (
		object_class,
		PROP_SHA,
		g_param_spec_string ("sha",
				     "SHA",
				     "SHA hash of the revision",
				     NULL,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (
		object_class,
		PROP_AUTHOR,
		g_param_spec_string ("author",
				     "Author",
				     "Author of the revision",
				     NULL,
				     G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_DATE,
		g_param_spec_pointer ("date",
				      "Date",
				      "Date of the revision",
				      G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_SHORT_LOG,
		g_param_spec_string ("short-log",
				     "Short log",
				     "Short log of the revision",
				     NULL,
				     G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_LONG_LOG,
		g_param_spec_string ("long-log",
				     "Long log",
				     "Long log of the revision",
				     NULL,
				     G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleRevisionPriv));
}

static void
giggle_revision_init (GiggleRevision *revision)
{
}

static void
revision_finalize (GObject *object)
{
	GiggleRevision     *revision;
	GiggleRevisionPriv *priv;

	revision = GIGGLE_REVISION (object);

	priv = GET_PRIV (revision);

	g_free (priv->sha);
	g_free (priv->author);
	g_free (priv->short_log);
	g_free (priv->long_log);
	
	if (priv->date) {
		g_free (priv->date);
	}

	G_OBJECT_CLASS (giggle_revision_parent_class)->finalize (object);
}

static void
revision_get_property (GObject    *object,
		       guint       param_id,
		       GValue     *value,
		       GParamSpec *pspec)
{
	GiggleRevisionPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_SHA:
		g_value_set_string (value, priv->sha);
		break;
	case PROP_AUTHOR:
		g_value_set_string (value, priv->author);
		break;
	case PROP_DATE:
		g_value_set_pointer (value, priv->date);
		break;
	case PROP_SHORT_LOG:
		g_value_set_string (value, priv->short_log);
		break;
	case PROP_LONG_LOG:
		g_value_set_string (value, priv->long_log);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
revision_set_property (GObject      *object,
		       guint         param_id,
		       const GValue *value,
		       GParamSpec   *pspec)
{
	GiggleRevisionPriv *priv;
	
	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_SHA:
		g_free (priv->sha);
		priv->sha = g_strdup (g_value_get_string (value));
		break;
	case PROP_AUTHOR:
		g_free (priv->author);
		priv->author = g_strdup (g_value_get_string (value));
		break;
	case PROP_DATE:
		if (priv->date) {
			g_free (priv->date);
		}
		priv->date = g_value_get_pointer (value);
		break;
	case PROP_SHORT_LOG:
		g_free (priv->short_log);
		priv->short_log = g_strdup (g_value_get_string (value));
		break;
	case PROP_LONG_LOG:
		g_free (priv->long_log);
		priv->long_log = g_strdup (g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

GiggleRevision *
giggle_revision_new (const gchar *sha)
{
	return g_object_new (GIGGLE_TYPE_REVISION,
			     "sha", sha,
			     NULL);
}

const gchar *
giggle_revision_get_sha (GiggleRevision *revision)
{
	GiggleRevisionPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);

	priv = GET_PRIV (revision);
	
	return priv->sha;
}

const gchar *
giggle_revision_get_author (GiggleRevision *revision)
{
	GiggleRevisionPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);

	priv = GET_PRIV (revision);
	
	return priv->author;
}

const struct tm *
giggle_revision_get_date (GiggleRevision *revision)
{
	GiggleRevisionPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);

	priv = GET_PRIV (revision);
	
	return priv->date;
}

const gchar *
giggle_revision_get_short_log (GiggleRevision *revision)
{
	GiggleRevisionPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);

	priv = GET_PRIV (revision);
	
	return priv->short_log;
}

const gchar *
giggle_revision_get_long_log  (GiggleRevision *revision)
{
	GiggleRevisionPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);

	priv = GET_PRIV (revision);
	
	return priv->long_log;
}

static void
giggle_revision_add_child (GiggleRevision *revision,
			   GiggleRevision *child)
{
	GiggleRevisionPriv *priv;

	g_return_if_fail (GIGGLE_IS_REVISION (revision));
	g_return_if_fail (GIGGLE_IS_REVISION (child));

	priv = GET_PRIV (revision);

	priv->children = g_list_prepend (priv->children, child);
}

static void
giggle_revision_remove_child (GiggleRevision *revision,
			      GiggleRevision *child)
{
	GiggleRevisionPriv *priv;

	g_return_if_fail (GIGGLE_IS_REVISION (revision));
	g_return_if_fail (GIGGLE_IS_REVISION (child));

	priv = GET_PRIV (revision);

	/* the child could have been added several times? */
	priv->children = g_list_remove_all (priv->children, child);
}

GList*
giggle_revision_get_parents (GiggleRevision *revision)
{
	GiggleRevisionPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);

	priv = GET_PRIV (revision);

	return priv->parents;
}

GList*
giggle_revision_get_children (GiggleRevision *revision)
{
	GiggleRevisionPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);

	priv = GET_PRIV (revision);

	return priv->children;
}

void
giggle_revision_add_parent (GiggleRevision *revision,
			    GiggleRevision *parent)
{
	GiggleRevisionPriv *priv;

	g_return_if_fail (GIGGLE_IS_REVISION (revision));
	g_return_if_fail (GIGGLE_IS_REVISION (parent));

	priv = GET_PRIV (revision);

	priv->parents = g_list_prepend (priv->parents, parent);
	giggle_revision_add_child (parent, revision);
}

void
giggle_revision_remove_parent (GiggleRevision *revision,
			       GiggleRevision *parent)
{
	GiggleRevisionPriv *priv;

	g_return_if_fail (GIGGLE_IS_REVISION (revision));
	g_return_if_fail (GIGGLE_IS_REVISION (parent));

	priv = GET_PRIV (revision);

	/* the parent could have been added several times? */
	priv->parents = g_list_remove_all (priv->parents, parent);
	giggle_revision_remove_child (parent, revision);
}
