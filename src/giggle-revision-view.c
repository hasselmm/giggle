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
#include "giggle-revision-view.h"
#include "giggle-avatar-image.h"

#include "libgiggle/giggle-git.h"
#include "libgiggle/giggle-git-log.h"
#include "libgiggle/giggle-revision.h"
#include "libgiggle/giggle-searchable.h"

#include <glib/gi18n.h>
#include <string.h>

typedef struct GiggleRevisionViewPriv GiggleRevisionViewPriv;

struct GiggleRevisionViewPriv {
	GiggleRevision *revision;

	GtkWidget      *table;
	GtkWidget      *branches;
	GtkWidget      *avatar;
	GtkWidget      *author;
	GtkWidget      *date;
	GtkWidget      *sha;
	GtkWidget      *log;

	GiggleGit      *git;
	GiggleJob      *job;

	GtkTextMark    *search_mark;
};

static void       giggle_revision_view_searchable_init (GiggleSearchableIface *iface);

static void       revision_view_finalize           (GObject        *object);
static void       revision_view_get_property       (GObject        *object,
						    guint           param_id,
						    GValue         *value,
						    GParamSpec     *pspec);
static void       revision_view_set_property       (GObject        *object,
						    guint           param_id,
						    const GValue   *value,
						    GParamSpec     *pspec);

static gboolean   revision_view_search             (GiggleSearchable      *searchable,
						    const gchar           *search_term,
						    GiggleSearchDirection  direction,
						    gboolean               full_search);

static void       revision_view_update             (GiggleRevisionView *view);


G_DEFINE_TYPE_WITH_CODE (GiggleRevisionView, giggle_revision_view, GIGGLE_TYPE_VIEW,
			 G_IMPLEMENT_INTERFACE (GIGGLE_TYPE_SEARCHABLE,
						giggle_revision_view_searchable_init))

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REVISION_VIEW, GiggleRevisionViewPriv))

enum {
	PROP_0,
	PROP_REVISION,
};

static void
giggle_revision_view_class_init (GiggleRevisionViewClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = revision_view_finalize;
	object_class->set_property = revision_view_set_property;
	object_class->get_property = revision_view_get_property;

	g_object_class_install_property (object_class,
					 PROP_REVISION,
					 g_param_spec_object ("revision",
							      "Revision",
							      "Revision to show",
							      GIGGLE_TYPE_REVISION,
							      G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleRevisionViewPriv));
}

static void
giggle_revision_view_searchable_init (GiggleSearchableIface *iface)
{
	iface->search = revision_view_search;
}

static GtkWidget *
revision_view_create_label (const char *label,
			    double      xalign,
			    double      yalign)
{
	GtkWidget *widget;
	char      *markup;

	widget = gtk_label_new (NULL);
	markup = g_markup_printf_escaped ("<b>%s</b>", label);
	gtk_misc_set_alignment (GTK_MISC (widget), xalign, yalign);
	gtk_label_set_markup (GTK_LABEL (widget), markup);

	g_free (markup);

	return widget;
}

static void
revision_view_attach_info (GtkWidget  *table,
			   const char *label,
			   GtkWidget  *info,
			   int         row)
{
	int              xpad = 6, ypad = 3;
	double           yalign = 0.5;
	GtkAttachOptions yattach = GTK_FILL;
	int              end = 2;

	if (GTK_IS_BUTTON (info))
		xpad = ypad = 0;

	if (GTK_IS_SCROLLED_WINDOW (info)) {
		xpad = MAX (0, xpad - info->style->xthickness);
		ypad = MAX (0, ypad - info->style->ythickness);
		yattach |= GTK_EXPAND;
		yalign = 0.0;
		end = 3;
	}

	gtk_table_attach (GTK_TABLE (table),
			  revision_view_create_label (label, 0.0, yalign),
			  0, 1, row, row + 1, GTK_FILL, GTK_FILL, 6, 2);
	gtk_table_attach (GTK_TABLE (table), info, 1, end, row, row + 1,
			  GTK_FILL | GTK_EXPAND, yattach, xpad, ypad);
}

static GtkWidget *
revision_view_create_info (GtkWidget  *table,
			   const char *label,
			   int         row)
{
	GtkWidget *info;

	info = gtk_label_new (NULL);
	gtk_label_set_ellipsize (GTK_LABEL (info), PANGO_ELLIPSIZE_END);
	gtk_label_set_selectable (GTK_LABEL (info), TRUE);
	gtk_misc_set_alignment (GTK_MISC (info), 0.0, 0.5);

	revision_view_attach_info (table, label, info, row);

	return info;
}

