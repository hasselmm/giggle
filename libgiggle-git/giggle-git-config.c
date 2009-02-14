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
#include "giggle-git-config.h"

#include "giggle-git.h"
#include "giggle-git-config-read.h"
#include "giggle-git-config-write.h"

#include <gdk/gdk.h>
#include <stdio.h>
#include <string.h>

/* Keep this list in sync with the
 * GiggleGitConfigField enum
 */
static const struct {
	const char *const name;
	const gboolean    global;
} fields[] = {
	[GIGGLE_GIT_CONFIG_FIELD_NAME] =
	{ "user.name", TRUE },
	[GIGGLE_GIT_CONFIG_FIELD_EMAIL] =
	{ "user.email", TRUE },

	[GIGGLE_GIT_CONFIG_FIELD_MAIN_WINDOW_MAXIMIZED] =
	{ "giggle.main-window-maximized", TRUE },
	[GIGGLE_GIT_CONFIG_FIELD_MAIN_WINDOW_GEOMETRY] =
	{ "giggle.main-window-geometry", TRUE },
	[GIGGLE_GIT_CONFIG_FIELD_MAIN_WINDOW_VIEW] =
	{ "giggle.main-window-view", TRUE },

	[GIGGLE_GIT_CONFIG_FIELD_SHOW_GRAPH] =
	{ "giggle.show-graph", TRUE },

	[GIGGLE_GIT_CONFIG_FIELD_FILE_VIEW_PATH] =
	{ "giggle.file-view-path", FALSE },
	[GIGGLE_GIT_CONFIG_FIELD_FILE_VIEW_HPANE_POSITION] =
	{ "giggle.file-view-hpane-position", TRUE },
	[GIGGLE_GIT_CONFIG_FIELD_FILE_VIEW_VPANE_POSITION] =
	{ "giggle.file-view-vpane-position", TRUE },

	[GIGGLE_GIT_CONFIG_FIELD_HISTORY_VIEW_VPANE_POSITION] =
	{ "giggle.history-view-vpane-position", TRUE },
};


typedef struct GiggleGitConfigPriv    GiggleGitConfigPriv;
typedef struct GiggleGitConfigTask    GiggleGitConfigTask;
typedef struct GiggleGitConfigBinding GiggleGitConfigBinding;

struct GiggleGitConfigPriv {
	GiggleGit    *git;
	GiggleJob    *current_job;
	GHashTable   *config;
	GList        *changed_keys;
	GList        *bindings;
	guint         commit_timeout_id;
};

struct GiggleGitConfigTask {
	GiggleGitConfigFunc  func;
	gpointer             data;
	GiggleGitConfig     *config;

	/* useful only for commit tasks */
	GList               *changed_keys;
	gboolean             success;
};

struct GiggleGitConfigBinding {
	GiggleGitConfig      *config;
	GiggleGitConfigField  field;
	GParamSpec           *pspec;
	GObject              *object;
	gulong                notify_id;

	void (* update) (GiggleGitConfigBinding *binding);
	void (* commit) (GiggleGitConfigBinding *binding,
			 const GValue           *value);
};

static void     git_config_read_cb     (GiggleGit *git,
					GiggleJob *job,
					GError    *error,
					gpointer   user_data);
static void     git_config_write_cb    (GiggleGit *git,
					GiggleJob *job,
					GError    *error,
					gpointer   user_data);

enum {
	CHANGED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GiggleGitConfig, giggle_git_config, G_TYPE_OBJECT);

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_CONFIG, GiggleGitConfigPriv))


static gboolean
giggle_git_config_real_get_int_field (GiggleGitConfig      *config,
				      GiggleGitConfigField  field,
				      int                  *value)
{
	const gchar *str;

	g_return_val_if_fail (GIGGLE_IS_GIT_CONFIG (config), FALSE);

	str = giggle_git_config_get_field (config, field);

	if (!str || 1 != sscanf (str, "%d", value))
		return FALSE;

	return TRUE;
}

static void
giggle_git_config_int_binding_update (GiggleGitConfigBinding *binding)
{
	int value;

	if (giggle_git_config_real_get_int_field (binding->config,
						     binding->field, &value))
		g_object_set (binding->object, binding->pspec->name, value, NULL);
}

static void
giggle_git_config_int_binding_commit (GiggleGitConfigBinding *binding,
				      const GValue           *value)
{
	giggle_git_config_set_int_field (binding->config,
					    binding->field,
					    g_value_get_int (value));
}

static void
giggle_git_config_string_binding_update (GiggleGitConfigBinding *binding)
{
	const char *value;

	value = giggle_git_config_get_field (binding->config,
					        binding->field);

	if (value)
		g_object_set (binding->object, binding->pspec->name, value, NULL);
}

