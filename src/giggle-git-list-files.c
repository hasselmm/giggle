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

#include "giggle-git-list-files.h"

typedef struct GiggleGitListFilesPriv GiggleGitListFilesPriv;

struct GiggleGitListFilesPriv {
	GList *files;
};

static void     git_list_files_finalize            (GObject           *object);

static gboolean git_list_files_get_command_line    (GiggleJob         *job,
						    gchar            **command_line);
static void     git_list_files_handle_output       (GiggleJob         *job,
						    const gchar       *output_str,
						    gsize              output_len);

G_DEFINE_TYPE (GiggleGitListFiles, giggle_git_list_files, GIGGLE_TYPE_JOB)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_LIST_FILES, GiggleGitListFilesPriv))

static void
giggle_git_list_files_class_init (GiggleGitListFilesClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GiggleJobClass *job_class    = GIGGLE_JOB_CLASS (class);

	object_class->finalize     = git_list_files_finalize;

	job_class->get_command_line = git_list_files_get_command_line;
	job_class->handle_output    = git_list_files_handle_output;

	g_type_class_add_private (object_class, sizeof (GiggleGitListFilesPriv));
}

static void
giggle_git_list_files_init (GiggleGitListFiles *list_files)
{
}

static void
git_list_files_finalize (GObject *object)
{
	GiggleGitListFilesPriv *priv;

	priv = GET_PRIV (object);

	g_list_foreach (priv->files, (GFunc) g_free, NULL);
	g_list_free (priv->files);

	G_OBJECT_CLASS (giggle_git_list_files_parent_class)->finalize (object);
}

static gboolean
git_list_files_get_command_line (GiggleJob *job, gchar **command_line)
{
	/* FIXME: At the moment, only lists files that are not in the repo */
	*command_line = g_strdup_printf ("git ls-files --others");

	return TRUE;
}

static void
git_list_files_handle_output (GiggleJob   *job,
			const gchar *output_str,
			gsize        output_len)
{
	GiggleGitListFilesPriv  *priv;
	gchar                  **lines;
	gint                     i;

	priv = GET_PRIV (job);

	g_list_foreach (priv->files, (GFunc) g_free, NULL);
	g_list_free (priv->files);

	lines = g_strsplit (output_str, "\n", -1);

	for (i = 0; lines[i] && *lines[i]; i++) {
		/* add filename */
		priv->files = g_list_prepend (priv->files, g_strdup (lines[i]));
	}

	priv->files = g_list_reverse (priv->files);
	g_strfreev (lines);
}

GiggleJob *
giggle_git_list_files_new ()
{
	return g_object_new (GIGGLE_TYPE_GIT_LIST_FILES, NULL);
}

GList *
giggle_git_list_files_get_files (GiggleGitListFiles *list_files)
{
	GiggleGitListFilesPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_LIST_FILES (list_files), NULL);

	priv = GET_PRIV (list_files);

	return priv->files;
}
