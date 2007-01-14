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

#include <glib/gi18n.h>

#include "giggle-error.h"
#include "giggle-sysdeps.h"
#include "giggle-dispatcher.h"

#define d(x) x

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_DISPATCHER, GiggleDispatcherPriv))

typedef struct GiggleDispatcherPriv GiggleDispatcherPriv;

typedef struct {
	gchar                 *command;
	gchar                 *wd;
	GiggleExecuteCallback  callback;
	guint                  id; GPid                   pid;
	gint                   std_out;
	gint                   std_err;
	gpointer               user_data;
} DispatcherJob;

struct GiggleDispatcherPriv {
	GQueue        *queue;

	DispatcherJob *current_job;
	guint          current_job_wait_id;
};

static void     giggle_dispatcher_finalize (GObject *object);

static void     dispatcher_add_job       (GiggleDispatcher  *dispatcher,
					  DispatcherJob     *job);

static gboolean dispatcher_run_job       (GiggleDispatcher  *dispatcher,
					  DispatcherJob     *job);
static void     dispatcher_run_next_job  (GiggleDispatcher *dispatcher);
static void     dispatcher_cancel_job    (GiggleDispatcher *dispatcher, 
					  DispatcherJob    *job);
static void     dispatcher_cancel_job_id (GiggleDispatcher *dispatcher,
					  guint             id);
static void     dispatcher_free_job      (GiggleDispatcher *dispatcher,
					  DispatcherJob    *job);
static void     dispatcher_job_wait_cb   (GPid              pid,
					  gint              status,
					  GiggleDispatcher *dispatcher);
static void     dispatcher_job_failed    (GiggleDispatcher *dispatcher,
					  DispatcherJob    *job,
					  GError           *error);

G_DEFINE_TYPE (GiggleDispatcher, giggle_dispatcher, G_TYPE_OBJECT);

static void
giggle_dispatcher_class_init (GiggleDispatcherClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = giggle_dispatcher_finalize;

	g_type_class_add_private (object_class, sizeof (GiggleDispatcherPriv));
}

static void
giggle_dispatcher_init (GiggleDispatcher *dispatcher)
{
	GiggleDispatcherPriv *priv;

	priv = GET_PRIV (dispatcher);

	priv->queue = g_queue_new ();
	priv->current_job = NULL;
	priv->current_job_wait_id = 0;
}

static void
giggle_dispatcher_finalize (GObject *object)
{
	GiggleDispatcher     *dispatcher = GIGGLE_DISPATCHER (object);
	GiggleDispatcherPriv *priv = GET_PRIV (object);
	DispatcherJob        *job;

	if (priv->current_job_wait_id) {
		g_source_remove (priv->current_job_wait_id);
	}

	while ((job = g_queue_pop_head (priv->queue))) {
		dispatcher_free_job (dispatcher, job);
	}
	g_queue_free (priv->queue);
	
	G_OBJECT_CLASS (giggle_dispatcher_parent_class)->finalize (object);
}

static void
dispatcher_add_job (GiggleDispatcher *dispatcher, DispatcherJob *job)
{
	GiggleDispatcherPriv *priv;

	priv = GET_PRIV (dispatcher);

	d(g_print ("GiggleDispatcher::add_job\n"));

	if (g_queue_is_empty (priv->queue) && priv->current_job == NULL) {
		dispatcher_run_job (dispatcher, job); 
	} else {
		g_queue_push_tail (priv->queue, job);
	}
}

static gboolean
dispatcher_run_job (GiggleDispatcher *dispatcher, DispatcherJob *job)
{
	GiggleDispatcherPriv  *priv;
	gint                   argc;
	gchar                **argv;
	GError                *error = NULL;

	priv = GET_PRIV (dispatcher);

	g_assert (priv->current_job == NULL);

	priv->current_job = job;

	if (!g_shell_parse_argv (job->command, &argc, &argv, &error)) {
		goto failed;
	}

	if (!g_spawn_async_with_pipes (job->wd, argv, 
				       NULL, /* envp */
				       G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
				       NULL, NULL,
				       &job->pid,
				       NULL, &job->std_out, &job->std_err,
				       &error)) {
		goto failed;
	}

	d(g_print ("GiggleDispatcher::run_job(job-started)\n"));

	priv->current_job_wait_id = g_child_watch_add (job->pid, 
						       (GChildWatchFunc) dispatcher_job_wait_cb,
						       dispatcher);
	return TRUE;

failed:
	dispatcher_job_failed (dispatcher, job, error);
	g_strfreev (argv);
	g_error_free (error);
	priv->current_job = NULL;
	priv->current_job_wait_id = 0;
	return FALSE;
}

