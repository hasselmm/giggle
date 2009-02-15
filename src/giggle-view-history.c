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

#include "config.h"
#include "giggle-view-history.h"

#include "giggle-rev-list-view.h"
#include "giggle-revision-view.h"
#include "giggle-view-diff.h"
#include "giggle-view-shell.h"

#include <libgiggle/giggle-history.h>
#include <libgiggle/giggle-searchable.h>

#include <libgiggle-git/giggle-git.h>
#include <libgiggle-git/giggle-git-diff.h>
#include <libgiggle-git/giggle-git-refs.h>
#include <libgiggle-git/giggle-git-revisions.h>
#include <libgiggle-git/giggle-git-config.h>

#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>

#define GIGGLE_TYPE_VIEW_HISTORY_SNAPSHOT            (giggle_view_history_snapshot_get_type ())
#define GIGGLE_VIEW_HISTORY_SNAPSHOT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_VIEW_HISTORY_SNAPSHOT, GiggleViewHistorySnapshot))
#define GIGGLE_VIEW_HISTORY_SNAPSHOT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_VIEW_HISTORY_SNAPSHOT, GiggleViewHistorySnapshotClass))
#define GIGGLE_IS_VIEW_HISTORY_SNAPSHOT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_VIEW_HISTORY_SNAPSHOT))
#define GIGGLE_IS_VIEW_HISTORY_SNAPSHOT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_VIEW_HISTORY_SNAPSHOT))
#define GIGGLE_VIEW_HISTORY_SNAPSHOT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_VIEW_HISTORY_SNAPSHOT, GiggleViewHistorySnapshotClass))

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_VIEW_HISTORY, GiggleViewHistoryPriv))

enum {
	PROP_0,
	PROP_UI_MANAGER,
};

enum {
	REVISION_COL_OBJECT,
	REVISION_NUM_COLS
};

typedef struct {
	GtkWidget               *main_vpaned;
	GtkWidget               *revision_shell;
	GtkWidget               *view_diff;
	GtkWidget               *revision_view;
	GtkWidget               *revision_list;
	GtkWidget               *revision_list_sw;
	GtkUIManager            *ui_manager;

	GiggleGit               *git;
	GiggleJob               *job;
	GiggleJob               *diff_current_job;
	GiggleGitConfig         *configuration;

	guint                    selection_changed_idle;
} GiggleViewHistoryPriv;

typedef struct {
	GObject              parent;
	GList               *rows;
	GtkTreeRowReference *cursor_row;
	GtkTreeViewColumn   *cursor_column;
} GiggleViewHistorySnapshot;

typedef struct {
	GObjectClass parent_class;
} GiggleViewHistorySnapshotClass;

typedef struct {
	GiggleViewHistory *view;
	GiggleRevision    *revision1;
	GiggleRevision    *revision2;
} ViewHistorySelectionIdleData;

typedef void (AddRefFunc) (GiggleRevision *revision,
			   GiggleRef      *ref);

static void	giggle_view_history_searchable_init	(GiggleSearchableIface *iface);
static void	giggle_view_history_history_init	(GiggleHistoryIface    *iface);

static GType	giggle_view_history_snapshot_get_type	(void) G_GNUC_CONST;


G_DEFINE_TYPE_WITH_CODE (GiggleViewHistory, giggle_view_history, GIGGLE_TYPE_VIEW,
			 G_IMPLEMENT_INTERFACE (GIGGLE_TYPE_SEARCHABLE,
						giggle_view_history_searchable_init)
			 G_IMPLEMENT_INTERFACE (GIGGLE_TYPE_HISTORY,
						giggle_view_history_history_init))

G_DEFINE_TYPE (GiggleViewHistorySnapshot,
	       giggle_view_history_snapshot, G_TYPE_OBJECT)

static void
view_history_get_property (GObject      *object,
			   guint         param_id,
			   GValue       *value,
			   GParamSpec   *pspec)
{
	GiggleViewHistoryPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_UI_MANAGER:
		g_value_set_object (value, priv->ui_manager);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}
