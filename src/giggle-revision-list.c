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

#include "giggle-git.h"
#include "giggle-job.h"
#include "giggle-git-log.h"
#include "giggle-git-diff.h"
#include "giggle-graph-renderer.h"
#include "giggle-revision-tooltip.h"
#include "giggle-revision-list.h"
#include "giggle-revision.h"
#include "giggle-marshal.h"
#include "giggle-searchable.h"
#include "giggle-branch.h"
#include "giggle-tag.h"
#include "giggle-git-add-ref.h"
#include "giggle-git-delete-ref.h"
#include "giggle-input-dialog.h"
#include "giggle-diff-window.h"

typedef struct GiggleRevisionListPriv GiggleRevisionListPriv;

struct GiggleRevisionListPriv {
	GtkTreeViewColumn *graph_column;
	GtkCellRenderer   *graph_renderer;

	GtkTreeViewColumn *emblem_column;
	GtkCellRenderer   *emblem_renderer;

	GtkCellRenderer   *log_renderer;
	GtkCellRenderer   *author_renderer;
	GtkCellRenderer   *date_renderer;

	GtkIconTheme      *icon_theme;

	GtkWidget         *revision_tooltip;

	GiggleGit         *git;
	GiggleJob         *job;

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
	guint              compact_mode : 1;
	guint              cancelled : 1;
};

typedef struct RevisionSearchData RevisionSearchData;

struct RevisionSearchData {
	GMainLoop          *main_loop;
	const gchar        *search_term;
	gboolean            match;
	GiggleRevisionList *list;
};

enum {
	COL_OBJECT,
	NUM_COLUMNS,
};

enum {
	PROP_0,
	PROP_GRAPH_VISIBLE,
	PROP_COMPACT_MODE,
};

enum {
	SELECTION_CHANGED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

#define EMBLEM_SIZE 16

static void revision_list_finalize                (GObject *object);
static void giggle_revision_list_searchable_init  (GiggleSearchableIface *iface);
static void revision_list_get_property            (GObject        *object,
						   guint           param_id,
						   GValue         *value,
						   GParamSpec     *pspec);
static void revision_list_set_property            (GObject        *object,
						   guint           param_id,
						   const GValue   *value,
						   GParamSpec     *pspec);

static gboolean revision_list_button_press        (GtkWidget      *widget,
						   GdkEventButton *event);
static gboolean revision_list_motion_notify       (GtkWidget      *widget,
						   GdkEventMotion *event);
static gboolean revision_list_leave_notify        (GtkWidget        *widget,
						   GdkEventCrossing *event);
static void revision_list_style_set               (GtkWidget        *widget,
						   GtkStyle         *prev_style);

static void revision_list_cell_data_emblem_func   (GtkCellLayout     *layout,
						   GtkCellRenderer   *cell,
						   GtkTreeModel      *model,
						   GtkTreeIter       *iter,
						   gpointer           data);
static void revision_list_cell_data_log_func      (GtkCellLayout     *layout,
						   GtkCellRenderer   *cell,
						   GtkTreeModel      *model,
						   GtkTreeIter       *iter,
						   gpointer           data);
static void revision_list_cell_data_author_func   (GtkCellLayout     *layout,
						   GtkCellRenderer   *cell,
						   GtkTreeModel      *model,
						   GtkTreeIter       *iter,
						   gpointer           data);
static void revision_list_cell_data_date_func     (GtkCellLayout     *layout,
						   GtkCellRenderer   *cell,
						   GtkTreeModel      *model,
						   GtkTreeIter       *iter,
						   gpointer           data);
static void revision_list_selection_changed_cb    (GtkTreeSelection  *selection,
						   gpointer           data);

static gboolean revision_list_search              (GiggleSearchable      *searchable,
						   const gchar           *search_term,
						   GiggleSearchDirection  direction,
						   gboolean               full_search);
static void revision_list_cancel_search           (GiggleSearchable      *searchable);

static void revision_list_commit                  (GtkAction          *action,
						   GiggleRevisionList *list);
static void revision_list_create_branch           (GtkAction          *action,
						   GiggleRevisionList *list);
static void revision_list_create_tag              (GtkAction          *action,
						   GiggleRevisionList *list);


G_DEFINE_TYPE_WITH_CODE (GiggleRevisionList, giggle_revision_list, GTK_TYPE_TREE_VIEW,
			 G_IMPLEMENT_INTERFACE (GIGGLE_TYPE_SEARCHABLE,
						giggle_revision_list_searchable_init))

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REVISION_LIST, GiggleRevisionListPriv))

