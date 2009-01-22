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
#include <string.h>

#include "giggle-diff-window.h"
#include "giggle-graph-renderer.h"
#include "giggle-input-dialog.h"
#include "giggle-rev-list-view.h"

#include "libgiggle/giggle-branch.h"
#include "libgiggle/giggle-clipboard.h"
#include "libgiggle/giggle-job.h"
#include "libgiggle/giggle-marshal.h"
#include "libgiggle/giggle-revision.h"
#include "libgiggle/giggle-searchable.h"
#include "libgiggle/giggle-tag.h"

#include "libgiggle/giggle-git-add-ref.h"
#include "libgiggle/giggle-git-delete-ref.h"
#include "libgiggle/giggle-git-diff.h"
#include "libgiggle/giggle-git-log.h"
#include "libgiggle/giggle-git.h"

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REV_LIST_VIEW, GiggleRevListViewPriv))

#define COMMIT_UI_PATH        "/ui/PopupMenu/Commit"
#define CREATE_BRANCH_UI_PATH "/ui/PopupMenu/CreateBranch"
#define CREATE_TAG_UI_PATH    "/ui/PopupMenu/CreateTag"
#define CREATE_PATCH_UI_PATH  "/ui/PopupMenu/CreatePatch"

typedef struct GiggleRevListViewPriv GiggleRevListViewPriv;

struct GiggleRevListViewPriv {
	GiggleGit         *git;
	GiggleJob         *job;

	GtkTreeViewColumn *emblem_column;
	int                emblem_size;

	GdkPixbuf         *emblem_branch;
	GdkPixbuf         *emblem_remote;
	GdkPixbuf         *emblem_tag;

	GtkTreeViewColumn *graph_column;
	GtkCellRenderer   *graph_renderer;

	GtkUIManager      *ui_manager;
	GtkWidget         *popup;

	GtkActionGroup    *refs_action_group;
	guint              refs_merge_id;

	/* used for search inside diffs */
	GMainLoop         *main_loop;

	/* revision caching */
	GiggleRevision    *first_revision;
	GiggleRevision    *last_revision;

	guint              show_graph : 1;
	guint              cancelled : 1;
};

typedef struct RevisionSearchData RevisionSearchData;

struct RevisionSearchData {
	GMainLoop          *main_loop;
	const gchar        *search_term;
	gboolean            match;
	GiggleRevListView *list;
};

enum {
	COL_OBJECT,
	NUM_COLUMNS,
};

enum {
	PROP_0,
	PROP_GRAPH_VISIBLE,
};

