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
#include "giggle-git-config-read.h"

#include <string.h>

typedef struct GiggleGitConfigReadPriv GiggleGitConfigReadPriv;

struct GiggleGitConfigReadPriv {
	GHashTable *hash_table;
};

static void     git_config_read_finalize         (GObject           *object);
static void     git_config_read_get_property     (GObject           *object,
						  guint              param_id,
						  GValue            *value,
						  GParamSpec        *pspec);
static void     git_config_read_set_property     (GObject           *object,
						  guint              param_id,
						  const GValue      *value,
						  GParamSpec        *pspec);

static gboolean git_config_read_get_command_line (GiggleJob         *job,
						  gchar            **command_line);
static void     git_config_read_handle_output    (GiggleJob         *job,
						  const gchar       *output_str,
						  gsize              output_len);


G_DEFINE_TYPE (GiggleGitConfigRead, giggle_git_config_read, GIGGLE_TYPE_JOB)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_CONFIG_READ, GiggleGitConfigReadPriv))


static void
giggle_git_config_read_class_init (GiggleGitConfigReadClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GiggleJobClass *job_class    = GIGGLE_JOB_CLASS (class);

	object_class->finalize     = git_config_read_finalize;
	object_class->get_property = git_config_read_get_property;
	object_class->set_property = git_config_read_set_property;

	job_class->get_command_line = git_config_read_get_command_line;
	job_class->handle_output    = git_config_read_handle_output;

	g_type_class_add_private (object_class, sizeof (GiggleGitConfigReadPriv));
}

static void
giggle_git_config_read_init (GiggleGitConfigRead *read_config)
{
	GiggleGitConfigReadPriv *priv;

	priv = GET_PRIV (read_config);

	priv->hash_table = g_hash_table_new_full (g_str_hash,
						  g_str_equal,
						  g_free,
						  g_free);
}

static void
git_config_read_finalize (GObject *object)
{
	GiggleGitConfigReadPriv *priv;

	priv = GET_PRIV (object);

	g_hash_table_unref (priv->hash_table);

	G_OBJECT_CLASS (giggle_git_config_read_parent_class)->finalize (object);
}

static void
git_config_read_get_property (GObject    *object,
			      guint       param_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
	GiggleGitConfigReadPriv *priv;

	priv = GET_PRIV (object);
	
	switch (param_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
git_config_read_set_property (GObject      *object,
			      guint         param_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
	GiggleGitConfigReadPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
git_config_read_get_command_line (GiggleJob  *job,
				  gchar     **command_line)
{
	*command_line = g_strdup_printf (GIT_COMMAND " repo-config --list");
	return TRUE;
}

static void
git_config_read_handle_output (GiggleJob   *job,
			       const gchar *output_str,
			       gsize        output_len)
{
	GiggleGitConfigReadPriv  *priv;
	gchar                   **lines, **line;
	gint                      i;

	priv = GET_PRIV (job);
	lines = g_strsplit (output_str, "\n", -1);

	for (i = 0; lines[i] && *lines[i]; i++) {
		line = g_strsplit (lines[i], "=", 2);
		g_hash_table_insert (priv->hash_table, g_strdup (line[0]), g_strdup (line[1]));
		g_strfreev (line);
	}

	g_strfreev (lines);
}

GiggleJob *
giggle_git_config_read_new (void)
{
	return g_object_new (GIGGLE_TYPE_GIT_CONFIG_READ, NULL);
}

GHashTable *
giggle_git_config_read_get_config (GiggleGitConfigRead *config)
{
	GiggleGitConfigReadPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_CONFIG_READ (config), NULL);

	priv = GET_PRIV (config);
	return priv->hash_table;
}
