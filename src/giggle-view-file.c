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
#include "giggle-view-file.h"

#include "giggle-file-list.h"
#include "giggle-rev-list-view.h"
#include "giggle-view-history.h"
#include "giggle-view-shell.h"

#include <libgiggle/giggle-history.h>
#include <libgiggle/giggle-searchable.h>

#include <libgiggle-git/giggle-git.h>
#include <libgiggle-git/giggle-git-blame.h>
#include <libgiggle-git/giggle-git-cat-file.h>
#include <libgiggle-git/giggle-git-revisions.h>
#include <libgiggle-git/giggle-git-list-tree.h>
#include <libgiggle-git/giggle-git-config.h>

#include <fnmatch.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

#include <gtksourceview/gtksourceiter.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <gtksourceview/gtksourceview.h>

#define GIGGLE_TYPE_VIEW_FILE_SNAPSHOT            (giggle_view_file_snapshot_get_type ())
#define GIGGLE_VIEW_FILE_SNAPSHOT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_VIEW_FILE_SNAPSHOT, GiggleViewFileSnapshot))
#define GIGGLE_VIEW_FILE_SNAPSHOT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_VIEW_FILE_SNAPSHOT, GiggleViewFileSnapshotClass))
#define GIGGLE_IS_VIEW_FILE_SNAPSHOT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_VIEW_FILE_SNAPSHOT))
#define GIGGLE_IS_VIEW_FILE_SNAPSHOT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_VIEW_FILE_SNAPSHOT))
#define GIGGLE_VIEW_FILE_SNAPSHOT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_VIEW_FILE_SNAPSHOT, GiggleViewFileSnapshotClass))

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_VIEW_FILE, GiggleViewFilePriv))

typedef struct {
	GtkWidget           *file_list;
	GtkWidget           *revision_list;
	GtkWidget           *source_view;
	GtkWidget           *goto_toolbar;
	GtkWidget           *goto_entry;
	GtkWidget           *hpaned;
	GtkWidget           *vpaned;

	GtkActionGroup      *action_group;

	GiggleGit           *git;
	GiggleJob           *job;

	char                *current_file;
	GiggleRevision      *current_revision;
	GiggleGitConfig     *configuration;
} GiggleViewFilePriv;

typedef struct {
	GObject parent;

	GtkTreeRowReference *file_list_row;
	GtkTreeViewColumn   *file_list_column;

	GtkTreeRowReference *revision_list_row;
	GtkTreeViewColumn   *revision_list_column;
} GiggleViewFileSnapshot;

typedef struct {
	GObjectClass parent_class;
} GiggleViewFileSnapshotClass;

enum {
	PROP_0,
	PROP_PATH,
};

static void	giggle_view_file_searchable_init	(GiggleSearchableIface *iface);
static void	giggle_view_file_history_init		(GiggleHistoryIface    *iface);

static GType	giggle_view_file_snapshot_get_type	(void) G_GNUC_CONST;


G_DEFINE_TYPE_WITH_CODE (GiggleViewFile, giggle_view_file, GIGGLE_TYPE_VIEW,
			 G_IMPLEMENT_INTERFACE (GIGGLE_TYPE_SEARCHABLE,
						giggle_view_file_searchable_init)
			 G_IMPLEMENT_INTERFACE (GIGGLE_TYPE_HISTORY,
						giggle_view_file_history_init))

G_DEFINE_TYPE (GiggleViewFileSnapshot,
	       giggle_view_file_snapshot, G_TYPE_OBJECT)

