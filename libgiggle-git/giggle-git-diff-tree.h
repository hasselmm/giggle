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

#ifndef __GIGGLE_GIT_DIFF_TREE_H__
#define __GIGGLE_GIT_DIFF_TREE_H__

#include <libgiggle/giggle-job.h>
#include <libgiggle/giggle-revision.h>

G_BEGIN_DECLS

#define GIGGLE_TYPE_GIT_DIFF_TREE            (giggle_git_diff_tree_get_type ())
#define GIGGLE_GIT_DIFF_TREE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_GIT_DIFF_TREE, GiggleGitDiffTree))
#define GIGGLE_GIT_DIFF_TREE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_GIT_DIFF_TREE, GiggleGitDiffTreeClass))
#define GIGGLE_IS_GIT_DIFF_TREE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_GIT_DIFF_TREE))
#define GIGGLE_IS_GIT_DIFF_TREE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_GIT_DIFF_TREE))
#define GIGGLE_GIT_DIFF_TREE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_GIT_DIFF_TREE, GiggleGitDiffTreeClass))

typedef struct GiggleGitDiffTree      GiggleGitDiffTree;
typedef struct GiggleGitDiffTreeClass GiggleGitDiffTreeClass;

struct GiggleGitDiffTree {
	GiggleJob parent;
};

struct GiggleGitDiffTreeClass {
	GiggleJobClass parent_class;
};

GType		      giggle_git_diff_tree_get_type   (void);
GiggleJob *           giggle_git_diff_tree_new        (GiggleRevision *rev1,
						       GiggleRevision *rev2);

GList *               giggle_git_diff_tree_get_files  (GiggleGitDiffTree *job);
const char *          giggle_git_diff_tree_get_sha1   (GiggleGitDiffTree *job,
						       const char        *file);
const char *          giggle_git_diff_tree_get_sha2   (GiggleGitDiffTree *job,
						       const char        *file);
char                  giggle_git_diff_tree_get_action (GiggleGitDiffTree *job,
						       const char        *file);

G_END_DECLS

#endif /* __GIGGLE_GIT_DIFF_TREE_H__ */
