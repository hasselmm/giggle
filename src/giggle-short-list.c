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
			      giggle_marshal_STRING__OBJECT,
			      G_TYPE_STRING, 1,
			      G_TYPE_OBJECT);

	g_type_class_add_private (object_class, sizeof (GiggleShortListPriv));
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
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled), GTK_SHADOW_IN);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), scrolled, TRUE, TRUE, 0);

	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (priv->liststore));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (treeview), -1,
						    "Object", renderer,
						    short_list_cell_data_func,
						    self, NULL);

	gtk_container_add (GTK_CONTAINER (scrolled), treeview);
	gtk_widget_show_all (scrolled);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static gboolean
short_list_update_label_idle (GiggleShortList* self)
{
	GString    * string;
	GtkTreeIter  iter;
	gint         i;
	GiggleShortListPriv *priv;

	priv = GET_PRIV (self);

	string = g_string_new ("");

	for (i = 0; i < 5; i++) {
		GObject* object = NULL;
		gchar* label = NULL;
		if (!gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (priv->liststore), &iter, NULL, i)) {
			break;
		}

		gtk_tree_model_get (GTK_TREE_MODEL (priv->liststore), &iter,
				    GIGGLE_SHORT_LIST_COL_OBJECT, &object,
				    -1);

		g_signal_emit (self, giggle_short_list_signals[SIGNAL_DISPLAY_OBJECT], 0,
			       object, &label);
		g_string_append_printf (string, (i > 0) ? "\n%s" : "%s", label);
		g_free (label);

		if (object) {
			g_object_unref (object);
		}
	}

	gtk_label_set_text (GTK_LABEL (priv->list_label), string->str);
	g_string_free (string, TRUE);

	if (gtk_tree_model_iter_n_children (GTK_TREE_MODEL (priv->liststore), NULL) > 5) {
		gtk_widget_show (priv->more_button);
	} else {
		gtk_widget_hide (priv->more_button);
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

	priv->liststore = gtk_list_store_new (GIGGLE_SHORT_LIST_N_COLUMNS,
					      G_TYPE_OBJECT);

	priv->list_label = gtk_label_new ("some items\nother items\n...");
	gtk_misc_set_alignment (GTK_MISC (priv->list_label), 0.0, 0.0);
	gtk_widget_show (priv->list_label);
	gtk_box_pack_start (GTK_BOX (self), priv->list_label, TRUE, TRUE, 0);

	priv->more_button = gtk_button_new_with_label (_("Show all..."));
	gtk_box_pack_start (GTK_BOX (self), priv->more_button, FALSE, FALSE, 0);
	g_signal_connect_swapped (priv->more_button, "clicked",
				  G_CALLBACK (short_list_show_dialog), self);

	g_signal_connect_swapped (priv->liststore, "row-changed",
				  G_CALLBACK (short_list_update_label), self);
	g_signal_connect_swapped (priv->liststore, "row-deleted",
				  G_CALLBACK (short_list_update_label), self);
	g_signal_connect_swapped (priv->liststore, "row-inserted",
				  G_CALLBACK (short_list_update_label), self);
	g_signal_connect_swapped (priv->liststore, "rows-reordered",
				  G_CALLBACK (short_list_update_label), self);
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