#define COMMIT_UI_PATH        "/ui/PopupMenu/Commit"
#define CREATE_BRANCH_UI_PATH "/ui/PopupMenu/CreateBranch"
#define CREATE_TAG_UI_PATH    "/ui/PopupMenu/CreateTag"

static GtkActionEntry menu_items [] = {
	{ "Commit",         NULL,                 N_("Commit"),         NULL, NULL, G_CALLBACK (revision_list_commit) },
	{ "CreateBranch",   NULL,                 N_("Create _Branch"), NULL, NULL, G_CALLBACK (revision_list_create_branch) },
	{ "CreateTag",      "stock_add-bookmark", N_("Create _Tag"),    NULL, NULL, G_CALLBACK (revision_list_create_tag) },
};

static const gchar *ui_description =
	"<ui>"
	"  <popup name='PopupMenu'>"
	"    <menuitem action='Commit'/>"
	"    <menuitem action='CreateBranch'/>"
	"    <menuitem action='CreateTag'/>"
	"    <separator/>"
	"    <placeholder name='Refs'/>"
	"  </popup>"
	"</ui>";

static void
giggle_revision_list_class_init (GiggleRevisionListClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

	object_class->finalize = revision_list_finalize;
	object_class->set_property = revision_list_set_property;
	object_class->get_property = revision_list_get_property;

	widget_class->button_press_event = revision_list_button_press;
	widget_class->motion_notify_event = revision_list_motion_notify;
	widget_class->leave_notify_event = revision_list_leave_notify;
	widget_class->style_set = revision_list_style_set;

	g_object_class_install_property (
		object_class,
		PROP_GRAPH_VISIBLE,
		g_param_spec_boolean ("graph-visible",
				      "Graph visible",
				      "Whether to show the revisions graph",
				      FALSE,
				      G_PARAM_READWRITE));
	g_object_class_install_property (
		object_class,
		PROP_COMPACT_MODE,
		g_param_spec_boolean ("compact-mode",
				      "Compact mode",
				      "Whether to show the list in compact mode or not",
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
giggle_revision_list_searchable_init (GiggleSearchableIface *iface)
{
	iface->search = revision_list_search;
	iface->cancel = revision_list_cancel_search;
}

static void
giggle_revision_list_init (GiggleRevisionList *revision_list)
{
	GiggleRevisionListPriv *priv;
	GtkTreeSelection       *selection;
	GtkActionGroup         *action_group;
	GtkTreeViewColumn      *column;
	gint                    font_size;

	priv = GET_PRIV (revision_list);
	font_size = pango_font_description_get_size (GTK_WIDGET (revision_list)->style->font_desc);
	font_size = PANGO_PIXELS (font_size);

	/* yes, it's a hack */
	priv->first_revision = (GiggleRevision *) 0x1;
	priv->last_revision = (GiggleRevision *) 0x1;

	priv->icon_theme = gtk_icon_theme_get_default ();
	priv->git = giggle_git_get ();
	priv->main_loop = g_main_loop_new (NULL, FALSE);

	gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (revision_list), TRUE);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (revision_list), TRUE);

	/* emblems renderer */
	priv->emblem_column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (priv->emblem_column,
					 GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_min_width (priv->emblem_column,
					    EMBLEM_SIZE + (2 * GTK_WIDGET (revision_list)->style->xthickness));
	g_object_ref_sink (priv->emblem_column);

	priv->emblem_renderer = gtk_cell_renderer_pixbuf_new ();
	g_object_ref_sink (priv->emblem_renderer);

	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->emblem_column),
				    priv->emblem_renderer, TRUE);

	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (priv->emblem_column),
					    priv->emblem_renderer,
					    revision_list_cell_data_emblem_func,
					    revision_list,
					    NULL);

	gtk_tree_view_insert_column (GTK_TREE_VIEW (revision_list),
				     priv->emblem_column, -1);

	/* graph renderer */
	priv->graph_column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_resizable (priv->graph_column, TRUE);
	gtk_tree_view_column_set_sizing (priv->graph_column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_min_width (priv->graph_column, font_size * 10);
	g_object_ref_sink (priv->graph_column);

	priv->graph_renderer = giggle_graph_renderer_new ();
	g_object_ref_sink (priv->graph_renderer);

	gtk_tree_view_column_set_title (priv->graph_column, _("Graph"));
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->graph_column),
				    priv->graph_renderer, FALSE);

	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->graph_column),
					priv->graph_renderer,
					"revision", COL_OBJECT,
					NULL);

	gtk_tree_view_insert_column (GTK_TREE_VIEW (revision_list),
				     priv->graph_column, -1);

	/* log cell renderer */
	priv->log_renderer = gtk_cell_renderer_text_new ();
	gtk_cell_renderer_text_set_fixed_height_from_font (
		GTK_CELL_RENDERER_TEXT (priv->log_renderer), 1);
	g_object_set(priv->log_renderer,
		     "ellipsize", PANGO_ELLIPSIZE_END,
		     NULL);

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Short Log"));
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_min_width (column, font_size * 10);
	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_resizable (column, TRUE);

	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column),
				    priv->log_renderer, TRUE);

	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (column),
					    priv->log_renderer,
					    revision_list_cell_data_log_func,
					    revision_list,
					    NULL);

	gtk_tree_view_insert_column (GTK_TREE_VIEW (revision_list), column, -1);

	/* Author cell renderer */
	priv->author_renderer = gtk_cell_renderer_text_new ();
	gtk_cell_renderer_text_set_fixed_height_from_font (
		GTK_CELL_RENDERER_TEXT (priv->author_renderer), 1);

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Author"));
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, font_size * 14);
	gtk_tree_view_column_set_resizable (column, TRUE);

	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column),
				    priv->author_renderer, TRUE);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (column),
					    priv->author_renderer,
					    revision_list_cell_data_author_func,
					    revision_list,
					    NULL);

	gtk_tree_view_insert_column (GTK_TREE_VIEW (revision_list), column, -1);

	/* Date cell renderer */
	priv->date_renderer = gtk_cell_renderer_text_new ();
	gtk_cell_renderer_text_set_fixed_height_from_font (
		GTK_CELL_RENDERER_TEXT (priv->date_renderer), 1);

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Date"));
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_min_width (column, font_size * 14);
	gtk_tree_view_column_set_resizable (column, TRUE);

	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column),
				    priv->date_renderer, TRUE);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (column),
					    priv->date_renderer,
					    revision_list_cell_data_date_func,
					    revision_list,
					    NULL);

	gtk_tree_view_insert_column (GTK_TREE_VIEW (revision_list), column, -1);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (revision_list));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW (revision_list), TRUE);

	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (revision_list_selection_changed_cb),
			  revision_list);

	priv->revision_tooltip = giggle_revision_tooltip_new ();

	gtk_rc_parse_string ("style \"revision-list-compact-style\""
			     "{"
			     "  GtkTreeView::vertical-separator = 0"
			     "}"
			     "widget \"*.revision-list\" style \"revision-list-compact-style\"");

	/* create the popup menu */
	action_group = gtk_action_group_new ("PopupActions");
	priv->refs_action_group = gtk_action_group_new ("Refs");

	gtk_action_group_set_translation_domain (action_group, NULL);
	gtk_action_group_add_actions (action_group, menu_items,
				      G_N_ELEMENTS (menu_items), revision_list);

	priv->ui_manager = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (priv->ui_manager, action_group, 0);
	gtk_ui_manager_insert_action_group (priv->ui_manager, priv->refs_action_group, 1);

	if (gtk_ui_manager_add_ui_from_string (priv->ui_manager, ui_description, -1, NULL)) {
		priv->popup = gtk_ui_manager_get_widget (priv->ui_manager, "/ui/PopupMenu");
	}
}

