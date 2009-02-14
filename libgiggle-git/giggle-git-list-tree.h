/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2008 Mathias Hasselmann
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

#ifndef __GIGGLE_GIT_LIST_TREE_H__
#define __GIGGLE_GIT_LIST_TREE_H__

#include <libgiggle/giggle-job.h>
#include <libgiggle/giggle-revision.h>

G_BEGIN_DECLS

#define GIGGLE_TYPE_GIT_LIST_TREE            (giggle_git_list_tree_get_type ())
#define GIGGLE_GIT_LIST_TREE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_GIT_LIST_TREE, GiggleGitListTree))
#define GIGGLE_GIT_LIST_TREE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_GIT_LIST_TREE, GiggleGitListTreeClass))
#define GIGGLE_IS_GIT_LIST_TREE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_GIT_LIST_TREE))
#define GIGGLE_IS_GIT_LIST_TREE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_GIT_LIST_TREE))
#define GIGGLE_GIT_LIST_TREE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_GIT_LIST_TREE, GiggleGitListTreeClass))

typedef struct GiggleGitListTree       GiggleGitListTree;
typedef struct GiggleGitListTreeClass  GiggleGitListTreeClass;

struct GiggleGitListTree {
	GiggleJob parent;
};

struct GiggleGitListTreeClass {
	GiggleJobClass parent_class;
};

GType        giggle_git_list_tree_get_type  (void);
GiggleJob *  giggle_git_list_tree_new       (GiggleRevision    *revision,
					     const char        *path);

unsigned     giggle_git_list_tree_get_mode  (GiggleGitListTree *job,
					     const char        *file);
const char * giggle_git_list_tree_get_kind  (GiggleGitListTree *job,
					     const char        *file);
const char * giggle_git_list_tree_get_sha   (GiggleGitListTree *job,
					     const char        *file);
GList *      giggle_git_list_tree_get_files (GiggleGitListTree *job);

G_END_DECLS

#endif /* __GIGGLE_GIT_LIST_TREE_H__ */
