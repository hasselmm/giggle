/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2008 Mathias Hasselmann
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
#include "giggle-git-blame.h"

#include <string.h>

typedef struct GiggleGitBlamePriv GiggleGitBlamePriv;

struct GiggleGitBlamePriv {
	GiggleRevision *revision;

	char           *file;
	GPtrArray      *chunks;
	GHashTable     *revision_cache;
};

G_DEFINE_TYPE (GiggleGitBlame, giggle_git_blame, GIGGLE_TYPE_JOB)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_BLAME, GiggleGitBlamePriv))

enum {
	PROP_0,
	PROP_REVISION,
	PROP_FILE,
};


static void
git_blame_finalize (GObject *object)
{
	GiggleGitBlamePriv  *priv;

	priv = GET_PRIV (object);

	g_ptr_array_free (priv->chunks, TRUE);
	g_free (priv->file);

	G_OBJECT_CLASS (giggle_git_blame_parent_class)->finalize (object);
}

static void
git_blame_dispose (GObject *object)
{
	GiggleGitBlamePriv *priv;
	int		    i;

	priv = GET_PRIV (object);

	if (priv->revision) {
		g_object_unref (priv->revision);
		priv->revision = NULL;
	}

	if (priv->revision_cache) {
		g_hash_table_destroy (priv->revision_cache);
		priv->revision_cache = NULL;
	}

	while (priv->chunks->len > 0) {
		i = priv->chunks->len - 1;
		g_slice_free (GiggleGitBlameChunk, priv->chunks->pdata[i]);
		g_ptr_array_remove_index_fast (priv->chunks, i);
	}

	G_OBJECT_CLASS (giggle_git_blame_parent_class)->dispose (object);
}

static void
git_blame_get_property (GObject    *object,
			    guint       param_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	GiggleGitBlamePriv *priv;

	priv = GET_PRIV (object);
	
	switch (param_id) {
	case PROP_REVISION:
		g_value_set_object (value, priv->revision);
		break;

	case PROP_FILE:
		g_value_set_string (value, priv->file);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
git_blame_set_property (GObject      *object,
			    guint         param_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	GiggleGitBlamePriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_REVISION:
		g_assert (NULL == priv->revision);
		priv->revision = g_value_dup_object (value);
		break;

	case PROP_FILE:
		g_assert (NULL == priv->file);
		priv->file = g_value_dup_string (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
git_blame_get_command_line (GiggleJob *job, gchar **command_line)
{
	GiggleGitBlamePriv *priv;
	const char         *sha = "";
	char		   *file;

	priv = GET_PRIV (job);

	if (priv->revision)
		sha = giggle_revision_get_sha (priv->revision);

	file = g_shell_quote (priv->file);

	*command_line = g_strconcat (GIT_COMMAND " blame --incremental",
				     sha, " ", file, NULL);

	g_free (file);

	return TRUE;
}

static void
git_blame_handle_output (GiggleJob   *job,
			 const gchar *output_str,
			 gsize        output_len)
{
	GiggleGitBlamePriv  *priv;
	const char          *start, *end;
	GiggleGitBlameChunk *chunk = NULL;
	char                 sha[41];
	time_t		     time;
	int		     i;

	priv = GET_PRIV (job);

	for (end = output_str; *end; ++end) {
		start = end;
		end = strchr (start, '\n');

		if (NULL == end)
			break;

		if (!chunk) {
			chunk = g_slice_new (GiggleGitBlameChunk);
			g_ptr_array_add (priv->chunks, chunk);

			sscanf (start, "%40s %d %d %d", sha,
				&chunk->source_line, &chunk->result_line,
				&chunk->num_lines);

			chunk->revision = g_hash_table_lookup (priv->revision_cache, sha);

			if (!chunk->revision) {
				chunk->revision = giggle_revision_new (sha);

				g_hash_table_insert (priv->revision_cache,
						     g_strdup (sha), chunk->revision);
			}
		} else if (g_str_has_prefix (start, "author ")) {
			char *author = g_strndup (start + 7, end - start - 7);
			g_object_set (chunk->revision, "author", author, NULL);
			g_free (author);
		} else if (1 == sscanf (start, "author-time %d\n", &i)) {
			struct tm *date = g_new (struct tm, 1);

			time = i;
			g_object_set (chunk->revision, "date", gmtime_r (&time, date), NULL);
		} else if (g_str_has_prefix (start, "summary ")) {
			char *summary = g_strndup (start + 8, end - start - 8);
			g_object_set (chunk->revision, "short-log", summary, NULL);
			g_free (summary);
		} else if (g_str_has_prefix (start, "filename ")) {
			chunk = NULL;
		}
	}
}

static void
giggle_git_blame_class_init (GiggleGitBlameClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GiggleJobClass *job_class    = GIGGLE_JOB_CLASS (class);

	object_class->get_property  = git_blame_get_property;
	object_class->set_property  = git_blame_set_property;
	object_class->finalize      = git_blame_finalize;
	object_class->dispose       = git_blame_dispose;

	job_class->get_command_line = git_blame_get_command_line;
	job_class->handle_output    = git_blame_handle_output;

	g_object_class_install_property (object_class,
					 PROP_REVISION,
					 g_param_spec_object ("revision",
							      "revision",
							      "revision of the file to annotate",
							      GIGGLE_TYPE_REVISION,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
					 PROP_FILE,
					 g_param_spec_string ("file",
							      "file",
							      "name of the file to annotate",
							      NULL,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private (object_class, sizeof (GiggleGitBlamePriv));
}

static void
giggle_git_blame_init (GiggleGitBlame *blame)
{
	GiggleGitBlamePriv *priv;

	priv = GET_PRIV (blame);

	priv->chunks = g_ptr_array_new ();

	priv->revision_cache = g_hash_table_new_full (g_str_hash, g_str_equal,
						      g_free, g_object_unref);
}

GiggleJob *
giggle_git_blame_new (GiggleRevision *revision,
		      const char     *file)
{
	g_return_val_if_fail (NULL != file, NULL);

	return g_object_new (GIGGLE_TYPE_GIT_BLAME,
			     "revision", revision,
			     "file", file, NULL);
}

const GiggleGitBlameChunk *
giggle_git_blame_get_chunk (GiggleGitBlame *blame,
			    int             index)
{
	GiggleGitBlamePriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_BLAME (blame), NULL);
	g_return_val_if_fail (index >= 0, NULL);

	priv = GET_PRIV (blame);

	if (index < priv->chunks->len)
		return priv->chunks->pdata[index];

	return NULL;
}

