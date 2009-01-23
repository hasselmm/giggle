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
#include "giggle-helpers.h"
#include "giggle-revision-info-action.h"

typedef struct GiggleViewDiffPriv GiggleViewDiffPriv;

struct GiggleViewDiffPriv {
	GtkWidget      *hpaned;
	GtkWidget      *file_view;
	GtkWidget      *diff_view;
	GtkWidget      *diff_view_sw;
	GtkWidget      *view_shell_separator;

	GtkActionGroup *action_group;
	GtkAction      *revision_info;

	GiggleRevision *revision;
};

G_DEFINE_TYPE (GiggleViewDiff, giggle_view_diff, GIGGLE_TYPE_VIEW)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_VIEW_DIFF, GiggleViewDiffPriv))
#define PATH_VIEW_SHELL_SEPARATOR "/ViewHistoryToolbar/ViewShellSeparator"

static void
view_diff_dispose (GObject *object)
{
	GiggleViewDiffPriv *priv;

	priv = GET_PRIV (object);

	if (priv->action_group) {
		g_object_unref (priv->action_group);
		priv->action_group = NULL;
	}

	if (priv->revision_info) {
		g_object_unref (priv->revision_info);
		priv->revision_info = NULL;
	}

	G_OBJECT_CLASS (giggle_view_diff_parent_class)->dispose (object);
}

static void
view_diff_patch_loaded (GiggleDiffView *view)
{
	if (giggle_diff_view_get_n_hunks (view) > 0) {
		giggle_diff_view_set_current_hunk (view, 0);
	} else {
		giggle_diff_view_set_current_hunk (view, -1);
	}
}

static void
view_diff_update_status (GiggleViewDiff *view)
{
	GiggleViewDiffPriv *priv;
	GtkAction	   *action;
	char		   *format, *markup;
	int		    current_hunk, n_hunks;
	char		   *current_file;

	priv = GET_PRIV (view);

	g_object_get (priv->diff_view,
		      "current-file", &current_file,
		      "current-hunk", &current_hunk,
		      "n-hunks", &n_hunks, NULL);

	if (priv->action_group) {
		gtk_action_group_set_sensitive (priv->action_group, n_hunks > 0);

		action = gtk_action_group_get_action (priv->action_group, "ViewDiffPreviousChange");
		gtk_action_set_sensitive (action, current_hunk > 0);

		action = gtk_action_group_get_action (priv->action_group, "ViewDiffNextChange");
		gtk_action_set_sensitive (action, current_hunk < n_hunks - 1);
	}

	format = g_markup_printf_escaped ("<b>%s</b>", _("Change %d of %d"));
	markup = g_markup_printf_escaped (format, current_hunk + 1, n_hunks);

	giggle_revision_info_action_set_markup
		(GIGGLE_REVISION_INFO_ACTION (priv->revision_info),
		 markup);
	giggle_revision_info_action_set_revision
		(GIGGLE_REVISION_INFO_ACTION (priv->revision_info),
		 priv->revision);

	g_free (markup);
	g_free (format);

	giggle_tree_view_select_row_by_string (priv->file_view, 0, current_file);

	g_free (current_file);
}

static void
view_diff_previous_change_cb (GtkAction      *action,
			      GiggleViewDiff *view)
{
	GiggleViewDiffPriv *priv;
	int		    current_hunk;

	priv = GET_PRIV (view);

	current_hunk = giggle_diff_view_get_current_hunk (GIGGLE_DIFF_VIEW (priv->diff_view));

	if (current_hunk > 0) {
		giggle_diff_view_set_current_hunk (GIGGLE_DIFF_VIEW (priv->diff_view),
						   current_hunk -1);
	}
}

static void
view_diff_next_change_cb (GtkAction      *action,
			  GiggleViewDiff *view)
{
	GiggleViewDiffPriv *priv;
	int		    current_hunk, n_hunks;

	priv = GET_PRIV (view);

	current_hunk = giggle_diff_view_get_current_hunk (GIGGLE_DIFF_VIEW (priv->diff_view));
	n_hunks = giggle_diff_view_get_n_hunks (GIGGLE_DIFF_VIEW (priv->diff_view));

	if (current_hunk < n_hunks - 1) {
		giggle_diff_view_set_current_hunk (GIGGLE_DIFF_VIEW (priv->diff_view),
						   current_hunk + 1);
	}
}

