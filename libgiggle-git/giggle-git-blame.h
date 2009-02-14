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

#ifndef __GIGGLE_GIT_BLAME_H__
#define __GIGGLE_GIT_BLAME_H__

#include <libgiggle/giggle-job.h>
#include <libgiggle/giggle-revision.h>

G_BEGIN_DECLS

#define GIGGLE_TYPE_GIT_BLAME            (giggle_git_blame_get_type ())
#define GIGGLE_GIT_BLAME(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_GIT_BLAME, GiggleGitBlame))
#define GIGGLE_GIT_BLAME_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_GIT_BLAME, GiggleGitBlameClass))
#define GIGGLE_IS_GIT_BLAME(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_GIT_BLAME))
#define GIGGLE_IS_GIT_BLAME_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_GIT_BLAME))
#define GIGGLE_GIT_BLAME_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_GIT_BLAME, GiggleGitBlameClass))

typedef struct GiggleGitBlame       GiggleGitBlame;
typedef struct GiggleGitBlameClass  GiggleGitBlameClass;
typedef struct GiggleGitBlameChunk  GiggleGitBlameChunk;

struct GiggleGitBlame {
	GiggleJob parent;
};

struct GiggleGitBlameClass {
	GiggleJobClass parent_class;
};

struct GiggleGitBlameChunk {
	GiggleRevision *revision;
	int             source_line;
	int             result_line;
	int             num_lines;
};

GType                       giggle_git_blame_get_type  (void);
GiggleJob *                 giggle_git_blame_new       (GiggleRevision *revision,
					                const char     *file);

const GiggleGitBlameChunk * giggle_git_blame_get_chunk (GiggleGitBlame *blame,
					                int             index);

G_END_DECLS

#endif /* __GIGGLE_GIT_BLAME_H__ */
