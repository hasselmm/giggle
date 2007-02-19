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
#include <gtk/gtk.h>

#include "giggle-graph-renderer.h"
#include "giggle-revision-list.h"
#include "giggle-revision.h"
#include "giggle-marshal.h"


typedef struct GiggleRevisionListPriv GiggleRevisionListPriv;

struct GiggleRevisionListPriv {
	GtkTreeViewColumn *graph_column;
	GtkCellRenderer   *graph_renderer;

	gboolean           show_graph : 1;
};

enum {
	COL_OBJECT,
	NUM_COLUMNS,
};

enum {
	PROP_0,
	PROP_SHOW_GRAPH,
};

enum {
	SELECTION_CHANGED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

static void revision_list_finalize                (GObject *object);
static void revision_list_get_property            (GObject        *object,
						   guint           param_id,
						   GValue         *value,
						   GParamSpec     *pspec);
static void revision_list_set_property            (GObject        *object,
						   guint           param_id,
						   const GValue   *value,
						   GParamSpec     *pspec);

static void revision_list_cell_data_log_func      (GtkTreeViewColumn *column,
						   GtkCellRenderer   *cell,
						   GtkTreeModel      *model,
						   GtkTreeIter       *iter,
						   gpointer           data);
static void revision_list_cell_data_author_func   (GtkTreeViewColumn *column,
						   GtkCellRenderer   *cell,
						   GtkTreeModel      *model,
						   GtkTreeIter       *iter,
						   gpointer           data);
static void revision_list_cell_data_date_func     (GtkTreeViewColumn *column,
						   GtkCellRenderer   *cell,
						   GtkTreeModel      *model,
						   GtkTreeIter       *iter,
						   gpointer           data);
static void revision_list_selection_changed_cb    (GtkTreeSelection  *selection,
						   gpointer           data);


G_DEFINE_TYPE (GiggleRevisionList, giggle_revision_list, GTK_TYPE_TREE_VIEW)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REVISION_LIST, GiggleRevisionListPriv))


static void
giggle_revision_list_class_init (GiggleRevisionListClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = revision_list_finalize;
	object_class->set_property = revision_list_set_property;
	object_class->get_property = revision_list_get_property;

	g_object_class_install_property (
		object_class,
		PROP_SHOW_GRAPH,
		g_param_spec_boolean ("show-graph",
				      "Show graph",
				      "Whether to show the revisions graph",
				      FALSE,
				      G_PARAM_READWRITE));

	signals[SELECTION_CHANGED] =
		g_signal_new ("selection-changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GiggleRevisionListClass, selection_changed),
			      NULL, NULL,
			      giggle_marshal_VOID__OBJECT_OBJECT,
			      G_TYPE_NONE,
			      2, GIGGLE_TYPE_REVISION, GIGGLE_TYPE_REVISION);

	g_type_class_add_private (object_class, sizeof (GiggleRevisionListPriv));
}

static void
giggle_revision_list_init (GiggleRevisionList *revision_list)
{
	GiggleRevisionListPriv *priv;
	GtkCellRenderer        *cell;
	GtkTreeSelection       *selection;
	gint                    n_columns;

	priv = GET_PRIV (revision_list);

	priv->graph_renderer = giggle_graph_renderer_new ();
	priv->graph_column = gtk_tree_view_column_new ();

	gtk_tree_view_column_set_title (priv->graph_column, _("Graph"));
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->graph_column),
				    priv->graph_renderer, FALSE);

	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->graph_column),
					priv->graph_renderer,
					"revision", COL_OBJECT,
					NULL);

	gtk_tree_view_insert_column (GTK_TREE_VIEW (revision_list),
				     priv->graph_column, -1);

	cell = gtk_cell_renderer_text_new ();
	g_object_set(cell,
		     "ellipsize", PANGO_ELLIPSIZE_END,
		     NULL);

	n_columns = gtk_tree_view_insert_column_with_data_func (
		GTK_TREE_VIEW (revision_list),
		-1,
		_("Short Log"),
		cell,
		revision_list_cell_data_log_func,
		revision_list,
		NULL);
	gtk_tree_view_column_set_expand (
		gtk_tree_view_get_column (GTK_TREE_VIEW (revision_list), n_columns - 1),
		TRUE);

	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_data_func (
		GTK_TREE_VIEW (revision_list),
		-1,
		_("Author"),
		cell,
		revision_list_cell_data_author_func,
		revision_list,
		NULL);

	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_data_func (
		GTK_TREE_VIEW (revision_list),
		-1,
		_("Date"),
		cell,
		revision_list_cell_data_date_func,
		revision_list,
		NULL);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (revision_list));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW (revision_list), TRUE);

	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (revision_list_selection_changed_cb),
			  revision_list);
}

static void
revision_list_finalize (GObject *object)
{
	GiggleRevisionListPriv *priv;

	priv = GET_PRIV (object);

	g_object_unref (priv->graph_renderer);

	G_OBJECT_CLASS (giggle_revision_list_parent_class)->finalize (object);
}

