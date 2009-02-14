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
#include "giggle-git-add.h"

typedef struct GiggleGitAddPriv GiggleGitAddPriv;

struct GiggleGitAddPriv {
	GList *files;
};

static void     git_add_finalize            (GObject           *object);
static void     git_add_get_property        (GObject           *object,
					     guint              param_id,
					     GValue            *value,
					     GParamSpec        *pspec);
static void     git_add_set_property        (GObject           *object,
					     guint              param_id,
					     const GValue      *value,
					     GParamSpec        *pspec);

static gboolean git_add_get_command_line    (GiggleJob         *job,
					     gchar            **command_line);

G_DEFINE_TYPE (GiggleGitAdd, giggle_git_add, GIGGLE_TYPE_JOB)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_ADD, GiggleGitAddPriv))

enum {
	PROP_0,
	PROP_FILES,
};

static void
giggle_git_add_class_init (GiggleGitAddClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GiggleJobClass *job_class    = GIGGLE_JOB_CLASS (class);

	object_class->finalize     = git_add_finalize;
	object_class->get_property = git_add_get_property;
	object_class->set_property = git_add_set_property;

	job_class->get_command_line = git_add_get_command_line;

	g_object_class_install_property (object_class,
					 PROP_FILES,
					 g_param_spec_pointer ("files",
							       "Files",
							       "List of files to add",
							       G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleGitAddPriv));
}

static void
giggle_git_add_init (GiggleGitAdd *dummy)
{
}

static void
git_add_finalize (GObject *object)
{
	GiggleGitAddPriv *priv;

	priv = GET_PRIV (object);

	g_list_foreach (priv->files, (GFunc) g_free, NULL);
	g_list_free (priv->files);

	G_OBJECT_CLASS (giggle_git_add_parent_class)->finalize (object);
}

static void
git_add_get_property (GObject    *object,
		      guint       param_id,
		      GValue     *value,
		      GParamSpec *pspec)
{
	GiggleGitAddPriv *priv;

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
git_add_set_property (GObject      *object,
		      guint         param_id,
		      const GValue *value,
		      GParamSpec   *pspec)
{
	GiggleGitAddPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_FILES:
		priv->files = g_value_get_pointer (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
git_add_get_command_line (GiggleJob *job, gchar **command_line)
{
	GiggleGitAddPriv *priv;
	GString          *str;
	GList            *files;

	priv = GET_PRIV (job);
	files = priv->files;

	str = g_string_new (GIT_COMMAND " add");

	while (files) {
		g_string_append_printf (str, " %s", (gchar *) files->data);
		files = files->next;
	}

	*command_line = g_string_free (str, FALSE);
	return TRUE;
}

GiggleJob *
giggle_git_add_new (void)
{
	return g_object_new (GIGGLE_TYPE_GIT_ADD, NULL);
}

void
giggle_git_add_set_files (GiggleGitAdd *add,
			  GList        *files)
{
	g_return_if_fail (GIGGLE_IS_GIT_ADD (add));

	g_object_set (add,
		      "files", files,
		      NULL);
}
