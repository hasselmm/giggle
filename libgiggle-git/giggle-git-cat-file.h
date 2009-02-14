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

#ifndef __GIGGLE_GIT_CAT_FILE_H__
#define __GIGGLE_GIT_CAT_FILE_H__

#include <libgiggle/giggle-job.h>

G_BEGIN_DECLS

#define GIGGLE_TYPE_GIT_CAT_FILE            (giggle_git_cat_file_get_type ())
#define GIGGLE_GIT_CAT_FILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_GIT_CAT_FILE, GiggleGitCatFile))
#define GIGGLE_GIT_CAT_FILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_GIT_CAT_FILE, GiggleGitCatFileClass))
#define GIGGLE_IS_GIT_CAT_FILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_GIT_CAT_FILE))
#define GIGGLE_IS_GIT_CAT_FILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_GIT_CAT_FILE))
#define GIGGLE_GIT_CAT_FILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_GIT_CAT_FILE, GiggleGitCatFileClass))

typedef struct GiggleGitCatFile       GiggleGitCatFile;
typedef struct GiggleGitCatFileClass  GiggleGitCatFileClass;

struct GiggleGitCatFile {
	GiggleJob parent;
};

struct GiggleGitCatFileClass {
	GiggleJobClass parent_class;
};

GType        giggle_git_cat_file_get_type     (void);
GiggleJob *  giggle_git_cat_file_new          (const char *type,
					       const char *sha);

const char * giggle_git_cat_file_get_contents (GiggleGitCatFile *job,
					       gsize            *length);

G_END_DECLS

#endif /* __GIGGLE_GIT_CAT_FILE_H__ */
