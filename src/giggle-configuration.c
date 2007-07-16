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
#include "giggle-configuration.h"
#include "giggle-git.h"
#include "giggle-git-read-config.h"
#include "giggle-git-write-config.h"

/* Keep this list in sync with the
 * GiggleConfigurationField enum
 */
static const gchar *fields[] = {
	"user.name",
	"user.email",
	"giggle.main-window-maximized",
	"giggle.main-window-geometry",
	"giggle.compact-mode",
	NULL
};


typedef struct GiggleConfigurationPriv GiggleConfigurationPriv;
typedef struct GiggleConfigurationTask GiggleConfigurationTask;

struct GiggleConfigurationPriv {
	GiggleGit    *git;
	GiggleJob    *current_job;
	GHashTable   *config;
	GList        *changed_keys;
};

struct GiggleConfigurationTask {
	GiggleConfigurationFunc  func;
	gpointer                 data;
	GiggleConfiguration     *configuration;

	/* useful only for commit tasks */
	GList                   *changed_keys;
	gboolean                 success;
};

static void     configuration_finalize         (GObject           *object);
static void     configuration_read_callback    (GiggleGit         *git,
						GiggleJob         *job,
						GError            *error,
						gpointer           user_data);
static void     configuration_write_callback   (GiggleGit         *git,
						GiggleJob         *job,
						GError            *error,
						gpointer           user_data);

enum {
	CHANGED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GiggleConfiguration, giggle_configuration, G_TYPE_OBJECT);

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_CONFIGURATION, GiggleConfigurationPriv))


static void
giggle_configuration_class_init (GiggleConfigurationClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = configuration_finalize;

	signals[CHANGED] =
		g_signal_new ("changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GiggleConfigurationClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	g_type_class_add_private (object_class, sizeof (GiggleConfigurationPriv));
}

static void
giggle_configuration_init (GiggleConfiguration *config)
{
	GiggleConfigurationPriv *priv;

	priv = GET_PRIV (config);

	priv->git = giggle_git_get ();
}

static void
configuration_finalize (GObject *object)
{
	GiggleConfigurationPriv *priv;

	priv = GET_PRIV (object);

	if (priv->current_job) {
		giggle_git_cancel_job (priv->git, priv->current_job);
		g_object_unref (priv->current_job);
		priv->current_job = NULL;
	}

	if (priv->config) {
		g_hash_table_unref (priv->config);
	}

	g_object_unref (priv->git);

	G_OBJECT_CLASS (giggle_configuration_parent_class)->finalize (object);
}

static void
configuration_read_callback (GiggleGit *git,
			     GiggleJob *job,
			     GError    *error,
			     gpointer   user_data)
{
	GiggleConfigurationTask *task;
	GiggleConfigurationPriv *priv;
	GHashTable              *config;
	gboolean                 success;

	task = (GiggleConfigurationTask *) user_data;
	priv = GET_PRIV (task->configuration);

	config = giggle_git_read_config_get_config (GIGGLE_GIT_READ_CONFIG (job));
	priv->config = g_hash_table_ref (config);
	success = (error == NULL);

	(task->func) (task->configuration, success, task->data);
}

static void
configuration_write (GiggleConfigurationTask *task)
{
	GiggleConfigurationPriv *priv;
	gchar                   *key, *value;
	GList                   *elem;

	priv = GET_PRIV (task->configuration);

	if (task->changed_keys) {
		elem = task->changed_keys;
		task->changed_keys = g_list_remove_link (task->changed_keys, elem);
		key = (gchar *) elem->data;
		g_list_free_1 (elem);

		value = g_hash_table_lookup (priv->config, key);

		priv->current_job = giggle_git_write_config_new (key, value);

		/* FIXME: valid for the parameters that we manage currently,
		 * but should be figured out per parameter.
		 */
		g_object_set (priv->current_job, "global", TRUE, NULL);

		giggle_git_run_job_full (priv->git,
					 priv->current_job,
					 configuration_write_callback,
					 task, NULL);
		g_free (key);
	} else {
		if (task->func) {
			(task->func) (task->configuration, task->success, task->data);
		}

		g_signal_emit (task->configuration, signals[CHANGED], 0);

		/* we're done with the task, free it */
		g_list_foreach (priv->changed_keys, (GFunc) g_free, NULL);
		g_list_free (priv->changed_keys);
		g_free (task);
	}
}

static void
configuration_write_callback (GiggleGit *git,
			      GiggleJob *job,
			      GError    *error,
			      gpointer   user_data)
{
	GiggleConfigurationTask *task;
	GiggleConfigurationPriv *priv;
	gboolean                 success;

	task = (GiggleConfigurationTask *) user_data;
	success = (error == NULL);
	priv = GET_PRIV (task->configuration);

	if (!success) {
		task->success = FALSE;
	}

	g_object_unref (priv->current_job);
	priv->current_job = NULL;

	configuration_write (task);
}

GiggleConfiguration *
giggle_configuration_new (void)
{
	return g_object_new (GIGGLE_TYPE_CONFIGURATION, NULL);
}

void
giggle_configuration_update (GiggleConfiguration     *configuration,
			     GiggleConfigurationFunc  func,
			     gpointer                 data)
{
	GiggleConfigurationPriv *priv;
	GiggleConfigurationTask *task;

	g_return_if_fail (GIGGLE_IS_CONFIGURATION (configuration));

	priv = GET_PRIV (configuration);

	if (priv->current_job) {
		giggle_git_cancel_job (priv->git, priv->current_job);
		g_object_unref (priv->current_job);
		priv->current_job = NULL;
	}

	if (priv->config) {
		g_hash_table_unref (priv->config);
	}

	task = g_new0 (GiggleConfigurationTask, 1);
	task->func = func;
	task->data = data;
	task->configuration = configuration;

	priv->current_job = giggle_git_read_config_new ();

	giggle_git_run_job_full (priv->git,
				 priv->current_job,
				 configuration_read_callback,
				 task,
				 (GDestroyNotify) g_free);
}

G_CONST_RETURN gchar *
giggle_configuration_get_field (GiggleConfiguration      *configuration,
				GiggleConfigurationField  field)
{
	GiggleConfigurationPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_CONFIGURATION (configuration), NULL);

	priv = GET_PRIV (configuration);

	return g_hash_table_lookup (priv->config, fields[field]);
}

