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

#include "giggle-view.h"

typedef struct GiggleViewPriv GiggleViewPriv;

struct GiggleViewPriv {
};

static void       view_finalize           (GObject        *object);


G_DEFINE_ABSTRACT_TYPE (GiggleView, giggle_view, GTK_TYPE_VBOX)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_VIEW, GiggleViewPriv))

static void
giggle_view_class_init (GiggleViewClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = view_finalize;

	g_type_class_add_private (object_class, sizeof (GiggleViewPriv));
}

static void
giggle_view_init (GiggleView *view)
{
}

static void
view_finalize (GObject *object)
{
	GiggleViewPriv *priv;

	priv = GET_PRIV (object);
}
