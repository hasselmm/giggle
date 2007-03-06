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

#include "giggle-short-list.h"

#include <gtk/gtklabel.h>
#include <gtk/gtkscrolledwindow.h>

typedef struct GiggleShortListPriv GiggleShortListPriv;

struct GiggleShortListPriv {
	GtkWidget* label;
	GtkWidget* scrolled_window;
};

enum {
	PROP_0,
	PROP_LABEL
};

static void     dummy_finalize            (GObject           *object);
static void     dummy_get_property        (GObject           *object,
					   guint              param_id,
					   GValue            *value,
					   GParamSpec        *pspec);
static void     dummy_set_property        (GObject           *object,
					   guint              param_id,
					   const GValue      *value,
					   GParamSpec        *pspec);

G_DEFINE_TYPE (GiggleShortList, giggle_short_list, GTK_TYPE_VBOX)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_SHORT_LIST, GiggleShortListPriv))

static void
giggle_short_list_class_init (GiggleShortListClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize     = dummy_finalize;
	object_class->get_property = dummy_get_property;
	object_class->set_property = dummy_set_property;

	g_object_class_install_property (object_class,
					 PROP_LABEL,
					 g_param_spec_string ("label",
							      "Label",
							      "The text of the displayed label",
							      NULL,
							      G_PARAM_WRITABLE));

	g_type_class_add_private (object_class, sizeof (GiggleShortListPriv));
}

static void
giggle_short_list_init (GiggleShortList *self)
{
	GiggleShortListPriv *priv;
	PangoAttrList       *attributes;
	PangoAttribute      *attribute;

	priv = GET_PRIV (self);

	attributes = pango_attr_list_new ();
	attribute = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
	attribute->start_index = 0;
	attribute->end_index = -1;
	pango_attr_list_insert (attributes, attribute);

	priv->label = gtk_label_new (NULL);
	gtk_label_set_attributes (GTK_LABEL (priv->label), attributes);
	gtk_misc_set_alignment (GTK_MISC (priv->label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (self), priv->label, FALSE, FALSE, 0);

	pango_attr_list_unref (attributes);

	priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (priv->scrolled_window), GTK_SHADOW_IN);
	gtk_box_pack_start (GTK_BOX (self), priv->scrolled_window, TRUE, TRUE, 0);
}

static void
dummy_finalize (GObject *object)
{
	GiggleShortListPriv *priv;

	priv = GET_PRIV (object);
	
	/* FIXME: Free object data */

	G_OBJECT_CLASS (giggle_short_list_parent_class)->finalize (object);
}

static void
dummy_get_property (GObject    *object,
		    guint       param_id,
		    GValue     *value,
		    GParamSpec *pspec)
{
	GiggleShortListPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
dummy_set_property (GObject      *object,
		    guint         param_id,
		    const GValue *value,
		    GParamSpec   *pspec)
{
	GiggleShortListPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_LABEL:
		gtk_label_set_text (GTK_LABEL (priv->label), g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

GtkWidget*
giggle_short_list_new (gchar const* label)
{
	return g_object_new (GIGGLE_TYPE_SHORT_LIST,
			     "homogeneous", FALSE,
			     "label", label,
			     "spacing", 6,
			     NULL);
}

GtkWidget*
giggle_short_list_get_swin (GiggleShortList* self)
{
	g_return_val_if_fail (GIGGLE_IS_SHORT_LIST (self), NULL);

	return GET_PRIV (self)->scrolled_window;
}