static void
view_diff_add_ui (GiggleView   *view,
		  GtkUIManager *manager)
{
	const static char layout[] =
		"<ui>"
		"  <menubar name='MainMenubar'>"
		"    <menu action='GoMenu'>"
		"      <separator />"
		"      <menuitem action='ViewDiffPreviousChange' />"
		"      <menuitem action='ViewDiffNextChange' />"
		"    </menu>"
		"  </menubar>"
		"  <toolbar name='ViewHistoryToolbar'>"
		"    <placeholder name='Actions'>"
		"      <toolitem action='ViewDiffPreviousChange' />"
		"      <toolitem action='ViewDiffNextChange' />"
		"      <toolitem action='ViewDiffRevisionInfo' />"
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
	int                 n_hunks;

	priv = GET_PRIV (view);

	priv->view_shell_separator = gtk_ui_manager_get_widget (manager, PATH_VIEW_SHELL_SEPARATOR);

	if (!priv->action_group) {
		n_hunks = giggle_diff_view_get_n_hunks (GIGGLE_DIFF_VIEW (priv->diff_view));
		priv->action_group = gtk_action_group_new (giggle_view_get_name (view));

		gtk_action_group_set_sensitive (priv->action_group, n_hunks > 0);
		gtk_action_group_set_translation_domain (priv->action_group, GETTEXT_PACKAGE);
		gtk_action_group_add_actions (priv->action_group, actions, G_N_ELEMENTS (actions), view);
		gtk_action_group_add_action (priv->action_group, priv->revision_info);

		gtk_ui_manager_insert_action_group (manager, priv->action_group, 0);
		gtk_ui_manager_add_ui_from_string (manager, layout, G_N_ELEMENTS (layout) - 1, NULL);
		gtk_ui_manager_ensure_update (manager);
	}

	if (priv->view_shell_separator) {
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->view_shell_separator), FALSE);
		gtk_widget_hide (priv->view_shell_separator);
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

	if (priv->view_shell_separator) {
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->view_shell_separator), TRUE);
		gtk_widget_show (priv->view_shell_separator);
	}
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
view_diff_path_selected (GtkTreeSelection *selection,
			 GiggleViewDiff   *view)
{
	GiggleViewDiffPriv *priv;
	const char         *path;

	priv = GET_PRIV (view);

	path = giggle_diff_tree_view_get_selection (GIGGLE_DIFF_TREE_VIEW (priv->file_view));

	if (path)
		giggle_diff_view_scroll_to_file (GIGGLE_DIFF_VIEW (priv->diff_view), path);
}

static void
giggle_view_diff_init (GiggleViewDiff *view)
{
	GiggleViewDiffPriv *priv;
	GtkWidget          *scrolled_window;

	priv = GET_PRIV (view);

	priv->revision_info = giggle_revision_info_action_new ("ViewDiffRevisionInfo");

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

	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->file_view)),
			  "changed", G_CALLBACK (view_diff_path_selected), view);

	/* diff view */
	priv->diff_view_sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->diff_view_sw),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (priv->diff_view_sw),
					     GTK_SHADOW_IN);

	priv->diff_view = giggle_diff_view_new ();
	gtk_container_add (GTK_CONTAINER (priv->diff_view_sw), priv->diff_view);

	g_signal_connect (priv->diff_view, "notify::n-hunks",
			  G_CALLBACK (view_diff_patch_loaded), NULL);
	g_signal_connect_swapped (priv->diff_view, "notify::current-hunk",
				  G_CALLBACK (view_diff_update_status), view);

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
	priv->revision = revision1;

	giggle_diff_tree_view_set_revisions (GIGGLE_DIFF_TREE_VIEW (priv->file_view), revision1, revision2);
	giggle_diff_view_set_revisions (GIGGLE_DIFF_VIEW (priv->diff_view), revision1, revision2, files);

	view_diff_update_status (view);
}

