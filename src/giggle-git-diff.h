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

#ifndef __GIGGLE_GIT_DIFF_H__
#define __GIGGLE_GIT_DIFF_H__

#include <glib-object.h>

#include "giggle-job.h"
#include "giggle-revision.h"

G_BEGIN_DECLS

#define GIGGLE_TYPE_GIT_DIFF            (giggle_git_diff_get_type ())
#define GIGGLE_GIT_DIFF(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_GIT_DIFF, GiggleGitDiff))
#define GIGGLE_GIT_DIFF_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_GIT_DIFF, GiggleGitDiffClass))
#define GIGGLE_IS_GIT_DIFF(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_GIT_DIFF))
#define GIGGLE_IS_GIT_DIFF_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_GIT_DIFF))
#define GIGGLE_GIT_DIFF_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_GIT_DIFF, GiggleGitDiffClass))

typedef struct GiggleGitDiff      GiggleGitDiff;
typedef struct GiggleGitDiffClass GiggleGitDiffClass;

struct GiggleGitDiff {
	GiggleJob parent;
};

struct GiggleGitDiffClass {
	GiggleJobClass parent_class;
};

GType              giggle_git_diff_get_type         (void);

GiggleJob *        giggle_git_diff_new              (void);
void               giggle_git_diff_set_revisions    (GiggleGitDiff  *diff,
						     GiggleRevision *rev1,
						     GiggleRevision *rev2);
void               giggle_git_diff_set_files        (GiggleGitDiff  *diff,
						     GList          *files);
void               giggle_git_diff_set_patch_format (GiggleGitDiff  *diff,
						     GiggleRevision *rev);
GiggleRevision    *giggle_git_diff_get_patch_format (GiggleGitDiff  *diff);
const gchar *      giggle_git_diff_get_result       (GiggleGitDiff  *diff);

G_END_DECLS

#endif /* __GIGGLE_GIT_DIFF_H__ */