static void
view_history_set_property (GObject      *object,
			   guint         param_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
	GiggleViewHistoryPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_UI_MANAGER:
		g_assert (NULL == priv->ui_manager);
		priv->ui_manager = g_value_dup_object (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
view_history_finalize (GObject *object)
{
	GiggleViewHistoryPriv *priv;

	priv = GET_PRIV (object);

	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	if (priv->diff_current_job) {
		giggle_git_cancel_job (priv->git, priv->diff_current_job);
		g_object_unref (priv->diff_current_job);
		priv->diff_current_job = NULL;
	}

	g_object_unref (priv->configuration);
	g_object_unref (priv->git);

	G_OBJECT_CLASS (giggle_view_history_parent_class)->finalize (object);
}

static void
view_history_set_busy (GtkWidget *widget,
		       gboolean   busy)
{
	GdkCursor *cursor;

	if (!GTK_WIDGET_REALIZED (widget)) {
		return;
	}

	if (busy) {
		cursor = gdk_cursor_new (GDK_WATCH);
		gdk_window_set_cursor (widget->window, cursor);
		gdk_cursor_unref (cursor);
	} else {
		gdk_window_set_cursor (widget->window, NULL);
	}
}

static gboolean
view_history_add_refs (GiggleRevision *revision,
		       GList          *list,
		       AddRefFunc      func)
{
	GiggleRef   *ref;
	const gchar *sha1, *sha2;
	gboolean     updated = FALSE;

	sha1 = giggle_revision_get_sha (revision);

	while (list) {
		ref = GIGGLE_REF (list->data);
		sha2 = giggle_ref_get_sha (ref);

		if (strcmp (sha1, sha2) == 0) {
			updated = TRUE;
			(* func) (revision, ref);
		}

		list = list->next;
	}

	return updated;
}

static void
view_history_get_branches_cb (GiggleGit    *git,
			      GiggleJob    *job,
			      GError       *error,
			      gpointer      user_data)
{
	GiggleViewHistory     *view;
	GiggleViewHistoryPriv *priv;
	GiggleRevision        *revision;
	GtkTreeModel          *model;
	GtkTreePath           *path;
	GtkTreeIter            iter;
	gboolean               valid;
	GList                 *branches, *tags, *remotes;
	gboolean               changed;

	view = GIGGLE_VIEW_HISTORY (user_data);
	priv = GET_PRIV (view);

	if (error) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view))),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("An error occurred when getting the revisions list:\n%s"),
						 error->message);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	} else {
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->revision_list));
		valid = gtk_tree_model_get_iter_first (model, &iter);
		branches = giggle_git_refs_get_branches (GIGGLE_GIT_REFS (job));
		tags = giggle_git_refs_get_tags (GIGGLE_GIT_REFS (job));
		remotes = giggle_git_refs_get_remotes (GIGGLE_GIT_REFS (job));

		while (valid) {
			gtk_tree_model_get (model, &iter,
					    REVISION_COL_OBJECT, &revision,
					    -1);

			if (revision) {
				changed  = view_history_add_refs (revision, branches, giggle_revision_add_branch_head);
				changed |= view_history_add_refs (revision, tags, giggle_revision_add_tag);
				changed |= view_history_add_refs (revision, remotes, giggle_revision_add_remote);

				if (changed) {
					path = gtk_tree_model_get_path (model, &iter);
					gtk_tree_model_row_changed (model, path, &iter);
					gtk_tree_path_free (path);
				}

				g_object_unref (revision);
			}

			valid = gtk_tree_model_iter_next (model, &iter);
		}
	}

	g_object_unref (priv->job);
	priv->job = NULL;
}

static void
view_history_diff_current_cb (GiggleGit *git,
			      GiggleJob *job,
			      GError    *error,
			      gpointer   user_data)
{
	GiggleViewHistoryPriv *priv;
	GtkTreeModel          *model;
	GtkTreePath           *path;
	GtkTreeIter            iter;
	const gchar           *text;

	priv = GET_PRIV (user_data);

	/* FIXME: error report missing */
	if (error) {
		return;
	}

	text = giggle_git_diff_get_result (GIGGLE_GIT_DIFF (job));

	if (text && *text) {
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->revision_list));
		gtk_list_store_insert (GTK_LIST_STORE (model), &iter, 0);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    REVISION_COL_OBJECT, NULL,
				    -1);

		path = gtk_tree_model_get_path (model, &iter);
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (priv->revision_list),
					      path, NULL, FALSE, 0.0, 0.0);
	}
}

