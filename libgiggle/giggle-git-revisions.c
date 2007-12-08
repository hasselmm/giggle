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

#include <string.h>

#include "giggle-git-revisions.h"

typedef struct GiggleGitRevisionsPriv GiggleGitRevisionsPriv;

struct GiggleGitRevisionsPriv {
	GList *revisions;
	GList *files;
};

static void     git_revisions_finalize            (GObject           *object);
static void     git_revisions_get_property        (GObject           *object,
						   guint              param_id,
						   GValue            *value,
						   GParamSpec        *pspec);
static void     git_revisions_set_property        (GObject           *object,
						   guint              param_id,
						   const GValue      *value,
						   GParamSpec        *pspec);

static gboolean git_revisions_get_command_line    (GiggleJob         *job,
						   gchar            **command_line);
static void     git_revisions_handle_output       (GiggleJob         *job,
						   const gchar       *output_str,
						   gsize              output_len);
static GiggleRevision* git_revisions_get_revision (const gchar *str,
						   GHashTable  *revisions_hash);
static void     git_revisions_get_committer_info  (GiggleRevision  *revision,
						   const gchar     *line,
						   gchar          **author,
						   struct tm      **tm);

enum {
	PROP_0,
	PROP_FILES,
};

G_DEFINE_TYPE (GiggleGitRevisions, giggle_git_revisions, GIGGLE_TYPE_JOB)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_REVISIONS, GiggleGitRevisionsPriv))