static void
view_file_get_property (GObject      *object,
			guint         param_id,
			GValue       *value,
			GParamSpec   *pspec)
{
	switch (param_id) {
	case PROP_PATH:
		g_value_set_string (value,
				    giggle_view_file_get_path (GIGGLE_VIEW_FILE (object)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}
static void
view_file_set_property (GObject      *object,
			guint         param_id,
			const GValue *value,
			GParamSpec   *pspec)
{
	switch (param_id) {
	case PROP_PATH:
		giggle_view_file_set_path (GIGGLE_VIEW_FILE (object),
					   g_value_get_string (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
view_file_finalize (GObject *object)
{
	GiggleViewFilePriv *priv;

	priv = GET_PRIV (object);

	g_free (priv->current_file);

	G_OBJECT_CLASS (giggle_view_file_parent_class)->finalize (object);
}

static void
view_file_dispose (GObject *object)
{
	GiggleViewFilePriv *priv;

	priv = GET_PRIV (object);

	if (priv->configuration) {
		g_object_unref (priv->configuration);
		priv->configuration = NULL;
	}

	if (priv->current_revision) {
		g_object_unref (priv->current_revision);
		priv->current_revision = NULL;
	}

	if (priv->action_group) {
		g_object_unref (priv->action_group);
		priv->action_group = NULL;
	}

	G_OBJECT_CLASS (giggle_view_file_parent_class)->dispose (object);
}

static void
view_file_goto_line_cb (GtkAction      *action,
			GiggleViewFile *view)
{
	GiggleViewFilePriv *priv;

	priv = GET_PRIV (view);

	g_object_set
		(priv->goto_toolbar, "visible",
		 !GTK_WIDGET_VISIBLE (priv->goto_toolbar), NULL);

	gtk_widget_grab_focus (priv->goto_entry);
}

static void
view_file_add_ui (GiggleView   *view,
		  GtkUIManager *manager)
{
	const static char layout[] =
		"<ui>"
		"  <menubar name='MainMenubar'>"
		"    <menu action='GoMenu'>"
		"      <separator />"
		"      <menuitem action='ViewFileGotoLine' />"
		"    </menu>"
		"  </menubar>"
		"</ui>";

	static const GtkActionEntry actions[] = {
		{ "ViewFileGotoLine", GTK_STOCK_JUMP_TO,
		  N_("_Goto Line Number"), "<control>L",
		  N_("Highlight specified line number"),
		  G_CALLBACK (view_file_goto_line_cb)
		},
	};

	GiggleViewFilePriv *priv;

	priv = GET_PRIV (view);

	if (!priv->action_group) {
		priv->action_group = gtk_action_group_new (giggle_view_get_name (view));

		gtk_action_group_set_translation_domain (priv->action_group, GETTEXT_PACKAGE);
		gtk_action_group_add_actions (priv->action_group, actions, G_N_ELEMENTS (actions), view);

		gtk_ui_manager_insert_action_group (manager, priv->action_group, 0);
		gtk_ui_manager_add_ui_from_string (manager, layout, G_N_ELEMENTS (layout) - 1, NULL);
		gtk_ui_manager_ensure_update (manager);
	}

	gtk_action_group_set_visible (priv->action_group, TRUE);
}

static void
view_file_remove_ui (GiggleView *view)
{
	GiggleViewFilePriv *priv;

	priv = GET_PRIV (view);

	if (priv->action_group)
		gtk_action_group_set_visible (priv->action_group, FALSE);
}

static void
giggle_view_file_class_init (GiggleViewFileClass *class)
{
	GObjectClass    *object_class;
	GiggleViewClass *view_class;

	object_class = G_OBJECT_CLASS (class);
	view_class = GIGGLE_VIEW_CLASS (class);

	object_class->get_property = view_file_get_property;
	object_class->set_property = view_file_set_property;
	object_class->finalize = view_file_finalize;
	object_class->dispose = view_file_dispose;

	view_class->add_ui = view_file_add_ui;
	view_class->remove_ui = view_file_remove_ui;

	g_object_class_install_property (object_class,
					 PROP_PATH,
					 g_param_spec_string ("path",
							      "Path",
							      "The currently selected path",
							      NULL, G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleViewFilePriv));
}

static void
show_error (GiggleViewFile *view,
	    const char     *message,
	    const GError   *error)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view))),
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_OK,
					 message, error->message);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static GtkSourceLanguage *
view_file_find_language (GiggleViewFile *view,
			 const char     *text,
			 gsize	         len)
{
	GtkSourceLanguageManager *manager;
	GiggleViewFilePriv       *priv;
	GtkSourceLanguage        *lang;
	const char *const        *ids;
	char                     *content_type, *filename;
	char                    **meta, **s;

	priv = GET_PRIV (view);

	manager = gtk_source_language_manager_get_default ();
	ids = gtk_source_language_manager_get_language_ids (manager);

	content_type = g_content_type_guess (priv->current_file, text, len, NULL);
	filename = g_path_get_basename (priv->current_file);

	for (; *ids; ++ids) {
		lang = gtk_source_language_manager_get_language (manager, *ids);
		meta = gtk_source_language_get_mime_types (lang);

		if (meta) {
			for (s = meta; *s; ++s) {
				if (!strcmp (*s, content_type))
					goto end;
			}

			g_strfreev (meta);
		}

		meta = gtk_source_language_get_globs (lang);

		if (meta) {
			for (s = meta; *s; ++s) {
				if (!fnmatch (*s, filename, 0)) {
					goto end;
				}
			}

			g_strfreev (meta);
		}
	}

	lang = NULL;

end:
	g_free (content_type);
	g_free (filename);

	return lang;
}

static void
view_file_set_source_code (GiggleViewFile *view,
			   const char     *text,
			   gsize	   len)
{
	GiggleViewFilePriv *priv;
	GtkTextBuffer      *buffer;
	GtkSourceLanguage  *language = NULL;
	GtkTextIter	    start, end;

	priv = GET_PRIV (view);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->source_view));
	gtk_widget_set_sensitive (priv->revision_list, NULL != text);
	gtk_widget_set_sensitive (priv->source_view, NULL != text);
	gtk_text_buffer_set_text (buffer, text ? text : "", len);

	if (priv->action_group)
		gtk_action_group_set_sensitive (priv->action_group, NULL != text);

	if (text)
		language = view_file_find_language (view, text, len);

	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gtk_source_buffer_remove_source_marks (GTK_SOURCE_BUFFER (buffer), &start, &end, NULL);
	gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (buffer), language);
}

static const char *
get_line_category (int line,
		   int num_lines)
{
	if (!line) {
		if (1 == num_lines)
			return "giggle-chunk-start-end";

		return "giggle-chunk-start";
	}

	if (line < num_lines - 1)
		return "giggle-chunk-middle";

	return "giggle-chunk-end";
}

