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

#ifndef __GIGGLE_REMOTE_H__
#define __GIGGLE_REMOTE_H__

#include <glib-object.h>
#include "giggle-remote-branch.h"

G_BEGIN_DECLS

#define GIGGLE_TYPE_REMOTE            (giggle_remote_get_type ())
#define GIGGLE_REMOTE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_REMOTE, GiggleRemote))
#define GIGGLE_REMOTE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_REMOTE, GiggleRemoteClass))
#define GIGGLE_IS_REMOTE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_REMOTE))
#define GIGGLE_IS_REMOTE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_REMOTE))
#define GIGGLE_REMOTE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_REMOTE, GiggleRemoteClass))

typedef struct GiggleRemote      GiggleRemote;
typedef struct GiggleRemoteClass GiggleRemoteClass;

struct GiggleRemote {
	GObject parent;
};

struct GiggleRemoteClass {
	GObjectClass parent_class;
};

GType		      giggle_remote_get_type        (void);
GiggleRemote *        giggle_remote_new             (gchar const  *name);
GiggleRemote *        giggle_remote_new_from_file   (gchar const  *filename);
void                  giggle_remote_add_branch      (GiggleRemote *remote,
						     GiggleRemoteBranch *branch);
GList *               giggle_remote_get_branches    (GiggleRemote *remote);
void                  giggle_remote_remove_branches (GiggleRemote *remote);
const gchar *         giggle_remote_get_name        (GiggleRemote *remote);
void                  giggle_remote_set_name        (GiggleRemote *remote,
						     gchar const  *name);
const gchar *         giggle_remote_get_url         (GiggleRemote *remote);
void                  giggle_remote_set_url         (GiggleRemote *remote,
					             gchar const  *url);
void                  giggle_remote_save_to_file    (GiggleRemote *remote,
						     gchar const  *filename);

G_END_DECLS

#endif /* __GIGGLE_REMOTE_H__ */