static void
giggle_git_config_string_binding_commit (GiggleGitConfigBinding *binding,
					 const GValue           *value)
{
	giggle_git_config_set_field (binding->config,
					binding->field,
					g_value_get_string (value));
}

static gboolean
giggle_git_config_real_get_boolean_field (GiggleGitConfig      *config,
					  GiggleGitConfigField  field,
					  gboolean             *value)
{
	const gchar *str;

	g_return_val_if_fail (GIGGLE_IS_GIT_CONFIG (config), FALSE);

	if (!(str = giggle_git_config_get_field (config, field)))
		return FALSE;

	*value = !strcmp (str, "true");

	return TRUE;
}

static void
giggle_git_config_boolean_binding_update (GiggleGitConfigBinding *binding)
{
	gboolean value;

	if (giggle_git_config_real_get_boolean_field (binding->config,
							 binding->field, &value))
		g_object_set (binding->object, binding->pspec->name, value, NULL);
}

static void
giggle_git_config_boolean_binding_commit (GiggleGitConfigBinding *binding,
					  const GValue           *value)
{
	giggle_git_config_set_boolean_field (binding->config,
						binding->field,
						g_value_get_boolean (value));
}

static void
giggle_git_config_binding_free (GiggleGitConfigBinding *binding)
{
	if (binding->config) {
		g_object_remove_weak_pointer (G_OBJECT (binding->config),
					      (gpointer) &binding->config);
	}

	if (binding->object) {
		if (binding->notify_id) {
			g_signal_handler_disconnect (binding->object,
						     binding->notify_id);
		}

		g_object_remove_weak_pointer (G_OBJECT (binding->object),
					      (gpointer) &binding->object);
	}

	g_slice_free (GiggleGitConfigBinding, binding);
}

static GiggleGitConfigBinding *
giggle_git_config_binding_new (GiggleGitConfig      *config,
			       GiggleGitConfigField  field,
			       GObject              *object,
			       GParamSpec           *pspec)
{
	GiggleGitConfigBinding *binding;

	binding = g_slice_new0 (GiggleGitConfigBinding);
	binding->config = config;
	binding->field = field;
	binding->object = object;
	binding->pspec = pspec;

	g_object_add_weak_pointer (G_OBJECT (binding->config),
				   (gpointer) &binding->config);
	g_object_add_weak_pointer (G_OBJECT (binding->object),
				   (gpointer) &binding->object);

	if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), G_TYPE_INT)) {
		binding->update = giggle_git_config_int_binding_update;
		binding->commit = giggle_git_config_int_binding_commit;
	} else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), G_TYPE_STRING)) {
		binding->update = giggle_git_config_string_binding_update;
		binding->commit = giggle_git_config_string_binding_commit;
	} else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), G_TYPE_BOOLEAN)) {
		binding->update = giggle_git_config_boolean_binding_update;
		binding->commit = giggle_git_config_boolean_binding_commit;
	} else {
		g_critical ("%s: unsupported property type `%s' for \"%s\" of `%s'",
			    G_STRFUNC, G_PARAM_SPEC_TYPE_NAME (pspec),
			    pspec->name, G_OBJECT_TYPE_NAME (object));

		giggle_git_config_binding_free (binding);
		binding = NULL;
	}

	return binding;
}

static void
binding_notify_callback (GObject    *object,
			 GParamSpec *pspec,
			 gpointer    user_data)
{
	GiggleGitConfigBinding *binding = user_data;
	GValue                  value = { 0, };

	if (binding->config) {
		g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
		g_object_get_property (object, pspec->name, &value);
		binding->commit (binding, &value);
		g_value_unset (&value);
	}
}