static void
revision_list_get_property (GObject    *object,
			    guint       param_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	GiggleRevisionListPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_SHOW_GRAPH:
		g_value_set_boolean (value, priv->show_graph);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
revision_list_set_property (GObject      *object,
			    guint         param_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	GiggleRevisionListPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_SHOW_GRAPH:
		giggle_revision_list_set_show_graph (GIGGLE_REVISION_LIST (object),
						     g_value_get_boolean (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
revision_list_cell_data_log_func (GtkTreeViewColumn *column,
				  GtkCellRenderer   *cell,
				  GtkTreeModel      *model,
				  GtkTreeIter       *iter,
				  gpointer           data)
{
	GiggleRevisionListPriv *priv;
	GiggleRevision         *revision;

	priv = GET_PRIV (data);

	gtk_tree_model_get (model, iter,
			    COL_OBJECT, &revision,
			    -1);

	g_object_set (cell,
		      "text", giggle_revision_get_short_log (revision),
		      NULL);

	g_object_unref (revision);
}

static void
revision_list_cell_data_author_func (GtkTreeViewColumn *column,
				     GtkCellRenderer   *cell,
				     GtkTreeModel      *model,
				     GtkTreeIter       *iter,
				     gpointer           data)
{
	GiggleRevisionListPriv *priv;
	GiggleRevision         *revision;

	priv = GET_PRIV (data);

	gtk_tree_model_get (model, iter,
			    COL_OBJECT, &revision,
			    -1);

	g_object_set (cell,
		      "text", giggle_revision_get_author (revision),
		      NULL);

	g_object_unref (revision);
}

static void
revision_list_cell_data_date_func (GtkTreeViewColumn *column,
				   GtkCellRenderer   *cell,
				   GtkTreeModel      *model,
				   GtkTreeIter       *iter,
				   gpointer           data)
{
	GiggleRevisionListPriv *priv;
	GiggleRevision         *revision;

	priv = GET_PRIV (data);

	gtk_tree_model_get (model, iter,
			    COL_OBJECT, &revision,
			    -1);

	g_object_set (cell,
		      "text", giggle_revision_get_date (revision),
		      NULL);

	g_object_unref (revision);
}

static void
revision_list_selection_changed_cb (GtkTreeSelection  *selection,
				    gpointer           data)
{
	GiggleRevisionList     *list;
	GiggleRevisionListPriv *priv;
	GtkTreeModel           *model;
	GList                  *rows, *last_row;
	GtkTreeIter             first_iter;
	GtkTreeIter             last_iter;
	GiggleRevision         *first_revision;
	GiggleRevision         *last_revision;
	gboolean                valid;

	list = GIGGLE_REVISION_LIST (data);
	priv = GET_PRIV (list);

	rows = gtk_tree_selection_get_selected_rows (selection, &model);

	if (!rows) {
		return;
	}

	/* get the first row iter */
	gtk_tree_model_get_iter (model, &first_iter,
				 (GtkTreePath *) rows->data);

	if (g_list_length (rows) > 1) {
		last_row = g_list_last (rows);
		valid = gtk_tree_model_get_iter (model, &last_iter,
						 (GtkTreePath *) last_row->data);
	} else {
		valid = FALSE;
	}

	gtk_tree_model_get (model, &first_iter,
			    COL_OBJECT, &first_revision,
			    -1);
	if (valid) {
		gtk_tree_model_get (model, &last_iter,
				    COL_OBJECT, &last_revision,
				    -1);
	} else {
		/* maybe select a better parent? */
		GList* parents = giggle_revision_get_parents (first_revision);
		last_revision = parents ? g_object_ref(parents->data) : NULL;
	}

	g_signal_emit (list, signals [SELECTION_CHANGED], 0,
		       g_object_ref (first_revision),
		       g_object_ref (last_revision));

	g_object_unref (first_revision);
	if (last_revision) {
		g_object_unref (last_revision);
	}

	g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (rows);
}

GtkWidget*
giggle_revision_list_new (void)
{
	return g_object_new (GIGGLE_TYPE_REVISION_LIST, NULL);
}

void
giggle_revision_list_set_model (GiggleRevisionList *list,
				GtkTreeModel       *model)
{
	GiggleRevisionListPriv *priv;
	GType                   type;

	g_return_if_fail (GIGGLE_IS_REVISION_LIST (list));
	g_return_if_fail (!model || GTK_IS_TREE_MODEL (model));

	if (model) {
		/* we want the first column to contain GiggleRevisions */
		type = gtk_tree_model_get_column_type (model, COL_OBJECT);
		g_return_if_fail (type == GIGGLE_TYPE_REVISION);
	}

	priv = GET_PRIV (list);

	if (model) {
		giggle_graph_renderer_validate_model (GIGGLE_GRAPH_RENDERER (priv->graph_renderer), model, COL_OBJECT);
	}

	gtk_tree_view_set_model (GTK_TREE_VIEW (list), model);
}

gboolean
giggle_revision_get_show_graph (GiggleRevisionList *list)
{
	GiggleRevisionListPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION_LIST (list), FALSE);

	priv = GET_PRIV (list);
	return priv->show_graph;
}

void
giggle_revision_list_set_show_graph (GiggleRevisionList *list,
				     gboolean            show_graph)
{
	GiggleRevisionListPriv *priv;

	g_return_if_fail (GIGGLE_IS_REVISION_LIST (list));

	priv = GET_PRIV (list);

	priv->show_graph = (show_graph == TRUE);
	gtk_tree_view_column_set_visible (priv->graph_column, priv->show_graph);
	g_object_notify (G_OBJECT (list), "show-graph");
}
