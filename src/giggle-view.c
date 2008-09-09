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
	GtkWidget *parent_view;
};

enum {
	PROP_0,
	PROP_ACTION,
	PROP_ACCELERATOR,
	PROP_NAME
};

enum {
	ADD_UI,
	REMOVE_UI,
	LAST_SIGNAL
};

G_DEFINE_ABSTRACT_TYPE (GiggleView, giggle_view, GTK_TYPE_VBOX)

static guint signals[LAST_SIGNAL] = { 0, };

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

	case PROP_NAME:
		g_value_set_string (value, giggle_view_get_name (GIGGLE_VIEW (object)));
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
view_realize (GtkWidget *widget)
{
	GiggleViewPriv *priv;
	GtkWidget      *parent;

	priv = GET_PRIV (widget);

	g_return_if_fail (NULL == priv->parent_view);

	parent = gtk_widget_get_parent (widget);

	if (parent)
		parent = gtk_widget_get_ancestor (parent, GIGGLE_TYPE_VIEW);

	if (parent) {
		priv->parent_view = parent;

		g_object_add_weak_pointer (G_OBJECT (priv->parent_view),
					 (gpointer) &priv->parent_view);

		g_signal_connect_swapped (priv->parent_view, "add-ui",
				  	  G_CALLBACK (giggle_view_add_ui),
					  widget);

		g_signal_connect_swapped (priv->parent_view, "remove-ui",
				  	  G_CALLBACK (giggle_view_remove_ui),
					  widget);
	}

	GTK_WIDGET_CLASS (giggle_view_parent_class)->realize (widget);
}

static void
view_unrealize (GtkWidget *widget)
{
	GiggleViewPriv *priv;

	priv = GET_PRIV (widget);

	if (priv->parent_view) {
		g_signal_handlers_disconnect_by_func (priv->parent_view,
						      giggle_view_add_ui,
						      widget);

		g_signal_handlers_disconnect_by_func (priv->parent_view,
						      giggle_view_remove_ui,
						      widget);

		g_object_remove_weak_pointer (G_OBJECT (priv->parent_view),
					    (gpointer) &priv->parent_view);

		priv->parent_view = NULL;

	}

	GTK_WIDGET_CLASS (giggle_view_parent_class)->unrealize (widget);
}

static void
giggle_view_class_init (GiggleViewClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

	object_class->get_property = view_get_property;
	object_class->set_property = view_set_property;
	object_class->finalize     = view_finalize;
	object_class->dispose      = view_dispose;

	widget_class->realize      = view_realize;
	widget_class->unrealize    = view_unrealize;

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

	g_object_class_install_property (object_class,
					 PROP_NAME,
					 g_param_spec_string ("name",
							      "name",
							      "The name of this view",
							      NULL,
							      G_PARAM_READABLE));

	signals[ADD_UI] = g_signal_new ("add-ui", GIGGLE_TYPE_VIEW, G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (GiggleViewClass, add_ui),
					NULL, NULL, g_cclosure_marshal_VOID__OBJECT,
					G_TYPE_NONE, 1, GTK_TYPE_UI_MANAGER);

	signals[REMOVE_UI] = g_signal_new ("remove-ui", GIGGLE_TYPE_VIEW, G_SIGNAL_RUN_LAST,
					   G_STRUCT_OFFSET (GiggleViewClass, remove_ui),
					   NULL, NULL, g_cclosure_marshal_VOID__VOID,
					   G_TYPE_NONE, 0);

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

const char *
giggle_view_get_name (GiggleView *view)
{
	GiggleViewPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_VIEW (view), NULL);

	priv = GET_PRIV (view);

	if (G_LIKELY (priv->action))
		return gtk_action_get_name (priv->action);

	return NULL;
}

void
giggle_view_add_ui (GiggleView   *view,
		    GtkUIManager *manager)
{
	g_return_if_fail (GIGGLE_IS_VIEW (view));
	g_return_if_fail (GTK_IS_UI_MANAGER (manager));

	g_signal_emit (view, signals[ADD_UI], 0, manager);
}

void
giggle_view_remove_ui (GiggleView   *view)
{
	g_return_if_fail (GIGGLE_IS_VIEW (view));

	g_signal_emit (view, signals[REMOVE_UI], 0);
}