static void
giggle_git_config_binding_update (GiggleGitConfigBinding *binding)
{
	char *signal_name;

	if (!binding->object ||
	    !binding->config ||
	    !GET_PRIV (binding->config)->config)
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
GIT_CONFIG_constructor (GType                  type,
			guint                  n_construct_params,
			GObjectConstructParam *construct_params)
{
	static GObject *instance = NULL;

	if (G_LIKELY (NULL != instance))
		return g_object_ref (instance);


	instance =
		G_OBJECT_CLASS (giggle_git_config_parent_class)->
		constructor (type, n_construct_params, construct_params);

	return instance;
}

static void
GIT_CONFIG_finalize (GObject *object)
{
	GiggleGitConfigPriv *priv;

	priv = GET_PRIV (object);

	while (priv->bindings) {
		giggle_git_config_binding_free (priv->bindings->data);
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

	G_OBJECT_CLASS (giggle_git_config_parent_class)->finalize (object);
}

static void
giggle_git_config_class_init (GiggleGitConfigClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->constructor = GIT_CONFIG_constructor;
	object_class->finalize = GIT_CONFIG_finalize;

	signals[CHANGED] =
		g_signal_new ("changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GiggleGitConfigClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	g_type_class_add_private (object_class, sizeof (GiggleGitConfigPriv));
}

static void
giggle_git_config_init (GiggleGitConfig *config)
{
	GiggleGitConfigPriv *priv;

	priv = GET_PRIV (config);

	priv->git = giggle_git_get ();
}

static void
git_config_read_cb (GiggleGit *git,
		    GiggleJob *job,
		    GError    *error,
		    gpointer   user_data)
{
	GiggleGitConfigTask *task;
	GiggleGitConfigPriv *priv;
	GHashTable          *config;
	gboolean             success;
	GList               *l;

	task = (GiggleGitConfigTask *) user_data;
	priv = GET_PRIV (task->config);

	config = giggle_git_config_read_get_config (GIGGLE_GIT_CONFIG_READ (job));
	priv->config = g_hash_table_ref (config);
	success = (error == NULL);

	for (l = priv->bindings; l; l = l->next)
		giggle_git_config_binding_update (l->data);

	if (task->func)
		task->func (task->config, success, task->data);
}

static gboolean
GIT_CONFIG_is_global (const char *key)
{
	int i;

	for (i = 0; i < G_N_ELEMENTS (fields); ++i) {
		if (!strcmp (key, fields[i].name))
			return fields[i].global;
	}

	return TRUE;
}

static void
GIT_CONFIG_write (GiggleGitConfigTask *task)
{
	GiggleGitConfigPriv *priv;
	gchar               *key, *value;
	GList               *elem;

	priv = GET_PRIV (task->config);

	if (task->changed_keys) {
		elem = task->changed_keys;
		task->changed_keys = g_list_remove_link (task->changed_keys, elem);
		key = (gchar *) elem->data;
		g_list_free_1 (elem);

		value = g_hash_table_lookup (priv->config, key);

		priv->current_job = giggle_git_config_write_new (key, value);

		/* FIXME: valid for the parameters that we manage currently,
		 * but should be figured out per parameter.
		 */
		g_object_set (priv->current_job,
			      "global", GIT_CONFIG_is_global (key), NULL);

		giggle_git_run_job_full (priv->git, priv->current_job,
					 git_config_write_cb, task, NULL);
		g_free (key);
	} else {
		if (task->func) {
			(task->func) (task->config, task->success, task->data);
		}

		g_signal_emit (task->config, signals[CHANGED], 0);

		/* we're done with the task, free it */
		g_list_foreach (priv->changed_keys, (GFunc) g_free, NULL);
		g_list_free (priv->changed_keys);
		g_free (task);
	}
}

static void
git_config_write_cb (GiggleGit *git,
		     GiggleJob *job,
		     GError    *error,
		     gpointer   user_data)
{
	GiggleGitConfigTask *task = user_data;
	GiggleGitConfigPriv *priv = GET_PRIV (task->config);
	gboolean             success = !error;

	if (!success)
		task->success = FALSE;

	g_object_unref (priv->current_job);
	priv->current_job = NULL;

	GIT_CONFIG_write (task);
}

static gboolean
GIT_CONFIG_commit_timeout_cb (gpointer data)
{
	GiggleGitConfig     *config = data;
	GiggleGitConfigPriv *priv = GET_PRIV (config);

	if (priv->current_job)
		return TRUE;

	priv->commit_timeout_id = 0;
	giggle_git_config_commit (config, NULL, NULL);

	return FALSE;
}

GiggleGitConfig *
giggle_git_config_new (void)
{
	return g_object_new (GIGGLE_TYPE_GIT_CONFIG, NULL);
}

void
giggle_git_config_update (GiggleGitConfig     *config,
			  GiggleGitConfigFunc  func,
			  gpointer             data)
{
	GiggleGitConfigPriv *priv;
	GiggleGitConfigTask *task;

	g_return_if_fail (GIGGLE_IS_GIT_CONFIG (config));

	priv = GET_PRIV (config);

	if (priv->current_job) {
		giggle_git_cancel_job (priv->git, priv->current_job);
		g_object_unref (priv->current_job);
		priv->current_job = NULL;
	}

	if (priv->config) {
		g_hash_table_unref (priv->config);
	}

	task = g_new0 (GiggleGitConfigTask, 1);
	task->func = func;
	task->data = data;
	task->config = config;

	priv->current_job = giggle_git_config_read_new ();

	giggle_git_run_job_full (priv->git, priv->current_job,
				 git_config_read_cb, task, g_free);
}

G_CONST_RETURN gchar *
giggle_git_config_get_field (GiggleGitConfig      *config,
			     GiggleGitConfigField  field)
{
	GiggleGitConfigPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_CONFIG (config), NULL);
	g_return_val_if_fail (field < G_N_ELEMENTS (fields), NULL);

	priv = GET_PRIV (config);

	return g_hash_table_lookup (priv->config, fields[field].name);
}

int
giggle_git_config_get_int_field (GiggleGitConfig      *config,
				 GiggleGitConfigField  field)
{
	int value = 0;

	giggle_git_config_real_get_int_field (config, field, &value);

	return value;
}

gboolean
giggle_git_config_get_boolean_field (GiggleGitConfig      *config,
				     GiggleGitConfigField  field)
{
	gboolean value = 0;

	giggle_git_config_real_get_boolean_field (config, field, &value);

	return value;
}

void
giggle_git_config_set_field (GiggleGitConfig      *config,
			     GiggleGitConfigField  field,
			     const gchar          *value)
{
	GiggleGitConfigPriv *priv;

	g_return_if_fail (GIGGLE_IS_GIT_CONFIG (config));
	g_return_if_fail (field < G_N_ELEMENTS (fields));

	priv = GET_PRIV (config);

	if (!priv->config) {
		g_warning ("trying to change config before it could be retrieved");
		return;
	}

	g_hash_table_insert (priv->config,
			     g_strdup (fields[field].name),
			     g_strdup (value));

	/* insert the key in the changed keys list */
	priv->changed_keys = g_list_prepend (priv->changed_keys, g_strdup (fields[field].name));

	if (!priv->commit_timeout_id) {
		priv->commit_timeout_id = gdk_threads_add_timeout
			(200, GIT_CONFIG_commit_timeout_cb,
			 config);
	}
}

void
giggle_git_config_set_int_field (GiggleGitConfig      *config,
				 GiggleGitConfigField  field,
				 int                   value)
{
	char *str;

	g_return_if_fail (GIGGLE_IS_GIT_CONFIG (config));

	str = g_strdup_printf ("%d", value);
	giggle_git_config_set_field (config, field, str);
	g_free (str);
}

void
giggle_git_config_set_boolean_field (GiggleGitConfig      *config,
				     GiggleGitConfigField  field,
				     gboolean              value)
{
	g_return_if_fail (GIGGLE_IS_GIT_CONFIG (config));

	giggle_git_config_set_field (config, field,
					value ? "true" : "false");
}

void
giggle_git_config_commit (GiggleGitConfig     *config,
			  GiggleGitConfigFunc  func,
			  gpointer             data)
{
	GiggleGitConfigPriv *priv;
	GiggleGitConfigTask *task;

	g_return_if_fail (GIGGLE_IS_GIT_CONFIG (config));

	priv = GET_PRIV (config);

	if (priv->current_job) {
		giggle_git_cancel_job (priv->git, priv->current_job);
		g_object_unref (priv->current_job);
		priv->current_job = NULL;
	}

	task = g_new0 (GiggleGitConfigTask, 1);
	task->func = func;
	task->data = data;
	task->changed_keys = priv->changed_keys;
	task->config = config;
	task->success = TRUE;

	/* the list has been passed to the task, set it to NULL
	 * so other commit operations may run without interferences
	 */
	priv->changed_keys = NULL;

	if (priv->commit_timeout_id) {
		g_source_remove (priv->commit_timeout_id);
		priv->commit_timeout_id = 0;
	}

	GIT_CONFIG_write (task);
}

void
giggle_git_config_bind (GiggleGitConfig      *config,
			GiggleGitConfigField  field,
			GObject              *object,
			const char           *property)
{
	GiggleGitConfigPriv    *priv;
	GiggleGitConfigBinding *binding;
	GParamSpec             *pspec;

	g_return_if_fail (GIGGLE_IS_GIT_CONFIG (config));
	g_return_if_fail (field < G_N_ELEMENTS (fields));

	g_return_if_fail (G_IS_OBJECT (object));
	g_return_if_fail (NULL != property);

	priv = GET_PRIV (config);
	pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), property);

	if (NULL == pspec) {
		g_critical ("%s: invalid property name \"%s\" for `%s'",
			    G_STRFUNC, property, G_OBJECT_TYPE_NAME (object));

		return;
	}

	binding = giggle_git_config_binding_new (config, field,
						    object, pspec);

	if (binding) {
		priv->bindings = g_list_prepend (priv->bindings, binding);
		giggle_git_config_binding_update (binding);
	}
}

