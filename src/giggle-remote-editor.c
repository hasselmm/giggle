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

#include "giggle-remote-editor.h"

#include <glib/gi18n.h>
#include <gtk/gtkbox.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkstock.h>
#include <gtk/gtktreeview.h>
#include <glade/glade.h>

#include "giggle-helpers.h"

typedef struct GiggleRemoteEditorPriv GiggleRemoteEditorPriv;

struct GiggleRemoteEditorPriv {
	gboolean      new_remote;
	GiggleRemote *remote;

	GtkWidget    *entry_name;
	GtkWidget    *entry_url;
	GtkWidget    *treeview_branches;
};

enum {
	COL_BRANCH,
	N_COLUMNS
};

enum {
	PROP_0,
	PROP_REMOTE
};

static GObject *remote_editor_constructor         (GType                  type,
						   guint                  n_param,
						   GObjectConstructParam *params);
static void     remote_editor_finalize            (GObject           *object);
static void     remote_editor_get_property        (GObject           *object,
					   guint              param_id,
					   GValue            *value,
					   GParamSpec        *pspec);
static void     remote_editor_set_property        (GObject           *object,
					   guint              param_id,
					   const GValue      *value,
					   GParamSpec        *pspec);
static void     remote_editor_response    (GtkDialog         *dialog,
					   gint               reponse);

G_DEFINE_TYPE (GiggleRemoteEditor, giggle_remote_editor, GTK_TYPE_DIALOG)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REMOTE_EDITOR, GiggleRemoteEditorPriv))

static void
giggle_remote_editor_class_init (GiggleRemoteEditorClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (class);

	object_class->constructor  = remote_editor_constructor;
	object_class->finalize     = remote_editor_finalize;
	object_class->get_property = remote_editor_get_property;
	object_class->set_property = remote_editor_set_property;

	g_object_class_install_property (object_class,
					 PROP_REMOTE,
					 g_param_spec_object ("remote",
							      "Remote",
							      "The remote being edited",
							      GIGGLE_TYPE_REMOTE,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	dialog_class->response = remote_editor_response;

	g_type_class_add_private (object_class, sizeof (GiggleRemoteEditorPriv));
}

static void
remote_editor_text_edited_cb (GiggleRemoteEditor  *self,
			      gchar               *path_s,
			      gchar               *value,
			      GtkCellRendererText *cell)
{
	GiggleRemoteEditorPriv *priv;
	GiggleRemoteBranch     *branch;
	GtkTreeModel           *model;
	GtkTreePath            *path;
	GtkTreeIter             iter;

	priv = GET_PRIV (self);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->treeview_branches));
	path = gtk_tree_path_new_from_string (path_s);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter,
			    COL_BRANCH, &branch,
			    -1);

	if (!branch) {
		GtkTreeIter iter2;
		branch = giggle_remote_branch_new (GIGGLE_REMOTE_DIRECTION_PULL,
						   value);
		gtk_list_store_insert_before (GTK_LIST_STORE (model), &iter2, &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter2,
				    COL_BRANCH, branch,
				    -1);
	} else {
		giggle_remote_branch_set_refspec (branch, value);
	}
	g_object_unref (branch);
	gtk_tree_path_free (path);
}

static void
remote_editor_tree_cell_data_func (GtkTreeViewColumn *tree_column,
		                   GtkCellRenderer   *cell,
				   GtkTreeModel      *model,
				   GtkTreeIter       *iter,
				   gpointer           data)
{
	GiggleRemoteBranch *branch = NULL;

	gtk_tree_model_get (model, iter, COL_BRANCH, &branch, -1);

	if (branch) {
		g_object_set (cell,
			      "text", giggle_remote_branch_get_refspec (branch),
			      "style", PANGO_STYLE_NORMAL, NULL);

		g_object_unref (branch);
	} else {
		g_object_set (cell,
			      "text", _("Click to add mapping..."),
			      "style", PANGO_STYLE_ITALIC, NULL);
	}
}