static void 
dispatcher_run_next_job (GiggleDispatcher *dispatcher)
{
	GiggleDispatcherPriv *priv;
	DispatcherJob        *job;

	priv = GET_PRIV (dispatcher);
	
	g_assert (priv->current_job == NULL);

	job = (DispatcherJob *) g_queue_pop_head (priv->queue);
	if (job) {
		if (!dispatcher_run_job (dispatcher, job)) {
			dispatcher_run_next_job (dispatcher);
		}
	}
}

static void
dispatcher_cancel_job (GiggleDispatcher *dispatcher, DispatcherJob *job)
{
	GiggleDispatcherPriv *priv;
	GError               *error;
	gboolean              current = FALSE;

	priv = GET_PRIV (dispatcher);

	if (priv->current_job == job) {
		giggle_sysdeps_kill_pid (job->pid);
		current = TRUE;
	}

	error = g_error_new (GIGGLE_ERROR,
			     GIGGLE_ERROR_DISPATCH_CANCELLED,
			     _("Job '%s' cancelled"),
			     job->command);

	job->callback (dispatcher, job->id, error, NULL, 0, job->user_data);

	g_error_free (error);

	dispatcher_free_job (dispatcher, job);

	if (current) {
		priv->current_job = NULL;

		g_source_remove (priv->current_job_wait_id);
		priv->current_job_wait_id = 0;

		dispatcher_run_next_job (dispatcher);
	}
}

static void
dispatcher_cancel_job_id (GiggleDispatcher *dispatcher, guint id)
{
	GiggleDispatcherPriv *priv;
	GList                *l;

	priv = GET_PRIV (dispatcher);

	if (priv->current_job && priv->current_job->id == id) {
		dispatcher_cancel_job (dispatcher, priv->current_job);
		return;
	}

	for (l = priv->queue->head; l; l = l->next) {
		DispatcherJob *job = (DispatcherJob *) l->data;

		if (job->id == id) {
			dispatcher_cancel_job (dispatcher, job);
			g_queue_delete_link (priv->queue, l);
			break;
		}
	}
}

static void
dispatcher_free_job (GiggleDispatcher *dispatcher, DispatcherJob *job)
{
	g_free (job->command);
	g_free (job->wd);

	if (job->pid) {
		g_spawn_close_pid (job->pid);
	}

	g_free (job);
}

static void
dispatcher_job_wait_cb (GPid              pid,
			gint              status,
			GiggleDispatcher *dispatcher)
{
	GiggleDispatcherPriv *priv;
	DispatcherJob        *job;
	GIOChannel           *channel;
	GIOStatus             io_status;
	gchar                *output;
	gsize                 length;
	GError               *error = NULL;

	priv = GET_PRIV (dispatcher);
	job = priv->current_job;

	d(g_print ("GiggleDispatcher::job_wait_cb\n"));

	channel = g_io_channel_unix_new (job->std_out);
	io_status = g_io_channel_read_to_end (channel, &output, &length, &error);

	if (io_status != G_IO_STATUS_NORMAL) {
		dispatcher_job_failed (dispatcher, job, error);
		goto finished;
	}

	job->callback (dispatcher, job->id, NULL, 
		       output, length, 
		       job->user_data);

	g_free (output);

	dispatcher_free_job (dispatcher, job);

finished:
	g_io_channel_unref (channel);
	priv->current_job = NULL;
	priv->current_job_wait_id = 0;
	dispatcher_run_next_job (dispatcher);
}

static void
dispatcher_job_failed (GiggleDispatcher *dispatcher,
		       DispatcherJob    *job,
		       GError           *error)
{
	job->callback (dispatcher, job->id, error, NULL, 0, job->user_data);
	
	dispatcher_free_job (dispatcher, job);
}

GiggleDispatcher *
giggle_dispatcher_new (void)
{
	return g_object_new (GIGGLE_TYPE_DISPATCHER, NULL);
}

guint
giggle_dispatcher_execute (GiggleDispatcher      *dispatcher,
			   const gchar           *wd,
			   const gchar           *command,
			   GiggleExecuteCallback  callback,
			   gpointer               user_data)
{
	DispatcherJob *job;
	static guint   id = 0;

	g_return_val_if_fail (GIGGLE_IS_DISPATCHER (dispatcher), 0);
	g_return_val_if_fail (command != NULL, 0);
	g_return_val_if_fail (callback != NULL, 0);

	job = g_new0 (DispatcherJob, 1);

	job->command = g_strdup (command);
	job->callback = callback;
	job->user_data = user_data;
	
	job->id = ++id;
	job->pid = 0;
	job->std_out = 0;
	job->std_err = 0;

	if (wd) {
		job->wd = g_strdup (wd);
	} else {
		job->wd = NULL;
	}

	dispatcher_add_job (dispatcher, job);

	return job->id;
}

void
giggle_dispatcher_cancel (GiggleDispatcher *dispatcher, guint id)
{
	g_return_if_fail (GIGGLE_IS_DISPATCHER (dispatcher));
	g_return_if_fail (id > 0);

	dispatcher_cancel_job_id (dispatcher, id);
}