static void
revision_list_finalize (GObject *object)
{
	GiggleRevisionListPriv *priv;

	priv = GET_PRIV (object);

	g_object_unref (priv->graph_column);
	g_object_unref (priv->emblem_renderer);
	g_object_unref (priv->graph_renderer);
	gtk_widget_destroy (priv->revision_tooltip);

	if (priv->job) {
		giggle_git_cancel_job (priv->git, priv->job);
		g_object_unref (priv->job);
	}

	g_object_unref (priv->git);
	g_object_unref (priv->refs_action_group);

	if (g_main_loop_is_running (priv->main_loop)) {
		g_main_loop_quit (priv->main_loop);
	}

	g_main_loop_unref (priv->main_loop);

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
	case PROP_GRAPH_VISIBLE:
		g_value_set_boolean (value, priv->show_graph);
		break;
	case PROP_COMPACT_MODE:
		g_value_set_boolean (value, priv->compact_mode);
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
	case PROP_GRAPH_VISIBLE:
		giggle_revision_list_set_graph_visible (GIGGLE_REVISION_LIST (object),
							g_value_get_boolean (value));
		break;
	case PROP_COMPACT_MODE:
		giggle_revision_list_set_compact_mode (GIGGLE_REVISION_LIST (object),
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
	GiggleRevisionListPriv *priv;

	priv = GET_PRIV (user_data);

	/* FIXME: error reporting missing */
	if (!error) {
		g_object_notify (G_OBJECT (priv->git), "git-dir");
	}

	g_object_unref (priv->job);
	priv->job = NULL;
}

static void
revision_list_activate_action (GtkAction          *action,
			       GiggleRevisionList *list)
{	
	GiggleRevisionListPriv *priv;
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
revision_list_clear_popup_refs (GiggleRevisionList *list)
{
	GiggleRevisionListPriv *priv;
	GList                  *actions, *l;

	priv = GET_PRIV (list);
	actions = gtk_action_group_list_actions (priv->refs_action_group);

	if (priv->refs_merge_id) {
		gtk_ui_manager_remove_ui (priv->ui_manager, priv->refs_merge_id);
	}

	for (l = actions; l != NULL; l = l->next) {
		g_signal_handlers_disconnect_by_func (GTK_ACTION (l->data),
                                                      G_CALLBACK (revision_list_activate_action),
                                                      list);

		gtk_action_group_remove_action (priv->refs_action_group, l->data);
	}

	/* prepare a new merge id */
	priv->refs_merge_id = gtk_ui_manager_new_merge_id (priv->ui_manager);

	g_list_free (actions);
}

static void
revision_list_add_popup_refs (GiggleRevisionList *list,
			      GiggleRevision     *revision,
			      GList              *refs,
			      const gchar        *label_str,
			      const gchar        *name_str)
{
	GiggleRevisionListPriv *priv;
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
				  G_CALLBACK (revision_list_activate_action),
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
revision_list_setup_popup (GiggleRevisionList *list,
			   GiggleRevision     *revision)
{
	GiggleRevisionListPriv *priv;
	GtkAction              *action;

	priv = GET_PRIV (list);

	/* clear action list */
	revision_list_clear_popup_refs (list);

	if (revision) {
		action = gtk_ui_manager_get_action (priv->ui_manager, COMMIT_UI_PATH);
		gtk_action_set_visible (action, FALSE);

		action = gtk_ui_manager_get_action (priv->ui_manager, CREATE_BRANCH_UI_PATH);
		gtk_action_set_visible (action, TRUE);

		action = gtk_ui_manager_get_action (priv->ui_manager, CREATE_TAG_UI_PATH);
		gtk_action_set_visible (action, TRUE);

		/* repopulate action list */
		revision_list_add_popup_refs (list, revision,
					      giggle_revision_get_branch_heads (revision),
					      _("Delete branch \"%s\""), "branch-%s");
		revision_list_add_popup_refs (list, revision,
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

	gtk_ui_manager_ensure_update (priv->ui_manager);
}

static void
revision_list_commit_changes (GiggleRevisionList *list)
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
revision_list_button_press (GtkWidget      *widget,
			    GdkEventButton *event)
{
	GiggleRevisionListPriv *priv;
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

		revision_list_setup_popup (GIGGLE_REVISION_LIST (widget), revision);

		gtk_menu_popup (GTK_MENU (priv->popup), NULL, NULL,
				NULL, NULL, event->button, event->time);

		gtk_tree_path_free (path);
	} else {
		GTK_WIDGET_CLASS (giggle_revision_list_parent_class)->button_press_event (widget, event);

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

			if (!revision) {
				/* clicked on the uncommitted changes revision */
				revision_list_commit_changes (GIGGLE_REVISION_LIST (widget));
			} else {
				g_object_unref (revision);
			}
		}
	}

	return TRUE;
}

static gboolean
revision_list_motion_notify (GtkWidget      *widget,
			     GdkEventMotion *event)
{
	GiggleRevisionListPriv *priv;
	GtkTreeModel           *model;
	GtkTreePath            *path = NULL;
	GtkTreeViewColumn      *column;
	GtkTreeIter             iter;
	gint                    cell_x, start, width;
	GiggleRevision         *revision = NULL;

	priv = GET_PRIV (widget);
	GTK_WIDGET_CLASS (giggle_revision_list_parent_class)->motion_notify_event (widget, event);

	if (event->window != gtk_tree_view_get_bin_window (GTK_TREE_VIEW (widget))) {
		goto failed;
	}

	/* are we in the correct column? */
	if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
					    event->x, event->y,
					    &path, &column, &cell_x, NULL) ||
	    column != priv->emblem_column) {
		goto failed;
	}

	gtk_tree_view_column_cell_get_position (column, priv->emblem_renderer,
						&start, &width);

	/* are we in the correct renderer? */
	if (cell_x < start ||
	    cell_x > start + width) {
		goto failed;
	}

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get (model, &iter,
			    COL_OBJECT, &revision,
			    -1);

	if (!revision) {
		goto failed;
	}

	if (!giggle_revision_get_remotes (revision) &&
	    !giggle_revision_get_tags (revision) &&
	    !giggle_revision_get_branch_heads (revision)) {
		goto failed;
	}

	giggle_revision_tooltip_set_revision (GIGGLE_REVISION_TOOLTIP (priv->revision_tooltip),
					      revision);
	gtk_widget_show (priv->revision_tooltip);

	gtk_window_move (GTK_WINDOW (priv->revision_tooltip),
			 event->x_root + 16,
			 event->y_root + 16);

	goto cleanup;

 failed:
	gtk_widget_hide (priv->revision_tooltip);
 cleanup:
	if (revision) {
		g_object_unref (revision);
	}

	if (path) {
		gtk_tree_path_free (path);
	}

	return FALSE;
}

static gboolean
revision_list_leave_notify (GtkWidget        *widget,
			    GdkEventCrossing *event)
{
	GiggleRevisionListPriv *priv;

	priv = GET_PRIV (widget);
	gtk_widget_hide (priv->revision_tooltip);

	GTK_WIDGET_CLASS (giggle_revision_list_parent_class)->leave_notify_event (widget, event);
	return FALSE;
}

static void
revision_list_update_compact_mode (GiggleRevisionList *list)
{
#if 0
	PangoFontDescription   *font_desc;
	GiggleRevisionListPriv *priv;
	gint                    size;

	priv = GET_PRIV (list);

	if (priv->compact_mode) {
		font_desc = GTK_WIDGET (list)->style->font_desc;
		size = pango_font_description_get_size (font_desc);
		pango_font_description_set_size (font_desc, size * PANGO_SCALE_SMALL);
	}
#endif
}

static void
revision_list_style_set (GtkWidget *widget,
			 GtkStyle  *prev_style)
{
	revision_list_update_compact_mode (GIGGLE_REVISION_LIST (widget));

	(GTK_WIDGET_CLASS (giggle_revision_list_parent_class)->style_set) (widget, prev_style);
}

static void
revision_list_cell_data_emblem_func (GtkCellLayout     *layout,
				     GtkCellRenderer   *cell,
				     GtkTreeModel      *model,
				     GtkTreeIter       *iter,
				     gpointer           data)
{
	GiggleRevisionListPriv *priv;
	GiggleRevisionList     *list;
	GiggleRevision         *revision;
	GdkPixbuf              *pixbuf = NULL;

	list = GIGGLE_REVISION_LIST (data);
	priv = GET_PRIV (list);

	gtk_tree_model_get (model, iter,
			    COL_OBJECT, &revision,
			    -1);

	if (revision &&
	    (giggle_revision_get_tags (revision) ||
	     giggle_revision_get_remotes (revision) ||
	     giggle_revision_get_branch_heads (revision))) {
		pixbuf = gtk_icon_theme_load_icon (priv->icon_theme,
						   "gtk-info", EMBLEM_SIZE, 0, NULL);
	}

	g_object_set (cell,
		      "pixbuf", pixbuf,
		      NULL);

	if (pixbuf) {
		g_object_unref (pixbuf);
	}

	if (revision) {
		g_object_unref (revision);
	}
}

static void
revision_list_cell_data_log_func (GtkCellLayout   *layout,
				  GtkCellRenderer *cell,
				  GtkTreeModel    *model,
				  GtkTreeIter     *iter,
				  gpointer         data)
{
	GiggleRevisionListPriv *priv;
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
revision_list_cell_data_author_func (GtkCellLayout   *layout,
				     GtkCellRenderer *cell,
				     GtkTreeModel    *model,
				     GtkTreeIter     *iter,
				     gpointer         data)
{
	GiggleRevisionListPriv *priv;
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
revision_list_get_formatted_time (const struct tm *rev_tm)
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
revision_list_cell_data_date_func (GtkCellLayout   *layout,
				   GtkCellRenderer *cell,
				   GtkTreeModel    *model,
				   GtkTreeIter     *iter,
				   gpointer         data)
{
	GiggleRevisionListPriv *priv;
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
			format = revision_list_get_formatted_time (tm);
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
	} else if (first_revision) {
		/* maybe select a better parent? */
		GList* parents = giggle_revision_get_parents (first_revision);
		last_revision = parents ? g_object_ref(parents->data) : NULL;
	}

	if (first_revision != priv->first_revision ||
	    last_revision != priv->last_revision) {
		priv->first_revision = first_revision;
		priv->last_revision = last_revision;

		g_signal_emit (list, signals [SELECTION_CHANGED], 0,
			       first_revision, last_revision);
	}

	if (first_revision) {
		g_object_unref (first_revision);
	}

	if (last_revision) {
		g_object_unref (last_revision);
	}

	g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (rows);
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
diff_matches_cb (GiggleGit *git,
		 GiggleJob *job,
		 GError    *error,
		 gpointer   user_data)
{
	RevisionSearchData     *data;
	GiggleRevisionListPriv *priv;
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

static void
log_matches_cb (GiggleGit *git,
		 GiggleJob *job,
		 GError    *error,
		 gpointer   user_data)
{
	RevisionSearchData     *data;
	GiggleRevisionListPriv *priv;
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
revision_diff_matches (GiggleRevisionList *list,
		       GiggleRevision     *revision,
		       const gchar        *search_term)
{
	GiggleRevisionListPriv *priv;
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
revision_log_matches (GiggleRevisionList *list,
		      GiggleRevision     *revision,
		      const gchar        *search_term)
{
	GiggleRevisionListPriv *priv;
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

static gboolean
revision_matches (GiggleRevisionList *list,
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
revision_list_search (GiggleSearchable      *searchable,
		      const gchar           *search_term,
		      GiggleSearchDirection  direction,
		      gboolean               full_search)
{
	GiggleRevisionListPriv *priv;
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
			found = revision_matches (GIGGLE_REVISION_LIST (searchable),
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
revision_list_cancel_search (GiggleSearchable *searchable)
{
	GiggleRevisionListPriv *priv;

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
revision_list_commit (GtkAction          *action,
		      GiggleRevisionList *list)
{
	revision_list_commit_changes (list);
}

static void
revision_list_create_branch (GtkAction          *action,
			     GiggleRevisionList *list)
{
	GiggleRevisionListPriv *priv;
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

/* FIXME: pretty equal to revision_list_create_branch() */
static void
revision_list_create_tag (GtkAction          *action,
			  GiggleRevisionList *list)
{
	GiggleRevisionListPriv *priv;
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
giggle_revision_list_get_graph_visible (GiggleRevisionList *list)
{
	GiggleRevisionListPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION_LIST (list), FALSE);

	priv = GET_PRIV (list);
	return priv->show_graph;
}

void
giggle_revision_list_set_graph_visible (GiggleRevisionList *list,
					gboolean            show_graph)
{
	GiggleRevisionListPriv *priv;

	g_return_if_fail (GIGGLE_IS_REVISION_LIST (list));

	priv = GET_PRIV (list);

	priv->show_graph = (show_graph == TRUE);
	gtk_tree_view_column_set_visible (priv->graph_column, priv->show_graph);
	g_object_notify (G_OBJECT (list), "graph-visible");
}

gboolean
giggle_revision_list_get_compact_mode (GiggleRevisionList *list)
{
	GiggleRevisionListPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION_LIST (list), FALSE);

	priv = GET_PRIV (list);
	return priv->compact_mode;
}

void
giggle_revision_list_set_compact_mode (GiggleRevisionList *list,
				       gboolean            compact_mode)
{
	GiggleRevisionListPriv *priv;
	GtkRcStyle             *rc_style;
	gint                    size;

	g_return_if_fail (GIGGLE_IS_REVISION_LIST (list));

	priv = GET_PRIV (list);

	if (compact_mode != priv->compact_mode) {
		priv->compact_mode = (compact_mode == TRUE);
		rc_style = gtk_widget_get_modifier_style (GTK_WIDGET (list));

		if (rc_style->font_desc) {
			/* free old font desc */
			pango_font_description_free (rc_style->font_desc);
			rc_style->font_desc = NULL;
		}

		if (priv->compact_mode) {
			rc_style->font_desc = pango_font_description_copy (GTK_WIDGET (list)->style->font_desc);
			size = pango_font_description_get_size (rc_style->font_desc);
			pango_font_description_set_size (rc_style->font_desc,
							 size * PANGO_SCALE_SMALL);
		}

		gtk_widget_modify_style (GTK_WIDGET (list), rc_style);
		gtk_widget_set_name (GTK_WIDGET (list),
				     (priv->compact_mode) ? "revision-list" : NULL);

		gtk_cell_renderer_text_set_fixed_height_from_font (
			GTK_CELL_RENDERER_TEXT (priv->log_renderer), 1);
		gtk_cell_renderer_text_set_fixed_height_from_font (
			GTK_CELL_RENDERER_TEXT (priv->author_renderer), 1);
		gtk_cell_renderer_text_set_fixed_height_from_font (
			GTK_CELL_RENDERER_TEXT (priv->date_renderer), 1);

		g_object_notify (G_OBJECT (list), "compact-mode");
	}
}
