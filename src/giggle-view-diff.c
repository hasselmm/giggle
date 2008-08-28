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
#include "giggle-label-action.h"

typedef struct GiggleViewDiffPriv GiggleViewDiffPriv;

struct GiggleViewDiffPriv {
	GtkWidget      *hpaned;
	GtkWidget      *file_view;
	GtkWidget      *diff_view;
	GtkWidget      *diff_view_sw;

	GtkActionGroup *action_group;
	GtkAction      *status_action;

	int		current_change;
	int		num_changes;
};

G_DEFINE_TYPE (GiggleViewDiff, giggle_view_diff, GIGGLE_TYPE_VIEW)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_VIEW_DIFF, GiggleViewDiffPriv))

static void
view_diff_dispose (GObject *object)
{
	GiggleViewDiffPriv *priv;

	priv = GET_PRIV (object);

	if (priv->action_group) {
		g_object_unref (priv->action_group);
		priv->action_group = NULL;
	}

	if (priv->status_action) {
		g_object_unref (priv->status_action);
		priv->status_action = NULL;
	}

	G_OBJECT_CLASS (giggle_view_diff_parent_class)->dispose (object);
}

static void
view_diff_update_status (GiggleViewDiff *view)
{
	GiggleViewDiffPriv *priv;
	GtkAction          *action;
	char		   *markup;

	priv = GET_PRIV (view);

	if (priv->action_group) {
		gtk_action_group_set_sensitive (priv->action_group, priv->num_changes > 0);

		action = gtk_action_group_get_action (priv->action_group, "ViewDiffPreviousChange");
		gtk_action_set_sensitive (action, priv->current_change > 0);

		action = gtk_action_group_get_action (priv->action_group, "ViewDiffNextChange");
		gtk_action_set_sensitive (action, priv->current_change < priv->num_changes - 1);
	}

	markup = g_markup_printf_escaped ("%s <b>%d/%d</b>", _("Change"),
					  priv->current_change + 1,
					  priv->num_changes);

	giggle_label_action_set_markup (GIGGLE_LABEL_ACTION (priv->status_action), markup);

	g_free (markup);
}

static void
view_diff_previous_change_cb (GtkAction      *action,
			      GiggleViewDiff *view)
{
	GiggleViewDiffPriv *priv;

	priv = GET_PRIV (view);

	if (priv->current_change > 0) {
		priv->current_change -= 1;
		view_diff_update_status (view);
		g_print ("show previous change\n");
	}
}

static void
view_diff_next_change_cb (GtkAction      *action,
			  GiggleViewDiff *view)
{
	GiggleViewDiffPriv *priv;

	priv = GET_PRIV (view);

	if (priv->current_change < priv->num_changes - 1) {
		priv->current_change += 1;
		view_diff_update_status (view);
		g_print ("show next change\n");
	}
}

static void
view_diff_add_ui (GiggleView   *view,
		  GtkUIManager *manager)
{
	const static char layout[] =
		"<ui>"
		"  <toolbar>"
		"    <placeholder name='Actions'>"
		"      <toolitem action='ViewDiffPreviousChange' />"
		"      <toolitem action='ViewDiffNextChange' />"
		"      <toolitem action='ViewDiffStatus' />"
		"     </placeholder>"
		"  </toolbar>"
		"</ui>";

	static const GtkActionEntry actions[] = {
		{ "ViewDiffPreviousChange", GTK_STOCK_GO_UP,
		  N_("_Previous Change"), "<alt>Up", N_("View previous change"),
		  G_CALLBACK (view_diff_previous_change_cb)
		},
		{ "ViewDiffNextChange", GTK_STOCK_GO_DOWN,
		  N_("_Next Change"), "<alt>Down", N_("View next change"),
		  G_CALLBACK (view_diff_next_change_cb)
		},
	};

	GiggleViewDiffPriv *priv;

	priv = GET_PRIV (view);

	if (!priv->action_group) {
		priv->action_group = gtk_action_group_new (giggle_view_get_name (view));
		gtk_action_group_set_sensitive (priv->action_group, priv->num_changes > 0);
		gtk_action_group_set_translation_domain (priv->action_group, GETTEXT_PACKAGE);
		gtk_action_group_add_actions (priv->action_group, actions, G_N_ELEMENTS (actions), view);
		gtk_action_group_add_action (priv->action_group, priv->status_action);

		gtk_ui_manager_insert_action_group (manager, priv->action_group, 0);
		gtk_ui_manager_add_ui_from_string (manager, layout, G_N_ELEMENTS (layout) - 1, NULL);
		gtk_ui_manager_ensure_update (manager);
	}

	gtk_action_group_set_visible (priv->action_group, TRUE);
}

static void
view_diff_remove_ui (GiggleView *view)
{
	GiggleViewDiffPriv *priv;

	priv = GET_PRIV (view);

	if (priv->action_group)
		gtk_action_group_set_visible (priv->action_group, FALSE);
}

static void
giggle_view_diff_class_init (GiggleViewDiffClass *class)
{
	GObjectClass    *object_class;
	GiggleViewClass *view_class;

	object_class = G_OBJECT_CLASS (class);
	view_class = GIGGLE_VIEW_CLASS (class);

	object_class->dispose = view_diff_dispose;

	view_class->add_ui = view_diff_add_ui;
	view_class->remove_ui = view_diff_remove_ui;

	g_type_class_add_private (class, sizeof (GiggleViewDiffPriv));
}

static void
giggle_view_diff_init (GiggleViewDiff *view)
{
	GiggleViewDiffPriv *priv;
	GtkWidget          *scrolled_window;

	priv = GET_PRIV (view);

	priv->status_action = giggle_label_action_new ("ViewDiffStatus");

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

	view_diff_update_status (view);
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

	priv->current_change = 0;
	priv->num_changes = g_random_int_range (1, 20);
	view_diff_update_status (view);
}