enum {
	SELECTION_CHANGED,
	REVISION_ACTIVATED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

static void giggle_rev_list_view_searchable_init  (GiggleSearchableIface *iface);
static void giggle_rev_list_view_clipboard_init   (GiggleClipboardIface  *iface);

G_DEFINE_TYPE_WITH_CODE (GiggleRevListView, giggle_rev_list_view, GTK_TYPE_TREE_VIEW,
			 G_IMPLEMENT_INTERFACE (GIGGLE_TYPE_SEARCHABLE,
						giggle_rev_list_view_searchable_init)
			 G_IMPLEMENT_INTERFACE (GIGGLE_TYPE_CLIPBOARD,
						giggle_rev_list_view_clipboard_init))

static void
rev_list_view_reset_emblems (GiggleRevListViewPriv *priv)
{
	if (priv->emblem_branch) {
		g_object_unref (priv->emblem_branch);
		priv->emblem_branch = NULL;
	}

	if (priv->emblem_remote) {
		g_object_unref (priv->emblem_remote);
		priv->emblem_remote = NULL;
	}

	if (priv->emblem_tag) {
		g_object_unref (priv->emblem_tag);
		priv->emblem_tag = NULL;
	}
}

static void
rev_list_view_dispose (GObject *object)
{
	GiggleRevListViewPriv *priv;

	priv = GET_PRIV (object);

	rev_list_view_reset_emblems (priv);

	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	if (priv->git) {
		g_object_unref (priv->git);
		priv->git = NULL;
	}

	if (priv->main_loop) {
		if (g_main_loop_is_running (priv->main_loop))
			g_main_loop_quit (priv->main_loop);

		g_main_loop_unref (priv->main_loop);
		priv->main_loop = NULL;
	}

	G_OBJECT_CLASS (giggle_rev_list_view_parent_class)->dispose (object);
}

static void
rev_list_view_get_property (GObject    *object,
			    guint       param_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	GiggleRevListViewPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_GRAPH_VISIBLE:
		g_value_set_boolean (value, priv->show_graph);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
rev_list_view_set_property (GObject      *object,
			    guint         param_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	GiggleRevListViewPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_GRAPH_VISIBLE:
		giggle_rev_list_view_set_graph_visible (GIGGLE_REV_LIST_VIEW (object),
							g_value_get_boolean (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
modify_ref_cb (GiggleGit *git,
	       GiggleJob *job,
	       GError    *error,
	       gpointer   user_data)
{
	GiggleRevListViewPriv *priv;

	priv = GET_PRIV (user_data);

	/* FIXME: error reporting missing */
	if (!error) {
		g_object_notify (G_OBJECT (priv->git), "git-dir");
	}

	g_object_unref (priv->job);
	priv->job = NULL;
}

static void
rev_list_view_activate_action (GtkAction          *action,
			       GiggleRevListView *list)
{	
	GiggleRevListViewPriv *priv;
	GiggleRef                *ref;

	priv = GET_PRIV (list);
	ref = g_object_get_data (G_OBJECT (action), "ref");
	priv->job = giggle_git_delete_ref_new (ref);

	giggle_git_run_job (priv->git,
			    priv->job,
			    modify_ref_cb,
			    list);
}

static void
rev_list_view_clear_popup_refs (GiggleRevListView *list)
{
	GiggleRevListViewPriv *priv;
	GList                  *actions, *l;

	priv = GET_PRIV (list);
	actions = gtk_action_group_list_actions (priv->refs_action_group);

	if (priv->refs_merge_id) {
		gtk_ui_manager_remove_ui (priv->ui_manager, priv->refs_merge_id);
	}

	for (l = actions; l != NULL; l = l->next) {
		g_signal_handlers_disconnect_by_func (GTK_ACTION (l->data),
                                                      G_CALLBACK (rev_list_view_activate_action),
                                                      list);

		gtk_action_group_remove_action (priv->refs_action_group, l->data);
	}

	/* prepare a new merge id */
	priv->refs_merge_id = gtk_ui_manager_new_merge_id (priv->ui_manager);

	g_list_free (actions);
}

static void
rev_list_view_add_popup_refs (GiggleRevListView *list,
			      GiggleRevision     *revision,
			      GList              *refs,
			      const gchar        *label_str,
			      const gchar        *name_str)
{
	GiggleRevListViewPriv *priv;
	GiggleRef              *ref;
	GtkAction              *action;
	gchar                  *action_name, *label;
	const gchar            *name;

	priv = GET_PRIV (list);

	while (refs) {
		ref = GIGGLE_REF (refs->data);

		name = giggle_ref_get_name (ref);
		label = g_strdup_printf (label_str, name);
		action_name = g_strdup_printf (name_str, name);

		action = gtk_action_new (action_name,
					 label,
					 NULL,
					 NULL);

		g_object_set_data_full (G_OBJECT (action), "ref",
					g_object_ref (ref),
					(GDestroyNotify) g_object_unref);

		g_signal_connect (action, "activate",
				  G_CALLBACK (rev_list_view_activate_action),
				  list);

		gtk_action_group_add_action (priv->refs_action_group, action);

		gtk_ui_manager_add_ui (priv->ui_manager,
				       priv->refs_merge_id,
				       "/ui/PopupMenu/Refs",
				       action_name,
				       action_name,
				       GTK_UI_MANAGER_MENUITEM,
				       FALSE);

		g_object_unref (action);
		g_free (action_name);

		refs = refs->next;
	}
}

static void
rev_list_view_setup_popup (GiggleRevListView *list,
			   GiggleRevision     *revision)
{
	GiggleRevListViewPriv *priv;
	GtkTreeSelection       *selection;
	GtkAction              *action;

	priv = GET_PRIV (list);
	
	/* clear action list */
	rev_list_view_clear_popup_refs (list);

	if (revision) {
		action = gtk_ui_manager_get_action (priv->ui_manager, COMMIT_UI_PATH);
		gtk_action_set_visible (action, FALSE); 

		action = gtk_ui_manager_get_action (priv->ui_manager, CREATE_BRANCH_UI_PATH);
		gtk_action_set_visible (action, TRUE);

		action = gtk_ui_manager_get_action (priv->ui_manager, CREATE_TAG_UI_PATH);
		gtk_action_set_visible (action, TRUE);

		/* repopulate action list */
		rev_list_view_add_popup_refs (list, revision,
					      giggle_revision_get_branch_heads (revision),
					      _("Delete branch \"%s\""), "branch-%s");
		rev_list_view_add_popup_refs (list, revision,
					      giggle_revision_get_tags (revision),
					      _("Delete tag \"%s\""), "tag-%s");
	} else {
		action = gtk_ui_manager_get_action (priv->ui_manager, COMMIT_UI_PATH);
		gtk_action_set_visible (action, TRUE);

		action = gtk_ui_manager_get_action (priv->ui_manager, CREATE_BRANCH_UI_PATH);
		gtk_action_set_visible (action, FALSE);

		action = gtk_ui_manager_get_action (priv->ui_manager, CREATE_TAG_UI_PATH);
		gtk_action_set_visible (action, FALSE);
	}

	/* We can always create a patch if there is something
	 * selected, the big thing here is, we can have NO revision
	 * but 1 item selected (i.e. when there are local changes.
	 */
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	if (gtk_tree_selection_count_selected_rows (selection) > 0) {
		action = gtk_ui_manager_get_action (priv->ui_manager, CREATE_PATCH_UI_PATH);
		gtk_action_set_visible (action, TRUE);
	}

	gtk_ui_manager_ensure_update (priv->ui_manager);
}

static void
rev_list_view_commit_changes (GiggleRevListView *list)
{
	GtkWidget *diff_window, *toplevel;

	diff_window = giggle_diff_window_new ();

	g_signal_connect_after (diff_window, "response",
				G_CALLBACK (gtk_widget_hide), NULL);

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (list));
	gtk_window_set_transient_for (GTK_WINDOW (diff_window),
				      GTK_WINDOW (toplevel));
	gtk_widget_show (diff_window);
}

static gboolean
rev_list_view_button_press (GtkWidget      *widget,
			    GdkEventButton *event)
{
	GiggleRevListViewPriv *priv;
	GtkTreeSelection       *selection;
	GtkTreeModel           *model;
	GtkTreePath            *path;
	GtkTreeIter             iter;
	GiggleRevision         *revision;

	if (event->button == 3) {
		priv = GET_PRIV (widget);
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));

		if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y,
						    &path, NULL, NULL, NULL)) {
			return TRUE;
		}

		gtk_tree_selection_unselect_all (selection);
		gtk_tree_selection_select_path (selection, path);

		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model, &iter,
				    COL_OBJECT, &revision,
				    -1);

		rev_list_view_setup_popup (GIGGLE_REV_LIST_VIEW (widget), revision);

		gtk_menu_popup (GTK_MENU (priv->popup), NULL, NULL,
				NULL, NULL, event->button, event->time);

		gtk_tree_path_free (path);
	} else {
		GTK_WIDGET_CLASS (giggle_rev_list_view_parent_class)->button_press_event (widget, event);

		if (event->button == 1 &&
		    event->type == GDK_2BUTTON_PRESS) {
			if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y,
							    &path, NULL, NULL, NULL)) {
				return TRUE;
			}

			model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
			gtk_tree_model_get_iter (model, &iter, path);
			gtk_tree_model_get (model, &iter,
					    COL_OBJECT, &revision,
					    -1);
			gtk_tree_path_free (path);

			if (!revision) {
				/* clicked on the uncommitted changes revision */
				rev_list_view_commit_changes (GIGGLE_REV_LIST_VIEW (widget));
			} else {
				g_object_unref (revision);
			}
		}
	}

	return TRUE;
}

static void
revision_tooltip_add_refs (GString  *str,
			   gchar    *label,
			   GList    *list)
{
	GiggleRef *ref;

	if (str->len > 0 && list)
		g_string_append (str, "\n");

	while (list) {
		ref = list->data;
		list = list->next;

		g_string_append_printf (str, "<b>%s</b>: %s", label,
					giggle_ref_get_name (ref));

		if (list)
			g_string_append (str, "\n");
	}
}

