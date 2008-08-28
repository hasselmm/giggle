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
#include <glib/gi18n.h>
#include "giggle-label-action.h"

typedef struct GiggleLabelActionPriv GiggleLabelActionPriv;

struct GiggleLabelActionPriv {
	gboolean use_markup : 1;
};

enum {
	PROP_0,
	PROP_USE_MARKUP
};

G_DEFINE_TYPE (GiggleLabelAction, giggle_label_action, GTK_TYPE_ACTION)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_LABEL_ACTION, GiggleLabelActionPriv))

static void
label_action_connect_proxy (GtkAction *action,
			    GtkWidget *widget)
{
	char      *label = NULL;
	gboolean   use_markup;
	GtkWidget *child;

	GTK_ACTION_CLASS (giggle_label_action_parent_class)->connect_proxy (action, widget);

	g_object_get (action, "label", &label, "use-markup", &use_markup, NULL);

	if (GTK_IS_TOOL_ITEM (widget)) {
		child = gtk_bin_get_child (GTK_BIN (widget));

		if (GTK_IS_LABEL (child))
			g_object_set (child, "label", label, "use-markup", use_markup, NULL);
	}

	g_free (label);
}

static void
label_action_update_proxies (GtkAction *action)
{
	GSList *l;

	for (l = gtk_action_get_proxies (action); l; l = l->next)
		gtk_action_connect_proxy (action, l->data);
		
}

static GtkWidget *
label_action_create_tool_item (GtkAction *action)
{
	GtkWidget   	      *label;
	GtkToolItem 	      *item;

	item = gtk_tool_item_new ();
	label = gtk_label_new (NULL);
	gtk_container_add (GTK_CONTAINER (item), label);
	gtk_widget_show (label);

	return GTK_WIDGET (item);
}

static void
label_action_get_property (GObject    *object,
			   guint       param_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
	GiggleLabelActionPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_USE_MARKUP:
		g_value_set_boolean (value, priv->use_markup);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
label_action_set_property (GObject      *object,
			   guint         param_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
	GiggleLabelActionPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_USE_MARKUP:
		priv->use_markup = g_value_get_boolean (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
giggle_label_action_class_init (GiggleLabelActionClass *class)
{
	GObjectClass     *object_class = G_OBJECT_CLASS (class);
	GtkActionClass   *action_class = GTK_ACTION_CLASS (class);

	object_class->get_property     = label_action_get_property;
	object_class->set_property     = label_action_set_property;
	action_class->connect_proxy    = label_action_connect_proxy;
	action_class->create_tool_item = label_action_create_tool_item;

	g_object_class_install_property (object_class,
					 PROP_USE_MARKUP,
					 g_param_spec_boolean ("use-markup",
							       "use markup",
							       "Wether to use markup",
							       FALSE,
							       G_PARAM_READWRITE));

	g_type_class_add_private (class, sizeof (GiggleLabelActionPriv));
}

static void
giggle_label_action_init (GiggleLabelAction *view)
{
}

GtkAction *
giggle_label_action_new (const char *name)
{
	return g_object_new (GIGGLE_TYPE_LABEL_ACTION, "name", name, NULL);
}

void
giggle_label_action_set_text (GiggleLabelAction *action,
			      const char        *text)
{
	g_return_if_fail (GIGGLE_IS_LABEL_ACTION (action));
	g_return_if_fail (NULL != text);

	g_object_set (action, "label", text, "use-markup", FALSE, NULL);
	label_action_update_proxies (GTK_ACTION (action));
}

void
giggle_label_action_set_markup (GiggleLabelAction *action,
				const char        *markup)
{
	g_return_if_fail (GIGGLE_IS_LABEL_ACTION (action));
	g_return_if_fail (NULL != markup);

	g_object_set (action, "label", markup, "use-markup", TRUE, NULL);
	label_action_update_proxies (GTK_ACTION (action));
}

void
giggle_label_action_set_use_markup (GiggleLabelAction *action,
				    gboolean           use_markup)
{
	g_return_if_fail (GIGGLE_IS_LABEL_ACTION (action));

	g_object_set (action, "use-markup", use_markup, NULL);
	label_action_update_proxies (GTK_ACTION (action));
}

gboolean
giggle_label_action_get_use_markup (GiggleLabelAction *action)
{
	g_return_val_if_fail (GIGGLE_IS_LABEL_ACTION (action), FALSE);
	return GET_PRIV (action)->use_markup;
}