static void
view_file_blame_job_callback (GiggleGit *git,
			      GiggleJob *job,
			      GError    *error,
			      gpointer   data)
{
	GiggleViewFile            *view;
	GiggleViewFilePriv        *priv;
	const GiggleGitBlameChunk *chunk;
	int			   i, l;
	GtkSourceBuffer		  *buffer;
	GtkSourceMark		  *mark;
	GtkTextIter		   iter;
	const char		  *category;

	view = GIGGLE_VIEW_FILE (data);
	priv = GET_PRIV (view);
	priv->job = NULL;

	if (error) {
		g_warning ("%s: %s", G_STRFUNC, error->message);
	} else {
		buffer = GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->source_view)));

		for (i = 0; (chunk = giggle_git_blame_get_chunk (GIGGLE_GIT_BLAME (job), i)); ++i) {
			for (l = 0; l < chunk->num_lines; ++l) {
				category = get_line_category (l, chunk->num_lines);

				gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (buffer),
								  &iter, chunk->result_line + l - 1);

				mark = gtk_source_buffer_create_source_mark (buffer, NULL,
									     category, &iter);

				g_object_set_data_full (G_OBJECT (mark), "giggle-revision",
							g_object_ref (chunk->revision),
							g_object_unref);
			}
		}
	}

	g_object_unref (job);
}

static void
view_file_cat_file_job_callback (GiggleGit *git,
				 GiggleJob *job,
				 GError    *error,
				 gpointer   data)
{
	GiggleViewFile     *view;
	GiggleViewFilePriv *priv;
	const char         *text;
	gsize               len;

	view = GIGGLE_VIEW_FILE (data);
	priv = GET_PRIV (view);
	priv->job = NULL;

	if (error) {
		view_file_set_source_code (view, error->message, -1);
	} else {
		text = giggle_git_cat_file_get_contents (GIGGLE_GIT_CAT_FILE (job), &len);
		view_file_set_source_code (view, text, len);

		priv->job = giggle_git_blame_new (priv->current_revision,
						  priv->current_file);

		giggle_git_run_job (priv->git,
				    priv->job,
				    view_file_blame_job_callback,
				    view);
	}

	g_object_unref (job);
}

static void
view_file_list_tree_job_callback (GiggleGit *git,
				  GiggleJob *job,
				  GError    *error,
				  gpointer   data)
{
	GiggleViewFile     *view;
	GiggleViewFilePriv *priv;
	const char         *type;
	const char         *sha = NULL;
	char		   *text;
	gsize		    len;

	view = GIGGLE_VIEW_FILE (data);
	priv = GET_PRIV (view);
	priv->job = NULL;

	if (error) {
		show_error (view, _("An error occurred when getting file ref:\n%s"), error);
	} else {
		type = giggle_git_list_tree_get_kind (GIGGLE_GIT_LIST_TREE (job),
						      priv->current_file);

		if (!g_strcmp0 (type, "blob")) {
			sha = giggle_git_list_tree_get_sha (GIGGLE_GIT_LIST_TREE (job),
						            priv->current_file);
		}

		if (sha) {
			priv->job = giggle_git_cat_file_new ("blob", sha);

			giggle_git_run_job (priv->git,
					    priv->job,
					    view_file_cat_file_job_callback,
					    view);

		} else if (g_file_test (priv->current_file, G_FILE_TEST_IS_DIR)) {
			view_file_set_source_code (view, NULL, 0);
		} else if (g_file_get_contents (priv->current_file, &text, &len, NULL)) {
			view_file_set_source_code (view, text, len);
			g_free (text);
		} else {
			view_file_set_source_code (view, NULL, 0);
		}
	}

	g_object_unref (job);
}

static void
view_file_read_source_code (GiggleViewFile *view)
{
	GiggleViewFilePriv *priv;
	char		   *directory;

	priv = GET_PRIV (view);

	if (priv->current_file) {
		directory = g_path_get_dirname (priv->current_file);
		priv->job = giggle_git_list_tree_new (priv->current_revision, directory);

		giggle_git_run_job (priv->git,
				    priv->job,
				    view_file_list_tree_job_callback,
				    view);

		g_free (directory);
	} else {
		view_file_set_source_code (view, NULL, 0);
	}
}

static void
view_file_select_file_job_callback (GiggleGit *git,
				    GiggleJob *job,
				    GError    *error,
				    gpointer   data)
{
	GiggleViewFile     *view;
	GiggleViewFilePriv *priv;
	GtkListStore       *store;
	GtkTreeIter         iter;
	GList              *revisions;

	view = GIGGLE_VIEW_FILE (data);
	priv = GET_PRIV (view);
	priv->job = NULL;

	if (error) {
		show_error (view, _("An error occurred when getting the revisions list:\n%s"), error);
	} else {
		store = gtk_list_store_new (1, GIGGLE_TYPE_REVISION);
		revisions = giggle_git_revisions_get_revisions (GIGGLE_GIT_REVISIONS (job));

		while (revisions) {
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter,
					    0, revisions->data,
					    -1);
			revisions = revisions->next;
		}

		giggle_rev_list_view_set_model (GIGGLE_REV_LIST_VIEW (priv->revision_list),
						GTK_TREE_MODEL (store));
		g_object_unref (store);

		view_file_read_source_code (view);
	}

	g_object_unref (job);
}

static void
view_file_selection_changed_cb (GtkTreeSelection *selection,
				GiggleViewFile   *view)
{
	GiggleViewFilePriv *priv;
	GList              *files;

	priv = GET_PRIV (view);

	files = giggle_file_list_get_selection (GIGGLE_FILE_LIST (priv->file_list));
	priv->job = giggle_git_revisions_new_for_files (files);

	g_free (priv->current_file);
	priv->current_file = files ? g_strdup (files->data) : NULL;

	giggle_git_run_job (priv->git,
			    priv->job,
			    view_file_select_file_job_callback,
			    view);

	g_object_notify (G_OBJECT (view), "path");
	giggle_history_changed (GIGGLE_HISTORY (view));
}