static gboolean
rev_list_view_query_tooltip (GtkWidget  *widget,
                             gint        x,
                             gint        y,
                             gboolean    keyboard_mode,
                             GtkTooltip *tooltip)
{
	GiggleRevListViewPriv *priv = GET_PRIV (widget);
	int                    bin_x, bin_y;
	GiggleRevision        *revision = NULL;
	GtkTreePath           *path = NULL;
	GtkTreeViewColumn     *column;
	GtkTreeIter            iter;
	GtkTreeModel          *model;
	GdkRectangle           cell_area;
	GString               *markup;
	GList                 *revs;

	gtk_tree_view_convert_widget_to_bin_window_coords
		(GTK_TREE_VIEW (widget), x, y, &bin_x, &bin_y);

	if (!gtk_tree_view_get_path_at_pos
			(GTK_TREE_VIEW (widget), bin_x, bin_y,
			 &path, &column, NULL, NULL))
		goto finish;

	if (column != priv->emblem_column)
		goto finish;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));

	if (gtk_tree_model_get_iter (model, &iter, path))
		gtk_tree_model_get (model, &iter, COL_OBJECT, &revision, -1);

	if (!revision)
		goto finish;

	markup = g_string_new (NULL);

	revs = giggle_revision_get_branch_heads (revision);
	revision_tooltip_add_refs (markup, _("Branch"), revs);

	revs = giggle_revision_get_tags (revision);
	revision_tooltip_add_refs (markup, _("Tag"), revs);

	revs = giggle_revision_get_remotes (revision);
	revision_tooltip_add_refs (markup, _("Remote"), revs);

	if (markup->len > 0) {
		gtk_tree_view_get_cell_area (GTK_TREE_VIEW (widget), path, column, &cell_area);

		gtk_tree_view_convert_bin_window_to_widget_coords
			(GTK_TREE_VIEW (widget), cell_area.x, cell_area.y,
			 &cell_area.x, &cell_area.y);

		gtk_tooltip_set_tip_area (tooltip, &cell_area);
		gtk_tooltip_set_markup (tooltip, markup->str);
	} else {
		g_object_unref (revision);
		revision = NULL;
	}

	g_string_free (markup, TRUE);

finish:
	gtk_tree_path_free (path);

	if (revision) {
		g_object_unref (revision);
		return TRUE;
	}

	return GTK_WIDGET_CLASS (giggle_rev_list_view_parent_class)->
		query_tooltip (widget, x, y, keyboard_mode, tooltip);
}

static void
rev_list_view_style_set (GtkWidget *widget,
			 GtkStyle  *prev_style)
{
	GiggleRevListViewPriv *priv = GET_PRIV (widget);
	GtkIconTheme          *icon_theme;
	GdkScreen             *screen;
	int                    w, h;

	rev_list_view_reset_emblems (priv);

	gtk_widget_style_get (widget, "emblem-size", &priv->emblem_size, NULL);

	if (-1 == priv->emblem_size) {
		if (gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h)) {
			priv->emblem_size = MIN (w, h);
		} else {
			priv->emblem_size = 16;
		}
	}

	screen = gtk_widget_get_screen (widget);
	icon_theme = gtk_icon_theme_get_for_screen (screen);

	priv->emblem_branch = gtk_icon_theme_load_icon (icon_theme, "giggle-branch",
							priv->emblem_size, 0, NULL);
	priv->emblem_remote = gtk_icon_theme_load_icon (icon_theme, "giggle-remote",
							priv->emblem_size, 0, NULL);
	priv->emblem_tag    = gtk_icon_theme_load_icon (icon_theme, "giggle-tag",
							priv->emblem_size, 0, NULL);

	gtk_tree_view_column_set_min_width (priv->emblem_column,
					    priv->emblem_size * 3 +
					    2 * widget->style->xthickness);

	GTK_WIDGET_CLASS (giggle_rev_list_view_parent_class)->style_set (widget, prev_style);
}

static void
rev_list_view_row_activated (GtkTreeView       *view,
			     GtkTreePath       *path,
			     GtkTreeViewColumn *column)
{
	GtkTreeModel   *model;
	GtkTreeIter     iter;
	GiggleRevision *revision;

	model = gtk_tree_view_get_model (view);

	if (gtk_tree_model_get_iter (model, &iter, path)) {
		gtk_tree_model_get (model, &iter, COL_OBJECT, &revision, -1);

		g_signal_emit (view, signals[REVISION_ACTIVATED], 0, revision);

		if (revision)
			g_object_unref (revision);
	}

	if (GTK_TREE_VIEW_CLASS (giggle_rev_list_view_parent_class)->row_activated)
		GTK_TREE_VIEW_CLASS (giggle_rev_list_view_parent_class)->row_activated (view, path, column);
}

