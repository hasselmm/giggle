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
#include "giggle-git-commit.h"

typedef struct GiggleGitCommitPriv GiggleGitCommitPriv;

struct GiggleGitCommitPriv {
	GList *files;
	gchar *log;
};

static void     git_commit_finalize            (GObject           *object);
static void     git_commit_get_property        (GObject           *object,
						guint              param_id,
						GValue            *value,
						GParamSpec        *pspec);
static void     git_commit_set_property        (GObject           *object,
						guint              param_id,
						const GValue      *value,
						GParamSpec        *pspec);

static gboolean git_commit_get_command_line    (GiggleJob         *job,
						gchar            **command_line);


G_DEFINE_TYPE (GiggleGitCommit, giggle_git_commit, GIGGLE_TYPE_JOB)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_COMMIT, GiggleGitCommitPriv))

enum {
	PROP_0,
	PROP_FILES,
	PROP_LOG,
};

static void
giggle_git_commit_class_init (GiggleGitCommitClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GiggleJobClass *job_class    = GIGGLE_JOB_CLASS (class);

	object_class->finalize     = git_commit_finalize;
	object_class->get_property = git_commit_get_property;
	object_class->set_property = git_commit_set_property;

	job_class->get_command_line = git_commit_get_command_line;

	g_object_class_install_property (object_class,
					 PROP_FILES,
					 g_param_spec_pointer ("files",
							       "Files",
							       "List of files to commit",
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_LOG,
					 g_param_spec_string ("log",
							      "Log",
							      "Log for the changeset",
							      NULL,
							      G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleGitCommitPriv));
}

static void
giggle_git_commit_init (GiggleGitCommit *dummy)
{
}

static void
git_commit_finalize (GObject *object)
{
	GiggleGitCommitPriv *priv;

	priv = GET_PRIV (object);

	g_free (priv->log);

	g_list_foreach (priv->files, (GFunc) g_free, NULL);
	g_list_free (priv->files);

	G_OBJECT_CLASS (giggle_git_commit_parent_class)->finalize (object);
}

static void
git_commit_get_property (GObject    *object,
			 guint       param_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	GiggleGitCommitPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_FILES:
		g_value_set_pointer (value, priv->files);
		break;
	case PROP_LOG:
		g_value_set_string (value, priv->log);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
git_commit_set_property (GObject      *object,
			 guint         param_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	GiggleGitCommitPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_FILES:
		priv->files = g_value_get_pointer (value);
		break;
	case PROP_LOG:
		priv->log = g_value_dup_string (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
git_commit_get_command_line (GiggleJob *job, gchar **command_line)
{
	GiggleGitCommitPriv *priv;
	GString             *str;
	GList               *files;
	gchar               *escaped;

	priv = GET_PRIV (job);
	files = priv->files;
	str = g_string_new (GIT_COMMAND " commit");

	if (priv->log) {
		escaped = g_strescape (priv->log, "\b\f\n\r\t\\");
	} else {
		escaped = g_strdup ("");
	}

	g_string_append_printf (str, " -m \"%s\"", escaped);

	if (!files) {
		g_string_append_printf (str, " -a");
	} else {
		while (files) {
			g_string_append_printf (str, " %s", (gchar *) files->data);
			files = files->next;
		}
	}

	*command_line = g_string_free (str, FALSE);
	return TRUE;
}

GiggleJob *
giggle_git_commit_new (const gchar *log)
{
	return g_object_new (GIGGLE_TYPE_GIT_COMMIT,
			     "log", log,
			     NULL);
}

void
giggle_git_commit_set_files (GiggleGitCommit *commit,
			     GList           *files)
{
	g_return_if_fail (GIGGLE_IS_GIT_COMMIT (commit));

	g_object_set (commit,
		      "files", files,
		      NULL);
}
