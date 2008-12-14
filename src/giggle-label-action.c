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
#include <pango/pango.h>
#include "giggle-label-action.h"

typedef struct GiggleLabelActionPriv GiggleLabelActionPriv;

struct GiggleLabelActionPriv {
	GtkJustification   justify;
	PangoEllipsizeMode ellipsize;
	float		   xalign, yalign;

	unsigned           use_markup : 1;
	unsigned           selectable : 1;

	int		   proxies_frozen;
};

enum {
	PROP_0,
	PROP_JUSTIFY,
	PROP_ELLIPSIZE,
	PROP_USE_MARKUP,
	PROP_SELECTABLE,
	PROP_XALIGN,
	PROP_YALIGN,
};

G_DEFINE_TYPE (GiggleLabelAction, giggle_label_action, GTK_TYPE_ACTION)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_LABEL_ACTION, GiggleLabelActionPriv))

static void
label_action_connect_proxy (GtkAction *action,
			    GtkWidget *widget)
{
	GiggleLabelActionPriv *priv;
	char		      *label;
	GtkWidget	      *child;

	priv = GET_PRIV (action);

	GTK_ACTION_CLASS (giggle_label_action_parent_class)->connect_proxy (action, widget);
	g_object_get (action, "label", &label, NULL);

	if (GTK_IS_TOOL_ITEM (widget)) {
		child = gtk_bin_get_child (GTK_BIN (widget));

		if (GTK_IS_LABEL (child)) {
			g_object_set (child,
				      "label",      label ? label : "",
				      "use-markup", priv->use_markup,
				      "xalign",     priv->xalign,
				      "yalign",     priv->yalign,
				      "selectable", priv->selectable,
				      "ellipsize",  priv->ellipsize,
				      "justify",    priv->justify,
				      NULL);
		}

		gtk_tool_item_set_expand (GTK_TOOL_ITEM (widget),
					  priv->ellipsize != PANGO_ELLIPSIZE_NONE);
	}

	g_free (label);
}

static void
label_action_update_proxies (GtkAction *action)
{
	GSList *l;

	if (GET_PRIV (action)->proxies_frozen > 0)
		return;

	for (l = gtk_action_get_proxies (action); l; l = l->next)
		gtk_action_connect_proxy (action, l->data);
		
}

static void
label_action_freeze_update_proxies (GiggleLabelAction *action)
{
	GET_PRIV (action)->proxies_frozen += 1;
}

static void
label_action_thaw_update_proxies (GiggleLabelAction *action)
{
	GET_PRIV (action)->proxies_frozen -= 1;
	label_action_update_proxies (GTK_ACTION (action));
}