static void
remote_editor_setup_treeview (GiggleRemoteEditor *self)
{
	GiggleRemoteEditorPriv *priv;
	GtkCellRenderer        *renderer;
	GtkListStore           *store;

	priv = GET_PRIV (self);

	g_signal_connect (priv->treeview_branches, "key-press-event",
			  G_CALLBACK (giggle_list_view_delete_selection), NULL);

	store = gtk_list_store_new (N_COLUMNS, G_TYPE_OBJECT);
	gtk_tree_view_set_model (GTK_TREE_VIEW (priv->treeview_branches), GTK_TREE_MODEL (store));
	g_object_unref (store);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "editable", TRUE, NULL);
	g_signal_connect_swapped (renderer, "edited",
				  G_CALLBACK (remote_editor_text_edited_cb), self);
	gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (priv->treeview_branches), -1,
						    _("Branches"), renderer,
						    remote_editor_tree_cell_data_func,
						    NULL, NULL);
}

static void
giggle_remote_editor_init (GiggleRemoteEditor *remote_editor)
{
	GiggleRemoteEditorPriv *priv;
	GladeXML               *xml;

	priv = GET_PRIV (remote_editor);

	gtk_dialog_set_has_separator (GTK_DIALOG (remote_editor), FALSE);

	xml = glade_xml_new (GLADEDIR "/main-window.glade", "remote_vbox", NULL);

	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (remote_editor)->vbox),
				     glade_xml_get_widget (xml, "remote_vbox"));

	priv->entry_name = glade_xml_get_widget (xml, "entry_remote_name");;
	priv->entry_url = glade_xml_get_widget (xml, "entry_remote_url");;
	priv->treeview_branches = glade_xml_get_widget (xml, "treeview_remote_branches");
	remote_editor_setup_treeview (remote_editor);

	g_object_unref (xml);

	gtk_window_set_default_size (GTK_WINDOW (remote_editor), 350, 200);
}

static GObject *
remote_editor_constructor (GType                  type,
			   guint                  n_params,
			   GObjectConstructParam *params)
{
	GiggleRemoteEditorPriv *priv;
	GObject                *object;

	object = G_OBJECT_CLASS(giggle_remote_editor_parent_class)->constructor (
			type, n_params, params);

	priv = GET_PRIV (object);
	priv->new_remote = G_OBJECT(priv->remote)->ref_count == 1;

	gtk_dialog_add_buttons (GTK_DIALOG (object),
				GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
				priv->new_remote ? GTK_STOCK_ADD : GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				NULL);

	return object;
}

static void
remote_editor_notify_branches_cb (GiggleRemoteEditor *editor)
{
	GiggleRemoteEditorPriv *priv;
	GtkListStore           *store;
	GtkTreeIter             iter;
	GList                  *branch;

	priv = GET_PRIV (editor);
	store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (priv->treeview_branches)));

	gtk_list_store_clear (store);
	for(branch = giggle_remote_get_branches (priv->remote); branch; branch = g_list_next (branch)) {
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COL_BRANCH, branch->data,
				    -1);
	}

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    COL_BRANCH, NULL,
			    -1);
}

static void
remote_editor_notify_name_cb (GiggleRemoteEditor *editor)
{
	GiggleRemoteEditorPriv *priv;
	const gchar            *name;

	priv = GET_PRIV (editor);
	name = giggle_remote_get_name (priv->remote);

	if(name) {
		gtk_entry_set_text (GTK_ENTRY (priv->entry_name), name);
	}
}

static void
remote_editor_notify_url_cb (GiggleRemoteEditor *editor)
{
	GiggleRemoteEditorPriv *priv;
	const gchar            *url;

	priv = GET_PRIV (editor);
	url = giggle_remote_get_url (priv->remote);

	if(url) {
		gtk_entry_set_text (GTK_ENTRY (priv->entry_url), url);
	}
}

