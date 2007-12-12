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

#include <glib/gi18n.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkstock.h>
#include "libgiggle/giggle-marshal.h"

typedef struct GiggleShortListPriv GiggleShortListPriv;

struct GiggleShortListPriv {
	GtkWidget    *label;
	GtkWidget    *content_box;
	GtkWidget    *more_button;

	GtkTreeModel *model;
};

enum {
	PROP_0,
	PROP_LABEL,
	PROP_MODEL
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

static void     short_list_size_request   (GtkWidget      *widget,
					   GtkRequisition *requisition);
static void     short_list_size_allocate  (GtkWidget      *widget,
					   GtkAllocation  *allocation);


G_DEFINE_TYPE (GiggleShortList, giggle_short_list, GTK_TYPE_VBOX)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_SHORT_LIST, GiggleShortListPriv))

static void
giggle_short_list_class_init (GiggleShortListClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

	object_class->finalize     = dummy_finalize;
	object_class->get_property = dummy_get_property;
	object_class->set_property = dummy_set_property;

	widget_class->size_request = short_list_size_request;
	widget_class->size_allocate = short_list_size_allocate;

	g_object_class_install_property (object_class,
					 PROP_LABEL,
					 g_param_spec_string ("label",
							      "Label",
							      "The text of the displayed label",
							      NULL,
							      G_PARAM_WRITABLE));
	g_object_class_install_property (object_class,
					 PROP_MODEL,
					 g_param_spec_object ("model",
							      "Model",
							      "Model to fill the list",
							      GTK_TYPE_TREE_MODEL,
							      G_PARAM_READWRITE));

	giggle_short_list_signals[SIGNAL_DISPLAY_OBJECT] =
		g_signal_new ("display-object", GIGGLE_TYPE_SHORT_LIST,
			      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GiggleShortListClass, display_object),
			      NULL, NULL,
			      giggle_marshal_STRING__OBJECT,
			      G_TYPE_STRING, 1,
			      G_TYPE_OBJECT);

	g_type_class_add_private (object_class, sizeof (GiggleShortListPriv));
}

static void
short_list_size_request (GtkWidget      *widget,
			 GtkRequisition *requisition)
{
	GiggleShortListPriv *priv;
	GtkRequisition       req;
	gint                 border_width, spacing;

	priv = GET_PRIV (widget);
	border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));
	spacing = gtk_box_get_spacing (GTK_BOX (widget));

	gtk_widget_size_request (priv->label, &req);
	*requisition = req;

	gtk_widget_size_request (priv->content_box, &req);

	gtk_widget_size_request (priv->more_button, &req);
	requisition->width = MAX (requisition->width, req.width) + (2 * border_width);
	requisition->height += req.width + spacing + (2 * border_width);
}

static void
short_list_size_allocate (GtkWidget     *widget,
			  GtkAllocation *allocation)
{
	GiggleShortListPriv *priv;
	GtkAllocation        child_allocation;
	GList               *children, *list;
	gint                 border_width, spacing;
	gint                 content_height = 0;

	priv = GET_PRIV (widget);
	border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));
	spacing = gtk_box_get_spacing (GTK_BOX (widget));
	widget->allocation = *allocation;

	/* FIXME: we're not taking content_box spacing into
	 * account, but we're setting it to 0 in init() anyway...
	 */

	/* from now we'll use allocation to calculate
	 * children allocation, it's safe to remove borders
	 */
	allocation->x += border_width;
	allocation->y += border_width;
	allocation->width -= 2 * border_width;
	allocation->height -= 2 * border_width;

	child_allocation.x = allocation->x;
	child_allocation.width = allocation->width;

	/* allocate label */
	child_allocation.y = allocation->y;
	child_allocation.height = priv->label->requisition.height;
	gtk_widget_size_allocate (priv->label, &child_allocation);

	allocation->y += priv->label->requisition.height + spacing;
	allocation->height -= priv->label->requisition.height + spacing;

	children = list = gtk_container_get_children (GTK_CONTAINER (priv->content_box));

	while (list) {
		content_height += GTK_WIDGET (list->data)->requisition.height;
		list = list->next;
	}

	if (content_height > allocation->height) {
		/* just show the elements that fit in the allocation */
		child_allocation.y = allocation->y;
		child_allocation.height = allocation->height - priv->more_button->requisition.height;
		gtk_widget_size_allocate (priv->content_box, &child_allocation);

		allocation->y += child_allocation.height + spacing;
		allocation->height -= child_allocation.height + spacing;

		list = children;
		content_height = child_allocation.height;

		while (list) {
			if (content_height > GTK_WIDGET (list->data)->requisition.height) {
				gtk_widget_set_child_visible (GTK_WIDGET (list->data), TRUE);
			} else {
				gtk_widget_set_child_visible (GTK_WIDGET (list->data), FALSE);
			}

			content_height -= GTK_WIDGET (list->data)->requisition.height;
			list = list->next;
		}

		/* allocate button */
		child_allocation.y = allocation->y;
		child_allocation.height = priv->more_button->requisition.height;
		gtk_widget_size_allocate (priv->more_button, &child_allocation);
		gtk_widget_set_child_visible (priv->more_button, TRUE);
	} else {
		/* show all the contents and hide the button */
		child_allocation.y = allocation->y;
		child_allocation.height = allocation->height;
		gtk_widget_size_allocate (priv->content_box, &child_allocation);

		list = children;

		while (list) {
			gtk_widget_set_child_visible (GTK_WIDGET (list->data), TRUE);
			list = list->next;
		}

		gtk_widget_set_child_visible (priv->more_button, FALSE);
	}
}

