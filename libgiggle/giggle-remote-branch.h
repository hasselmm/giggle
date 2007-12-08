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

#ifndef __GIGGLE_REMOTE_BRANCH_H__
#define __GIGGLE_REMOTE_BRANCH_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GIGGLE_TYPE_REMOTE_BRANCH            (giggle_remote_branch_get_type ())
#define GIGGLE_REMOTE_BRANCH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_REMOTE_BRANCH, GiggleRemoteBranch))
#define GIGGLE_REMOTE_BRANCH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_REMOTE_BRANCH, GiggleRemoteBranchClass))
#define GIGGLE_IS_REMOTE_BRANCH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_REMOTE_BRANCH))
#define GIGGLE_IS_REMOTE_BRANCH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_REMOTE_BRANCH))
#define GIGGLE_REMOTE_BRANCH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_REMOTE_BRANCH, GiggleRemoteBranchClass))

typedef struct GiggleRemoteBranch      GiggleRemoteBranch;
typedef struct GiggleRemoteBranchClass GiggleRemoteBranchClass;

typedef enum {
	GIGGLE_REMOTE_DIRECTION_PUSH,
	GIGGLE_REMOTE_DIRECTION_PULL
} GiggleRemoteDirection;

struct GiggleRemoteBranch {
	GObject parent;
};

struct GiggleRemoteBranchClass {
	GObjectClass parent_class;
};

GType		      giggle_remote_branch_get_type     (void);
GiggleRemoteBranch *  giggle_remote_branch_new          (GiggleRemoteDirection dir,
							 const gchar          *refspec);
GiggleRemoteDirection giggle_remote_branch_get_direction(GiggleRemoteBranch   *branch);
const gchar *         giggle_remote_branch_get_refspec  (GiggleRemoteBranch   *branch);
void                  giggle_remote_branch_set_refspec  (GiggleRemoteBranch   *branch,
							 const gchar          *refspec);

G_END_DECLS

#endif /* __GIGGLE_REMOTE_BRANCH_H__ */