static void
remote_editor_set_remote (GiggleRemoteEditor *editor,
			  GiggleRemote       *remote)
{
	GiggleRemoteEditorPriv *priv;

	priv = GET_PRIV (editor);

	if(priv->remote == remote) {
		return;
	}

	if(priv->remote) {
		g_signal_handlers_disconnect_by_func (priv->remote,
						      remote_editor_notify_branches_cb,
						      editor);
		g_signal_handlers_disconnect_by_func (priv->remote,
						      remote_editor_notify_name_cb,
						      editor);
		g_signal_handlers_disconnect_by_func (priv->remote,
						      remote_editor_notify_url_cb,
						      editor);
		g_object_unref (priv->remote);
		priv->remote = NULL;
	}

	if(remote) {
		priv->remote = g_object_ref (remote);
		g_signal_connect_swapped (remote, "notify::branches",
					  G_CALLBACK (remote_editor_notify_branches_cb), editor);
		remote_editor_notify_branches_cb (editor);
		g_signal_connect_swapped (remote, "notify::name",
					  G_CALLBACK (remote_editor_notify_name_cb), editor);
		remote_editor_notify_name_cb (editor);
		g_signal_connect_swapped (remote, "notify::url",
					  G_CALLBACK (remote_editor_notify_url_cb), editor);
		remote_editor_notify_url_cb (editor);
	}

	g_object_notify (G_OBJECT (editor), "remote");
}

static void
remote_editor_finalize (GObject *object)
{
	GiggleRemoteEditorPriv *priv;

	priv = GET_PRIV (object);
	
	remote_editor_set_remote (GIGGLE_REMOTE_EDITOR (object), NULL);

	G_OBJECT_CLASS (giggle_remote_editor_parent_class)->finalize (object);
}

static void
remote_editor_get_property (GObject    *object,
		    guint       param_id,
		    GValue     *value,
		    GParamSpec *pspec)
{
	GiggleRemoteEditorPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_REMOTE:
		g_value_set_object (value, priv->remote);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
remote_editor_set_property (GObject      *object,
		    guint         param_id,
		    const GValue *value,
		    GParamSpec   *pspec)
{
	GiggleRemoteEditorPriv *priv;
	GiggleRemote           *remote;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_REMOTE:
		remote = g_value_get_object (value);
		if(!remote) {
			GiggleRemoteBranch* branch;
			remote = giggle_remote_new (_("Unnamed"));
			branch = giggle_remote_branch_new (GIGGLE_REMOTE_DIRECTION_PULL,
							   "ref/heads/master:ref/heads/incoming");
			giggle_remote_add_branch (remote, branch);
			g_object_unref (branch);
		}
		remote_editor_set_remote (GIGGLE_REMOTE_EDITOR (object),
					  remote);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
remote_editor_response (GtkDialog *dialog,
			gint       response)
{
	GiggleRemoteEditorPriv *priv;

	priv = GET_PRIV (dialog);

	if (response == GTK_RESPONSE_ACCEPT) {
		GtkTreeModel*model;
		GtkTreeIter  iter;
		gchar const *data;
		GList       *branches;

		/* 1. name */
		data = gtk_entry_get_text (GTK_ENTRY (priv->entry_name));
		giggle_remote_set_name (priv->remote, data);

		/* 2. url */
		data = gtk_entry_get_text (GTK_ENTRY (priv->entry_url));
		giggle_remote_set_url (priv->remote, data);

		/* 3. branches */
		/* create a list first so the model doesn't change while updating the branches */
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->treeview_branches));
		branches = NULL;
		if (gtk_tree_model_iter_children (model, &iter, NULL)) {
			do {
				GiggleRemoteBranch *branch = NULL;
				gtk_tree_model_get (model, &iter,
						    COL_BRANCH, &branch,
						    -1);

				if (branch) {
					branches = g_list_prepend (branches, branch);
				}
			} while (gtk_tree_model_iter_next (model, &iter));
		}

		giggle_remote_remove_branches (priv->remote);
		for(branches = g_list_reverse (branches); branches; branches = g_list_next (branches)) {
			giggle_remote_add_branch (priv->remote, branches->data);
			g_object_unref (branches->data);
		}
		g_list_free (branches);
	}

	if(GTK_DIALOG_CLASS(giggle_remote_editor_parent_class)->response) {
		GTK_DIALOG_CLASS(giggle_remote_editor_parent_class)->response (dialog, response);
	}
}

GtkWidget *
giggle_remote_editor_new (GiggleRemote *remote)
{
	return g_object_new (GIGGLE_TYPE_REMOTE_EDITOR, "remote", remote, NULL);
}