static void
giggle_rev_list_view_class_init (GiggleRevListViewClass *class)
{
	GObjectClass     *object_class    = G_OBJECT_CLASS (class);
	GtkWidgetClass   *widget_class    = GTK_WIDGET_CLASS (class);
	GtkTreeViewClass *tree_view_class = GTK_TREE_VIEW_CLASS (class);

	object_class->dispose             = rev_list_view_dispose;
	object_class->set_property        = rev_list_view_set_property;
	object_class->get_property        = rev_list_view_get_property;

	widget_class->button_press_event  = rev_list_view_button_press;
	widget_class->query_tooltip       = rev_list_view_query_tooltip;
	widget_class->style_set           = rev_list_view_style_set;

	tree_view_class->row_activated    = rev_list_view_row_activated;

	g_object_class_install_property
		(object_class, PROP_GRAPH_VISIBLE,
		 g_param_spec_boolean ("graph-visible",
				       "Graph visible",
				       "Whether to show the revisions graph",
				       FALSE,
				       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	gtk_widget_class_install_style_property
		(widget_class, g_param_spec_int ("emblem-size",
						 "Emblem Size",
						 "Pixel sizes of emblems in the revision list",
						 -1, G_MAXINT, -1,
						 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	signals[SELECTION_CHANGED] =
		g_signal_new ("selection-changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GiggleRevListViewClass, selection_changed),
			      NULL, NULL,
			      giggle_marshal_VOID__OBJECT_OBJECT,
			      G_TYPE_NONE,
			      2, GIGGLE_TYPE_REVISION, GIGGLE_TYPE_REVISION);

	signals[REVISION_ACTIVATED] =
		g_signal_new ("revision-activated",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GiggleRevListViewClass, revision_activated),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1, GIGGLE_TYPE_REVISION);

	g_type_class_add_private (object_class, sizeof (GiggleRevListViewPriv));
}

static gboolean
revision_property_matches (GiggleRevision *revision,
			   const gchar    *property,
			   const gchar    *search_term)
{
	gboolean  match;
	gchar    *str, *casefold_str;

	g_object_get (revision, property, &str, NULL);
	casefold_str = g_utf8_casefold (str, -1);
	match = strstr (casefold_str, search_term) != NULL;

	g_free (casefold_str);
	g_free (str);

	return match;
}

static void
log_matches_cb (GiggleGit *git,
		 GiggleJob *job,
		 GError    *error,
		 gpointer   user_data)
{
	RevisionSearchData     *data;
	GiggleRevListViewPriv *priv;
	const gchar            *log;
	gchar                  *casefold_log;

	data = (RevisionSearchData *) user_data;
	priv = GET_PRIV (data->list);

	if (error) {
		data->match = FALSE;
	} else {
		log = giggle_git_log_get_log (GIGGLE_GIT_LOG (job));
		casefold_log = g_utf8_casefold (log, -1);
		data->match = (strstr (casefold_log, data->search_term) != NULL);

		g_free (casefold_log);
	}

	g_object_unref (priv->job);
	priv->job = NULL;

	g_main_loop_quit (data->main_loop);
}

static gboolean
revision_log_matches (GiggleRevListView *list,
		      GiggleRevision     *revision,
		      const gchar        *search_term)
{
	GiggleRevListViewPriv *priv;
	RevisionSearchData     *data;
	gboolean                match;

	priv = GET_PRIV (list);

	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	priv->job = giggle_git_log_new (revision);

	data = g_slice_new0 (RevisionSearchData);
	data->main_loop = g_main_loop_ref (priv->main_loop);
	data->search_term = search_term;
	data->list = list;

	giggle_git_run_job (priv->git,
			    priv->job,
			    log_matches_cb,
			    data);

	/* wait here */
	g_main_loop_run (data->main_loop);

	/* At this point the match job has already returned */
	g_main_loop_unref (data->main_loop);
	match = data->match;
	g_slice_free (RevisionSearchData, data);

	return match;
}

static void
diff_matches_cb (GiggleGit *git,
		 GiggleJob *job,
		 GError    *error,
		 gpointer   user_data)
{
	RevisionSearchData     *data;
	GiggleRevListViewPriv *priv;
	const gchar            *diff_str;

	data = (RevisionSearchData *) user_data;
	priv = GET_PRIV (data->list);

	if (error) {
		data->match = FALSE;
	} else {
		diff_str = giggle_git_diff_get_result (GIGGLE_GIT_DIFF (job));
		data->match = (strstr (diff_str, data->search_term) != NULL);
	}

	g_object_unref (priv->job);
	priv->job = NULL;

	g_main_loop_quit (data->main_loop);
}

static gboolean
revision_diff_matches (GiggleRevListView *list,
		       GiggleRevision     *revision,
		       const gchar        *search_term)
{
	GiggleRevListViewPriv *priv;
	GiggleRevision         *parent;
	GList                  *parents;
	RevisionSearchData     *data;
	gboolean                match;

	priv = GET_PRIV (list);
	parents = giggle_revision_get_parents (revision);

	if (!parents) {
		return FALSE;
	}

	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}

	parent = parents->data;
	priv->job = giggle_git_diff_new ();
	giggle_git_diff_set_revisions (GIGGLE_GIT_DIFF (priv->job),
				       parent, revision);

	data = g_slice_new0 (RevisionSearchData);
	data->main_loop = g_main_loop_ref (priv->main_loop);
	data->search_term = search_term;
	data->list = list;

	giggle_git_run_job (priv->git,
			    priv->job,
			    diff_matches_cb,
			    data);

	/* wait here */
	g_main_loop_run (data->main_loop);

	/* At this point the match job has already returned */
	g_main_loop_unref (data->main_loop);
	match = data->match;
	g_slice_free (RevisionSearchData, data);

	return match;
}

static gboolean
revision_matches (GiggleRevListView *list,
		  GiggleRevision     *revision,
		  const gchar        *search_term,
		  gboolean            full_search)
{
	gboolean match = FALSE;

	match = (revision_property_matches (revision, "author", search_term) ||
		 revision_property_matches (revision, "sha", search_term) ||
		 revision_log_matches (list, revision, search_term));

	if (!match && full_search) {
		match = revision_diff_matches (list, revision, search_term);
	}

	return match;
}

static gboolean
rev_list_view_search (GiggleSearchable      *searchable,
		      const gchar           *search_term,
		      GiggleSearchDirection  direction,
		      gboolean               full_search)
{
	GiggleRevListViewPriv *priv;
	GtkTreeModel           *model;
	GtkTreeSelection       *selection;
	GList                  *list;
	GtkTreeIter             iter;
	gboolean                valid, found;
	GiggleRevision         *revision;
	GtkTreePath            *path;

	priv = GET_PRIV (searchable);

	found = FALSE;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (searchable));
	list = gtk_tree_selection_get_selected_rows (selection, &model);
	priv->cancelled = FALSE;

	/* Find the first/current element */
	if (list) {
		if (direction == GIGGLE_SEARCH_DIRECTION_NEXT) {
			path = gtk_tree_path_copy (list->data);
			gtk_tree_path_next (path);
			valid = TRUE;
		} else {
			path = gtk_tree_path_copy ((g_list_last (list))->data);
			valid = gtk_tree_path_prev (path);
		}
	} else {
		path = gtk_tree_path_new_first ();
		valid = TRUE;
	}

	while (valid && !found && !priv->cancelled) {
		valid = gtk_tree_model_get_iter (model, &iter, path);

		if (!valid) {
			break;
		}

		gtk_tree_model_get (model, &iter, 0, &revision, -1);

		if (revision) {
			found = revision_matches (GIGGLE_REV_LIST_VIEW (searchable),
					  revision, search_term, full_search);

			g_object_unref (revision);
		}

		if (!found && !priv->cancelled) {
			if (direction == GIGGLE_SEARCH_DIRECTION_NEXT) {
				gtk_tree_path_next (path);
			} else {
				valid = gtk_tree_path_prev (path);
			}
		}
	}

	if (found && !priv->cancelled) {
		gtk_tree_selection_unselect_all (selection);
		gtk_tree_selection_select_iter (selection, &iter);

		/* scroll to row */
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (searchable),
					      path, NULL, FALSE, 0., 0.);
	}

	gtk_tree_path_free (path);
	g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (list);

	return (found && !priv->cancelled);
}

static void
rev_list_view_cancel_search (GiggleSearchable *searchable)
{
	GiggleRevListViewPriv *priv;

	priv = GET_PRIV (searchable);

	if (!priv->cancelled) {
		priv->cancelled = TRUE;

		if (priv->job) {
			/* cancel the current search inside diffs job */
			giggle_git_cancel_job (priv->git, priv->job);
			g_object_unref (priv->job);
			priv->job = NULL;
		}

		if (g_main_loop_is_running (priv->main_loop)) {
			g_main_loop_quit (priv->main_loop);
		}
	}
}

static void
giggle_rev_list_view_searchable_init (GiggleSearchableIface *iface)
{
	iface->search = rev_list_view_search;
	iface->cancel = rev_list_view_cancel_search;
}