static void
view_file_revision_list_selection_changed_cb (GiggleRevListView *list,
					      GiggleRevision    *revision1,
					      GiggleRevision    *revision2,
					      GiggleViewFile    *view)
{
	GiggleViewFilePriv *priv;

	priv = GET_PRIV (view);

	if (revision1)
		g_object_ref (revision1);
	if (priv->current_revision)
		g_object_unref (priv->current_revision);

	priv->current_revision = revision1;

	giggle_file_list_highlight_revisions (GIGGLE_FILE_LIST (priv->file_list),
					      revision1, revision2);

	view_file_read_source_code (view);

	giggle_history_changed (GIGGLE_HISTORY (view));
}

static void
view_file_revision_list_revision_activated_cb (GiggleRevListView *list,
					       GiggleRevision    *revision,
					       GiggleViewFile    *view)
{
	GiggleView *history_view = NULL;
	GtkWidget  *shell;

	shell = gtk_widget_get_ancestor (GTK_WIDGET (view), GIGGLE_TYPE_VIEW_SHELL);

	if (shell)
		history_view = giggle_view_shell_find_view (GIGGLE_VIEW_SHELL (shell),
							    GIGGLE_TYPE_VIEW_HISTORY);

	if (history_view) {
		giggle_view_history_select (GIGGLE_VIEW_HISTORY (history_view), revision);
		giggle_view_shell_select (GIGGLE_VIEW_SHELL (shell), history_view);
	}
}

static gboolean
is_numeric (const char *text)
{
	int i, end;

	return (1 == sscanf (text, "%d%n", &i, &end) && '\0' == text[end]);
}

static gboolean
goto_entry_key_press_event_cb (GtkWidget      *widget,
			       GdkEventKey    *event,
			       GiggleViewFile *view)
{
	GiggleViewFilePriv *priv;

	priv = GET_PRIV (view);

	if ('\0' == event->string[0] ||
	    is_numeric (event->string) ||
	    !strcmp (event->string, "\r"))
		return FALSE;

	if (!strcmp (event->string, "\033")) {
		gtk_widget_hide (priv->goto_toolbar);
		return TRUE;
	}

	gdk_beep ();

	return TRUE;
}

static void
goto_entry_insert_text_cb (GtkEditable *editable,
                           char        *new_text,
                           int          new_text_length,
                           int         *position)
{
	if (!is_numeric (new_text)) {
		g_signal_stop_emission_by_name (editable, "insert-text");
		gdk_beep ();
	}
}

static void
goto_entry_activate_cb (GtkEntry       *entry,
			GiggleViewFile *view)
{
	GiggleViewFilePriv *priv;
	const char         *text;
	int                 line;
	GtkTextIter         iter;
	GtkTextBuffer	   *buffer;

	priv = GET_PRIV (view);
	text = gtk_entry_get_text (entry);

	if (1 == sscanf (text, "%d", &line)) {
		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->source_view));
		gtk_text_buffer_get_iter_at_line (buffer, &iter, line - 1);

		gtk_text_view_scroll_to_iter
			(GTK_TEXT_VIEW (priv->source_view), &iter,
			 FALSE, 0, 0.0, 0.0);

		gtk_text_buffer_place_cursor (buffer, &iter);
	}
}

static void
goto_item_clicked_cb (GtkObject *button,
	 	      GtkWidget *entry)
{
	g_signal_emit_by_name (entry, "activate", 0);
}

static GtkWidget *
goto_toolbar_init (GiggleViewFile *view)
{
	GiggleViewFilePriv *priv;
	GtkToolItem	   *item;
	GtkWidget	   *alignment, *box, *label;

	priv = GET_PRIV (view);

	priv->goto_toolbar = gtk_toolbar_new ();
	gtk_widget_set_no_show_all (priv->goto_toolbar, TRUE);

	label = gtk_label_new_with_mnemonic (_("Line Number:"));

	priv->goto_entry = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (priv->goto_entry), 8);
	gtk_entry_set_max_length (GTK_ENTRY (priv->goto_entry), 16);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), priv->goto_entry);

	box = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), priv->goto_entry, TRUE, TRUE, 0);

	alignment = gtk_alignment_new (0.0, 0.5, 1.0, 0.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 2, 2);
	gtk_container_add (GTK_CONTAINER (alignment), box);

	item = gtk_tool_item_new ();
	gtk_container_add (GTK_CONTAINER (item), alignment);
	gtk_toolbar_insert (GTK_TOOLBAR (priv->goto_toolbar), item, -1);
	gtk_widget_show_all (GTK_WIDGET (item));

	g_signal_connect
		(priv->goto_entry, "key-press-event",
		 G_CALLBACK (goto_entry_key_press_event_cb), view);
	g_signal_connect
		(priv->goto_entry, "insert-text",
		 G_CALLBACK (goto_entry_insert_text_cb), view);
	g_signal_connect
		(priv->goto_entry, "activate",
		 G_CALLBACK (goto_entry_activate_cb), view);

	item = gtk_tool_button_new_from_stock (GTK_STOCK_JUMP_TO);
	gtk_tool_item_set_is_important (item, TRUE);
	gtk_toolbar_insert (GTK_TOOLBAR (priv->goto_toolbar), item, -1);
	gtk_widget_show_all (GTK_WIDGET (item));

	g_signal_connect
		(item, "clicked",
		 G_CALLBACK (goto_item_clicked_cb), priv->goto_entry);

	return priv->goto_toolbar;
}