static void
short_list_cell_data_func (GtkTreeViewColumn* column,
			   GtkCellRenderer  * renderer,
			   GtkTreeModel     * model,
			   GtkTreeIter      * iter,
			   gpointer           data)
{
	GiggleShortList* self = data;
	GObject        * object = NULL;
	gchar          * label = NULL;

	gtk_tree_model_get (model, iter,
			    GIGGLE_SHORT_LIST_COL_OBJECT, &object,
			    -1);

	g_signal_emit (self, giggle_short_list_signals[SIGNAL_DISPLAY_OBJECT], 0,
		       object, &label);

	g_object_set (renderer, "text", label, NULL);
	g_free (label);

	if (object) {
		g_object_unref (object);
	}
}

static void
short_list_show_dialog (GiggleShortList* self)
{
	GtkCellRenderer     *renderer;
	GtkWidget           *dialog;
	GtkWidget           *scrolled;
	GtkWidget           *treeview;

	GiggleShortListPriv *priv;

	priv = GET_PRIV (self);

	dialog = gtk_dialog_new_with_buttons (_("Details"),
					      GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self))),
					      GTK_DIALOG_NO_SEPARATOR,
					      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					      NULL);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (scrolled), 7);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled), GTK_SHADOW_IN);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), scrolled, TRUE, TRUE, 0);

	treeview = gtk_tree_view_new_with_model (priv->model);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (treeview), -1,
						    "Object", renderer,
						    short_list_cell_data_func,
						    self, NULL);

	gtk_container_add (GTK_CONTAINER (scrolled), treeview);
	gtk_widget_show_all (scrolled);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 300, 300);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static gboolean
short_list_update_label_idle (GiggleShortList* self)
{
	GiggleShortListPriv *priv;
	GtkTreeIter          iter;
	gboolean             valid;

	priv = GET_PRIV (self);

	/* empty content_box */
	gtk_container_foreach (GTK_CONTAINER (priv->content_box),
			       (GtkCallback) gtk_widget_destroy, NULL);

	valid = gtk_tree_model_get_iter_first (priv->model, &iter);

	while (valid) {
		GtkWidget *label;
		GObject   *object = NULL;
		gchar     *text = NULL;

		gtk_tree_model_get (priv->model, &iter,
				    GIGGLE_SHORT_LIST_COL_OBJECT, &object,
				    -1);

		if (object) {
			g_signal_emit (self, giggle_short_list_signals[SIGNAL_DISPLAY_OBJECT], 0,
				       object, &text);

			label = gtk_label_new (text);
			gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
			gtk_widget_show (label);
			gtk_box_pack_start (GTK_BOX (priv->content_box), label, FALSE, FALSE, 0);

			g_free (text);
			g_object_unref (object);
		}

		valid = gtk_tree_model_iter_next (priv->model, &iter);
	}

	return FALSE;
}

static void
short_list_update_label (GiggleShortList* self)
{
	g_idle_add_full (G_PRIORITY_LOW,
			 (GSourceFunc)short_list_update_label_idle,
			 g_object_ref (self),
			 g_object_unref);
}

static void
giggle_short_list_init (GiggleShortList *self)
{
	GiggleShortListPriv *priv;
	PangoAttrList       *attributes;
	PangoAttribute      *attribute;

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

	priv->content_box = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (self), priv->content_box, TRUE, TRUE, 0);

	priv->more_button = gtk_button_new_with_label (_("Show all..."));
	gtk_box_pack_end (GTK_BOX (self), priv->more_button, FALSE, FALSE, 0);
	g_signal_connect_swapped (priv->more_button, "clicked",
				  G_CALLBACK (short_list_show_dialog), self);
}

static void
dummy_finalize (GObject *object)
{
	GiggleShortListPriv *priv;

	priv = GET_PRIV (object);

	if (priv->model) {
		g_object_unref (priv->model);
	}

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
	case PROP_MODEL:
		g_value_set_object (value, priv->model);
		break;
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
	case PROP_MODEL:
		if (priv->model) {
			g_object_unref (priv->model);
			priv->model = NULL;
		}

		giggle_short_list_set_model (GIGGLE_SHORT_LIST (object),
					     g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

GtkTreeModel *
giggle_short_list_get_model (GiggleShortList *short_list)
{
	GiggleShortListPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_SHORT_LIST (short_list), NULL);

	priv = GET_PRIV (short_list);
	return priv->model;
}

void
giggle_short_list_set_model (GiggleShortList *short_list,
			     GtkTreeModel    *model)
{
	GiggleShortListPriv *priv;

	g_return_if_fail (GIGGLE_IS_SHORT_LIST (short_list));
	g_return_if_fail (GTK_IS_TREE_MODEL (model));

	priv = GET_PRIV (short_list);
	priv->model = g_object_ref (model);

	short_list_update_label (short_list);
}
