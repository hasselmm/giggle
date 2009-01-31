/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2008 Mathias Hasselmann
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
#include <string.h>

#include "giggle-view-shell.h"

typedef struct GiggleViewShellPriv GiggleViewShellPriv;

struct GiggleViewShellPriv {
	GtkUIManager   *ui_manager;
	GtkWidget      *current_view;
	GtkActionGroup *action_group;
	GPtrArray      *placeholders;
	GtkAction      *first_action;
	int             action_value;
	unsigned        merge_id;
};

G_DEFINE_TYPE (GiggleViewShell, giggle_view_shell, GTK_TYPE_NOTEBOOK)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_VIEW_SHELL, GiggleViewShellPriv))

enum {
	PROP_0,
	PROP_UI_MANAGER,
	PROP_VIEW_NAME,
};


static void
view_shell_set_ui_manager (GiggleViewShell *shell,
			   GtkUIManager    *ui_manager)
{
	GiggleViewShellPriv *priv = GET_PRIV (shell);

	if (ui_manager)
		g_object_ref (ui_manager);

	if (priv->ui_manager) {
		gtk_ui_manager_remove_action_group (priv->ui_manager, priv->action_group);
		gtk_ui_manager_remove_ui (priv->ui_manager, priv->merge_id);
		g_object_unref (priv->ui_manager);
	}

	priv->ui_manager = ui_manager;

	if (priv->ui_manager) {
		gtk_ui_manager_insert_action_group (priv->ui_manager, priv->action_group, 0);
		priv->merge_id = gtk_ui_manager_new_merge_id (priv->ui_manager);
	}
}

static void
view_shell_finalize (GObject *object)
{
	GiggleViewShellPriv *priv;

	priv = GET_PRIV (object);

	g_ptr_array_foreach (priv->placeholders, (GFunc) g_free, NULL);
	g_ptr_array_free (priv->placeholders, TRUE);

	G_OBJECT_CLASS (giggle_view_shell_parent_class)->finalize (object);
}

static void
view_shell_dispose (GObject *object)
{
	GiggleViewShellPriv *priv;

	priv = GET_PRIV (object);

	view_shell_set_ui_manager (GIGGLE_VIEW_SHELL (object), NULL);

	if (priv->action_group) {
		g_object_unref (priv->action_group);
		priv->action_group = NULL;
	}

	G_OBJECT_CLASS (giggle_view_shell_parent_class)->dispose (object);
}