static void
giggle_revision_view_init (GiggleRevisionView *view)
{
	GiggleRevisionViewPriv *priv;
	GtkWidget              *scrolled_window;
	GtkTextBuffer          *buffer;
	GtkTextIter             iter;
	int                     row = 0;

	priv = GET_PRIV (view);
	priv->git = giggle_git_get ();

	priv->table = gtk_table_new (5, 3, FALSE);

	priv->avatar = giggle_avatar_image_new ();
	gtk_misc_set_alignment (GTK_MISC (priv->avatar), 0.5, 0.0);
	giggle_avatar_image_set_image_size (GIGGLE_AVATAR_IMAGE (priv->avatar), 80);

	gtk_table_attach (GTK_TABLE (priv->table), priv->avatar,
			  2, 3, 0, 4, GTK_FILL, GTK_FILL, 3, 3);

	priv->author = gtk_link_button_new ("");
	gtk_button_set_alignment (GTK_BUTTON (priv->author), 0.0, 0.5);
	revision_view_attach_info (priv->table, _("Author:"), priv->author, row++);

	priv->date     = revision_view_create_info (priv->table, _("Date:"),     row++);
	priv->sha      = revision_view_create_info (priv->table, _("SHA:"),      row++);
	priv->branches = revision_view_create_info (priv->table, _("Branches:"), row++);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
					     GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request (scrolled_window, -1, 60);
	gtk_widget_show (scrolled_window);

	priv->log = gtk_text_view_new ();
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->log));
	gtk_text_view_set_editable (GTK_TEXT_VIEW (priv->log), FALSE);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (priv->log), GTK_WRAP_WORD_CHAR);
	gtk_widget_show (priv->log);

	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->log);

	revision_view_attach_info (priv->table, _("Change Log:"), scrolled_window, row);

	gtk_text_buffer_get_start_iter (buffer, &iter);
	priv->search_mark = gtk_text_buffer_create_mark (buffer,
							 "search-mark",
							 &iter, FALSE);

	gtk_container_add (GTK_CONTAINER (view), priv->table);
	gtk_widget_show_all (priv->table);
	gtk_widget_hide (priv->table);

	gtk_widget_grab_focus (priv->log);
}

static void
revision_view_finalize (GObject *object)
{
	GiggleRevisionViewPriv *priv;

	priv = GET_PRIV (object);

	if (priv->revision) {
		g_object_unref (priv->revision);
	}

	G_OBJECT_CLASS (giggle_revision_view_parent_class)->finalize (object);
}

static void
revision_view_get_property (GObject    *object,
			    guint       param_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	GiggleRevisionViewPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_REVISION:
		g_value_set_object (value, priv->revision);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
revision_view_set_property (GObject      *object,
			    guint         param_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	GiggleRevisionViewPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_REVISION:
		if (priv->revision)
			g_object_unref (priv->revision);

		priv->revision = g_value_dup_object (value);
		revision_view_update (GIGGLE_REVISION_VIEW (object));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
revision_view_search (GiggleSearchable      *searchable,
		      const gchar           *search_term,
		      GiggleSearchDirection  direction,
		      gboolean               full_search)
{
	GiggleRevisionViewPriv *priv;
	const gchar            *str, *p;
	gchar                  *log, *casefold_log, *casefold_str;
	glong                   offset, len;
	GtkTextBuffer          *buffer;
	GtkTextIter             start_iter, end_iter;
	gboolean                result = FALSE;

	priv = GET_PRIV (searchable);

	/* search in SHA label */
	str = gtk_label_get_text (GTK_LABEL (priv->sha));
	casefold_str = g_utf8_casefold (str, -1);

	if ((p = strstr (casefold_str, search_term)) != NULL) {
		offset = g_utf8_pointer_to_offset (casefold_str, p);
		len = g_utf8_strlen (search_term, -1);

		gtk_label_select_region (GTK_LABEL (priv->sha),
					 (gint) offset, (gint) offset + len);

		result = TRUE;
	}

	g_free (casefold_str);

	if (result) {
		return TRUE;
	}

	/* search in log */
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->log));
	gtk_text_buffer_get_bounds (buffer, &start_iter, &end_iter);
	log = gtk_text_buffer_get_text (buffer, &start_iter, &end_iter, FALSE);
	casefold_log = g_utf8_casefold (log, -1);

	if ((p = strstr (casefold_log, search_term)) != NULL) {
		offset = g_utf8_pointer_to_offset (casefold_log, p);
		len = g_utf8_strlen (search_term, -1);

		gtk_text_buffer_get_iter_at_offset (buffer, &start_iter, (gint) offset);
		gtk_text_buffer_get_iter_at_offset (buffer, &end_iter, (gint) offset + len);

		gtk_text_buffer_select_range (buffer, &start_iter, &end_iter);

		gtk_text_buffer_move_mark (buffer, priv->search_mark, &start_iter);
		gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (priv->log), priv->search_mark,
					      0.0, FALSE, 0.5, 0.5);
		result = TRUE;
	}

	g_free (casefold_log);
	g_free (log);

	return result;
}