static void
view_history_get_revisions_cb (GiggleGit    *git,
			       GiggleJob    *job,
			       GError       *error,
			       gpointer      user_data)
{
	GiggleViewHistory     *view;
	GiggleViewHistoryPriv *priv;
	GtkListStore          *store;
	GtkTreeIter            iter;
	GList                 *revisions;

	view = GIGGLE_VIEW_HISTORY (user_data);
	priv = GET_PRIV (view);

	if (error) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view))),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("An error occurred when getting the revisions list:\n%s"),
						 error->message);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_object_unref (priv->job);
		priv->job = NULL;
	} else {
		store = gtk_list_store_new (REVISION_NUM_COLS, GIGGLE_TYPE_REVISION);
		revisions = giggle_git_revisions_get_revisions (GIGGLE_GIT_REVISIONS (job));

		while (revisions) {
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter,
					    REVISION_COL_OBJECT, revisions->data,
					    -1);
			revisions = revisions->next;
		}

		view_history_set_busy (GTK_WIDGET (priv->revision_list), FALSE);
		giggle_rev_list_view_set_model (GIGGLE_REV_LIST_VIEW (priv->revision_list),
						GTK_TREE_MODEL (store));
		g_object_unref (store);
		g_object_unref (priv->job);
		priv->job = NULL;

		/* now get the list of branches */
		priv->job = giggle_git_refs_new ();

		giggle_git_run_job (priv->git, priv->job,
				    view_history_get_branches_cb,
				    view);

		/* and current diff row */
		if (priv->diff_current_job) {
			giggle_git_cancel_job (priv->git, priv->diff_current_job);
			g_object_unref (priv->diff_current_job);
			priv->diff_current_job = NULL;
		}

		priv->diff_current_job = giggle_git_diff_new ();

		giggle_git_run_job (priv->git, priv->diff_current_job,
				    view_history_diff_current_cb,
				    view);

		giggle_history_reset (GIGGLE_HISTORY (view));
	}
}

static void
view_history_update_revisions (GiggleViewHistory  *view)
{
	GiggleViewHistoryPriv *priv;

	priv = GET_PRIV (view);

	view_history_set_busy (GTK_WIDGET (priv->revision_list), TRUE);
	giggle_rev_list_view_set_model (GIGGLE_REV_LIST_VIEW (priv->revision_list), NULL);

	/* get revision list */
	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	priv->job = giggle_git_revisions_new ();

	giggle_git_run_job (priv->git, priv->job,
			    view_history_get_revisions_cb,
			    view);
}

static void
view_history_git_dir_notify (GiggleViewHistory *view)
{
	GiggleViewHistoryPriv *priv;

	priv = GET_PRIV (view);

	view_history_update_revisions (view);

	/* empty views */
	giggle_view_diff_set_revisions (GIGGLE_VIEW_DIFF (priv->view_diff), NULL, NULL, NULL);
	giggle_revision_view_set_revision (GIGGLE_REVISION_VIEW (priv->revision_view), NULL);
}

static void
view_history_git_changed (GiggleViewHistory *view)
{
	view_history_update_revisions (view);
}

static gboolean
view_history_idle_cb (gpointer data)
{
	GiggleViewHistoryPriv *priv;

	priv = GET_PRIV (data);

	giggle_git_config_bind (priv->configuration,
				GIGGLE_GIT_CONFIG_FIELD_HISTORY_VIEW_VPANE_POSITION,
				G_OBJECT (priv->main_vpaned), "position");

	return FALSE;
}

static gboolean
view_history_selection_changed_idle (gpointer user_data)
{
	ViewHistorySelectionIdleData *data = user_data;
	GiggleViewHistoryPriv        *priv;
	GList                        *files;

	priv = GET_PRIV (data->view);
	files = NULL;

	giggle_view_diff_set_revisions (GIGGLE_VIEW_DIFF (priv->view_diff),
					data->revision1, data->revision2, files);

	return FALSE;
}