static inline guint8
convert_color_channel (guint  src,
                       guint8 alpha)
{
	return (alpha ? ((src << 8) - src) / alpha : 0);
}


static GdkPixbuf *
create_pixbuf_from_image (cairo_surface_t *image)
{
	int           width, height, stride, x, y;
	const guint8 *source, *s, *rs;
	guchar       *target, *t, *rt;

	width  = cairo_image_surface_get_width  (image);
	height = cairo_image_surface_get_height (image);
	stride = cairo_image_surface_get_stride (image);
	source = cairo_image_surface_get_data   (image);

	target = g_new (guchar, stride * height);

	for (y = 0, s = source, t = target; y < height; ++y, s += stride, t += stride) {
		for (x = 0, rs = s, rt = t; x < width; ++x, rs += 4, rt += 4) {
			rt[0] = convert_color_channel (rs[2], rs[3]);
			rt[1] = convert_color_channel (rs[1], rs[3]);
			rt[2] = convert_color_channel (rs[0], rs[3]);
			rt[3] =                               rs[3];
		}
	}

	return gdk_pixbuf_new_from_data (target, GDK_COLORSPACE_RGB,
			                 TRUE, 8, width, height, stride,
					 (GdkPixbufDestroyNotify) g_free, NULL);
}

static GdkPixbuf *
render_category_icon (GiggleViewFilePriv *priv,
		      const char         *name)
{
	double           r, g, b;
	double		 x0, y0, x1, y1;
	int		 width, height;
	gboolean         start = FALSE;
	gboolean         end = FALSE;
	GdkPixbuf       *pixbuf;
	cairo_pattern_t *gradient;
	cairo_surface_t *image;
	cairo_t         *canvas;

	if (!g_str_has_prefix (name, "giggle-chunk-"))
		return NULL;

	if (strstr (name, "-start"))
		start = TRUE;
	if (strstr (name, "-end"))
		end = TRUE;

	gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, &height); //--height; /* FIXME */

	image = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	gradient = cairo_pattern_create_linear (0, 0, 0, height - 1);
	canvas = cairo_create (image);

	x0 = 2;
	y0 = 0;
	x1 = width - 3;
	y1 = height - 1;

	r = priv->source_view->style->base[GTK_STATE_SELECTED].red   / 65535.0;
	g = priv->source_view->style->base[GTK_STATE_SELECTED].green / 65535.0;
	b = priv->source_view->style->base[GTK_STATE_SELECTED].blue  / 65535.0;

	if (start) {
		cairo_pattern_add_color_stop_rgba (gradient, 0.0, r, g, b, 0.00);
		cairo_pattern_add_color_stop_rgba (gradient, 0.1, r, g, b, 0.05);
		cairo_pattern_add_color_stop_rgba (gradient, 0.4, r, g, b, 0.15);

		cairo_move_to  (canvas, x0,     y0 + 9);
		cairo_curve_to (canvas, x0,     y0 + 1, x0, y0 + 1, x0 + 8, y0 + 1);
		cairo_line_to  (canvas, x1 - 8, y0 + 1);
		cairo_curve_to (canvas, x1,     y0 + 1, x1, y0 + 1, x1,     y0 + 9);
	} else {
		cairo_pattern_add_color_stop_rgba (gradient, 0.0, r, g, b, 0.10);

		cairo_move_to (canvas, x0, y0);
		cairo_line_to (canvas, x1, y0);
	}

	if (end) {
		cairo_pattern_add_color_stop_rgba (gradient, 1.0, r, g, b, 0.00);
		cairo_pattern_add_color_stop_rgba (gradient, 0.9, r, g, b, 0.05);
		cairo_pattern_add_color_stop_rgba (gradient, 0.6, r, g, b, 0.10);

		cairo_line_to  (canvas, x1,     y1 - 9);
		cairo_curve_to (canvas, x1,     y1 - 1, x1, y1 - 1, x1 - 8, y1 - 1);
		cairo_line_to  (canvas, x0 + 8, y1 - 1);
		cairo_curve_to (canvas, x0,     y1 - 1, x0, y1 - 1, x0,     y1 - 9);
	} else {
		cairo_pattern_add_color_stop_rgba (gradient, 1.0, r, g, b, 0.10);

		cairo_line_to (canvas, x1, y1);
		cairo_line_to (canvas, x0, y1);
	}

	cairo_set_source (canvas, gradient);
	cairo_fill (canvas);

	x0 += 0.5;
	y0 += 0.5;
	x1 += 0.5;
	y1 += 0.5;

	if (start) {
		if (end) {
			cairo_move_to  (canvas, x0, y0 + 9);
		} else {
			cairo_move_to  (canvas, x0, y1);
			cairo_line_to  (canvas, x0, y0 + 8);
		}

		cairo_curve_to (canvas, x0,     y0 + 1, x0, y0 + 1, x0 + 8, y0 + 1);
		cairo_line_to  (canvas, x1 - 8, y0 + 1);
		cairo_curve_to (canvas, x1,     y0 + 1, x1, y0 + 1, x1,     y0 + 9);
	} else {
		cairo_move_to (canvas, x1, y0);
	}

	if (end) {
		cairo_line_to  (canvas, x1,     y1 - 9);
		cairo_curve_to (canvas, x1,     y1 - 1, x1, y1 - 1, x1 - 8, y1 - 1);
		cairo_line_to  (canvas, x0 + 8, y1 - 1);
		cairo_curve_to (canvas, x0,     y1 - 1, x0, y1 - 1, x0,     y1 - 9);
	} else {
		cairo_line_to (canvas, x1, y1 + 1);
		cairo_move_to (canvas, x0, y1);
	}

	if (!start)
		cairo_line_to (canvas, x0, y0);

	cairo_set_line_width (canvas, 1);
	cairo_set_source_rgba (canvas, r, g, b, 0.3);
	cairo_stroke (canvas);

	pixbuf = create_pixbuf_from_image (image);

	cairo_pattern_destroy (gradient);
	cairo_surface_destroy (image);
	cairo_destroy (canvas);

	return pixbuf;
}

