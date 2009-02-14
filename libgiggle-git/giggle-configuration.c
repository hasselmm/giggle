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
#include "giggle-configuration.h"

#include "giggle-git.h"
#include "giggle-git-read-config.h"
#include "giggle-git-write-config.h"

#include <gdk/gdk.h>
#include <stdio.h>
#include <string.h>

/* Keep this list in sync with the
 * GiggleConfigurationField enum
 */
static const struct {
	const char *const name;
	const gboolean    global;
} fields[] = {
	[CONFIG_FIELD_NAME] =
	{ "user.name", TRUE },
	[CONFIG_FIELD_EMAIL] =
	{ "user.email", TRUE },

	[CONFIG_FIELD_MAIN_WINDOW_MAXIMIZED] =
	{ "giggle.main-window-maximized", TRUE },
	[CONFIG_FIELD_MAIN_WINDOW_GEOMETRY] =
	{ "giggle.main-window-geometry", TRUE },
	[CONFIG_FIELD_MAIN_WINDOW_VIEW] =
	{ "giggle.main-window-view", TRUE },

	[CONFIG_FIELD_SHOW_GRAPH] =
	{ "giggle.show-graph", TRUE },

	[CONFIG_FIELD_FILE_VIEW_PATH] =
	{ "giggle.file-view-path", FALSE },
	[CONFIG_FIELD_FILE_VIEW_HPANE_POSITION] =
	{ "giggle.file-view-hpane-position", TRUE },
	[CONFIG_FIELD_FILE_VIEW_VPANE_POSITION] =
	{ "giggle.file-view-vpane-position", TRUE },

	[CONFIG_FIELD_HISTORY_VIEW_VPANE_POSITION] =
	{ "giggle.history-view-vpane-position", TRUE },
};


typedef struct GiggleConfigurationPriv    GiggleConfigurationPriv;
typedef struct GiggleConfigurationTask    GiggleConfigurationTask;
typedef struct GiggleConfigurationBinding GiggleConfigurationBinding;

struct GiggleConfigurationPriv {
	GiggleGit    *git;
	GiggleJob    *current_job;
	GHashTable   *config;
	GList        *changed_keys;
	GList        *bindings;
	guint         commit_timeout_id;
};

struct GiggleConfigurationTask {
	GiggleConfigurationFunc  func;
	gpointer                 data;
	GiggleConfiguration     *configuration;

	/* useful only for commit tasks */
	GList                   *changed_keys;
	gboolean                 success;
};

struct GiggleConfigurationBinding {
	GiggleConfiguration      *configuration;
	GiggleConfigurationField  field;
	GParamSpec               *pspec;
	GObject                  *object;
	gulong                    notify_id;

	void (* update) (GiggleConfigurationBinding *binding);
	void (* commit) (GiggleConfigurationBinding *binding,
			 const GValue               *value);
};

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


static gboolean
giggle_configuration_real_get_int_field (GiggleConfiguration      *configuration,
					 GiggleConfigurationField  field,
					 int                      *value)
{
	const gchar *str;

	g_return_val_if_fail (GIGGLE_IS_CONFIGURATION (configuration), FALSE);

	str = giggle_configuration_get_field (configuration, field);

	if (!str || 1 != sscanf (str, "%d", value))
		return FALSE;

	return TRUE;
}

static void
giggle_configuration_int_binding_update (GiggleConfigurationBinding *binding)
{
	int value;

	if (giggle_configuration_real_get_int_field (binding->configuration,
						     binding->field, &value))
		g_object_set (binding->object, binding->pspec->name, value, NULL);
}

static void
giggle_configuration_int_binding_commit (GiggleConfigurationBinding *binding,
					 const GValue               *value)
{
	giggle_configuration_set_int_field (binding->configuration,
					    binding->field,
					    g_value_get_int (value));
}

static void
giggle_configuration_string_binding_update (GiggleConfigurationBinding *binding)
{
	const char *value;

	value = giggle_configuration_get_field (binding->configuration,
					        binding->field);

	if (value)
		g_object_set (binding->object, binding->pspec->name, value, NULL);
}

static void
giggle_configuration_string_binding_commit (GiggleConfigurationBinding *binding,
					    const GValue               *value)
{
	giggle_configuration_set_field (binding->configuration,
					binding->field,
					g_value_get_string (value));
}

static gboolean
giggle_configuration_real_get_boolean_field (GiggleConfiguration      *configuration,
					     GiggleConfigurationField  field,
					     gboolean                 *value)
{
	const gchar *str;

	g_return_val_if_fail (GIGGLE_IS_CONFIGURATION (configuration), FALSE);

	if (!(str = giggle_configuration_get_field (configuration, field)))
		return FALSE;

	*value = !strcmp (str, "true");

	return TRUE;
}