static void
view_history_revision_list_selection_changed_cb (GiggleRevListView *list,
						 GiggleRevision     *revision1,
						 GiggleRevision     *revision2,
						 GiggleViewHistory  *view)
{
	GiggleViewHistoryPriv        *priv;
	ViewHistorySelectionIdleData *data;

	priv = GET_PRIV (view);

	giggle_revision_view_set_revision (
		GIGGLE_REVISION_VIEW (priv->revision_view), revision1);

	if (priv->selection_changed_idle)
		g_source_remove (priv->selection_changed_idle);

	data = g_new0 (ViewHistorySelectionIdleData, 1);
	data->view = view;
	data->revision1 = revision1;
	data->revision2 = revision2;

	priv->selection_changed_idle =
		gdk_threads_add_idle_full (G_PRIORITY_DEFAULT_IDLE,
					   view_history_selection_changed_idle,
					   data, g_free);

	giggle_history_changed (GIGGLE_HISTORY (data->view));
}

static gboolean
view_history_revision_list_key_press_cb (GiggleRevListView *list,
					 GdkEventKey        *event,
					 GiggleViewHistory  *view)
{
	GiggleViewHistoryPriv *priv = GET_PRIV (view);
	GtkTreePath           *path, *start, *end;
	GtkTreeSelection      *selection;
	GtkAdjustment         *adj;
	gdouble                value;

	if (event->keyval != GDK_space && event->keyval != GDK_BackSpace)
		return FALSE;

	giggle_view_shell_set_view_name (GIGGLE_VIEW_SHELL (priv->revision_shell), "DiffView");
	adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->revision_list_sw));

	value = event->keyval == GDK_space ?
		adj->value + (adj->page_size * 0.8) :
		adj->value - (adj->page_size * 0.8);

	value = CLAMP (value, adj->lower, adj->upper - adj->page_size);
	g_object_set (adj, "value", value, NULL);

	if (gtk_tree_view_get_visible_range (GTK_TREE_VIEW (priv->revision_list), &start, &end)) {
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->revision_list));
		path = event->keyval == GDK_space ? end : start;

		gtk_tree_selection_unselect_all (selection);
		gtk_tree_selection_select_path (selection, path);

		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (priv->revision_list),
					      path, NULL, FALSE, 0, 0);

		gtk_tree_path_free (start);
		gtk_tree_path_free (end);
	}

	return TRUE;
}

static void
view_history_setup_revision_list (GObject *object)
{
	GiggleViewHistoryPriv *priv;

	priv = GET_PRIV (object);

	priv->revision_list_sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->revision_list_sw),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (priv->revision_list_sw),
					     GTK_SHADOW_IN);

	priv->revision_list = giggle_rev_list_view_new ();

	g_signal_connect (priv->revision_list, "selection-changed",
			  G_CALLBACK (view_history_revision_list_selection_changed_cb), object);
	g_signal_connect (priv->revision_list, "key-press-event",
			  G_CALLBACK (view_history_revision_list_key_press_cb), object);

	gtk_container_add (GTK_CONTAINER (priv->revision_list_sw), priv->revision_list);
	gtk_widget_show_all (priv->revision_list_sw);

	gtk_paned_pack1 (GTK_PANED (priv->main_vpaned), priv->revision_list_sw, TRUE, FALSE);
}

static void
view_history_setup_revision_pane (GObject *object)
{
	GiggleViewHistoryPriv *priv;
	GtkWidget             *vbox;
	GtkWidget	      *toolbar;

	priv = GET_PRIV (object);

	gtk_rc_parse_string ("style \"view-history-toolbar-style\" {"
			     "  GtkToolbar::shadow-type = none"
			     "}"
			     "widget \"*.ViewHistoryToolbar\" "
			     "style \"view-history-toolbar-style\"");

	toolbar = gtk_ui_manager_get_widget (priv->ui_manager, "/ViewHistoryToolbar");

	priv->revision_shell = giggle_view_shell_new_with_ui (priv->ui_manager,
							      "ViewHistoryShellActions");
	giggle_view_shell_add_placeholder (GIGGLE_VIEW_SHELL (priv->revision_shell),
					   "/ViewHistoryToolbar/ViewShell");
	gtk_container_set_border_width (GTK_CONTAINER (priv->revision_shell), 6);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, TRUE, 0);
	gtk_box_pack_start_defaults (GTK_BOX (vbox), priv->revision_shell);
	gtk_paned_pack2 (GTK_PANED (priv->main_vpaned), vbox, FALSE, FALSE);
	gtk_widget_show_all (vbox);

	/* diff view */
	priv->view_diff = giggle_view_diff_new ();
	giggle_view_shell_append_view (GIGGLE_VIEW_SHELL (priv->revision_shell),
				       GIGGLE_VIEW (priv->view_diff));

	/* revision view */
	priv->revision_view = giggle_revision_view_new ();
	giggle_view_shell_append_view (GIGGLE_VIEW_SHELL (priv->revision_shell),
				       GIGGLE_VIEW (priv->revision_view));
}

