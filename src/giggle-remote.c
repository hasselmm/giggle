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

#include "giggle-remote.h"

typedef struct GiggleRemotePriv GiggleRemotePriv;

struct GiggleRemotePriv {
	guint i;
};

static void     remote_finalize            (GObject           *object);
static void     remote_get_property        (GObject           *object,
					   guint              param_id,
					   GValue            *value,
					   GParamSpec        *pspec);
static void     remote_set_property        (GObject           *object,
					   guint              param_id,
					   const GValue      *value,
					   GParamSpec        *pspec);

G_DEFINE_TYPE (GiggleRemote, giggle_remote, G_TYPE_OBJECT)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REMOTE, GiggleRemotePriv))

static void
giggle_remote_class_init (GiggleRemoteClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize     = remote_finalize;
	object_class->get_property = remote_get_property;
	object_class->set_property = remote_set_property;

#if 0
	g_object_class_install_property (object_class,
					 PROP_MY_PROP,
					 g_param_spec_string ("my-prop",
							      "My Prop",
							      "Describe the property",
							      NULL,
							      G_PARAM_READABLE));
#endif

	g_type_class_add_private (object_class, sizeof (GiggleRemotePriv));
}

static void
giggle_remote_init (GiggleRemote *remote)
{
	GiggleRemotePriv *priv;

	priv = GET_PRIV (remote);
}

static void
remote_finalize (GObject *object)
{
	GiggleRemotePriv *priv;

	priv = GET_PRIV (object);
	
	/* FIXME: Free object data */

	G_OBJECT_CLASS (giggle_remote_parent_class)->finalize (object);
}

static void
remote_get_property (GObject    *object,
		    guint       param_id,
		    GValue     *value,
		    GParamSpec *pspec)
{
	GiggleRemotePriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
remote_set_property (GObject      *object,
		    guint         param_id,
		    const GValue *value,
		    GParamSpec   *pspec)
{
	GiggleRemotePriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

GiggleRemote *
giggle_remote_new (gchar const *name)
{
	return g_object_new (GIGGLE_TYPE_REMOTE, "name", name, NULL);
}

const gchar *
giggle_remote_get_name (GiggleRemote *remote)
{
	// FIXME: implement
	g_return_val_if_fail (GIGGLE_IS_REMOTE (remote), NULL);

	return "origin";
}

