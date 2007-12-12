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
#include <unistd.h>

#include "giggle-error.h"
#include "giggle-sysdeps.h"
#include "giggle-dispatcher.h"

#define d(x)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_DISPATCHER, GiggleDispatcherPriv))

typedef struct GiggleDispatcherPriv GiggleDispatcherPriv;

typedef struct {
	gchar                 *command;
	gchar                 *wd;
	GiggleExecuteCallback  callback;
	guint                  id;
	GPid                   pid;
	gint                   std_out;
	gint                   std_err;
	gpointer               user_data;
} DispatcherJob;

struct GiggleDispatcherPriv {
	GQueue        *queue;

	DispatcherJob *current_job;
	guint          current_job_wait_id;

	guint          current_job_read_id;
	GIOChannel    *channel;
	GString       *output;
	gsize          length;
};

static void     giggle_dispatcher_finalize (GObject *object);


static void      dispatcher_queue_job        (GiggleDispatcher  *dispatcher,
					      DispatcherJob     *job);
static void      dispatcher_unqueue_job      (GiggleDispatcher  *dispatcher,
					      guint              id);
static gboolean  dispatcher_start_job        (GiggleDispatcher  *dispatcher,
					      DispatcherJob     *job);
static void      dispatcher_stop_current_job (GiggleDispatcher *dispatcher);
static void      dispatcher_start_next_job   (GiggleDispatcher *dispatcher);
static void      dispatcher_signal_job_failed (GiggleDispatcher *dispatcher, 
					       DispatcherJob    *job,
					       GError           *error);
static void      dispatcher_job_free         (DispatcherJob    *job);
static gboolean  dispatcher_is_free_to_start (GiggleDispatcher *dispatcher);
static gboolean  dispatcher_is_current_job   (GiggleDispatcher *dispatcher,
					      guint             id);

static void      dispatcher_job_finished_cb  (GPid              pid,
					      gint              status,
					      GiggleDispatcher *dispatcher);
static gboolean  dispatcher_job_read_cb      (GIOChannel       *source,
					      GIOCondition      condition,
					      GiggleDispatcher *dispatcher);


G_DEFINE_TYPE (GiggleDispatcher, giggle_dispatcher, G_TYPE_OBJECT)

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
		dispatcher_stop_current_job (dispatcher);
	}

	while ((job = g_queue_pop_head (priv->queue))) {
		dispatcher_job_free (job);
	}
	g_queue_free (priv->queue);
	
	G_OBJECT_CLASS (giggle_dispatcher_parent_class)->finalize (object);
}

static void
dispatcher_queue_job (GiggleDispatcher *dispatcher, DispatcherJob *job)
{
	GiggleDispatcherPriv *priv;

	priv = GET_PRIV (dispatcher);

	d(g_print ("GiggleDispatcher::queue_job\n"));

	g_queue_push_tail (priv->queue, job);
}

static void
dispatcher_unqueue_job (GiggleDispatcher *dispatcher, guint id)
{
	GiggleDispatcherPriv *priv;
	GList                *l;

	priv = GET_PRIV (dispatcher);

	d(g_print ("GiggleDispatcher::unqueue_job\n"));

	for (l = priv->queue->head; l; l = l->next) {
		DispatcherJob *job = (DispatcherJob *) l->data;

		if (job->id == id) {
			g_queue_delete_link (priv->queue, l);
			dispatcher_job_free (job);
			break;
		}
	}
}

static gboolean
dispatcher_start_job (GiggleDispatcher *dispatcher, DispatcherJob *job)
{
	GiggleDispatcherPriv  *priv;
	gint                   argc;
	gchar                **argv;
	GError                *error = NULL;

	priv = GET_PRIV (dispatcher);

	g_assert (priv->current_job == NULL);

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

	priv->channel = g_io_channel_unix_new (job->std_out);
	g_io_channel_set_encoding (priv->channel, NULL, NULL);
	priv->output = g_string_new ("");
	priv->length = 0;

	priv->current_job = job;
	priv->current_job_read_id = g_io_add_watch_full (priv->channel,
							 G_PRIORITY_HIGH_IDLE,
							 G_IO_IN,
							 (GIOFunc) dispatcher_job_read_cb,
							 dispatcher, NULL);
	priv->current_job_wait_id = g_child_watch_add (job->pid,
						       (GChildWatchFunc) dispatcher_job_finished_cb,
						       dispatcher);
	g_strfreev (argv);

	return TRUE;

failed:
	dispatcher_signal_job_failed (dispatcher, job, error);
	dispatcher_job_free (job);
	g_strfreev (argv);
	g_error_free (error);
	priv->current_job = NULL;
	priv->current_job_wait_id = 0;

	return FALSE;
}

