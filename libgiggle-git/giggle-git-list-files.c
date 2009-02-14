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
#include "giggle-git-list-files.h"

typedef struct GiggleGitListFilesPriv GiggleGitListFilesPriv;

struct GiggleGitListFilesPriv {
	GHashTable *files;
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
	GiggleGitListFilesPriv *priv;

	priv = GET_PRIV (list_files);

	priv->files = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}

static void
git_list_files_finalize (GObject *object)
{
	GiggleGitListFilesPriv *priv;

	priv = GET_PRIV (object);

	g_hash_table_destroy (priv->files);

	G_OBJECT_CLASS (giggle_git_list_files_parent_class)->finalize (object);
}

static gboolean
git_list_files_get_command_line (GiggleJob *job, gchar **command_line)
{
	*command_line = g_strdup_printf (GIT_COMMAND " ls-files "
					 "--cached --deleted --modified --others "
					 "--killed -t --full-name");

	return TRUE;
}

static GiggleGitListFilesStatus
git_list_files_char_to_status (gchar status)
{
	switch (status) {
	case 'H':
		return GIGGLE_GIT_FILE_STATUS_CACHED;
	case 'M':
		return GIGGLE_GIT_FILE_STATUS_UNMERGED;
	case 'R':
		return GIGGLE_GIT_FILE_STATUS_DELETED;
	case 'C':
		return GIGGLE_GIT_FILE_STATUS_CHANGED;
	case 'K':
		return GIGGLE_GIT_FILE_STATUS_KILLED;
	case '?':
		return GIGGLE_GIT_FILE_STATUS_OTHER;
	default:
		g_assert_not_reached ();
		return GIGGLE_GIT_FILE_STATUS_OTHER;
	}
}

static void
git_list_files_handle_output (GiggleJob   *job,
			const gchar *output_str,
			gsize        output_len)
{
	GiggleGitListFilesPriv    *priv;
	GiggleGitListFilesStatus   status;
	gchar                    **lines;
	gchar                     *file;
	gchar                      status_char;
	gint                       i;

	priv = GET_PRIV (job);
	lines = g_strsplit (output_str, "\n", -1);

	for (i = 0; lines[i] && *lines[i]; i++) {
		status_char = lines[i][0];
		file = g_strdup (&lines[i][2]); /* just the file name */
		status = git_list_files_char_to_status (status_char);
		
		/* add filename */
		g_hash_table_insert (priv->files, file, GINT_TO_POINTER (status));
	}

	g_strfreev (lines);
}

GiggleJob *
giggle_git_list_files_new ()
{
	return g_object_new (GIGGLE_TYPE_GIT_LIST_FILES, NULL);
}

GiggleGitListFilesStatus
giggle_git_list_files_get_file_status (GiggleGitListFiles *list_files,
				       const gchar        *file)
{
	GiggleGitListFilesPriv   *priv;
	GiggleGitListFilesStatus  status;

	g_return_val_if_fail (GIGGLE_IS_GIT_LIST_FILES (list_files),
			      GIGGLE_GIT_FILE_STATUS_OTHER);

	priv = GET_PRIV (list_files);

	status = GPOINTER_TO_INT (g_hash_table_lookup (priv->files, file));
	return status;
}
