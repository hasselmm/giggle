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

#ifndef __GIGGLE_GIT_REMOTE_LIST_H__
#define __GIGGLE_GIT_REMOTE_LIST_H__

#include <libgiggle/giggle-job.h>

G_BEGIN_DECLS

#define GIGGLE_TYPE_GIT_REMOTE_LIST            (giggle_git_remote_list_get_type ())
#define GIGGLE_GIT_REMOTE_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_GIT_REMOTE_LIST, GiggleGitRemoteList))
#define GIGGLE_GIT_REMOTE_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_GIT_REMOTE_LIST, GiggleGitRemoteListClass))
#define GIGGLE_IS_GIT_REMOTE_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_GIT_REMOTE_LIST))
#define GIGGLE_IS_GIT_REMOTE_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_GIT_REMOTE_LIST))
#define GIGGLE_GIT_REMOTE_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_GIT_REMOTE_LIST, GiggleGitRemoteListClass))

typedef struct GiggleGitRemoteList       GiggleGitRemoteList;
typedef struct GiggleGitRemoteListClass  GiggleGitRemoteListClass;

struct GiggleGitRemoteList {
	GiggleJob parent;
};

struct GiggleGitRemoteListClass {
	GiggleJobClass parent_class;
};

GType        giggle_git_remote_list_get_type  (void);
GiggleJob *  giggle_git_remote_list_new       (void);

GList *      giggle_git_remote_list_get_names (GiggleGitRemoteList *job);

G_END_DECLS

#endif /* __GIGGLE_GIT_REMOTE_LIST_H__ */