static void
dispatcher_stop_current_job (GiggleDispatcher *dispatcher)
{
	GiggleDispatcherPriv *priv;

	priv = GET_PRIV (dispatcher);

	g_assert (priv->current_job_wait_id != 0);
	g_source_remove (priv->current_job_wait_id);
	priv->current_job_wait_id = 0;

	g_source_remove (priv->current_job_read_id);
	priv->current_job_read_id = 0;

	g_io_channel_unref (priv->channel);
	priv->channel = NULL;

	g_string_free (priv->output, TRUE);
	priv->output = NULL;

	g_assert (priv->current_job != NULL);
	giggle_sysdeps_kill_pid (priv->current_job->pid);

	dispatcher_job_free (priv->current_job);
	priv->current_job = NULL;
}

static void
dispatcher_start_next_job (GiggleDispatcher *dispatcher)
{
	GiggleDispatcherPriv *priv;
	DispatcherJob        *job;

	priv = GET_PRIV (dispatcher);

	while ((job = (DispatcherJob *) g_queue_pop_head (priv->queue))) {
		if (dispatcher_start_job (dispatcher, job)) {
			break;
		}
	}
}

static void
dispatcher_signal_job_failed (GiggleDispatcher *dispatcher, 
			      DispatcherJob    *job,
			      GError           *error)
{
	job->callback (dispatcher, job->id, error, NULL, 0, job->user_data);
}

static void
dispatcher_job_free (DispatcherJob *job)
{
	g_free (job->command);
	g_free (job->wd);

	if (job->pid) {
		g_spawn_close_pid (job->pid);
	}

	if (job->std_out) {
		close (job->std_out);
	}

	if (job->std_err) {
		close (job->std_err);
	}

	g_slice_free (DispatcherJob, job);
}

static gboolean
dispatcher_is_free_to_start (GiggleDispatcher *dispatcher)
{
	GiggleDispatcherPriv *priv;

	priv = GET_PRIV (dispatcher);

	if (priv->current_job) {
		return FALSE;
	} 

	return TRUE;
}

static gboolean
dispatcher_is_current_job (GiggleDispatcher *dispatcher, guint id)
{
	GiggleDispatcherPriv *priv;

	priv = GET_PRIV (dispatcher);

	if (priv->current_job && priv->current_job->id == id) {
		return TRUE;
	}

	return FALSE;
}

static void
dispatcher_job_finished_cb (GPid              pid,
			    gint              status,
			    GiggleDispatcher *dispatcher)
{
	GiggleDispatcherPriv *priv;
	DispatcherJob        *job;
	gchar                *str;
	gsize                 len;

	priv = GET_PRIV (dispatcher);
	job = priv->current_job;

	d(g_print ("GiggleDispatcher::job_finished_cb\n"));

	g_source_remove (priv->current_job_read_id);
	priv->current_job_read_id = 0;

	g_io_channel_read_to_end (priv->channel, &str, &len, NULL);

	if (str) {
		g_string_append_len (priv->output, str, len);
		g_free (str);
	}

	job->callback (dispatcher, job->id, NULL,
		       priv->output->str, priv->length,
		       job->user_data);

	dispatcher_job_free (job);
	g_io_channel_unref (priv->channel);
	g_string_free (priv->output, TRUE);
	priv->current_job = NULL;
	priv->current_job_wait_id = 0;
	dispatcher_start_next_job (dispatcher);
}

static gboolean
dispatcher_job_read_cb (GIOChannel       *source,
			GIOCondition      condition,
			GiggleDispatcher *dispatcher)
{
	GiggleDispatcherPriv *priv;
	gsize                 length;
	gchar                *str;
	gint                  count = 0;
	GIOStatus             status;
	GError               *error = NULL;

	priv = GET_PRIV (dispatcher);
	status = G_IO_STATUS_NORMAL;

	while (count < 10 && status == G_IO_STATUS_NORMAL) {
		status = g_io_channel_read_line (source, &str, &length, NULL, &error);
		count++;

		if (str) {
			g_string_append_len (priv->output, str, length);
			priv->length += length;
			g_free (str);
		}
	}

	if (status == G_IO_STATUS_ERROR) {
		dispatcher_signal_job_failed (dispatcher, priv->current_job, error);
		dispatcher_stop_current_job (dispatcher);
		dispatcher_start_next_job (dispatcher);

		return FALSE;
	}

	return TRUE;
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

	job = g_slice_new0 (DispatcherJob);

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

	if (dispatcher_is_free_to_start (dispatcher)) {
		dispatcher_start_job (dispatcher, job);
	} else {
		dispatcher_queue_job (dispatcher, job);
	}

	return job->id;
}

void
giggle_dispatcher_cancel (GiggleDispatcher *dispatcher, guint id)
{
	g_return_if_fail (GIGGLE_IS_DISPATCHER (dispatcher));
	g_return_if_fail (id > 0);

	if (dispatcher_is_current_job (dispatcher, id)) {
		dispatcher_stop_current_job (dispatcher);
		dispatcher_start_next_job (dispatcher);
	} else {
		dispatcher_unqueue_job (dispatcher, id);
	}
}

