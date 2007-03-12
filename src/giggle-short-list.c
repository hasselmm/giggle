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

#include <gtk/gtkbutton.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkscrolledwindow.h>
#include "giggle-marshal.h"

typedef struct GiggleShortListPriv GiggleShortListPriv;

struct GiggleShortListPriv {
	GtkWidget   * label;
#ifdef OLD_LIST
	GtkWidget   * scrolled_window;
	GtkWidget   * treeview;
#else
	GtkWidget   * list_label;
	GtkWidget   * more_button;
#endif
	GtkListStore* liststore;
};

enum {
	PROP_0,
	PROP_LABEL
};

enum {
	SIGNAL_DISPLAY_OBJECT,
	N_SIGNALS
};

static guint giggle_short_list_signals[N_SIGNALS] = {0};

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

	giggle_short_list_signals[SIGNAL_DISPLAY_OBJECT] =
		g_signal_new ("display-object", GIGGLE_TYPE_SHORT_LIST,
			      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GiggleShortListClass, display_object),
			      NULL, NULL,
			      giggle_marshal_VOID__OBJECT_OBJECT,
			      G_TYPE_NONE, 2,
			      G_TYPE_OBJECT,
			      GTK_TYPE_CELL_RENDERER_TEXT);

	g_type_class_add_private (object_class, sizeof (GiggleShortListPriv));
}
#ifdef OLD_LIST
static void
short_list_cell_data_func (GtkTreeViewColumn* column,
			   GtkCellRenderer  * renderer,
			   GtkTreeModel     * model,
			   GtkTreeIter      * iter,
			   gpointer           data)
{
	GiggleShortList* self = data;
	GObject        * object = NULL;

	gtk_tree_model_get (model, iter,
			    GIGGLE_SHORT_LIST_COL_OBJECT, &object,
			    -1);

	g_signal_emit (self, giggle_short_list_signals[SIGNAL_DISPLAY_OBJECT], 0,
		       object, renderer);

	if (object) {
		g_object_unref (object);
	}
}
#endif
static void
giggle_short_list_init (GiggleShortList *self)
{
	GiggleShortListPriv *priv;
	PangoAttrList       *attributes;
	PangoAttribute      *attribute;
#ifdef OLD_LIST
	GtkCellRenderer     *renderer;
#endif

	priv = GET_PRIV (self);

	gtk_box_set_homogeneous (GTK_BOX (self), FALSE);
	gtk_box_set_spacing (GTK_BOX (self), 6);

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

	priv->liststore = gtk_list_store_new (GIGGLE_SHORT_LIST_N_COLUMNS,
					      G_TYPE_OBJECT);

#ifdef OLD_LIST
	priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (priv->scrolled_window), GTK_SHADOW_IN);
	gtk_box_pack_start (GTK_BOX (self), priv->scrolled_window, TRUE, TRUE, 0);

	priv->treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (priv->liststore));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->treeview), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (priv->treeview), -1,
						    "Object", renderer,
						    short_list_cell_data_func,
						    self, NULL);

	gtk_container_add (GTK_CONTAINER (priv->scrolled_window), priv->treeview);
#else
	priv->list_label = gtk_label_new ("some items\nother items\n...");
	gtk_misc_set_alignment (GTK_MISC (priv->list_label), 0.0, 0.5);
	gtk_widget_show (priv->list_label);
	gtk_box_pack_start (GTK_BOX (self), priv->list_label, TRUE, TRUE, 0);

	priv->more_button = gtk_button_new_with_label ("More...");
	gtk_box_pack_start (GTK_BOX (self), priv->more_button, FALSE, FALSE, 0);
#endif
}

static void
dummy_finalize (GObject *object)
{
	GiggleShortListPriv *priv;

	priv = GET_PRIV (object);
	
	g_object_unref (priv->liststore);

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

GtkListStore*
giggle_short_list_get_liststore (GiggleShortList* self)
{
	g_return_val_if_fail (GIGGLE_IS_SHORT_LIST (self), NULL);

	return GET_PRIV (self)->liststore;
}

