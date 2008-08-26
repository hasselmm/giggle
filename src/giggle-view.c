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

#include "giggle-view.h"

typedef struct GiggleViewPriv GiggleViewPriv;

struct GiggleViewPriv {
	GtkAction *action;
	char      *accelerator;
};

static void       view_finalize           (GObject        *object);

enum {
	PROP_0,
	PROP_ACTION,
	PROP_ACCELERATOR
};

G_DEFINE_ABSTRACT_TYPE (GiggleView, giggle_view, GTK_TYPE_VBOX)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_VIEW, GiggleViewPriv))

static void
view_get_property (GObject    *object,
			 guint       param_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	GiggleViewPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_ACTION:
		g_value_set_object (value, priv->action);
		break;

	case PROP_ACCELERATOR:
		g_value_set_string (value, priv->accelerator);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
view_set_property (GObject      *object,
		   guint         param_id,
		   const GValue *value,
		   GParamSpec   *pspec)
{
	GiggleViewPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_ACTION:
		g_assert (NULL == priv->action);
		priv->action = g_value_dup_object (value);
		break;

	case PROP_ACCELERATOR:
		g_assert (NULL == priv->accelerator);
		priv->accelerator = g_value_dup_string (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
view_finalize (GObject *object)
{
	GiggleViewPriv *priv;

	priv = GET_PRIV (object);

	g_free (priv->accelerator);

	G_OBJECT_CLASS (giggle_view_parent_class)->finalize (object);
}

static void
view_dispose (GObject *object)
{
	GiggleViewPriv *priv;

	priv = GET_PRIV (object);

	if (priv->action) {
		g_object_unref (priv->action);
		priv->action = NULL;
	}

	G_OBJECT_CLASS (giggle_view_parent_class)->dispose (object);
}

static void
giggle_view_class_init (GiggleViewClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->get_property = view_get_property;
	object_class->set_property = view_set_property;
	object_class->finalize     = view_finalize;
	object_class->dispose      = view_dispose;

	g_object_class_install_property (object_class,
					 PROP_ACTION,
					 g_param_spec_object ("action",
							      "action",
							      "The action for this view",
							      GTK_TYPE_RADIO_ACTION,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
					 PROP_ACCELERATOR,
					 g_param_spec_string ("accelerator",
							      "accelerator",
							      "The accelerator for this view",
							      NULL,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private (class, sizeof (GiggleViewPriv));
}

static void
giggle_view_init (GiggleView *view)
{
}

GtkAction *
giggle_view_get_action (GiggleView *view)
{
	g_return_val_if_fail (GIGGLE_IS_VIEW (view), NULL);
	return GET_PRIV (view)->action;
}

const char *
giggle_view_get_accelerator (GiggleView *view)
{
	g_return_val_if_fail (GIGGLE_IS_VIEW (view), NULL);
	return GET_PRIV (view)->accelerator;
}