static void
view_history_constructed (GObject *object)
{
	GiggleViewHistoryPriv *priv;

	priv = GET_PRIV (object);

	/* git interaction */
	priv->git = giggle_git_get ();

	g_signal_connect_swapped (priv->git, "notify::git-dir",
				  G_CALLBACK (view_history_git_dir_notify), object);
	g_signal_connect_swapped (priv->git, "changed",
				  G_CALLBACK (view_history_git_changed), object);

	priv->configuration = giggle_git_config_new ();

	gtk_widget_push_composite_child ();

	/* widgets */
	priv->main_vpaned = gtk_vpaned_new ();
	gtk_widget_show (priv->main_vpaned);
	gtk_container_add (GTK_CONTAINER (object), priv->main_vpaned);

	view_history_setup_revision_list (object);
	view_history_setup_revision_pane (object);

	gtk_widget_pop_composite_child ();

	/* configuration bindings */
	gdk_threads_add_idle (view_history_idle_cb, object);
}

static void
giggle_view_history_class_init (GiggleViewHistoryClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->get_property = view_history_get_property;
	object_class->set_property = view_history_set_property;
	object_class->constructed  = view_history_constructed;
	object_class->finalize     = view_history_finalize;

	g_object_class_install_property (object_class,
					 PROP_UI_MANAGER,
					 g_param_spec_object ("ui-manager",
							      "ui manager",
							      "The UI manager to use",
							      GTK_TYPE_UI_MANAGER,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private (object_class, sizeof (GiggleViewHistoryPriv));
}

static gboolean
view_history_search (GiggleSearchable      *searchable,
		     const gchar           *search_term,
		     GiggleSearchDirection  direction,
		     gboolean               full_search)
{
	GiggleViewHistoryPriv *priv;

	priv = GET_PRIV (searchable);

	if (!giggle_searchable_search (GIGGLE_SEARCHABLE (priv->revision_list),
				       search_term, direction, full_search)) {
		return FALSE;
	}

	if (giggle_searchable_search (GIGGLE_SEARCHABLE (priv->revision_view),
				      search_term, direction, full_search)) {
		/* search term is contained in the
		 * revision description, expand it
		 */
		giggle_view_shell_set_view_name (GIGGLE_VIEW_SHELL (priv->revision_shell),
						 "RevisionView");
#if 0
	} else if (giggle_searchable_search (GIGGLE_SEARCHABLE (priv->diff_view),
					     search_term, direction, full_search)) {
		giggle_view_shell_set_view_name (GIGGLE_VIEW_SHELL (priv->revision_shell),
						 "DiffView");
#endif
	}

	return TRUE;
}

static void
view_history_cancel_search (GiggleSearchable *searchable)
{
	GiggleViewHistoryPriv *priv;

	priv = GET_PRIV (searchable);

	giggle_searchable_cancel (GIGGLE_SEARCHABLE (priv->revision_list));
}

static void
giggle_view_history_searchable_init (GiggleSearchableIface *iface)
{
	iface->search = view_history_search;
	iface->cancel = view_history_cancel_search;
}

static void
giggle_view_history_snapshot_init (GiggleViewHistorySnapshot *snapshot)
{
}

static void
view_history_snapshot_dispose (GObject *object)
{
	GiggleViewHistorySnapshot *snapshot;

	snapshot = GIGGLE_VIEW_HISTORY_SNAPSHOT (object);

	while (snapshot->rows) {
		gtk_tree_row_reference_free (snapshot->rows->data);
		snapshot->rows = g_list_delete_link (snapshot->rows, snapshot->rows);
	}

	if (snapshot->cursor_row) {
		gtk_tree_row_reference_free (snapshot->cursor_row);
		snapshot->cursor_row = NULL;
	}

	if (snapshot->cursor_column) {
		g_object_unref (snapshot->cursor_column);
		snapshot->cursor_column = NULL;
	}

	G_OBJECT_CLASS (giggle_view_history_snapshot_parent_class)->dispose (object);
}

static void
giggle_view_history_snapshot_class_init (GiggleViewHistorySnapshotClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	object_class->dispose = view_history_snapshot_dispose;
}

static GObject *
view_history_capture (GiggleHistory *history)
{
	GiggleViewHistoryPriv     *priv = GET_PRIV (history);
	GiggleViewHistorySnapshot *snapshot;
	GtkTreeRowReference       *reference;
	GtkTreeSelection          *selection;
	GtkTreeViewColumn         *column;
	GtkTreeModel              *model;
	GtkTreePath               *path;
	GList                     *l;

	snapshot = g_object_new (GIGGLE_TYPE_VIEW_HISTORY_SNAPSHOT, NULL);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->revision_list));
	snapshot->rows = gtk_tree_selection_get_selected_rows (selection, &model);

	for (l = snapshot->rows; l; l = l->next) {
		reference = gtk_tree_row_reference_new (model, l->data);
		gtk_tree_path_free (l->data);
		l->data = reference;
	}

	gtk_tree_view_get_cursor (GTK_TREE_VIEW (priv->revision_list), &path, &column);

	if (path) {
		snapshot->cursor_row = gtk_tree_row_reference_new (model, path);
		gtk_tree_path_free (path);
	}

	if (column)
		snapshot->cursor_column = g_object_ref (column);

	return G_OBJECT (snapshot);
}

