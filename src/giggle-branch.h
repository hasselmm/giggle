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

#ifndef __GIGGLE_BRANCH_H__
#define __GIGGLE_BRANCH_H__

#include <glib-object.h>

#include "giggle-ref.h"

G_BEGIN_DECLS

#define GIGGLE_TYPE_BRANCH            (giggle_branch_get_type ())
#define GIGGLE_BRANCH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_BRANCH, GiggleBranch))
#define GIGGLE_BRANCH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_BRANCH, GiggleBranchClass))
#define GIGGLE_IS_BRANCH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_BRANCH))
#define GIGGLE_IS_BRANCH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_BRANCH))
#define GIGGLE_BRANCH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_BRANCH, GiggleBranchClass))

typedef struct _GiggleBranch      GiggleBranch;
typedef struct _GiggleBranchClass GiggleBranchClass;

struct _GiggleBranch {
	GiggleRef parent_instance;
};

struct _GiggleBranchClass {
	GiggleRefClass parent_class;
};

GType                   giggle_branch_get_type          (void);
GiggleRef             * giggle_branch_new               (const gchar *name);


G_END_DECLS

#endif /* __GIGGLE_BRANCH_H__ */