static gboolean
rev_list_view_can_copy (GiggleClipboard *clipboard)
{
	gboolean result = FALSE;
	GList   *selection, *l;
	
	selection = giggle_rev_list_view_get_selection (GIGGLE_REV_LIST_VIEW (clipboard));

	for (l = selection; l; l = l->next) {
		if (selection->data && giggle_revision_get_sha (selection->data)) {
			result = TRUE;
			break;
		}
	}

	while (selection) {
		if (selection->data)
			g_object_unref (selection->data);

		selection = g_list_delete_link (selection, selection);
	}

	return result;
}

static void
rev_list_view_do_copy (GiggleClipboard *clipboard)
{
	GtkClipboard *widget_clipboard;
	GList        *selection;
	GString      *text;
	const char   *sha;

	selection = giggle_rev_list_view_get_selection (GIGGLE_REV_LIST_VIEW (clipboard));
	text = g_string_new (NULL);

	while (selection) {
		if (selection->data) {
			sha = giggle_revision_get_sha (selection->data);

			if (sha) {
				if (text->len)
					g_string_append_c (text, ' ');

				g_string_append (text, sha);
			}

			g_object_unref (selection->data);
		}

		selection = g_list_delete_link (selection, selection);
	}

	if (text->len) {
		widget_clipboard = gtk_widget_get_clipboard (GTK_WIDGET (clipboard),
							     GDK_SELECTION_CLIPBOARD);
		gtk_clipboard_set_text (widget_clipboard, text->str, text->len);
	}

	g_string_free (text, TRUE);
}

static void
giggle_rev_list_view_clipboard_init (GiggleClipboardIface *iface)
{
	iface->can_copy = rev_list_view_can_copy;
	iface->do_copy  = rev_list_view_do_copy;
}

static void
rev_list_view_commit (GtkAction          *action,
		      GiggleRevListView *list)
{
	rev_list_view_commit_changes (list);
}

static void
rev_list_view_create_branch (GtkAction          *action,
			     GiggleRevListView *list)
{
	GiggleRevListViewPriv *priv;
	GtkTreeSelection       *selection;
	GtkTreeIter             iter;
	GtkTreeModel           *model;
	GiggleRevision         *revision;
	GList                  *paths;
	GiggleRef              *branch;
	GtkWidget              *input_dialog;
	const gchar            *branch_name;

	priv = GET_PRIV (list);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	paths = gtk_tree_selection_get_selected_rows (selection, &model);

	g_return_if_fail (paths != NULL);

	gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) paths->data);

	gtk_tree_model_get (model, &iter,
			    COL_OBJECT, &revision,
			    -1);

	input_dialog = giggle_input_dialog_new (_("Enter branch name:"));
	gtk_window_set_transient_for (GTK_WINDOW (input_dialog),
				      GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (list))));

	if (gtk_dialog_run (GTK_DIALOG (input_dialog)) == GTK_RESPONSE_OK) {
		branch_name = giggle_input_dialog_get_text (GIGGLE_INPUT_DIALOG (input_dialog));

		branch = giggle_branch_new (branch_name);
		priv->job = giggle_git_add_ref_new (branch, revision);

		giggle_git_run_job (priv->git,
				    priv->job,
				    modify_ref_cb,
				    list);
		g_object_unref (branch);
	}

	g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (paths);
	g_object_unref (revision);
	gtk_widget_destroy (input_dialog);
}

/* FIXME: pretty equal to rev_list_view_create_branch() */
static void
rev_list_view_create_tag (GtkAction          *action,
			  GiggleRevListView *list)
{
	GiggleRevListViewPriv *priv;
	GtkTreeSelection       *selection;
	GtkTreeIter             iter;
	GtkTreeModel           *model;
	GiggleRevision         *revision;
	GList                  *paths;
	GiggleRef              *tag;
	GtkWidget              *input_dialog;
	const gchar            *tag_name;

	priv = GET_PRIV (list);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	paths = gtk_tree_selection_get_selected_rows (selection, &model);

	g_return_if_fail (paths != NULL);

	gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) paths->data);

	gtk_tree_model_get (model, &iter,
			    COL_OBJECT, &revision,
			    -1);

	input_dialog = giggle_input_dialog_new (_("Enter tag name:"));
	gtk_window_set_transient_for (GTK_WINDOW (input_dialog),
				      GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (list))));

	if (gtk_dialog_run (GTK_DIALOG (input_dialog)) == GTK_RESPONSE_OK) {
		tag_name = giggle_input_dialog_get_text (GIGGLE_INPUT_DIALOG (input_dialog));

		tag = giggle_tag_new (tag_name);
		priv->job = giggle_git_add_ref_new (tag, revision);

		giggle_git_run_job (priv->git,
				    priv->job,
				    modify_ref_cb,
				    list);

		g_object_unref (tag);
	}

	g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (paths);
	g_object_unref (revision);
	gtk_widget_destroy (input_dialog);
}

