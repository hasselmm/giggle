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
#include "giggle-git-config-write.h"

#include <string.h>

typedef struct GiggleGitConfigWritePriv GiggleGitConfigWritePriv;

struct GiggleGitConfigWritePriv {
	gboolean  global;
	gchar    *field;
	gchar    *value;
};

static void     git_config_write_finalize         (GObject           *object);
static void     git_config_write_get_property     (GObject           *object,
						   guint              param_id,
						   GValue            *value,
						   GParamSpec        *pspec);
static void     git_config_write_set_property     (GObject           *object,
						   guint              param_id,
						   const GValue      *value,
						   GParamSpec        *pspec);

static gboolean git_config_write_get_command_line (GiggleJob         *job,
						   gchar            **command_line);
static void     git_config_write_handle_output    (GiggleJob         *job,
						   const gchar       *output_str,
						   gsize              output_len);


G_DEFINE_TYPE (GiggleGitConfigWrite, giggle_git_config_write, GIGGLE_TYPE_JOB)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_CONFIG_WRITE, GiggleGitConfigWritePriv))

enum {
	PROP_0,
	PROP_GLOBAL,
	PROP_FIELD,
	PROP_VALUE,
};


static void
giggle_git_config_write_class_init (GiggleGitConfigWriteClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GiggleJobClass *job_class    = GIGGLE_JOB_CLASS (class);

	object_class->finalize     = git_config_write_finalize;
	object_class->get_property = git_config_write_get_property;
	object_class->set_property = git_config_write_set_property;

	job_class->get_command_line = git_config_write_get_command_line;
	job_class->handle_output    = git_config_write_handle_output;

	g_object_class_install_property (object_class,
					 PROP_GLOBAL,
					 g_param_spec_boolean ("global",
							       "global",
							       "Whether the setting is global",
							       FALSE,
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_FIELD,
					 g_param_spec_string ("field",
							      "field",
							      "configuration field to modify",
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_VALUE,
					 g_param_spec_string ("value",
							      "value",
							      "value to assign to the field",
							      NULL,
							      G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleGitConfigWritePriv));
}

static void
giggle_git_config_write_init (GiggleGitConfigWrite *config)
{
}

static void
git_config_write_finalize (GObject *object)
{
	GiggleGitConfigWritePriv *priv;

	priv = GET_PRIV (object);

	g_free (priv->field);
	g_free (priv->value);

	G_OBJECT_CLASS (giggle_git_config_write_parent_class)->finalize (object);
}

static void
git_config_write_get_property (GObject    *object,
			       guint       param_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
	GiggleGitConfigWritePriv *priv;

	priv = GET_PRIV (object);
	
	switch (param_id) {
	case PROP_GLOBAL:
		g_value_set_boolean (value, priv->global);
		break;
	case PROP_FIELD:
		g_value_set_string (value, priv->field);
		break;
	case PROP_VALUE:
		g_value_set_string (value, priv->value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
git_config_write_set_property (GObject      *object,
			       guint         param_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
	GiggleGitConfigWritePriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_GLOBAL:
		priv->global = g_value_get_boolean (value);
		break;
	case PROP_FIELD:
		if (priv->field) {
			g_free (priv->field);
		}
		priv->field = g_value_dup_string (value);
		break;
	case PROP_VALUE:
		if (priv->value) {
			g_free (priv->value);
		}
		priv->value = g_value_dup_string (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
git_config_write_get_command_line (GiggleJob  *job,
				   gchar     **command_line)
{
	GiggleGitConfigWritePriv *priv;
	char                     *value;

	priv = GET_PRIV (job);

	if (priv->value) {
		value = g_shell_quote (priv->value);

		*command_line = g_strdup_printf (GIT_COMMAND " repo-config %s %s %s",
					         priv->global ? "--global" : "",
						 priv->field, value);

		g_free (value);
	} else {
		*command_line = g_strdup_printf (GIT_COMMAND " repo-config %s --unset %s",
					         priv->global ? "--global" : "", priv->field);
	}

	return TRUE;
}

static void
git_config_write_handle_output (GiggleJob   *job,
				const gchar *output_str,
				gsize        output_len)
{
	/* Ignore output */
}

GiggleJob *
giggle_git_config_write_new (const gchar *field,
			     const gchar *value)
{
	g_return_val_if_fail (field != NULL, NULL);

	return g_object_new (GIGGLE_TYPE_GIT_CONFIG_WRITE,
			     "field", field,
			     "value", value,
			     NULL);
}