static void
create_category (GiggleViewFilePriv *priv,
		 const char         *name)
{
	GdkPixbuf *pixbuf;

	pixbuf = gtk_widget_render_icon (priv->source_view, name,
					 GTK_ICON_SIZE_MENU, NULL);

	if (!pixbuf)
		pixbuf = render_category_icon (priv, name);
	
	if (pixbuf) {
		gtk_source_view_set_mark_category_pixbuf
			(GTK_SOURCE_VIEW (priv->source_view),
			 name, pixbuf);

		g_object_unref (pixbuf);
	}
}

static gboolean
source_view_query_tooltip_cb (GtkWidget  *widget,
                              gint        x,
                              gint        y,
                              gboolean    keyboard_mode,
                              GtkTooltip *tooltip)
{
	char            *markup = NULL, date[256];
	GiggleRevision  *revision;
	GdkRectangle     bounds;
	GtkTextIter      iter;
	GSList          *l;

	gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (widget),
					       GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
	gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (widget), &iter, x, y);
	gtk_text_iter_backward_chars (&iter, gtk_text_iter_get_line_offset (&iter));

	for (l = gtk_text_iter_get_marks (&iter); l; l = l->next) {
		revision = g_object_get_data (l->data, "giggle-revision");

		if (!revision)
			continue;

		if (giggle_revision_get_date (revision)) {
			strftime (date, sizeof date, "%c",
				  giggle_revision_get_date (revision));
		} else {
			*date = '\0';
		}

		markup = g_markup_printf_escaped
			("<b>%s</b> %s\n<b>%s</b> %s\n<b>%s</b> %s\n%s",
			 _("Author:"),   giggle_revision_get_author (revision),
			 _("Date:"),     date,
			 _("SHA:"),      giggle_revision_get_sha (revision),
			 giggle_revision_get_short_log (revision));

		gtk_text_view_get_line_yrange (GTK_TEXT_VIEW (widget),
				               &iter, &bounds.y, &bounds.height);
		gtk_text_view_buffer_to_window_coords (GTK_TEXT_VIEW (widget),
						       GTK_TEXT_WINDOW_WIDGET,
						       0, bounds.y, NULL, &bounds.y);

		bounds.x = 0;
		bounds.width = widget->allocation.width;

		gtk_tooltip_set_tip_area (tooltip, &bounds);
		gtk_tooltip_set_markup (tooltip, markup);

		g_free (markup);

		return TRUE;
	}


	return FALSE;
}

static gboolean
view_file_search (GiggleSearchable      *searchable,
		  const gchar           *search_term,
		  GiggleSearchDirection  direction,
		  gboolean               full_search)
{
	GiggleViewFilePriv   *priv;
	GtkSourceSearchFlags  flags;
	gboolean	      result = FALSE;
	int		      cursor_position;
	GtkTextIter	      start, end, cursor;
	GtkTextIter	      match_start, match_end;
	GtkTextBuffer        *buffer;

	priv = GET_PRIV (searchable);

	flags = GTK_SOURCE_SEARCH_TEXT_ONLY |
		GTK_SOURCE_SEARCH_CASE_INSENSITIVE;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->source_view));
	g_object_get (buffer, "cursor-position", &cursor_position, NULL);
	gtk_text_buffer_get_bounds (buffer, &start, &end);

	cursor = start;
	gtk_text_iter_forward_chars (&cursor, cursor_position);

	switch (direction) {
	case GIGGLE_SEARCH_DIRECTION_NEXT:
		result = gtk_source_iter_forward_search (&cursor, search_term,
							 flags, &match_start, &match_end, NULL);

		if (result && gtk_text_iter_get_offset (&cursor) == gtk_text_iter_get_offset (&match_start)) {
			gtk_text_iter_forward_char (&cursor);

			result = gtk_source_iter_forward_search (&cursor, search_term,
								 flags, &match_start, &match_end, NULL);
		}

		if (!result) {
			result = gtk_source_iter_forward_search (&start, search_term,
								 flags, &match_start, &match_end, NULL);
		}

		break;

	case GIGGLE_SEARCH_DIRECTION_PREV:
		result = gtk_source_iter_backward_search (&cursor, search_term,
							  flags, &match_start, &match_end, NULL);

		if (result && gtk_text_iter_get_offset (&cursor) == gtk_text_iter_get_offset (&match_start)) {
			gtk_text_iter_backward_char (&cursor);

			result = gtk_source_iter_backward_search (&cursor, search_term,
								  flags, &match_start, &match_end, NULL);
		}

		if (!result) {
			result = gtk_source_iter_backward_search (&end, search_term,
								  flags, &match_start, &match_end, NULL);
		}

		break;
	}

	if (result) {
		gtk_text_buffer_move_mark_by_name (buffer, "insert", &match_start);
		gtk_text_buffer_move_mark_by_name (buffer, "selection_bound", &match_end);

		gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (priv->source_view),
					      &match_start, 0, FALSE, 0, 0);
	}

	return result;
}