static void
rev_list_view_create_patch_callback (GiggleGit *git,
				     GiggleJob *job,
				     GError    *error,
				     gpointer   user_data)
{
	GiggleRevListView     *list;
	GiggleRevListViewPriv *priv;
	GtkWidget              *dialog;
	gboolean                result_is_diff;
	gboolean                show_success = TRUE; 
	const gchar            *filename;
	gchar                  *primary_str;
		
	list = GIGGLE_REV_LIST_VIEW (user_data);
	priv = GET_PRIV (list);

	/* Then there is no patch format revision then we have a diff
	 * not a filename in the _get_result() function const gchar *
	 * returned.
	 */
	result_is_diff = giggle_git_diff_get_patch_format (GIGGLE_GIT_DIFF (priv->job)) == NULL;
	if (result_is_diff) {
		filename = g_object_get_data (G_OBJECT (priv->job), "create-patch-filename");
	} else {
		filename = giggle_git_diff_get_result (GIGGLE_GIT_DIFF (priv->job));
	}

	/* Set up the dialog we show */
	if (error) {
		const gchar *secondary_str;
		
		show_success = FALSE;

		if (result_is_diff) {
			primary_str = g_strdup_printf (_("Could not save the patch as %s"), filename);
		} else {
			primary_str = g_strdup_printf (_("Could not create patch"));
		}
		
		if (error->message) {
			secondary_str = error->message;
		} else {
			secondary_str = _("No error was given");
		}

		dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (list))),
							     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							     GTK_MESSAGE_ERROR,
							     GTK_BUTTONS_OK,
							     "<b>%s</b>",
							     primary_str);
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  "%s", secondary_str);

		g_free (primary_str);
	} else if (result_is_diff) {
		GError      *save_error = NULL;
		const gchar *result;
	
		result = giggle_git_diff_get_result (GIGGLE_GIT_DIFF (priv->job));
		
		show_success = g_file_set_contents (filename, result, -1, &save_error);
		if (!show_success) {
			const gchar *secondary_str;

			primary_str = g_strdup_printf (_("Could not save the patch as %s"), filename);

			if (save_error && save_error->message) {
				secondary_str = save_error->message;
			} else {
				secondary_str = _("No error was given");
			}

			dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (list))),
								     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
								     GTK_MESSAGE_ERROR,
								     GTK_BUTTONS_OK,
								     "<b>%s</b>",
								     primary_str);
			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
								  "%s", secondary_str);

			g_free (primary_str);
			g_error_free (save_error);
		}
	} 

	/* We didn't show any of the errors above, report the success */
	if (show_success) {
		gchar *dirname;
		gchar *basename;
		gchar *secondary_str;

		dirname = g_path_get_dirname (filename);
		basename = g_path_get_basename (filename);
		
		primary_str = g_strdup_printf (_("Patch saved as %s"), basename);
		g_free (basename);
		
		if (!dirname || strcmp (dirname, ".") == 0) {
			secondary_str = g_strdup_printf (_("Created in project directory"));
			g_free (dirname);
		} else {
			secondary_str = g_strdup_printf (_("Created in directory %s"), dirname);
			g_free (dirname);
		}		
		
		dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (list))),
							     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							     GTK_MESSAGE_INFO,
							     GTK_BUTTONS_OK,
							     "<b>%s</b>",
							     primary_str);
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), 
							  "%s", secondary_str);
			
		g_free (secondary_str);
		g_free (primary_str);
	}

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	
	g_object_unref (priv->job);
	priv->job = NULL;
}

static void
rev_list_view_create_patch (GtkAction          *action,
			    GiggleRevListView *list)
{
	GiggleRevListViewPriv *priv;
	GtkTreeSelection       *selection;
	GtkTreeIter             iter;
	GtkTreeModel           *model;
	GiggleRevision         *revision;
	GList                  *paths;
	GtkWidget              *dialog;
	gchar                  *filename = NULL;

	priv = GET_PRIV (list);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	paths = gtk_tree_selection_get_selected_rows (selection, &model);

	g_return_if_fail (paths != NULL);

	gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) paths->data);
	gtk_tree_model_get (model, &iter, COL_OBJECT, &revision, -1);
	g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (paths);

	/* If we don't have a revision, it means we selected the item
	 * for uncommitted changes, and we can only do a diff here.
	 */
	if (!revision) {
		dialog = gtk_file_chooser_dialog_new (_("Create Patch"), 
						      GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (list))),
						      GTK_FILE_CHOOSER_ACTION_SAVE,
						      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
						      NULL);
		gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), TRUE);
		
		if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_ACCEPT) {
			gtk_widget_destroy (dialog);
			return;
		}
		
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		gtk_widget_destroy (dialog);
		
		if (!filename || filename[0] == '\0') {
			return;
		}
	}
	
	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
		priv->job = NULL;
	}
		
	priv->job = giggle_git_diff_new ();

	if (!revision) {
		g_object_set_data_full (G_OBJECT (priv->job), "create-patch-filename", filename, g_free);
	} else {
		giggle_git_diff_set_patch_format (GIGGLE_GIT_DIFF (priv->job), revision);
	}

	giggle_git_run_job (priv->git,
			    priv->job,
			    rev_list_view_create_patch_callback,
			    list);

	if (revision) {
		g_object_unref (revision);
	}
}

static void
rev_list_view_cell_data_emblem_func (GtkCellLayout     *layout,
				     GtkCellRenderer   *cell,
				     GtkTreeModel      *model,
				     GtkTreeIter       *iter,
				     gpointer           data)
{
	GiggleRevListViewPriv *priv;
	GiggleRevListView     *list;
	GiggleRevision        *revision;
	GdkPixbuf             *pixbuf = NULL;
	int                    columns = 0, x = 0;
	GList                 *branch_list = NULL;
	GList                 *remote_list = NULL;
	GList                 *tag_list = NULL;

	list = GIGGLE_REV_LIST_VIEW (data);
	priv = GET_PRIV (list);

	gtk_tree_model_get (model, iter,
			    COL_OBJECT, &revision,
			    -1);

	if (revision) {
		branch_list = giggle_revision_get_branch_heads (revision);
		remote_list = giggle_revision_get_remotes (revision);
		tag_list    = giggle_revision_get_tags (revision);
	}

	if (branch_list)
		++columns;
	if (remote_list)
		++columns;
	if (tag_list)
		++columns;

	if (columns > 0) {
		pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
					 priv->emblem_size * columns,
					 priv->emblem_size);
		gdk_pixbuf_fill (pixbuf, 0x00000000);

		if (branch_list) {
			gdk_pixbuf_copy_area (priv->emblem_branch, 0, 0,
					      priv->emblem_size, priv->emblem_size,
					      pixbuf, x, 0);
			x += priv->emblem_size;
		}

		if (remote_list) {
			gdk_pixbuf_copy_area (priv->emblem_remote, 0, 0,
					      priv->emblem_size, priv->emblem_size,
					      pixbuf, x, 0);
			x += priv->emblem_size;
		}

		if (tag_list) {
			gdk_pixbuf_copy_area (priv->emblem_tag, 0, 0,
					      priv->emblem_size, priv->emblem_size,
					      pixbuf, x, 0);
			x += priv->emblem_size;
		}
	}

	g_object_set (cell, "pixbuf", pixbuf, NULL);

	if (pixbuf)
		g_object_unref (pixbuf);

	if (revision)
		g_object_unref (revision);
}

static void
rev_list_view_cell_data_log_func (GtkCellLayout   *layout,
				  GtkCellRenderer *cell,
				  GtkTreeModel    *model,
				  GtkTreeIter     *iter,
				  gpointer         data)
{
	GiggleRevListViewPriv *priv;
	GiggleRevision         *revision;
	gchar                  *markup;

	priv = GET_PRIV (data);

	gtk_tree_model_get (model, iter,
			    COL_OBJECT, &revision,
			    -1);

	if (revision) {
		g_object_set (cell,
			      "text", giggle_revision_get_short_log (revision),
			      NULL);
		g_object_unref (revision);
	} else {
		markup = g_strdup_printf ("<b>%s</b>", _("Uncommitted changes"));
		g_object_set (cell,
			      "markup", markup,
			      NULL);
		g_free (markup);
	}
}

