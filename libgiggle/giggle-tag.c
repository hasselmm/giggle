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

#include "giggle-tag.h"
#include "giggle-revision.h"
#include "giggle-ref.h"

G_DEFINE_TYPE (GiggleTag, giggle_tag, GIGGLE_TYPE_REF)


static void
giggle_tag_class_init (GiggleTagClass *class)
{
}

static void
giggle_tag_init (GiggleTag *ref)
{
}

GiggleRef *
giggle_tag_new (const gchar *name)
{
	return g_object_new (GIGGLE_TYPE_TAG,
			     "name", name,
			     NULL);
}
