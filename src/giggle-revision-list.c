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
#include "giggle-git-diff.h"
#include "giggle-graph-renderer.h"
#include "giggle-revision-tooltip.h"
#include "giggle-revision-list.h"
#include "giggle-revision.h"
#include "giggle-marshal.h"
#include "giggle-searchable.h"

typedef struct GiggleRevisionListPriv GiggleRevisionListPriv;

struct GiggleRevisionListPriv {
	GtkTreeViewColumn *graph_column;
	GtkCellRenderer   *emblem_renderer;
	GtkCellRenderer   *graph_renderer;

	GtkIconTheme      *icon_theme;

	GtkWidget         *revision_tooltip;

	GiggleGit         *git;
	GiggleJob         *job;

	/* used for search inside diffs */
	GMainLoop         *main_loop;

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
	PROP_SHOW_GRAPH,
	PROP_COMPACT_MODE,
};

enum {
	SELECTION_CHANGED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

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

static gboolean revision_list_search              (GiggleSearchable      *searchable,
						   const gchar           *search_term,
						   GiggleSearchDirection  direction,
						   gboolean               full_search);
static void revision_list_cancel_search           (GiggleSearchable      *searchable);


G_DEFINE_TYPE_WITH_CODE (GiggleRevisionList, giggle_revision_list, GTK_TYPE_TREE_VIEW,
			 G_IMPLEMENT_INTERFACE (GIGGLE_TYPE_SEARCHABLE,
						giggle_revision_list_searchable_init))

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REVISION_LIST, GiggleRevisionListPriv))


static void
giggle_revision_list_class_init (GiggleRevisionListClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

	object_class->finalize = revision_list_finalize;
	object_class->set_property = revision_list_set_property;
	object_class->get_property = revision_list_get_property;

	widget_class->motion_notify_event = revision_list_motion_notify;
	widget_class->leave_notify_event = revision_list_leave_notify;
	widget_class->style_set = revision_list_style_set;

	g_object_class_install_property (
		object_class,
		PROP_SHOW_GRAPH,
		g_param_spec_boolean ("show-graph",
				      "Show graph",
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
	GtkCellRenderer        *cell;
	GtkTreeSelection       *selection;
	gint                    n_columns;

	priv = GET_PRIV (revision_list);

	priv->icon_theme = gtk_icon_theme_get_default ();
	priv->git = giggle_git_get ();
	priv->main_loop = g_main_loop_new (NULL, FALSE);

	priv->graph_column = gtk_tree_view_column_new ();
	g_object_ref_sink (priv->graph_column);

	/* emblems renderer */
	priv->emblem_renderer = gtk_cell_renderer_pixbuf_new ();
	g_object_ref_sink (priv->emblem_renderer);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->graph_column), priv->emblem_renderer, FALSE);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (priv->graph_column),
					    priv->emblem_renderer,
					    revision_list_cell_data_emblem_func,
					    revision_list,
					    NULL);

	/* graph renderer */
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

	priv->revision_tooltip = giggle_revision_tooltip_new ();

	gtk_rc_parse_string ("style \"revision-list-compact-style\""
			     "{"
			     "  GtkTreeView::vertical-separator = 0"
			     "}"
			     "widget \"*.revision-list\" style \"revision-list-compact-style\"");
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
	case PROP_SHOW_GRAPH:
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
	case PROP_SHOW_GRAPH:
		giggle_revision_list_set_show_graph (GIGGLE_REVISION_LIST (object),
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

static gboolean
revision_list_motion_notify (GtkWidget      *widget,
			     GdkEventMotion *event)
{
	GiggleRevisionListPriv *priv;
	GtkTreeModel           *model;
	GtkTreePath            *path;
	GtkTreeViewColumn      *column;
	GtkTreeIter             iter;
	gint                    cell_x, start, width;
	GiggleRevision         *revision = NULL;

	priv = GET_PRIV (widget);
	GTK_WIDGET_CLASS (giggle_revision_list_parent_class)->motion_notify_event (widget, event);

	/* are we in the correct column? */
	if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
					    event->x, event->y,
					    &path, &column, &cell_x, NULL) ||
	    column != priv->graph_column) {
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

	if (!giggle_revision_get_tags (revision) &&
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
	gtk_tree_path_free (path);
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

	if (giggle_revision_get_tags (revision) ||
	    giggle_revision_get_branch_heads (revision)) {
		pixbuf = gtk_icon_theme_load_icon (priv->icon_theme,
						   "gtk-info", 16, 0, NULL);
	}

	g_object_set (cell,
		      "pixbuf", pixbuf,
		      NULL);

	if (pixbuf) {
		g_object_unref (pixbuf);
	}

	g_object_unref (revision);
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
revision_list_cell_data_date_func (GtkTreeViewColumn *column,
				   GtkCellRenderer   *cell,
				   GtkTreeModel      *model,
				   GtkTreeIter       *iter,
				   gpointer           data)
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

	tm = giggle_revision_get_date (revision);

	if (tm) {
		format = revision_list_get_formatted_time (tm);
		strftime (buf, sizeof (buf), format, tm);

		g_object_set (cell,
			      "text", buf,
			      NULL);

		g_free (format);
	}

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
		       first_revision, last_revision);

	g_object_unref (first_revision);
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
	gchar    *str;

	g_object_get (revision, property, &str, NULL);
	match = strcasestr (str, search_term) != NULL;
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
revision_matches (GiggleRevisionList *list,
		  GiggleRevision     *revision,
		  const gchar        *search_term,
		  gboolean            full_search)
{
	gboolean match = FALSE;

	match = (revision_property_matches (revision, "author", search_term) ||
		 revision_property_matches (revision, "long-log", search_term) ||
		 revision_property_matches (revision, "sha", search_term));

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
		found = revision_matches (GIGGLE_REVISION_LIST (searchable),
					  revision, search_term, full_search);

		g_object_unref (revision);

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
giggle_revision_list_get_show_graph (GiggleRevisionList *list)
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
		g_object_notify (G_OBJECT (list), "compact-mode");
	}
}
