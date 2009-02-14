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
#include "giggle-git-remote-list.h"

#include <string.h>

typedef struct GiggleGitRemoteListPriv GiggleGitRemoteListPriv;

struct GiggleGitRemoteListPriv {
	GList *names;
};

G_DEFINE_TYPE (GiggleGitRemoteList, giggle_git_remote_list, GIGGLE_TYPE_JOB)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_REMOTE_LIST, GiggleGitRemoteListPriv))

static void
git_remote_list_finalize (GObject *object)
{
	GiggleGitRemoteListPriv *priv;

	priv = GET_PRIV (object);

	while (priv->names) {
		g_free (priv->names->data);
		priv->names = g_list_delete_link (priv->names, priv->names);
	}

	G_OBJECT_CLASS (giggle_git_remote_list_parent_class)->finalize (object);
}

static gboolean
git_remote_list_get_command_line (GiggleJob *job, gchar **command_line)
{
	*command_line = g_strdup (GIT_COMMAND " remote");

	return TRUE;
}

static void
git_remote_list_handle_output (GiggleJob   *job,
			       const gchar *output_str,
			       gsize        output_len)
{
	GiggleGitRemoteListPriv  *priv;
	const char		 *start, *end;
	char			 *name;

	priv = GET_PRIV (job);

	for (end = output_str; *end; ++end) {
		end = strchr (start = end, '\n');

		if (NULL == end)
			break;

		name = g_strndup (start, end - start);
		priv->names = g_list_prepend (priv->names, name);
	}

	priv->names = g_list_reverse (priv->names);
}

static void
giggle_git_remote_list_class_init (GiggleGitRemoteListClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GiggleJobClass *job_class    = GIGGLE_JOB_CLASS (class);

	object_class->finalize      = git_remote_list_finalize;

	job_class->get_command_line = git_remote_list_get_command_line;
	job_class->handle_output    = git_remote_list_handle_output;

	g_type_class_add_private (object_class, sizeof (GiggleGitRemoteListPriv));
}

static void
giggle_git_remote_list_init (GiggleGitRemoteList *job)
{
}

GiggleJob *
giggle_git_remote_list_new (void)
{
	return g_object_new (GIGGLE_TYPE_GIT_REMOTE_LIST, NULL);
}

GList *
giggle_git_remote_list_get_names (GiggleGitRemoteList *job)
{
	g_return_val_if_fail (GIGGLE_IS_GIT_REMOTE_LIST (job), NULL);
	return GET_PRIV (job)->names;
}
