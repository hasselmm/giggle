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

#include "giggle-remote-branch.h"
#include "giggle-enums.h"

typedef struct GiggleRemoteBranchPriv GiggleRemoteBranchPriv;

struct GiggleRemoteBranchPriv {
	GiggleRemoteDirection direction;
	gchar                *refspec;
};

enum {
	PROP_0,
	PROP_DIRECTION,
	PROP_REFSPEC
};

static void     remote_branch_finalize            (GObject           *object);
static void     remote_branch_get_property        (GObject           *object,
					   guint              param_id,
					   GValue            *value,
					   GParamSpec        *pspec);
static void     remote_branch_set_property        (GObject           *object,
					   guint              param_id,
					   const GValue      *value,
					   GParamSpec        *pspec);

G_DEFINE_TYPE (GiggleRemoteBranch, giggle_remote_branch, G_TYPE_OBJECT)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REMOTE_BRANCH, GiggleRemoteBranchPriv))

static void
giggle_remote_branch_class_init (GiggleRemoteBranchClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize     = remote_branch_finalize;
	object_class->get_property = remote_branch_get_property;
	object_class->set_property = remote_branch_set_property;

	g_object_class_install_property (object_class,
					 PROP_DIRECTION,
					 g_param_spec_enum ("direction",
							    "Direction",
							    "The direction of the remote branch (push or pull)",
							    GIGGLE_TYPE_REMOTE_DIRECTION,
							    GIGGLE_REMOTE_DIRECTION_PULL,
							    G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_REFSPEC,
					 g_param_spec_string ("refspec",
						 	      "RefSpec",
							      "The specification for the head to be synchronized",
							      NULL,
							      G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleRemoteBranchPriv));
}

static void
giggle_remote_branch_init (GiggleRemoteBranch *remote_branch)
{
	GiggleRemoteBranchPriv *priv;

	priv = GET_PRIV (remote_branch);
}

static void
remote_branch_finalize (GObject *object)
{
	GiggleRemoteBranchPriv *priv;

	priv = GET_PRIV (object);
	
	g_free (priv->refspec);
	priv->refspec = NULL;

	G_OBJECT_CLASS (giggle_remote_branch_parent_class)->finalize (object);
}

static void
remote_branch_get_property (GObject    *object,
		    guint       param_id,
		    GValue     *value,
		    GParamSpec *pspec)
{
	GiggleRemoteBranchPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_DIRECTION:
		g_value_set_enum (value, priv->direction);
		break;
	case PROP_REFSPEC:
		g_value_set_string (value, priv->refspec);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
remote_branch_set_property (GObject      *object,
		    guint         param_id,
		    const GValue *value,
		    GParamSpec   *pspec)
{
	GiggleRemoteBranchPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_DIRECTION:
		priv->direction = g_value_get_enum (value);
		g_object_notify (object, "direction");
		break;
	case PROP_REFSPEC:
		giggle_remote_branch_set_refspec (GIGGLE_REMOTE_BRANCH (object), g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

GiggleRemoteBranch *
giggle_remote_branch_new (GiggleRemoteDirection direction,
			  const gchar          *refspec)
{
	return g_object_new (GIGGLE_TYPE_REMOTE_BRANCH,
			     "direction", direction,
			     "refspec", refspec,
			     NULL);
}

GiggleRemoteDirection
giggle_remote_branch_get_direction (GiggleRemoteBranch *self)
{
	g_return_val_if_fail (GIGGLE_IS_REMOTE_BRANCH (self), GIGGLE_REMOTE_DIRECTION_PULL);

	return GET_PRIV (self)->direction;
}

gchar const *
giggle_remote_branch_get_refspec (GiggleRemoteBranch *branch)
{
	g_return_val_if_fail (GIGGLE_IS_REMOTE_BRANCH (branch), NULL);

	return GET_PRIV (branch)->refspec;
}

void
giggle_remote_branch_set_refspec (GiggleRemoteBranch *self,
				  const gchar        *refspec)
{
	GiggleRemoteBranchPriv *priv;

	g_return_if_fail (GIGGLE_IS_REMOTE_BRANCH (self));

	priv = GET_PRIV (self);

	if (priv->refspec == refspec) {
		return;
	}

	g_free (priv->refspec);
	priv->refspec = g_strdup (refspec);
	g_object_notify (G_OBJECT (self), "refspec");
}