static void
giggle_git_revisions_class_init (GiggleGitRevisionsClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GiggleJobClass *job_class    = GIGGLE_JOB_CLASS (class);

	object_class->finalize     = git_revisions_finalize;
	object_class->get_property = git_revisions_get_property;
	object_class->set_property = git_revisions_set_property;

	job_class->get_command_line = git_revisions_get_command_line;
	job_class->handle_output    = git_revisions_handle_output;

	g_object_class_install_property (object_class,
					 PROP_FILES,
					 g_param_spec_pointer ("files",
							       "files",
							       "files to filter the revisions",
							       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private (object_class, sizeof (GiggleGitRevisionsPriv));
}

static void
giggle_git_revisions_init (GiggleGitRevisions *revisions)
{
	GiggleGitRevisionsPriv *priv;

	priv = GET_PRIV (revisions);

	priv->revisions = NULL;
}

static void
git_revisions_finalize (GObject *object)
{
	GiggleGitRevisionsPriv *priv;

	priv = GET_PRIV (object);

	g_list_foreach (priv->revisions, (GFunc) g_object_unref, NULL);
	g_list_free (priv->revisions);

	g_list_foreach (priv->files, (GFunc) g_free, NULL);
	g_list_free (priv->files);

	G_OBJECT_CLASS (giggle_git_revisions_parent_class)->finalize (object);
}

static void
git_revisions_get_property (GObject    *object,
			    guint       param_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	GiggleGitRevisionsPriv *priv;

	priv = GET_PRIV (object);
	
	switch (param_id) {
	case PROP_FILES:
		g_value_set_pointer (value, priv->files);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
git_revisions_set_property (GObject      *object,
			    guint         param_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	GiggleGitRevisionsPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_FILES:
		/* FIXME: we know it'll be a list of strings and
		 * this object will manage its memory, maybe it's
		 * not the best thing design-wise
		 */
		priv->files = g_value_get_pointer (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
git_revisions_get_command_line (GiggleJob *job, gchar **command_line)
{
	GiggleGitRevisionsPriv *priv;
	GString                *str;
	GList                  *files;

	priv = GET_PRIV (job);
	files = priv->files;
	str = g_string_new (GIT_COMMAND " rev-list --all --header --topo-order --parents");

	while (files) {
		g_string_append_printf (str, " %s", (gchar *) files->data);
		files = files->next;
	}

	*command_line = g_string_free (str, FALSE);
	return TRUE;
}

static void
git_revisions_handle_output (GiggleJob   *job,
			     const gchar *output_str,
			     gsize        output_len)
{
	GiggleGitRevisionsPriv *priv;
	GHashTable             *revisions_hash;
	gchar                  *str;

	priv = GET_PRIV (job);
	priv->revisions = NULL;
	str = (gchar *) output_str;
	revisions_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
						g_free, g_object_unref);

	while (strlen (str) > 0) {
		GiggleRevision *revision;

		revision = git_revisions_get_revision (str, revisions_hash);
		priv->revisions = g_list_prepend (priv->revisions, revision);

		/* go to the next entry, they're separated by NULLs */
		str += strlen (str) + 1;
	}

	priv->revisions = g_list_reverse (priv->revisions);
	g_hash_table_destroy (revisions_hash);
}

static struct tm *
git_revisions_get_time (const gchar *date)
{
	const gchar *returned;
	struct tm   *tm = NULL;
	time_t       time;

	/* shut up gcc */
	returned = NULL;
	time = 0;

#if STRPTIME_HAS_GNU
	tm = g_new0 (struct tm, 1);
	returned = strptime (date, "%s %z", tm);

	if (!returned) {
		g_free (tm);
		return NULL;
	}
#else
	tm = g_new0 (struct tm, 1);

	/* FIXME: looks awful */
	sscanf (date, "%ld", &time);
	localtime_r (&time, tm);
#endif
	return tm;
}

static void
git_revisions_get_committer_info (GiggleRevision  *revision,
				  const gchar     *line,
				  gchar          **author,
				  struct tm      **tm)
{
	const gchar *str;
	const gchar *ch;

	str = line;
	
	ch = strstr (str, " <");
	*author = g_strndup (str, ch - str);
	str = ch + 2;

	ch = strstr (str, "> ");

	*tm = git_revisions_get_time (ch + 2);
}

static void
git_revisions_parse_revision_info (GiggleRevision  *revision,
				   gchar          **lines)
{
	gint       i = 0;
	struct tm *tm = NULL;
	gchar     *author, *short_log;

	author = short_log = NULL;

	while (lines[i]) {
		gchar* converted = NULL;

		if (g_utf8_validate (lines[i], -1, NULL)) {
			converted = g_strdup (lines[i]);
		}

		if (!converted) {
			converted = g_locale_to_utf8 (lines[i], -1,
						      NULL, NULL,
						      NULL); // FIXME: add GError
		}

		if (!converted) {
			converted = g_filename_to_utf8 (lines[i], -1,
							NULL, NULL,
							NULL); // FIXME: add GError
		}

		if (!converted) {
			converted = g_convert (lines[i], -1,
					       "UTF-8", "ISO-8859-15",
					       NULL, NULL, NULL); // FIXME: add GError
		}

		if (!converted) {
			converted = g_strescape (lines[i], "\n\r\\\"\'");
		}

		if (!converted) {
			g_warning ("Error while converting string");
			// FIXME: fallback
		}

		if (g_str_has_prefix (converted, "author ")) {
			git_revisions_get_committer_info (revision,
							  converted + strlen ("author "),
							  &author, &tm);
		} else if (!short_log && g_str_has_prefix (converted, " ")) {
			g_strstrip (converted);
			short_log = g_strdup (converted);
		}

		g_free (converted);

		i++;
	}

	g_object_set (revision,
		      "author", author,
		      "date", tm,
		      "short-log", short_log,
		      NULL);

	g_free (author);
	g_free (short_log);
}

static GiggleRevision*
git_revisions_get_revision (const gchar *str,
			    GHashTable  *revisions_hash)
{
	GiggleRevision *revision, *parent;
	gchar **lines, **ids;
	gint i = 1;

	lines = g_strsplit (str, "\n", -1);
	ids = g_strsplit (lines[0], " ", -1);

	if (!(revision = g_hash_table_lookup (revisions_hash, ids[0]))) {
		/* revision hasn't been created in a previous step, create it */
		revision = giggle_revision_new (ids[0]);
		g_hash_table_insert (revisions_hash, g_strdup (ids[0]), revision);
	}

	/* add parents */
	while (ids[i] != NULL) {
		if (!(parent = g_hash_table_lookup (revisions_hash, ids[i]))) {
			parent = giggle_revision_new (ids[i]);
			g_hash_table_insert (revisions_hash, g_strdup (ids[i]), parent);
		}

		giggle_revision_add_parent (revision, parent);
		i++;
	}

	git_revisions_parse_revision_info (revision, lines);

	g_strfreev (ids);
	g_strfreev (lines);

	return g_object_ref (revision);
}

GiggleJob *
giggle_git_revisions_new (void)
{
	return g_object_new (GIGGLE_TYPE_GIT_REVISIONS, NULL);
}

GiggleJob *
giggle_git_revisions_new_for_files (GList *files)
{
	return g_object_new (GIGGLE_TYPE_GIT_REVISIONS,
			     "files", files,
			     NULL);
}

GList *
giggle_git_revisions_get_revisions (GiggleGitRevisions *revisions)
{
	GiggleGitRevisionsPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_REVISIONS (revisions), NULL);

	priv = GET_PRIV (revisions);

	return priv->revisions;
}
