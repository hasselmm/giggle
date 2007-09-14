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
#include <gtk/gtk.h>

#include "giggle-remote-ref.h"
#include "giggle-revision.h"
#include "giggle-ref.h"

G_DEFINE_TYPE (GiggleRemoteRef, giggle_remote_ref, GIGGLE_TYPE_REF)


static void
giggle_remote_ref_class_init (GiggleRemoteRefClass *class)
{
}

static void
giggle_remote_ref_init (GiggleRemoteRef *ref)
{
}

GiggleRef *
giggle_remote_ref_new (const gchar *name)
{
	return g_object_new (GIGGLE_TYPE_REMOTE_REF,
			     "name", name,
			     NULL);
}
