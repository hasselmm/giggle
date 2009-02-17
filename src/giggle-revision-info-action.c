/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2009 Mathias Hasselmann
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

#include "giggle-revision-info-action.h"
#include "giggle-revision-info.h"

typedef struct GiggleRevisionInfoActionPriv GiggleRevisionInfoActionPriv;

struct GiggleRevisionInfoActionPriv {
	GiggleRevision *revision;
	unsigned        expand : 1;
};

enum {
	PROP_0,
	PROP_REVISION,
	PROP_EXPAND,
};

G_DEFINE_TYPE (GiggleRevisionInfoAction, giggle_revision_info_action, GTK_TYPE_ACTION)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REVISION_INFO_ACTION, GiggleRevisionInfoActionPriv))

static void
notify_cb (GtkAction  *action,
	   GParamSpec *action_prop,
	   GtkWidget  *widget)
{
	GParamSpec *widget_prop;
	GValue      value = {0,};

	widget_prop = g_object_class_find_property (G_OBJECT_GET_CLASS (widget),
						    action_prop->name);

	if (widget_prop && widget_prop->value_type == action_prop->value_type) {
		g_value_init (&value, widget_prop->value_type);
		g_object_get_property (G_OBJECT (action), action_prop->name, &value);
		g_object_set_property (G_OBJECT (widget), widget_prop->name, &value);
		g_value_unset (&value);
	}
}

static void
revision_info_action_connect_proxy (GtkAction *action,
				    GtkWidget *widget)
{
	GiggleRevisionInfoActionPriv *priv = GET_PRIV (action);
	GtkWidget                    *child;
	GParamSpec                   *pspec;

	GTK_ACTION_CLASS (giggle_revision_info_action_parent_class)->connect_proxy (action, widget);

	if (GTK_IS_TOOL_ITEM (widget)) {
		child = gtk_bin_get_child (GTK_BIN (widget));
		child = gtk_bin_get_child (GTK_BIN (child));

		gtk_tool_item_set_expand (GTK_TOOL_ITEM (widget), priv->expand);
		g_signal_connect (action, "notify", G_CALLBACK (notify_cb), child);

		pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (action), "revision");
		notify_cb (action, pspec, child);

		pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (action), "label");
		notify_cb (action, pspec, child);
	}
}

static GtkWidget *
revision_info_action_create_tool_item (GtkAction *action)
{
	GtkWidget   *info, *align;
	GtkToolItem *item;

	info = giggle_revision_info_new ();

	align = gtk_alignment_new (0.5, 0.5, 1.0, 0.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 6, 6);
	gtk_container_add (GTK_CONTAINER (align), info);
	gtk_widget_show_all (align);

	item = gtk_tool_item_new ();
	gtk_container_add (GTK_CONTAINER (item), align);

	return GTK_WIDGET (item);
}

static void
revision_info_action_get_property (GObject    *object,
				   guint       param_id,
				   GValue     *value,
				   GParamSpec *pspec)
{
	GiggleRevisionInfoActionPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_REVISION:
		g_value_set_object (value, priv->revision);
		break;

	case PROP_EXPAND:
		g_value_set_boolean (value, priv->expand);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
revision_info_action_set_property (GObject      *object,
				   guint         param_id,
				   const GValue *value,
				   GParamSpec   *pspec)
{
	GiggleRevisionInfoActionPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_REVISION:
		if (priv->revision)
			g_object_unref (priv->revision);

		priv->revision = g_value_dup_object (value);
		break;

	case PROP_EXPAND:
		priv->expand = g_value_get_boolean (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		return;
	}
}

static void
revision_info_action_dispose (GObject *object)
{
	GiggleRevisionInfoActionPriv *priv;

	priv = GET_PRIV (object);

	if (priv->revision) {
		g_object_unref (priv->revision);
		priv->revision = NULL;
	}

	G_OBJECT_CLASS (giggle_revision_info_action_parent_class)->dispose (object);
}

static void
giggle_revision_info_action_class_init (GiggleRevisionInfoActionClass *class)
{
	GObjectClass     *object_class = G_OBJECT_CLASS (class);
	GtkActionClass   *action_class = GTK_ACTION_CLASS (class);

	object_class->get_property     = revision_info_action_get_property;
	object_class->set_property     = revision_info_action_set_property;
	object_class->dispose          = revision_info_action_dispose;

	action_class->connect_proxy    = revision_info_action_connect_proxy;
	action_class->create_tool_item = revision_info_action_create_tool_item;

	g_object_class_install_property (object_class,
					 PROP_REVISION,
					 g_param_spec_object ("revision",
							      "Revision",
							      "The git revsion to show",
							      GIGGLE_TYPE_REVISION,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class,
					 PROP_EXPAND,
					 g_param_spec_boolean ("expand",
							       "Expand",
							       "Wether to expand tool items",
							       TRUE,
							       G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_type_class_add_private (class, sizeof (GiggleRevisionInfoActionPriv));
}

static void
giggle_revision_info_action_init (GiggleRevisionInfoAction *action)
{
}

GtkAction *
giggle_revision_info_action_new (const char *name)
{
	return g_object_new (GIGGLE_TYPE_REVISION_INFO_ACTION, "name", name, NULL);
}

void
giggle_revision_info_action_set_markup (GiggleRevisionInfoAction *action,
					const char               *markup)
{
	g_return_if_fail (GIGGLE_IS_REVISION_INFO_ACTION (action));
	g_return_if_fail (NULL != markup);

	g_object_set (action, "label", markup, NULL);
}

void
giggle_revision_info_action_set_revision (GiggleRevisionInfoAction *action,
					  GiggleRevision           *revision)
{
	g_return_if_fail (GIGGLE_IS_REVISION_INFO_ACTION (action));
	g_return_if_fail (GIGGLE_IS_REVISION (revision) || !revision);
	g_object_set (action, "revision", revision, NULL);
}

GiggleRevision *
giggle_revision_info_action_get_revision (GiggleRevisionInfoAction *action)
{
	g_return_val_if_fail (GIGGLE_IS_REVISION_INFO_ACTION (action), NULL);
	return GET_PRIV (action)->revision;
}

void
giggle_revision_info_action_set_expand (GiggleRevisionInfoAction *action,
					gboolean                  expand)
{
	g_return_if_fail (GIGGLE_IS_REVISION_INFO_ACTION (action));
	g_object_set (action, "expand", expand, NULL);
}

gboolean
giggle_revision_info_action_get_expand (GiggleRevisionInfoAction *action)
{
	g_return_val_if_fail (GIGGLE_IS_REVISION_INFO_ACTION (action), TRUE);
	return GET_PRIV (action)->expand;
}