gboolean
giggle_configuration_get_boolean_field (GiggleConfiguration      *configuration,
					GiggleConfigurationField  field)
{
	const gchar *str;

	g_return_val_if_fail (GIGGLE_IS_CONFIGURATION (configuration), FALSE);

	str = giggle_configuration_get_field (configuration, field);

	if (str) {
		return (strcmp (str, "true") == 0);
	}

	return FALSE;
}

void
giggle_configuration_set_field (GiggleConfiguration      *configuration,
				GiggleConfigurationField  field,
				const gchar              *value)
{
	GiggleConfigurationPriv *priv;

	g_return_if_fail (GIGGLE_IS_CONFIGURATION (configuration));

	priv = GET_PRIV (configuration);

	if (!priv->config) {
		g_warning ("trying to change configuration before it could be retrieved");
		return;
	}

	g_hash_table_insert (priv->config,
			     g_strdup (fields[field]),
			     g_strdup (value));

	/* insert the key in the changed keys list */
	priv->changed_keys = g_list_prepend (priv->changed_keys, g_strdup (fields[field]));
}

void
giggle_configuration_set_boolean_field (GiggleConfiguration      *configuration,
					GiggleConfigurationField  field,
					gboolean                  value)
{
	g_return_if_fail (GIGGLE_IS_CONFIGURATION (configuration));

	giggle_configuration_set_field (configuration, field,
					(value) ? "true" : "false");
}

void
giggle_configuration_commit (GiggleConfiguration     *configuration,
			     GiggleConfigurationFunc  func,
			     gpointer                 data)
{
	GiggleConfigurationPriv *priv;
	GiggleConfigurationTask *task;

	g_return_if_fail (GIGGLE_IS_CONFIGURATION (configuration));

	priv = GET_PRIV (configuration);

	if (priv->current_job) {
		giggle_git_cancel_job (priv->git, priv->current_job);
		g_object_unref (priv->current_job);
		priv->current_job = NULL;
	}

	task = g_new0 (GiggleConfigurationTask, 1);
	task->func = func;
	task->data = data;
	task->changed_keys = priv->changed_keys;
	task->configuration = configuration;
	task->success = TRUE;

	/* the list has been passed to the task, set it to NULL
	 * so other commit operations may run without interferences
	 */
	priv->changed_keys = NULL;

	configuration_write (task);
}