static GtkWidget *
label_action_create_tool_item (GtkAction *action)
{
	GtkWidget   	      *label;
	GtkToolItem 	      *item;

	item = gtk_tool_item_new ();
	label = gtk_label_new (NULL);

	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
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
	case PROP_JUSTIFY:
		g_value_set_enum (value, priv->justify);
		break;

	case PROP_ELLIPSIZE:
		g_value_set_enum (value, priv->ellipsize);
		break;

	case PROP_USE_MARKUP:
		g_value_set_boolean (value, priv->use_markup);
		break;

	case PROP_SELECTABLE:
		g_value_set_boolean (value, priv->selectable);
		break;

	case PROP_XALIGN:
		g_value_set_float (value, priv->xalign);
		break;

	case PROP_YALIGN:
		g_value_set_float (value, priv->yalign);
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
	case PROP_JUSTIFY:
		priv->justify = g_value_get_enum (value);
		break;

	case PROP_ELLIPSIZE:
		priv->ellipsize = g_value_get_enum (value);
		break;

	case PROP_USE_MARKUP:
		priv->use_markup = g_value_get_boolean (value);
		break;

	case PROP_SELECTABLE:
		priv->selectable = g_value_get_boolean (value);
		break;

	case PROP_XALIGN:
		priv->xalign = g_value_get_float (value);
		break;

	case PROP_YALIGN:
		priv->yalign = g_value_get_float (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		return;
	}

	label_action_update_proxies (GTK_ACTION (object));
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
					 PROP_JUSTIFY,
					 g_param_spec_enum ("justify",
							    "Justify",
							    "The alignment of the lines",
							    GTK_TYPE_JUSTIFICATION,
							    GTK_JUSTIFY_LEFT,
							    G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_ELLIPSIZE,
					 g_param_spec_enum ("ellipsize",
							    "Ellipsize",
							    "The preferred place to ellipsize the string",
							    PANGO_TYPE_ELLIPSIZE_MODE,
							    PANGO_ELLIPSIZE_NONE,
							    G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_USE_MARKUP,
					 g_param_spec_boolean ("use-markup",
							       "Use Markup",
							       "Wether to use markup",
							       FALSE,
							       G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_SELECTABLE,
					 g_param_spec_boolean ("selectable",
							       "Selectable",
							       "Whether the label text can be selected with the mouse",
							       FALSE,
							       G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_XALIGN,
					 g_param_spec_float ("xalign",
							     "Horizontal Alignment",
							     "Horizontal alignment, from 0 (left) to 1 (right)",
							     0, 1, 0.0, G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_YALIGN,
					 g_param_spec_float ("yalign",
							     "Vertical Alignment",
							     "Vertical alignment, from 0 (top) to 1 (bottom)",
							     0, 1, 0.5, G_PARAM_READWRITE));

	g_type_class_add_private (class, sizeof (GiggleLabelActionPriv));
}

static void
giggle_label_action_init (GiggleLabelAction *view)
{
	GiggleLabelActionPriv *priv;

	priv = GET_PRIV (view);
	priv->xalign = 0.0;
	priv->yalign = 0.5;
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

	label_action_freeze_update_proxies (action);
	g_object_set (action, "label", text, "use-markup", FALSE, NULL);
	label_action_thaw_update_proxies (action);
}

void
giggle_label_action_set_ellipsize (GiggleLabelAction *action,
				   PangoEllipsizeMode mode)
{
	g_return_if_fail (GIGGLE_IS_LABEL_ACTION (action));
	g_object_set (action, "ellipsize", mode, NULL);
}

PangoEllipsizeMode
giggle_label_action_get_ellipsize (GiggleLabelAction *action)
{
	g_return_val_if_fail (GIGGLE_IS_LABEL_ACTION (action), PANGO_ELLIPSIZE_NONE);
	return GET_PRIV (action)->ellipsize;
}

void
giggle_label_action_set_justify (GiggleLabelAction *action,
				 GtkJustification   justify)
{
	g_return_if_fail (GIGGLE_IS_LABEL_ACTION (action));
	g_object_set (action, "justify", justify, NULL);
}

GtkJustification
giggle_label_action_get_justify (GiggleLabelAction *action)
{
	g_return_val_if_fail (GIGGLE_IS_LABEL_ACTION (action), GTK_JUSTIFY_LEFT);
	return GET_PRIV (action)->justify;
}

void
giggle_label_action_set_markup (GiggleLabelAction *action,
				const char        *markup)
{
	g_return_if_fail (GIGGLE_IS_LABEL_ACTION (action));
	g_return_if_fail (NULL != markup);

	label_action_freeze_update_proxies (action);
	g_object_set (action, "label", markup, "use-markup", TRUE, NULL);
	label_action_thaw_update_proxies (action);
}

void
giggle_label_action_set_use_markup (GiggleLabelAction *action,
				    gboolean           use_markup)
{
	g_return_if_fail (GIGGLE_IS_LABEL_ACTION (action));
	g_object_set (action, "use-markup", use_markup, NULL);
}

gboolean
giggle_label_action_get_use_markup (GiggleLabelAction *action)
{
	g_return_val_if_fail (GIGGLE_IS_LABEL_ACTION (action), FALSE);
	return GET_PRIV (action)->use_markup;
}

void
giggle_label_action_set_selectable (GiggleLabelAction *action,
				    gboolean           selectable)
{
	g_return_if_fail (GIGGLE_IS_LABEL_ACTION (action));
	g_object_set (action, "selectable", selectable, NULL);
}

gboolean
giggle_label_action_get_selectable (GiggleLabelAction *action)
{
	g_return_val_if_fail (GIGGLE_IS_LABEL_ACTION (action), FALSE);
	return GET_PRIV (action)->selectable;
}

void
giggle_label_action_set_alignment (GiggleLabelAction *action,
				   float	      xalign,
				   float	      yalign)
{
	g_return_if_fail (GIGGLE_IS_LABEL_ACTION (action));

	label_action_freeze_update_proxies (action);
	g_object_set (action, "xalign", xalign, "yalign", yalign, NULL);
	label_action_thaw_update_proxies (action);
}

void
giggle_label_action_get_alignment (GiggleLabelAction *action,
				   float	     *xalign,
				   float	     *yalign)
{
	GiggleLabelActionPriv *priv;

	g_return_if_fail (GIGGLE_IS_LABEL_ACTION (action));

	priv = GET_PRIV (action);

	if (xalign)
		*xalign = priv->xalign;
	if (yalign)
		*yalign = priv->yalign;
}