static void
giggle_configuration_boolean_binding_update (GiggleConfigurationBinding *binding)
{
	gboolean value;

	if (giggle_configuration_real_get_boolean_field (binding->configuration,
							 binding->field, &value))
		g_object_set (binding->object, binding->pspec->name, value, NULL);
}

static void
giggle_configuration_boolean_binding_commit (GiggleConfigurationBinding *binding,
					     const GValue               *value)
{
	giggle_configuration_set_boolean_field (binding->configuration,
						binding->field,
						g_value_get_boolean (value));
}

static void
giggle_configuration_binding_free (GiggleConfigurationBinding *binding)
{
	if (binding->configuration) {
		g_object_remove_weak_pointer (G_OBJECT (binding->configuration),
					      (gpointer) &binding->configuration);
	}

	if (binding->object) {
		if (binding->notify_id) {
			g_signal_handler_disconnect (binding->object,
						     binding->notify_id);
		}

		g_object_remove_weak_pointer (G_OBJECT (binding->object),
					      (gpointer) &binding->object);
	}

	g_slice_free (GiggleConfigurationBinding, binding);
}

static GiggleConfigurationBinding *
giggle_configuration_binding_new (GiggleConfiguration      *configuration,
				  GiggleConfigurationField  field,
				  GObject                  *object,
				  GParamSpec               *pspec)
{
	GiggleConfigurationBinding *binding;

	binding = g_slice_new0 (GiggleConfigurationBinding);
	binding->configuration = configuration;
	binding->field = field;
	binding->object = object;
	binding->pspec = pspec;

	g_object_add_weak_pointer (G_OBJECT (binding->configuration),
				   (gpointer) &binding->configuration);
	g_object_add_weak_pointer (G_OBJECT (binding->object),
				   (gpointer) &binding->object);

	if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), G_TYPE_INT)) {
		binding->update = giggle_configuration_int_binding_update;
		binding->commit = giggle_configuration_int_binding_commit;
	} else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), G_TYPE_STRING)) {
		binding->update = giggle_configuration_string_binding_update;
		binding->commit = giggle_configuration_string_binding_commit;
	} else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), G_TYPE_BOOLEAN)) {
		binding->update = giggle_configuration_boolean_binding_update;
		binding->commit = giggle_configuration_boolean_binding_commit;
	} else {
		g_critical ("%s: unsupported property type `%s' for \"%s\" of `%s'",
			    G_STRFUNC, G_PARAM_SPEC_TYPE_NAME (pspec),
			    pspec->name, G_OBJECT_TYPE_NAME (object));

		giggle_configuration_binding_free (binding);
		binding = NULL;
	}

	return binding;
}

static void
binding_notify_callback (GObject    *object,
			 GParamSpec *pspec,
			 gpointer    user_data)
{
	GiggleConfigurationBinding *binding = user_data;
	GValue                      value = { 0, };

	if (binding->configuration) {
		g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
		g_object_get_property (object, pspec->name, &value);
		binding->commit (binding, &value);
		g_value_unset (&value);
	}
}

static void
giggle_configuration_binding_update (GiggleConfigurationBinding *binding)
{
	char *signal_name;

	if (!binding->object ||
	    !binding->configuration ||
	    !GET_PRIV (binding->configuration)->config)
	       return;

	binding->update (binding);

	if (G_UNLIKELY (!binding->notify_id)) {
		signal_name = g_strconcat ("notify::", binding->pspec->name, NULL);

		g_signal_connect (binding->object, signal_name,
				  G_CALLBACK (binding_notify_callback), binding);

		g_free (signal_name);
	}
}

static GObject*
configuration_constructor (GType                  type,
			   guint                  n_construct_params,
			   GObjectConstructParam *construct_params)
{
	static GObject *instance = NULL;

	if (G_LIKELY (NULL != instance))
		return g_object_ref (instance);


	instance =
		G_OBJECT_CLASS (giggle_configuration_parent_class)->
		constructor (type, n_construct_params, construct_params);

	return instance;
}

