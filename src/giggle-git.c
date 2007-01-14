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

#include "giggle-dispatcher.h"
#include "giggle-git.h"

#define d(x) x

typedef struct GiggleGitPriv GiggleGitPriv;

struct GiggleGitPriv {
	GiggleDispatcher *dispatcher;
	gchar            *directory;

	GHashTable       *jobs;
};

typedef enum {
	GIT_JOB_TYPE_GET_PATCH
} GitJobType;

typedef struct {
	guint                  id;
	GiggleJob             *job;
	GiggleJobDoneCallback  callback;
	gpointer               user_data;
} GitJobData;

static void     git_finalize            (GObject           *object);
static void     git_get_property        (GObject           *object,
					 guint              param_id,
					 GValue            *value,
					 GParamSpec        *pspec);
static void     git_set_property        (GObject           *object,
					 guint              param_id,
					 const GValue      *value,
					 GParamSpec        *pspec);
static gboolean git_verify_directory    (GiggleGit         *git,
					 const gchar       *directory,
					 GError           **error);
static void     git_job_data_free       (GitJobData        *data);
static void     git_execute_callback    (GiggleDispatcher  *dispatcher,
					 guint              id,
					 GError            *error,
					 const gchar       *output_str,
					 gsize              output_len,
					 GiggleGit         *git);

G_DEFINE_TYPE (GiggleGit, giggle_git, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_DIRECTORY
};

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT, GiggleGitPriv))

static void
giggle_git_class_init (GiggleGitClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = git_finalize;
	object_class->get_property = git_get_property;
	object_class->set_property = git_set_property;

	g_object_class_install_property (object_class,
					 PROP_DIRECTORY,
					 g_param_spec_string ("directory",
							      "Directory",
							      "The Git repository path",
							      NULL,
							      G_PARAM_READABLE));

	g_type_class_add_private (object_class, sizeof (GiggleGitPriv));
}

static void
giggle_git_init (GiggleGit *git)
{
	GiggleGitPriv *priv;

	priv = GET_PRIV (git);

	priv->directory = NULL;
	priv->dispatcher = giggle_dispatcher_new ();

	priv->jobs = g_hash_table_new_full (g_direct_hash, g_direct_equal,
					    NULL, 
					    (GDestroyNotify) git_job_data_free);
}

static void
foreach_job_cancel (gpointer key, GitJobData *data, GiggleGit *git)
{
	GiggleGitPriv *priv;

	priv = GET_PRIV (git);

	giggle_dispatcher_cancel (priv->dispatcher, data->id);
}

static void
git_finalize (GObject *object)
{
	GiggleGitPriv *priv;

	priv = GET_PRIV (object);

	g_hash_table_foreach (priv->jobs, 
			      (GHFunc) foreach_job_cancel,
			      object);

	g_hash_table_destroy (priv->jobs);
	g_free (priv->directory);

	g_object_unref (priv->dispatcher);

	G_OBJECT_CLASS (giggle_git_parent_class)->finalize (object);
}

static void
git_get_property (GObject    *object,
		  guint       param_id,
		  GValue     *value,
		  GParamSpec *pspec)
{
	GiggleGitPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_DIRECTORY:
		g_value_set_string (value, priv->directory);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
git_set_property (GObject      *object,
		  guint         param_id,
		  const GValue *value,
		  GParamSpec   *pspec)
{
	GiggleGitPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean 
git_verify_directory (GiggleGit    *git,
		      const gchar  *directory,
		      GError      **error)
{
	/* FIXME: Do some funky stuff to verify that it's a valid GIT repo */
	return TRUE;
}

static void
git_job_data_free (GitJobData *data)
{
	g_object_unref (data->job);

	g_slice_free (GitJobData, data);
}

static void
git_execute_callback (GiggleDispatcher *dispatcher,
		      guint             id,
		      GError           *error,
		      const gchar      *output_str,
		      gsize             output_len,
		      GiggleGit        *git)
{
	GiggleGitPriv *priv;
	GitJobData    *data;

	priv = GET_PRIV (git);

	data = g_hash_table_lookup (priv->jobs, GINT_TO_POINTER (id));
	g_assert (data != NULL);

	if (!error) {
		giggle_job_handle_output (data->job, output_str, output_len);
	}

	data->callback (git, data->job, error, data->user_data);

	g_hash_table_remove (priv->jobs, GINT_TO_POINTER (id));
}

GiggleGit *
giggle_git_new (void)
{
	return g_object_new (GIGGLE_TYPE_GIT, NULL);
}

const gchar *
giggle_git_get_directory (GiggleGit *git)
{
	GiggleGitPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT (git), NULL);

	priv = GET_PRIV (git);

	return priv->directory;
}

gboolean
giggle_git_set_directory (GiggleGit    *git, 
			  const gchar  *directory,
			  GError      **error)
{
	GiggleGitPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT (git), FALSE);
	g_return_val_if_fail (directory != NULL, FALSE);

	priv = GET_PRIV (git);

	if (!git_verify_directory (git, directory, error)) {
		return FALSE;
	}

	g_free (priv->directory);
	priv->directory = g_strdup (directory);

	g_object_notify (G_OBJECT (git), "directory");

	return TRUE;
}

void 
giggle_git_run_job (GiggleGit             *git,
		    GiggleJob             *job,
		    GiggleJobDoneCallback  callback,
		    gpointer               user_data)
{
	GiggleGitPriv *priv;
	gchar         *command;

	g_return_if_fail (GIGGLE_IS_GIT (git));
	g_return_if_fail (GIGGLE_IS_JOB (job));
	g_return_if_fail (callback != NULL);
	
	priv = GET_PRIV (git);

	if (giggle_job_get_command_line (job, &command)) {
		GitJobData    *data;
		
		data = g_slice_new0 (GitJobData);
		data->id = giggle_dispatcher_execute (priv->dispatcher,
						      priv->directory,
						      command,
						      (GiggleExecuteCallback) git_execute_callback,
						      git);

		data->job = g_object_ref (job);
		data->callback = callback;
		data->user_data = user_data;
		
		g_object_set (job, "id", data->id, NULL);

		g_hash_table_insert (priv->jobs, 
				     GINT_TO_POINTER (data->id), data);
	} else {
		g_warning ("Couldn't get command line for job");
	}
	
	g_free (command);
}

void
giggle_git_cancel_job (GiggleGit *git, GiggleJob *job)
{
	GiggleGitPriv *priv;
	guint          id;

	g_return_if_fail (GIGGLE_IS_GIT (git));
	g_return_if_fail (GIGGLE_IS_JOB (job));

	priv = GET_PRIV (git);

	g_object_get (job, "id", &id, NULL);
	
	giggle_dispatcher_cancel (priv->dispatcher, id);

	g_hash_table_remove (priv->jobs, GINT_TO_POINTER (id));
}