static void
giggle_view_file_searchable_init (GiggleSearchableIface *iface)
{
	iface->search = view_file_search;
}

static void
giggle_view_file_snapshot_init (GiggleViewFileSnapshot *snapshot)
{
}

static void
view_file_snapshot_dispose (GObject *object)
{
	GiggleViewFileSnapshot *snapshot;

	snapshot = GIGGLE_VIEW_FILE_SNAPSHOT (object);

	if (snapshot->file_list_row) {
		gtk_tree_row_reference_free (snapshot->file_list_row);
		snapshot->file_list_row = NULL;
	}

	if (snapshot->file_list_column) {
		g_object_unref (snapshot->file_list_column);
		snapshot->file_list_column = NULL;
	}

	if (snapshot->revision_list_row) {
		gtk_tree_row_reference_free (snapshot->revision_list_row);
		snapshot->revision_list_row = NULL;
	}

	if (snapshot->revision_list_column) {
		g_object_unref (snapshot->revision_list_column);
		snapshot->revision_list_column = NULL;
	}

	G_OBJECT_CLASS (giggle_view_file_snapshot_parent_class)->dispose (object);
}

static void
giggle_view_file_snapshot_class_init (GiggleViewFileSnapshotClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	object_class->dispose = view_file_snapshot_dispose;
}

static GObject *
view_file_capture (GiggleHistory *history)
{
	GiggleViewFilePriv     *priv = GET_PRIV (history);
	GiggleViewFileSnapshot *snapshot;
	GtkTreeViewColumn      *column;
	GtkTreeModel           *model;
	GtkTreePath            *path;

	snapshot = g_object_new (GIGGLE_TYPE_VIEW_FILE_SNAPSHOT, NULL);

	gtk_tree_view_get_cursor (GTK_TREE_VIEW (priv->file_list), &path, &column);

	if (path) {
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->file_list));
		snapshot->file_list_row = gtk_tree_row_reference_new (model, path);
		gtk_tree_path_free (path);
	}

	if (column)
		snapshot->file_list_column = g_object_ref (column);

	gtk_tree_view_get_cursor (GTK_TREE_VIEW (priv->revision_list), &path, &column);

	if (path) {
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->revision_list));
		snapshot->revision_list_row = gtk_tree_row_reference_new (model, path);
		gtk_tree_path_free (path);
	}

	if (column)
		snapshot->revision_list_column = g_object_ref (column);

	return G_OBJECT (snapshot);
}

static void
view_file_restore (GiggleHistory *history,
		   GObject       *object)
{
	GiggleViewFilePriv     *priv = GET_PRIV (history);
	GiggleViewFileSnapshot *snapshot;
	GtkTreePath            *path;

	g_return_if_fail (GIGGLE_IS_VIEW_FILE_SNAPSHOT (object));
	snapshot = GIGGLE_VIEW_FILE_SNAPSHOT (object);

	if (snapshot->file_list_row) {
		path = gtk_tree_row_reference_get_path (snapshot->file_list_row);
		gtk_tree_view_set_cursor (GTK_TREE_VIEW (priv->file_list), path,
					  snapshot->file_list_column, FALSE);
	}

	if (snapshot->revision_list_row) {
		path = gtk_tree_row_reference_get_path (snapshot->revision_list_row);
		gtk_tree_view_set_cursor (GTK_TREE_VIEW (priv->revision_list), path,
					  snapshot->revision_list_column, FALSE);
	}
}

static void
giggle_view_file_history_init (GiggleHistoryIface *iface)
{
	iface->capture = view_file_capture;
	iface->restore = view_file_restore;
}

static gboolean
view_file_idle_cb (gpointer data)
{
	GiggleViewFilePriv *priv;

	priv = GET_PRIV (data);

	giggle_git_config_bind (priv->configuration,
				GIGGLE_GIT_CONFIG_FIELD_FILE_VIEW_HPANE_POSITION,
				G_OBJECT (priv->hpaned), "position");

	giggle_git_config_bind (priv->configuration,
				GIGGLE_GIT_CONFIG_FIELD_FILE_VIEW_VPANE_POSITION,
				G_OBJECT (priv->vpaned), "position");

	return FALSE;
}