static void
view_shell_get_property (GObject    *object,
			 guint       param_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	GiggleViewShellPriv *priv;

	priv = GET_PRIV (object);
	
	switch (param_id) {
	case PROP_UI_MANAGER:
		g_value_set_object (value, priv->ui_manager);
		break;

	case PROP_VIEW_NAME:
		g_value_set_string (value, giggle_view_shell_get_view_name (GIGGLE_VIEW_SHELL (object)));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
view_shell_set_view_name (GiggleViewShell *shell,
			  const char      *name)
{
	GList *children;
	int    page_num;

	g_return_if_fail (NULL != name);

	children = gtk_container_get_children (GTK_CONTAINER (shell));

	for (page_num = 0; children; ++page_num) {
		if (GIGGLE_IS_VIEW (children->data) &&
			!g_strcmp0 (name, giggle_view_get_name (children->data))) {
			gtk_notebook_set_current_page (GTK_NOTEBOOK (shell), page_num);
			break;
		}

		children = g_list_delete_link (children, children);
	}

	g_list_free (children);
}

static void
view_shell_set_property (GObject      *object,
			 guint         param_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	switch (param_id) {
	case PROP_UI_MANAGER:
		view_shell_set_ui_manager (GIGGLE_VIEW_SHELL (object),
					   g_value_get_object (value));
		break;

	case PROP_VIEW_NAME:
		view_shell_set_view_name (GIGGLE_VIEW_SHELL (object),
					  g_value_get_string (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
view_shell_switch_page (GtkNotebook     *notebook,
			GtkNotebookPage *page,
			guint            page_num)
{
	GiggleViewShellPriv *priv;
	GtkAction           *action;
	GtkWidget           *view;
	int                  value;

	priv = GET_PRIV (notebook);

	if (GIGGLE_IS_VIEW (priv->current_view)) {
		giggle_view_remove_ui (GIGGLE_VIEW (priv->current_view));
		priv->current_view = NULL;
	}

	view = gtk_notebook_get_nth_page (notebook, page_num);

	if (GIGGLE_IS_VIEW (view)) {
		priv->current_view = view;

		action = giggle_view_get_action (GIGGLE_VIEW (view));
		g_object_get (action, "value", &value, NULL);

		gtk_radio_action_set_current_value (GTK_RADIO_ACTION (action), value);
		giggle_view_add_ui (GIGGLE_VIEW (priv->current_view), priv->ui_manager);
	}

	GTK_NOTEBOOK_CLASS
		(giggle_view_shell_parent_class)->
		 switch_page (notebook, page, page_num);

	g_object_notify (G_OBJECT (notebook), "view-name");
}

static void
view_shell_show (GtkWidget *widget)
{
	GiggleViewShellPriv *priv = GET_PRIV (widget);

	GTK_WIDGET_CLASS (giggle_view_shell_parent_class)->show (widget);
	gtk_action_group_set_visible (priv->action_group, TRUE);
}

static void
view_shell_hide (GtkWidget *widget)
{
	GiggleViewShellPriv *priv = GET_PRIV (widget);

	gtk_action_group_set_visible (priv->action_group, FALSE);
	GTK_WIDGET_CLASS (giggle_view_shell_parent_class)->hide (widget);
}

static void
giggle_view_shell_class_init (GiggleViewShellClass *class)
{
	GObjectClass     *object_class   = G_OBJECT_CLASS (class);
	GtkWidgetClass   *widget_class   = GTK_WIDGET_CLASS (class);
	GtkNotebookClass *notebook_class = GTK_NOTEBOOK_CLASS (class);

	object_class->get_property  = view_shell_get_property;
	object_class->set_property  = view_shell_set_property;
	object_class->finalize      = view_shell_finalize;
	object_class->dispose       = view_shell_dispose;

	widget_class->show          = view_shell_show;
	widget_class->hide          = view_shell_hide;

	notebook_class->switch_page = view_shell_switch_page;

	g_object_class_install_property (object_class,
					 PROP_UI_MANAGER,
					 g_param_spec_object ("ui-manager",
							      "ui manager",
							      "The UI manager to use",
							      GTK_TYPE_UI_MANAGER,
							      G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_VIEW_NAME,
					 g_param_spec_string ("view-name",
							      "view name",
							      "The name of the current view",
							      NULL,
							      G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleViewShellPriv));
}

static void
giggle_view_shell_init (GiggleViewShell *shell)
{
	GiggleViewShellPriv *priv = GET_PRIV (shell);

	priv->placeholders = g_ptr_array_new ();
	priv->action_group = gtk_action_group_new ("ViewShell");

	gtk_action_group_set_visible (priv->action_group, FALSE);

	gtk_action_group_set_translation_domain (priv->action_group, GETTEXT_PACKAGE);
}

GtkWidget *
giggle_view_shell_new (void)
{
	static const char layout[] =
		"<ui>"
		"  <toolbar>"
		"    <placeholder name='Actions' />"
		"    <separator name='ViewShellSeparator' expand='true' />"
		"    <placeholder name='ViewShell' />"
		"  </toolbar>"
		"</ui>";

	GtkUIManager *ui_manager;
	GtkWidget    *widget;

	gtk_ui_manager_add_ui_from_string (ui_manager = gtk_ui_manager_new (),
					   layout, G_N_ELEMENTS (layout) - 1, NULL);

	widget = giggle_view_shell_new_with_ui (ui_manager);

	giggle_view_shell_add_placeholder (GIGGLE_VIEW_SHELL (widget),
					   "/toolbar/ViewShell");

	return widget;
}

GtkWidget *
giggle_view_shell_new_with_ui (GtkUIManager *manager)
{
	g_return_val_if_fail (GTK_IS_UI_MANAGER (manager), NULL);

	return g_object_new (GIGGLE_TYPE_VIEW_SHELL, "ui-manager", manager,
			     "show-border", FALSE, "show-tabs", FALSE, NULL);
}

void
giggle_view_shell_add_placeholder (GiggleViewShell *shell,
				   const char      *path)
{
	GiggleViewShellPriv *priv;

	g_return_if_fail (GIGGLE_IS_VIEW_SHELL (shell));
	g_return_if_fail (NULL != path);

	priv = GET_PRIV (shell);

	g_ptr_array_add (priv->placeholders, g_strdup (path));

}

static void
view_shell_value_changed (GtkRadioAction  *action,
                 	  GtkRadioAction  *current,
                 	  GiggleViewShell *shell)
{
	const char *view_name;

	view_name = gtk_action_get_name (GTK_ACTION (current));
	view_shell_set_view_name (shell, view_name);
}

void
giggle_view_shell_append_view (GiggleViewShell *shell,
			       GiggleView      *view)
{
	GiggleViewShellPriv *priv;
	const char *accelerator;
	GtkAction *action;
	unsigned i;

	g_return_if_fail (GIGGLE_IS_VIEW_SHELL (shell));
	g_return_if_fail (GIGGLE_IS_VIEW (view));

	priv = GET_PRIV (shell);
	
	action = giggle_view_get_action (view);
	g_return_if_fail (GTK_IS_RADIO_ACTION (action));
	accelerator = giggle_view_get_accelerator (view);

	g_object_set (action, "value", priv->action_value++, NULL);

	if (!priv->first_action) {
		priv->first_action = action;

		g_signal_connect (action, "changed",
				  G_CALLBACK (view_shell_value_changed), shell);
	} else {
		g_object_set (action, "group", priv->first_action, NULL);
	}

	if (accelerator) {
		gtk_action_group_add_action_with_accel
			(priv->action_group, action, accelerator);
	} else {
		gtk_action_group_add_action
			(priv->action_group, action);
	}

	gtk_notebook_append_page (GTK_NOTEBOOK (shell),
				  GTK_WIDGET (view), NULL);

	if (!priv->ui_manager)
		view_shell_set_ui_manager (shell, gtk_ui_manager_new ());

	for (i = 0; i < priv->placeholders->len; ++i) {
		gtk_ui_manager_add_ui (priv->ui_manager, priv->merge_id,
				       priv->placeholders->pdata[i],
				       gtk_action_get_name (action),
				       gtk_action_get_name (action),
				       GTK_UI_MANAGER_AUTO, FALSE);
	}
}

void
giggle_view_shell_set_view_name (GiggleViewShell *shell,
				 const char      *name)
{
	g_return_if_fail (GIGGLE_IS_VIEW_SHELL (shell));
	g_return_if_fail (NULL != name);

	g_object_set (shell, "view-name", name, NULL);
}

const char *
giggle_view_shell_get_view_name (GiggleViewShell *shell)
{
	GiggleView *view;

	g_return_val_if_fail (GIGGLE_IS_VIEW_SHELL (shell), NULL);

	view = giggle_view_shell_get_selected (shell);

	if (view)
		return giggle_view_get_name (GIGGLE_VIEW (view));

	return NULL;
}

void
giggle_view_shell_set_ui_manager (GiggleViewShell *shell,
				  GtkUIManager    *ui_manager)
{
	g_return_if_fail (GIGGLE_IS_VIEW_SHELL (shell));
	g_return_if_fail (GTK_IS_UI_MANAGER (ui_manager));

	g_object_set (shell, "ui-manager", ui_manager, NULL);
}

GtkUIManager *
giggle_view_shell_get_ui_manager (GiggleViewShell *shell)
{
	g_return_val_if_fail (GIGGLE_IS_VIEW_SHELL (shell), NULL);
	return GET_PRIV (shell)->ui_manager;
}

GiggleView *
giggle_view_shell_find_view (GiggleViewShell *shell,
			     GType            type)
{
	int        n_pages, i;
	GtkWidget *page;

	g_return_val_if_fail (GIGGLE_IS_VIEW_SHELL (shell), NULL);
	g_return_val_if_fail (g_type_is_a  (type, GIGGLE_TYPE_VIEW), NULL);

	n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (shell));

	for (i = 0; i < n_pages; ++i) {
		page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (shell), i);

		if (G_OBJECT_TYPE (page) == type)
			return GIGGLE_VIEW (page);
	}

	return NULL;
}

gboolean
giggle_view_shell_select (GiggleViewShell *shell,
			  GiggleView      *view)
{
	int        n_pages, i;
	GtkWidget *page;

	g_return_val_if_fail (GIGGLE_IS_VIEW_SHELL (shell), FALSE);
	g_return_val_if_fail (GIGGLE_IS_VIEW (view), FALSE);

	n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (shell));

	for (i = 0; i < n_pages; ++i) {
		page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (shell), i);

		if (GIGGLE_IS_VIEW (page) && GIGGLE_VIEW (page) == view) {
			gtk_notebook_set_current_page (GTK_NOTEBOOK (shell), i);
			return TRUE;
		}
	}

	return FALSE;
}

GiggleView *
giggle_view_shell_get_selected (GiggleViewShell *shell)
{
	int        page_num;
	GtkWidget *page;

	g_return_val_if_fail (GIGGLE_IS_VIEW_SHELL (shell), NULL);

	page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (shell));
	page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (shell), page_num);

	if (GIGGLE_IS_VIEW (page))
		return GIGGLE_VIEW (page);

	return NULL;
}