static void
configuration_finalize (GObject *object)
{
	GiggleConfigurationPriv *priv;

	priv = GET_PRIV (object);

	while (priv->bindings) {
		giggle_configuration_binding_free (priv->bindings->data);
		priv->bindings = g_list_delete_link (priv->bindings, priv->bindings);
	}

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
giggle_configuration_class_init (GiggleConfigurationClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->constructor = configuration_constructor;
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
configuration_read_callback (GiggleGit *git,
			     GiggleJob *job,
			     GError    *error,
			     gpointer   user_data)
{
	GiggleConfigurationTask *task;
	GiggleConfigurationPriv *priv;
	GHashTable              *config;
	gboolean                 success;
	GList                   *l;

	task = (GiggleConfigurationTask *) user_data;
	priv = GET_PRIV (task->configuration);

	config = giggle_git_read_config_get_config (GIGGLE_GIT_READ_CONFIG (job));
	priv->config = g_hash_table_ref (config);
	success = (error == NULL);

	for (l = priv->bindings; l; l = l->next)
		giggle_configuration_binding_update (l->data);

	if (task->func)
		task->func (task->configuration, success, task->data);
}

static gboolean
configuration_is_global (const char *key)
{
	int i;

	for (i = 0; i < G_N_ELEMENTS (fields); ++i) {
		if (!strcmp (key, fields[i].name))
			return fields[i].global;
	}

	return TRUE;
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
		g_object_set (priv->current_job,
			      "global", configuration_is_global (key), NULL);

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

static gboolean
configuration_commit_timeout_cb (gpointer data)
{
	GiggleConfiguration     *configuration = data;
	GiggleConfigurationPriv *priv;

	priv = GET_PRIV (configuration);

	if (priv->current_job)
		return TRUE;

	priv->commit_timeout_id = 0;
	giggle_configuration_commit (configuration, NULL, NULL);

	return FALSE;
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
	g_return_val_if_fail (field < G_N_ELEMENTS (fields), NULL);

	priv = GET_PRIV (configuration);

	return g_hash_table_lookup (priv->config, fields[field].name);
}

int
giggle_configuration_get_int_field (GiggleConfiguration      *configuration,
				    GiggleConfigurationField  field)
{
	int value = 0;

	giggle_configuration_real_get_int_field (configuration, field, &value);

	return value;
}

gboolean
giggle_configuration_get_boolean_field (GiggleConfiguration      *configuration,
					GiggleConfigurationField  field)
{
	gboolean value = 0;

	giggle_configuration_real_get_boolean_field (configuration, field, &value);

	return value;
}

void
giggle_configuration_set_field (GiggleConfiguration      *configuration,
				GiggleConfigurationField  field,
				const gchar              *value)
{
	GiggleConfigurationPriv *priv;

	g_return_if_fail (GIGGLE_IS_CONFIGURATION (configuration));
	g_return_if_fail (field < G_N_ELEMENTS (fields));

	priv = GET_PRIV (configuration);

	if (!priv->config) {
		g_warning ("trying to change configuration before it could be retrieved");
		return;
	}

	g_hash_table_insert (priv->config,
			     g_strdup (fields[field].name),
			     g_strdup (value));

	/* insert the key in the changed keys list */
	priv->changed_keys = g_list_prepend (priv->changed_keys, g_strdup (fields[field].name));

	if (!priv->commit_timeout_id) {
		priv->commit_timeout_id = gdk_threads_add_timeout
			(200, configuration_commit_timeout_cb,
			 configuration);
	}
}

void
giggle_configuration_set_int_field (GiggleConfiguration      *configuration,
				    GiggleConfigurationField  field,
				    int                       value)
{
	char *str;

	g_return_if_fail (GIGGLE_IS_CONFIGURATION (configuration));

	str = g_strdup_printf ("%d", value);
	giggle_configuration_set_field (configuration, field, str);
	g_free (str);
}

void
giggle_configuration_set_boolean_field (GiggleConfiguration      *configuration,
					GiggleConfigurationField  field,
					gboolean                  value)
{
	g_return_if_fail (GIGGLE_IS_CONFIGURATION (configuration));

	giggle_configuration_set_field (configuration, field,
					value ? "true" : "false");
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

	if (priv->commit_timeout_id) {
		g_source_remove (priv->commit_timeout_id);
		priv->commit_timeout_id = 0;
	}

	configuration_write (task);
}

void
giggle_configuration_bind (GiggleConfiguration      *configuration,
			   GiggleConfigurationField  field,
			   GObject                  *object,
			   const char               *property)
{
	GiggleConfigurationPriv    *priv;
	GiggleConfigurationBinding *binding;
	GParamSpec                 *pspec;

	g_return_if_fail (GIGGLE_IS_CONFIGURATION (configuration));
	g_return_if_fail (field < G_N_ELEMENTS (fields));

	g_return_if_fail (G_IS_OBJECT (object));
	g_return_if_fail (NULL != property);

	priv = GET_PRIV (configuration);
	pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), property);

	if (NULL == pspec) {
		g_critical ("%s: invalid property name \"%s\" for `%s'",
			    G_STRFUNC, property, G_OBJECT_TYPE_NAME (object));

		return;
	}

	binding = giggle_configuration_binding_new (configuration, field,
						    object, pspec);

	if (binding) {
		priv->bindings = g_list_prepend (priv->bindings, binding);
		giggle_configuration_binding_update (binding);
	}
}