static void
revision_view_update_log_cb  (GiggleGit *git,
			      GiggleJob *job,
			      GError    *error,
			      gpointer   user_data)
{
	GiggleRevisionViewPriv *priv;
	GtkTextBuffer          *buffer;
	const gchar            *log;

	priv = GET_PRIV (user_data);

	/* FIXME: error reporting missing */
	if (!error) {
		log = giggle_git_log_get_log (GIGGLE_GIT_LOG (job));
		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->log));
		gtk_text_buffer_set_text (buffer, log, -1);
	}

	g_object_unref (priv->job);
	priv->job = NULL;
}

static void
revision_view_update_branches_label (GiggleRevisionView *view)
{
	GiggleRevisionViewPriv *priv;
	GiggleRef              *ref;
	GList                  *branches;
	GString                *str;

	priv = GET_PRIV (view);

	gtk_label_set_text (GTK_LABEL (priv->branches), NULL);

	if (!priv->revision) {
		return;
	}

	branches = giggle_revision_get_descendent_branches (priv->revision);

	if (branches) {
		str = g_string_new ("");

		while (branches) {
			ref = GIGGLE_REF (branches->data);

			if (str->len)
				g_string_append (str, ", ");
				
			g_string_append (str, giggle_ref_get_name (ref));
			branches = branches->next;
		}

		gtk_label_set_markup (GTK_LABEL (priv->branches), str->str);
		g_string_free (str, TRUE);
	}
}

static void
revision_view_update (GiggleRevisionView *view)
{
	GiggleRevisionViewPriv *priv;
	GtkTextBuffer          *buffer;
	const char             *sha = NULL;
	const char             *author = NULL;
	const char             *email = NULL;
	const struct tm        *date;
	char                    str[256];
	char                   *uri = NULL;

	priv = GET_PRIV (view);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->log));
	gtk_text_buffer_set_text (buffer, "", -1);

	if (priv->revision) {
		sha    = giggle_revision_get_sha    (priv->revision);
		date   = giggle_revision_get_date   (priv->revision);
		author = giggle_revision_get_author (priv->revision);
		email  = giggle_revision_get_email  (priv->revision);

		if (email)
			uri = g_strdup_printf ("mailto:%s <%s>", author, email);

		if (date) {
			strftime (str, sizeof (str), "%c", date);
			gtk_label_set_text (GTK_LABEL (priv->date), str);
		} else {
			gtk_label_set_text (GTK_LABEL (priv->date), NULL);
		}

		if (priv->job) {
			giggle_git_cancel_job (priv->git, priv->job);
			g_object_unref (priv->job);
		}

		priv->job = giggle_git_log_new (priv->revision);

		giggle_git_run_job (priv->git,
				    priv->job,
				    revision_view_update_log_cb,
				    view);
	} else {
		gtk_label_set_text (GTK_LABEL (priv->date), NULL);
	}

	gtk_label_set_text (GTK_LABEL (priv->sha), sha);
	gtk_button_set_label (GTK_BUTTON (priv->author), author);
	gtk_link_button_set_uri (GTK_LINK_BUTTON (priv->author), uri ? uri : "");
	giggle_avatar_image_set_gravatar_id (GIGGLE_AVATAR_IMAGE (priv->avatar), email);

	revision_view_update_branches_label (view);

	if (priv->revision) {
		gtk_widget_show (priv->table);
	} else {
		gtk_widget_hide (priv->table);
	}

	g_free (uri);
}

GtkWidget *
giggle_revision_view_new (void)
{
	GtkAction *action;

	action = g_object_new (GTK_TYPE_RADIO_ACTION,
			       "name", "RevisionView",
			       "label", _("_Details"),
			       "tooltip", _("Display revision details"),
			       "stock-id", GTK_STOCK_PROPERTIES,
			       "is-important", TRUE, NULL);

	return g_object_new (GIGGLE_TYPE_REVISION_VIEW, "action", action, NULL);
}

void
giggle_revision_view_set_revision (GiggleRevisionView *view,
				   GiggleRevision     *revision)
{
	g_return_if_fail (GIGGLE_IS_REVISION_VIEW (view));
	g_return_if_fail (!revision || GIGGLE_IS_REVISION (revision));

	g_object_set (view,
		      "revision", revision,
		      NULL);
}

GiggleRevision *
giggle_revision_view_get_revision (GiggleRevisionView *view)
{
	GiggleRevisionViewPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION_VIEW (view), NULL);

	priv = GET_PRIV (view);
	return priv->revision;
}
