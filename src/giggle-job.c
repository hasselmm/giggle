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
	guint i;
};

static void  giggle_job_finalize (GObject *object);

G_DEFINE_TYPE (GiggleJob, giggle_job, G_TYPE_OBJECT);

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_JOB, GiggleJobPriv))

static void
giggle_job_class_init (GiggleJobClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = giggle_job_finalize;

	g_type_class_add_private (object_class, sizeof (GiggleJobPriv));
}

static void
giggle_job_init (GiggleJob *job)
{
}

static void
giggle_job_finalize (GObject *object)
{
	/* FIXME: Free object data */

	G_OBJECT_CLASS (giggle_job_parent_class)->finalize (object);
}

