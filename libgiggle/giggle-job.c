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

#include "giggle-job.h"

typedef struct GiggleJobPriv GiggleJobPriv;

struct GiggleJobPriv {
	guint id;
};

static void     job_finalize     (GObject      *object);
static void     job_set_property (GObject      *object,
				  guint         param_id,
				  const GValue *value,
				  GParamSpec   *pspec);
static void     job_get_property (GObject      *object,
				  guint         param_id,
				  GValue       *value,
				  GParamSpec   *pspec);

G_DEFINE_ABSTRACT_TYPE (GiggleJob, giggle_job, G_TYPE_OBJECT)

enum {
	PROP_0,
	PROP_ID
};

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_JOB, GiggleJobPriv))

static void
giggle_job_class_init (GiggleJobClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = job_finalize;
	object_class->get_property = job_get_property;
	object_class->set_property = job_set_property;

	g_object_class_install_property (object_class,
					 PROP_ID,
					 g_param_spec_uint ("id",
							    "Id",
							    "A unique identifier for the job.",
							    0, G_MAXUINT,
							    0,
							    G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleJobPriv));
}

static void
giggle_job_init (GiggleJob *job)
{
	GiggleJobPriv *priv;

	priv = GET_PRIV (job);

	priv->id = 0;
}

static void
job_finalize (GObject *object)
{
	/* FIXME: Free object data */

	G_OBJECT_CLASS (giggle_job_parent_class)->finalize (object);
}

static void
job_get_property (GObject    *object,
		  guint       param_id,
		  GValue     *value,
		  GParamSpec *pspec)
{
	GiggleJobPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_ID:
		g_value_set_uint (value, priv->id);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
job_set_property (GObject      *object,
		  guint         param_id,
		  const GValue *value,
		  GParamSpec   *pspec)
{
	GiggleJobPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_ID:
		priv->id = g_value_get_uint (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

gboolean
giggle_job_get_command_line (GiggleJob *job, gchar **command_line)
{
	GiggleJobClass *klass;

	g_return_val_if_fail (GIGGLE_IS_JOB (job), FALSE);
	g_return_val_if_fail (command_line != NULL, FALSE);

	klass = GIGGLE_JOB_GET_CLASS (job);
	if (klass->get_command_line) {
		return klass->get_command_line (job, command_line);
	} 
	
	*command_line = NULL;
	return FALSE;
}

void
giggle_job_handle_output (GiggleJob   *job,
			  const gchar *output_str,
			  gsize        output_len)
{
	GiggleJobClass *klass;

	g_return_if_fail (GIGGLE_IS_JOB (job));

	klass = GIGGLE_JOB_GET_CLASS (job);
	if (klass->handle_output) {
		klass->handle_output (job, output_str, output_len);
	}
}

