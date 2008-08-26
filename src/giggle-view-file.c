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
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <fnmatch.h>
#include <string.h>

#include "libgiggle/giggle-git.h"
#include "libgiggle/giggle-git-cat-file.h"
#include "libgiggle/giggle-git-list-tree.h"
#include "libgiggle/giggle-git-revisions.h"
#include "giggle-view-file.h"
#include "giggle-file-list.h"
#include "giggle-rev-list-view.h"
#include "giggle-revision-view.h"
#include "giggle-diff-view.h"

typedef struct GiggleViewFilePriv GiggleViewFilePriv;

struct GiggleViewFilePriv {
	GtkWidget      *file_list;
	GtkWidget      *revision_list;
	GtkWidget      *source_view;

	GiggleGit      *git;
	GiggleJob      *job;

	char           *current_file;
	GiggleRevision *current_revision;
	
};

G_DEFINE_TYPE (GiggleViewFile, giggle_view_file, GIGGLE_TYPE_VIEW)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_VIEW_FILE, GiggleViewFilePriv))

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

	if (priv->current_revision) {
		g_object_unref (priv->current_revision);
		priv->current_revision = NULL;
	}

	G_OBJECT_CLASS (giggle_view_file_parent_class)->dispose (object);
}

static void
giggle_view_file_class_init (GiggleViewFileClass *class)
{
	GObjectClass    *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = view_file_finalize;
	object_class->dispose = view_file_dispose;

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

	priv = GET_PRIV (view);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->source_view));
	gtk_widget_set_sensitive (priv->revision_list, NULL != text);
	gtk_widget_set_sensitive (priv->source_view, NULL != text);
	gtk_text_buffer_set_text (buffer, text ? text : "", len);

	if (text)
		language = view_file_find_language (view, text, len);

	gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (buffer), language);
}

static void
view_file_cat_file_job_callback (GiggleGit *git,
				 GiggleJob *job,
				 GError    *error,
				 gpointer   data)
{
	GiggleViewFile *view;
	const char     *text;
	gsize           len;

	view = GIGGLE_VIEW_FILE (data);

	if (error) {
		view_file_set_source_code (view, error->message, -1);
	} else {
		text = giggle_git_cat_file_get_contents (GIGGLE_GIT_CAT_FILE (job), &len);
		view_file_set_source_code (view, text, len);
	}
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
		show_error (view, _("An error ocurred when getting file ref:\n%s"), error);
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
		show_error (view, _("An error ocurred when getting the revisions list:\n%s"), error);
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
}

static void
view_file_revision_list_selection_changed_cb (GiggleRevListView *list,
					      GiggleRevision     *revision1,
					      GiggleRevision     *revision2,
					      GiggleViewFile     *view)
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
}


static void
giggle_view_file_init (GiggleViewFile *view)
{
	GiggleViewFilePriv   *priv;
	GtkWidget            *hpaned, *vpaned;
	GtkWidget            *scrolled_window;
	GtkTreeSelection     *selection;
	PangoFontDescription *monospaced;
	GtkTextBuffer        *buffer;

	priv = GET_PRIV (view);

	priv->git = giggle_git_get ();

	gtk_widget_push_composite_child ();

	hpaned = gtk_hpaned_new ();
	gtk_widget_show (hpaned);
	gtk_container_add (GTK_CONTAINER (view), hpaned);

	vpaned = gtk_vpaned_new ();
	gtk_widget_show (vpaned);
	gtk_paned_pack2 (GTK_PANED (hpaned), vpaned, TRUE, FALSE);

	/* FIXME: hardcoded sizes are evil */
	gtk_paned_set_position (GTK_PANED (hpaned), 150);

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

	gtk_paned_pack1 (GTK_PANED (hpaned), scrolled_window, FALSE, FALSE);

	/* diff view */

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);

	priv->source_view = gtk_source_view_new ();
	gtk_text_view_set_editable (GTK_TEXT_VIEW (priv->source_view), FALSE);
	monospaced = pango_font_description_from_string ("Mono");
	gtk_widget_modify_font (priv->source_view, monospaced);
	pango_font_description_free (monospaced);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->source_view));
	gtk_source_buffer_set_highlight_syntax (GTK_SOURCE_BUFFER (buffer), TRUE);

	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->source_view);
	gtk_paned_pack1 (GTK_PANED (vpaned), scrolled_window, TRUE, FALSE);

	/* revisions list */
	priv->revision_list = giggle_rev_list_view_new ();
	g_signal_connect (priv->revision_list, "selection-changed",
			  G_CALLBACK (view_file_revision_list_selection_changed_cb), view);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->revision_list));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->revision_list);
	gtk_paned_pack2 (GTK_PANED (vpaned), scrolled_window, FALSE, TRUE);

	gtk_widget_pop_composite_child ();
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