static void
rev_list_view_cell_data_author_func (GtkCellLayout   *layout,
				     GtkCellRenderer *cell,
				     GtkTreeModel    *model,
				     GtkTreeIter     *iter,
				     gpointer         data)
{
	GiggleRevListViewPriv *priv;
	GiggleRevision         *revision;
	const gchar            *author = NULL;

	priv = GET_PRIV (data);

	gtk_tree_model_get (model, iter,
			    COL_OBJECT, &revision,
			    -1);

	if (revision) {
		author = giggle_revision_get_author (revision);
	}

	g_object_set (cell, "text", author, NULL);

	if (revision) {
		g_object_unref (revision);
	}
}

static gchar *
rev_list_view_get_formatted_time (const struct tm *rev_tm)
{
	struct tm *tm;
	time_t t1, t2;

	t1 = mktime ((struct tm *) rev_tm);

	/* check whether it's ahead in time */
	time (&t2);
	if (t1 > t2) {
		return g_strdup ("%c");
	}

	/* check whether it's as fresh as today's bread */
	t2 = time (NULL);
	tm = localtime (&t2);
	tm->tm_sec = tm->tm_min = tm->tm_hour = 0;
	t2 = mktime (tm);

	if (t1 > t2) {
		/* TRANSLATORS: it's a strftime format string */
		return g_strdup (_("%I:%M %p"));
	}

	/* check whether it's older than a week */
	t2 = time (NULL);
	tm = localtime (&t2);
	tm->tm_sec = tm->tm_min = tm->tm_hour = 0;
	t2 = mktime (tm);

	t2 -= 60 * 60 * 24 * 6; /* substract 1 week */

	if (t1 > t2) {
		/* TRANSLATORS: it's a strftime format string */
		return g_strdup (_("%a %I:%M %p"));
	}

	/* check whether it's more recent than the new year hangover */
	t2 = time (NULL);
	tm = localtime (&t2);
	tm->tm_sec = tm->tm_min = tm->tm_hour = tm->tm_mon = 0;
	tm->tm_mday = 1;
	t2 = mktime (tm);

	if (t1 > t2) {
		/* TRANSLATORS: it's a strftime format string */
		return g_strdup (_("%b %d %I:%M %p"));
	}

	/* it's older */
	/* TRANSLATORS: it's a strftime format string */
	return g_strdup (_("%b %d %Y"));
}

static void
rev_list_view_cell_data_date_func (GtkCellLayout   *layout,
				   GtkCellRenderer *cell,
				   GtkTreeModel    *model,
				   GtkTreeIter     *iter,
				   gpointer         data)
{
	GiggleRevListViewPriv *priv;
	GiggleRevision         *revision;
	gchar                  *format;
	gchar                   buf[256];
	const struct tm        *tm;

	priv = GET_PRIV (data);

	gtk_tree_model_get (model, iter,
			    COL_OBJECT, &revision,
			    -1);

	if (revision) {
		tm = giggle_revision_get_date (revision);

		if (tm) {
			format = rev_list_view_get_formatted_time (tm);
			strftime (buf, sizeof (buf), format, tm);

			g_object_set (cell,
				      "text", buf,
				      NULL);

			g_free (format);
			g_object_unref (revision);
		}
	} else {
		g_object_set (cell, "text", NULL, NULL);
	}
}

static void
rev_list_view_selection_changed_cb (GtkTreeSelection  *selection,
				    gpointer           data)
{
	GiggleRevListView     *list;
	GiggleRevListViewPriv *priv;
	GtkTreeModel           *model;
	GList                  *rows, *last_row;
	GtkTreeIter             first_iter;
	GtkTreeIter             last_iter;
	GiggleRevision         *first_revision;
	GiggleRevision         *last_revision;
	gboolean                valid;

	list = GIGGLE_REV_LIST_VIEW (data);
	priv = GET_PRIV (list);

	giggle_clipboard_changed (GIGGLE_CLIPBOARD (list));

	rows = gtk_tree_selection_get_selected_rows (selection, &model);
	first_revision = last_revision = NULL;

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
	}

	if (first_revision != priv->first_revision ||
	    last_revision != priv->last_revision) {
		priv->first_revision = first_revision;
		priv->last_revision = last_revision;

		g_signal_emit (list, signals [SELECTION_CHANGED], 0,
			       first_revision, last_revision);
	}

	if (first_revision)
		g_object_unref (first_revision);
	if (last_revision)
		g_object_unref (last_revision);

	g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (rows);
}

