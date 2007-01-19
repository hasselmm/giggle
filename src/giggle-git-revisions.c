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
static void     git_revisions_set_committer_info  (GiggleRevision *revision,
						   const gchar    *line);


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
#if 0
	g_object_class_install_property (object_class,
					 PROP_MY_PROP,
					 g_param_spec_string ("my-prop",
							      "My Prop",
							      "Describe the property",
							      NULL,
							      G_PARAM_READABLE));
#endif

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
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
git_revisions_get_command_line (GiggleJob *job, gchar **command_line)
{
	*command_line = g_strdup_printf ("git rev-list --all --header --topo-order --parents");

	return TRUE;
}

static void
git_revisions_handle_output (GiggleJob   *job,
			     const gchar *output_str,
			     gsize        output_len)
{
	GiggleGitRevisionsPriv *priv;
	GiggleRevision         *revision;
	GHashTable             *revisions_hash;
	gchar                  *str;

	priv = GET_PRIV (job);
	priv->revisions = NULL;
	str = (gchar *) output_str;
	revisions_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
						g_free, g_object_unref);

	while (strlen (str) > 0) {
		revision = git_revisions_get_revision (str, revisions_hash);
		priv->revisions = g_list_prepend (priv->revisions, revision);

		/* go to the next entry, they're separated by NULLs */
		str += strlen (str) + 1;
	}

	priv->revisions = g_list_reverse (priv->revisions);
	g_hash_table_destroy (revisions_hash);
}

static void
git_revisions_set_committer_info (GiggleRevision *revision, const gchar *line)
{
	gchar *author, *date;
	gchar **strarr;

	/* parse author */
	strarr = g_strsplit (line, " <", 2);
	author = g_strdup (strarr[0]);
	g_strfreev (strarr);

	/* parse timestamp */
	strarr = g_strsplit (line, "> ", 2);
	date = g_strdup (strarr[1]);
	g_strfreev (strarr);

	g_object_set (revision,
		      "author", author,
		      "date", date,
		      NULL);

	g_free (author);
	g_free (date);
}

static void
git_revisions_parse_revision_info (GiggleRevision  *revision,
				   gchar          **lines)
{
	gint i = 0;
	GString *long_log = NULL;
	gchar *short_log;

	while (lines[i]) {
		if (g_str_has_prefix (lines[i], "committer ")) {
			git_revisions_set_committer_info (revision, lines[i] + strlen ("committer "));
		} else if (g_str_has_prefix (lines[i], " ")) {
			if (!long_log) {
				/* no short log neither, get some */
				short_log = g_strdup (lines[i]);
				g_object_set (revision, "short-log", g_strstrip (short_log), NULL);
				long_log = g_string_new ("");

				g_free (short_log);
			}

			g_string_append_printf (long_log, "%s\n", lines[i]);
		}

		i++;
	}
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
	GiggleGitRevisions     *revisions;
	GiggleGitRevisionsPriv *priv;

	revisions = g_object_new (GIGGLE_TYPE_GIT_REVISIONS, NULL);
	priv = GET_PRIV (revisions);

	return GIGGLE_JOB (revisions);
}

GList *
giggle_git_revisions_get_revisions (GiggleGitRevisions *revisions)
{
	GiggleGitRevisionsPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_REVISIONS (revisions), NULL);

	priv = GET_PRIV (revisions);

	return priv->revisions;
}
