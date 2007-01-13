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

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_DISPATCHER, GiggleDispatcherPriv))

typedef struct GiggleDispatcherPriv GiggleDispatcherPriv;

typedef struct {
	gchar                 *command;
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
};

static void giggle_dispatcher_finalize (GObject *object);

static void dispatcher_add_job       (GiggleDispatcher  *dispatcher,
				      DispatcherJob     *job);

static void dispatcher_run_job       (GiggleDispatcher  *dispatcher,
				      DispatcherJob     *job);
static void dispatcher_run_next_job  (GiggleDispatcher *dispatcher);
static void dispatcher_cancel_job    (GiggleDispatcher *dispatcher, 
				      DispatcherJob    *job);
static void dispatcher_cancel_job_id (GiggleDispatcher *dispatcher,
				      guint             id);
static void dispatcher_free_job      (GiggleDispatcher *dispatcher,
				      DispatcherJob    *job);

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
}

static void
giggle_dispatcher_finalize (GObject *object)
{
	GiggleDispatcherPriv *priv = GET_PRIV (object);

	/* FIXME: Free object data */

	G_OBJECT_CLASS (giggle_dispatcher_parent_class)->finalize (object);
}

static void
dispatcher_add_job (GiggleDispatcher *dispatcher, DispatcherJob *job)
{
	GiggleDispatcherPriv *priv;

	priv = GET_PRIV (dispatcher);

	if (priv->queue == NULL && priv->current_job == NULL) {
		dispatcher_run_job (dispatcher, job); 
	} else {
		g_queue_push_tail (priv->queue, job);
	}
}

static void
dispatcher_run_job (GiggleDispatcher *dispatcher, DispatcherJob *job)
{
	GiggleDispatcherPriv *priv;

	priv = GET_PRIV (dispatcher);

	g_assert (priv->current_job);

	priv->current_job = job;

	//g_spawn_async_with_pipes ();
	//g_child_watch_add();
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
		dispatcher_run_job (dispatcher, job);
	}
}

static void
dispatcher_cancel_job (GiggleDispatcher *dispatcher, DispatcherJob *job)
{
	/* FIXME: Check if running */
	/* FIXME: Signal the callback */
}

static void
dispatcher_cancel_job_id (GiggleDispatcher *dispatcher, guint id)
{
	GiggleDispatcherPriv *priv;
	GList                *l;

	priv = GET_PRIV (dispatcher);

	if (priv->current_job && priv->current_job->id == id) {
		dispatcher_cancel_job (dispatcher, priv->current_job);
		dispatcher_free_job (dispatcher, priv->current_job);
		priv->current_job = NULL;
		dispatcher_run_next_job (dispatcher);
	}

	for (l = priv->queue->head; l; l = l->next) {
		DispatcherJob *job = (DispatcherJob *) l->data;

		if (job->id == id) {
			dispatcher_cancel_job (dispatcher, job);
			g_queue_delete_link (priv->queue, l);
			dispatcher_free_job (dispatcher, job);
			break;
		}
	}
}

static void
dispatcher_free_job (GiggleDispatcher *dispatcher, DispatcherJob *job)
{
	g_free (job->command);

	g_free (job);
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

