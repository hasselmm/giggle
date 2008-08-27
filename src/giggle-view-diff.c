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
#include "giggle-view-diff.h"
#include "giggle-diff-tree-view.h"
#include "giggle-diff-view.h"

typedef struct GiggleViewDiffPriv GiggleViewDiffPriv;

struct GiggleViewDiffPriv {
	GtkWidget *hpaned;
	GtkWidget *file_view;
	GtkWidget *diff_view;
	GtkWidget *diff_view_sw;
};

G_DEFINE_TYPE (GiggleViewDiff, giggle_view_diff, GIGGLE_TYPE_VIEW)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_VIEW_DIFF, GiggleViewDiffPriv))

static void
giggle_view_diff_class_init (GiggleViewDiffClass *class)
{
	g_type_class_add_private (class, sizeof (GiggleViewDiffPriv));
}

#if 0
static GtkWidget *
view_history_create_toolbar (GiggleViewHistory *view)
{
	GiggleViewHistoryPriv *priv;
	GtkWidget             *toolbar;
	GtkWidget             *label;
	GtkToolItem           *item;

	priv = GET_PRIV (view);

	toolbar = gtk_toolbar_new ();

	item = gtk_tool_button_new_from_stock (GTK_STOCK_GO_UP);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

	item = gtk_tool_button_new_from_stock (GTK_STOCK_GO_DOWN);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

	item = gtk_tool_item_new ();
	label = gtk_label_new (_("Change <b>1/12</b>"));
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
	gtk_container_add (GTK_CONTAINER (item), label);

	item = gtk_separator_tool_item_new  ();
	gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (item), FALSE);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
	gtk_tool_item_set_expand (item, TRUE);

	item = gtk_radio_tool_button_new_from_stock (NULL, GTK_STOCK_COPY);
	gtk_tool_button_set_label (GTK_TOOL_BUTTON (item), _("_Changes"));
	gtk_tool_button_set_use_underline (GTK_TOOL_BUTTON (item), TRUE);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
	gtk_tool_item_set_is_important (item, TRUE);

	priv->revision_tabs = GTK_RADIO_TOOL_BUTTON (item);
	item = gtk_radio_tool_button_new_with_stock_from_widget (priv->revision_tabs,
								 GTK_STOCK_DIALOG_INFO);
	gtk_tool_button_set_label (GTK_TOOL_BUTTON (item), _("_Details"));
	gtk_tool_button_set_use_underline (GTK_TOOL_BUTTON (item), TRUE);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
	gtk_tool_item_set_is_important (item, TRUE);

	return toolbar;
}
#endif

static void
giggle_view_diff_init (GiggleViewDiff *view)
{
	GiggleViewDiffPriv *priv;
	GtkWidget          *scrolled_window;

	priv = GET_PRIV (view);

	gtk_widget_push_composite_child ();

	/* file view */
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
					     GTK_SHADOW_IN);

	/* FIXME: fixed sizes suck */
	gtk_widget_set_size_request (scrolled_window, 200, -1);

	priv->file_view = giggle_diff_tree_view_new ();
	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->file_view);

	/* diff view */
	priv->diff_view_sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->diff_view_sw),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (priv->diff_view_sw),
					     GTK_SHADOW_IN);

	priv->diff_view = giggle_diff_view_new ();
	gtk_container_add (GTK_CONTAINER (priv->diff_view_sw), priv->diff_view);

	/* hpaned */
	priv->hpaned = gtk_hpaned_new ();
	gtk_paned_pack1 (GTK_PANED (priv->hpaned), scrolled_window, FALSE, FALSE);
	gtk_paned_pack2 (GTK_PANED (priv->hpaned), priv->diff_view_sw, TRUE, FALSE);
	gtk_widget_show_all (priv->hpaned);

	gtk_container_add (GTK_CONTAINER (view), priv->hpaned);
	gtk_widget_pop_composite_child ();
}

GtkWidget *
giggle_view_diff_new (void)
{
	GtkAction *action;

	action = g_object_new (GTK_TYPE_RADIO_ACTION,
			       "name", "DiffView",
			       "label", _("_Changes"),
			       "tooltip", _("Browse a revision's changes"),
			       "stock-id", GTK_STOCK_COPY,
			       "is-important", TRUE, NULL);

	return g_object_new (GIGGLE_TYPE_VIEW_DIFF, "action", action, NULL);
}

void
giggle_view_diff_set_revisions (GiggleViewDiff *view,
				GiggleRevision *revision1,
				GiggleRevision *revision2,
				GList          *files)
{
	GiggleViewDiffPriv *priv;

	g_return_if_fail (GIGGLE_IS_VIEW_DIFF (view));
	g_return_if_fail (GIGGLE_IS_REVISION (revision1) || !revision1);
	g_return_if_fail (GIGGLE_IS_REVISION (revision2) || !revision2);

	priv = GET_PRIV (view);

	giggle_diff_tree_view_set_revisions (GIGGLE_DIFF_TREE_VIEW (priv->file_view), revision1, revision2);
	giggle_diff_view_set_revisions (GIGGLE_DIFF_VIEW (priv->diff_view), revision1, revision2, files);
}

