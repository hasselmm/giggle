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

#include "giggle-branch.h"

typedef struct GiggleBranchPriv GiggleBranchPriv;

struct GiggleBranchPriv {
	gchar          *name;
	GiggleRevision *head_revision;
};

static void giggle_branch_finalize           (GObject             *object);
static void giggle_branch_get_property       (GObject             *object,
					      guint                param_id,
					      GValue              *value,
					      GParamSpec          *pspec);
static void giggle_branch_set_property       (GObject             *object,
					      guint                param_id,
					      const GValue        *value,
					      GParamSpec          *pspec);

G_DEFINE_TYPE (GiggleBranch, giggle_branch, G_TYPE_OBJECT)

enum {
	PROP_0,
	PROP_NAME,
	PROP_HEAD,
};

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_BRANCH, GiggleBranchPriv))

static void
giggle_branch_class_init (GiggleBranchClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize     = giggle_branch_finalize;
	object_class->get_property = giggle_branch_get_property;
	object_class->set_property = giggle_branch_set_property;

	g_object_class_install_property (
		object_class,
		PROP_NAME,
		g_param_spec_string ("name",
				     "Branch name",
				     "Branch name",
				     NULL,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (
		object_class,
		PROP_HEAD,
		g_param_spec_object ("head",
				     "Head revision",
				     "Head revision of this branch",
				     GIGGLE_TYPE_REVISION,
				     G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleBranchPriv));
}

static void
giggle_branch_init (GiggleBranch *branch)
{
}

static void
giggle_branch_finalize (GObject *object)
{
	GiggleBranchPriv *priv;

	priv = GET_PRIV (object);

	g_free (priv->name);

	if (priv->head_revision) {
		g_object_unref (priv->head_revision);
	}
	
	G_OBJECT_CLASS (giggle_branch_parent_class)->finalize (object);
}

static void
giggle_branch_get_property (GObject    *object,
			    guint       param_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	GiggleBranchPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_NAME:
		g_value_set_string (value, priv->name);
		break;
	case PROP_HEAD:
		g_value_set_object (value, priv->name);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
giggle_branch_set_property (GObject      *object,
			    guint         param_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	GiggleBranchPriv *priv;
	
	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_NAME:
		g_free (priv->name);
		priv->name = g_value_dup_string (value);
		break;
	case PROP_HEAD:
		if (priv->head_revision) {
			g_object_unref (priv->head_revision);
		}
		priv->head_revision = GIGGLE_REVISION (g_value_dup_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

GiggleBranch *
giggle_branch_new (const gchar *name)
{
	return g_object_new (GIGGLE_TYPE_BRANCH,
			     "name", name,
			     NULL);
}

G_CONST_RETURN gchar *
giggle_branch_get_name (GiggleBranch *branch)
{
	GiggleBranchPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_BRANCH (branch), NULL);

	priv = GET_PRIV (branch);

	return priv->name;
}

GiggleRevision *
giggle_branch_get_head_revision (GiggleBranch *branch)
{
	GiggleBranchPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_BRANCH (branch), NULL);

	priv = GET_PRIV (branch);

	return priv->head_revision;
}