static void
view_history_restore (GiggleHistory *history,
		      GObject       *object)
{
	GiggleViewHistoryPriv     *priv = GET_PRIV (history);
	GtkTreeSelection          *selection;
	GiggleViewHistorySnapshot *snapshot;
	GtkTreePath               *path;
	GList                     *l;

	g_return_if_fail (GIGGLE_IS_VIEW_HISTORY_SNAPSHOT (object));
	snapshot = GIGGLE_VIEW_HISTORY_SNAPSHOT (object);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->revision_list));

	gtk_tree_selection_unselect_all (selection);

	if (snapshot->cursor_row) {
		gtk_tree_view_set_cursor (GTK_TREE_VIEW (priv->revision_list),
					  gtk_tree_row_reference_get_path (snapshot->cursor_row),
					  snapshot->cursor_column, FALSE);
	}

	for (l = snapshot->rows; l; l = l->next) {
		path = gtk_tree_row_reference_get_path (l->data);
		gtk_tree_selection_select_path (selection, path);
	}
}

static void
giggle_view_history_history_init (GiggleHistoryIface *iface)
{
	iface->capture = view_history_capture;
	iface->restore = view_history_restore;
}

static void
giggle_view_history_init (GiggleViewHistory *view)
{
}

GtkWidget *
giggle_view_history_new (GtkUIManager *manager)
{
	GtkAction *action;

	g_return_val_if_fail (GTK_IS_UI_MANAGER (manager), NULL);

	action = g_object_new (GTK_TYPE_RADIO_ACTION,
			       "name", "HistoryView",
			       "label", _("_History"),
			       "tooltip", _("Browse the history of this project"),
			       "icon-name", "giggle-history-view",
			       "is-important", TRUE, NULL);

	return g_object_new (GIGGLE_TYPE_VIEW_HISTORY,
			     "action", action, "accelerator", "F5",
			     "ui-manager", manager, NULL);
}

void
giggle_view_history_set_graph_visible (GiggleViewHistory *view,
				       gboolean           visible)
{
	GiggleViewHistoryPriv *priv;

	g_return_if_fail (GIGGLE_IS_VIEW_HISTORY (view));

	priv = GET_PRIV (view);

	giggle_rev_list_view_set_graph_visible (
		GIGGLE_REV_LIST_VIEW (priv->revision_list), visible);
}

gboolean
giggle_view_history_select (GiggleViewHistory *view,
			    GiggleRevision    *revision)
{
	GiggleViewHistoryPriv *priv;
	GList                 *selection;
	int                    count = 0;

	g_return_val_if_fail (GIGGLE_IS_VIEW_HISTORY (view), FALSE);
	g_return_val_if_fail (GIGGLE_IS_REVISION (revision) || !revision, FALSE);

	priv = GET_PRIV (view);

	selection = g_list_prepend (NULL, revision);
	count = giggle_rev_list_view_set_selection (GIGGLE_REV_LIST_VIEW (priv->revision_list), selection);
	g_list_free (selection);

	return count > 0;
}