static void
giggle_rev_list_view_init (GiggleRevListView *rev_list_view)
{
	GtkActionEntry menu_items [] = {
		{ "Commit",         NULL,                 N_("Commit"),         NULL, NULL, G_CALLBACK (rev_list_view_commit) },
		{ "CreateBranch",   NULL,                 N_("Create _Branch"), NULL, NULL, G_CALLBACK (rev_list_view_create_branch) },
		{ "CreateTag",      "stock_add-bookmark", N_("Create _Tag"),    NULL, NULL, G_CALLBACK (rev_list_view_create_tag) },
		{ "CreatePatch",    NULL,                 N_("Create _Patch"),  NULL, NULL, G_CALLBACK (rev_list_view_create_patch) },
	};

	const gchar *ui_description =
		"<ui>"
		"  <popup name='PopupMenu'>"
		"    <menuitem action='Commit'/>"
		"    <menuitem action='CreateBranch'/>"
		"    <menuitem action='CreateTag'/>"
		"    <separator/>"
		"    <menuitem action='CreatePatch'/>"
		"    <separator/>"
		"    <placeholder name='Refs'/>"
		"  </popup>"
		"</ui>";

	GiggleRevListViewPriv *priv;
	GtkTreeSelection      *selection;
	GtkActionGroup        *action_group;
	GtkTreeViewColumn     *column;
	gint                   font_size;
	GtkCellRenderer       *cell;

	priv = GET_PRIV (rev_list_view);
	font_size = pango_font_description_get_size (GTK_WIDGET (rev_list_view)->style->font_desc);
	font_size = PANGO_PIXELS (font_size);

	/* yes, it's a hack */
	priv->first_revision = (GiggleRevision *) 0x1;
	priv->last_revision = (GiggleRevision *) 0x1;

	priv->git = giggle_git_get ();
	priv->main_loop = g_main_loop_new (NULL, FALSE);

	gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (rev_list_view), TRUE);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (rev_list_view), TRUE);

	/* emblems renderer */
	priv->emblem_column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (priv->emblem_column, GTK_TREE_VIEW_COLUMN_FIXED);

	cell = gtk_cell_renderer_pixbuf_new ();

	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->emblem_column), cell, TRUE);

	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (priv->emblem_column), cell,
					    rev_list_view_cell_data_emblem_func,
					    rev_list_view, NULL);

	gtk_tree_view_insert_column (GTK_TREE_VIEW (rev_list_view),
				     priv->emblem_column, -1);

	/* graph renderer */
	priv->graph_column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_resizable (priv->graph_column, TRUE);
	gtk_tree_view_column_set_sizing (priv->graph_column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_min_width (priv->graph_column, font_size * 10);

	priv->graph_renderer = giggle_graph_renderer_new ();

	gtk_tree_view_column_set_title (priv->graph_column, _("Graph"));
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->graph_column),
				    priv->graph_renderer, FALSE);

	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->graph_column),
					priv->graph_renderer,
					"revision", COL_OBJECT,
					NULL);

	gtk_tree_view_insert_column (GTK_TREE_VIEW (rev_list_view),
				     priv->graph_column, -1);

	/* log cell renderer */
	cell = gtk_cell_renderer_text_new ();
	gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT (cell), 1);
	g_object_set (cell, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Short Log"));
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_min_width (column, font_size * 10);
	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_resizable (column, TRUE);

	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column), cell, TRUE);

	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (column), cell,
					    rev_list_view_cell_data_log_func,
					    rev_list_view, NULL);

	gtk_tree_view_insert_column (GTK_TREE_VIEW (rev_list_view), column, -1);

	/* Author cell renderer */
	cell = gtk_cell_renderer_text_new ();
	gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT (cell), 1);

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Author"));
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, font_size * 14);
	gtk_tree_view_column_set_resizable (column, TRUE);

	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column), cell, TRUE);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (column), cell,
					    rev_list_view_cell_data_author_func,
					    rev_list_view, NULL);

	gtk_tree_view_insert_column (GTK_TREE_VIEW (rev_list_view), column, -1);

	/* Date cell renderer */
	cell = gtk_cell_renderer_text_new ();
	gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT (cell), 1);

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Date"));
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_min_width (column, font_size * 14);
	gtk_tree_view_column_set_resizable (column, TRUE);

	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column), cell, TRUE);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (column), cell,
					    rev_list_view_cell_data_date_func,
					    rev_list_view, NULL);

	gtk_tree_view_insert_column (GTK_TREE_VIEW (rev_list_view), column, -1);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (rev_list_view));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW (rev_list_view), TRUE);

	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (rev_list_view_selection_changed_cb),
			  rev_list_view);

	gtk_widget_set_has_tooltip (GTK_WIDGET (rev_list_view), TRUE);

	/* create the popup menu */
	action_group = gtk_action_group_new ("PopupActions");
	priv->refs_action_group = gtk_action_group_new ("Refs");

	gtk_action_group_set_translation_domain (action_group, NULL);
	gtk_action_group_add_actions (action_group, menu_items,
				      G_N_ELEMENTS (menu_items), rev_list_view);

	priv->ui_manager = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (priv->ui_manager, action_group, 0);
	gtk_ui_manager_insert_action_group (priv->ui_manager, priv->refs_action_group, 1);

	g_object_unref (priv->refs_action_group);

	if (gtk_ui_manager_add_ui_from_string (priv->ui_manager, ui_description, -1, NULL))
		priv->popup = gtk_ui_manager_get_widget (priv->ui_manager, "/ui/PopupMenu");
}

GtkWidget*
giggle_rev_list_view_new (void)
{
	return g_object_new (GIGGLE_TYPE_REV_LIST_VIEW, NULL);
}

void
giggle_rev_list_view_set_model (GiggleRevListView *list,
				GtkTreeModel      *model)
{
	GiggleRevListViewPriv *priv;
	GType                   type;

	g_return_if_fail (GIGGLE_IS_REV_LIST_VIEW (list));
	g_return_if_fail (!model || GTK_IS_TREE_MODEL (model));

	if (model) {
		/* we want the first column to contain GiggleRevisions */
		type = gtk_tree_model_get_column_type (model, COL_OBJECT);
		g_return_if_fail (type == GIGGLE_TYPE_REVISION);
	}

	priv = GET_PRIV (list);

	if (model) {
		giggle_graph_renderer_validate_model
			(GIGGLE_GRAPH_RENDERER (priv->graph_renderer),
			 model, COL_OBJECT);
	}

	gtk_tree_view_set_model (GTK_TREE_VIEW (list), model);
}

gboolean
giggle_rev_list_view_get_graph_visible (GiggleRevListView *list)
{
	GiggleRevListViewPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REV_LIST_VIEW (list), FALSE);

	priv = GET_PRIV (list);
	return priv->show_graph;
}

void
giggle_rev_list_view_set_graph_visible (GiggleRevListView *list,
					gboolean            show_graph)
{
	GiggleRevListViewPriv *priv;

	g_return_if_fail (GIGGLE_IS_REV_LIST_VIEW (list));

	priv = GET_PRIV (list);

	priv->show_graph = (show_graph == TRUE);
	gtk_tree_view_column_set_visible (priv->graph_column, priv->show_graph);
	g_object_notify (G_OBJECT (list), "graph-visible");
}

GList *
giggle_rev_list_view_get_selection (GiggleRevListView *list)
{
	GList            *result = NULL;
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GList            *rows;

	g_return_val_if_fail (GIGGLE_IS_REV_LIST_VIEW (list), NULL);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	rows = gtk_tree_selection_get_selected_rows (selection, &model);

	while (rows) {
		GiggleRevision *revision;
		GtkTreeIter     iter;

		if (gtk_tree_model_get_iter (model, &iter, rows->data)) {
			gtk_tree_model_get (model, &iter,
					    COL_OBJECT, &revision,
					    -1);

			result = g_list_prepend (result, revision);
		}

		gtk_tree_path_free (rows->data);
		rows = g_list_delete_link (rows, rows);
	}

	return result;
}

int
giggle_rev_list_view_set_selection (GiggleRevListView *list,
				    GList             *revisions)
{
	int               count = 0;
	GtkTreeSelection *selection;
	GiggleRevision   *revision;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	GList            *l;

	g_return_val_if_fail (GIGGLE_IS_REV_LIST_VIEW (list), 0);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));
	revisions = g_list_copy (revisions);

	gtk_tree_selection_unselect_all (selection);

	if (revisions && gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			gtk_tree_model_get (model, &iter, COL_OBJECT, &revision, -1);
			l = g_list_find_custom (revisions, revision, giggle_revision_compare);

			if (l) {
				gtk_tree_selection_select_iter (selection, &iter);
				revisions = g_list_delete_link (revisions, l);
				count += 1;

				if (!revisions)
					break;
			}
		} while (gtk_tree_model_iter_next (model, &iter));
	}

	return count;
}