static void
giggle_view_file_init (GiggleViewFile *view)
{
	GiggleViewFilePriv   *priv;
	GtkWidget            *vbox;
	GtkWidget            *scrolled_window;
	GtkTreeSelection     *selection;
	PangoFontDescription *monospaced;
	GtkTextBuffer        *buffer;

	priv = GET_PRIV (view);

	priv->git = giggle_git_get ();
	priv->configuration = giggle_git_config_new ();

	gtk_widget_push_composite_child ();

	goto_toolbar_init (view);

	priv->hpaned = gtk_hpaned_new ();
	gtk_paned_set_position (GTK_PANED (priv->hpaned), 200);
	gtk_widget_show (priv->hpaned);

	priv->vpaned = gtk_vpaned_new ();
	gtk_widget_show (priv->vpaned);
	gtk_paned_pack2 (GTK_PANED (priv->hpaned), priv->vpaned, TRUE, FALSE);

	/* file view */
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);

	priv->file_list = giggle_file_list_new ();
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->file_list));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	g_signal_connect (selection, "changed",
			  G_CALLBACK (view_file_selection_changed_cb), view);

	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->file_list);
	gtk_widget_show_all (scrolled_window);

	gtk_paned_pack1 (GTK_PANED (priv->hpaned), scrolled_window, FALSE, FALSE);

	/* diff view */

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);

	priv->source_view = gtk_source_view_new ();

	gtk_widget_set_has_tooltip (priv->source_view, TRUE);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (priv->source_view), FALSE);
	gtk_source_view_set_show_line_marks (GTK_SOURCE_VIEW (priv->source_view), TRUE);
	gtk_source_view_set_show_line_numbers (GTK_SOURCE_VIEW (priv->source_view), TRUE);
	gtk_source_view_set_highlight_current_line (GTK_SOURCE_VIEW (priv->source_view), TRUE);

	g_signal_connect (priv->source_view, "query-tooltip",
			  G_CALLBACK (source_view_query_tooltip_cb), view);

	monospaced = pango_font_description_from_string ("Mono");
	gtk_widget_modify_font (priv->source_view, monospaced);
	pango_font_description_free (monospaced);

	create_category (priv, "giggle-chunk-start");
	create_category (priv, "giggle-chunk-start-end");
	create_category (priv, "giggle-chunk-end");
	create_category (priv, "giggle-chunk-middle");

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->source_view));
	gtk_source_buffer_set_highlight_syntax (GTK_SOURCE_BUFFER (buffer), TRUE);

	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->source_view);
	gtk_paned_pack1 (GTK_PANED (priv->vpaned), scrolled_window, TRUE, FALSE);

	/* revisions list */
	priv->revision_list = giggle_rev_list_view_new ();

	g_signal_connect (priv->revision_list, "selection-changed",
			  G_CALLBACK (view_file_revision_list_selection_changed_cb), view);
	g_signal_connect (priv->revision_list, "revision-activated",
			  G_CALLBACK (view_file_revision_list_revision_activated_cb), view);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->revision_list));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->revision_list);
	gtk_paned_pack2 (GTK_PANED (priv->vpaned), scrolled_window, FALSE, TRUE);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), priv->hpaned, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), priv->goto_toolbar, FALSE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (view), vbox);

	gtk_widget_show (vbox);

	gtk_widget_pop_composite_child ();

	/* bindings */
	gdk_threads_add_idle (view_file_idle_cb, view);
}

GtkWidget *
giggle_view_file_new (void)
{
	GtkAction *action;

	action = g_object_new (GTK_TYPE_RADIO_ACTION,
			       "name", "FileView",
			       "label", _("_Browse"),
			       "tooltip", _("Browse the files of this project"),
			       "stock-id", GTK_STOCK_DIRECTORY,
			       "is-important", TRUE, NULL);

	return g_object_new (GIGGLE_TYPE_VIEW_FILE,
			     "action", action, "accelerator", "F4",
			     NULL);
}

/* FIXME: this function is ugly, but we somehow want
 * the revisions list to be global to all the application */
void
giggle_view_file_set_model (GiggleViewFile *view_history,
			       GtkTreeModel   *model)
{
	GiggleViewFilePriv *priv;

	g_return_if_fail (GIGGLE_IS_VIEW_FILE (view_history));
	g_return_if_fail (GTK_IS_TREE_MODEL (model));

	priv = GET_PRIV (view_history);

	giggle_rev_list_view_set_model (GIGGLE_REV_LIST_VIEW (priv->revision_list), model);
}

void
giggle_view_file_set_graph_visible (GiggleViewFile *view,
				    gboolean           visible)
{
	GiggleViewFilePriv *priv;

	g_return_if_fail (GIGGLE_IS_VIEW_FILE (view));

	priv = GET_PRIV (view);

	giggle_rev_list_view_set_graph_visible (
		GIGGLE_REV_LIST_VIEW (priv->revision_list), visible);
}

void
giggle_view_file_set_path (GiggleViewFile *view,
			   const char     *path)
{
	GiggleViewFilePriv *priv;

	g_return_if_fail (GIGGLE_IS_VIEW_FILE (view));

	priv = GET_PRIV (view);

	giggle_file_list_select (GIGGLE_FILE_LIST (priv->file_list), path);
}

const char *
giggle_view_file_get_path (GiggleViewFile *view)
{
	GiggleViewFilePriv *priv;
	GList              *files;

	g_return_val_if_fail (GIGGLE_IS_VIEW_FILE (view), NULL);

	priv = GET_PRIV (view);

	files = giggle_file_list_get_selection (GIGGLE_FILE_LIST (priv->file_list));

	if (files)
		return files->data;

	return NULL;
}

