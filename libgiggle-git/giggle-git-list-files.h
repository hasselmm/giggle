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

#ifndef __GIGGLE_GIT_LIST_FILES_H__
#define __GIGGLE_GIT_LIST_FILES_H__

#include <libgiggle/giggle-job.h>

G_BEGIN_DECLS

#define GIGGLE_TYPE_GIT_LIST_FILES            (giggle_git_list_files_get_type ())
#define GIGGLE_GIT_LIST_FILES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_GIT_LIST_FILES, GiggleGitListFiles))
#define GIGGLE_GIT_LIST_FILES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_GIT_LIST_FILES, GiggleGitListFilesClass))
#define GIGGLE_IS_GIT_LIST_FILES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_GIT_LIST_FILES))
#define GIGGLE_IS_GIT_LIST_FILES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_GIT_LIST_FILES))
#define GIGGLE_GIT_LIST_FILES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_GIT_LIST_FILES, GiggleGitListFilesClass))

typedef struct GiggleGitListFiles       GiggleGitListFiles;
typedef struct GiggleGitListFilesClass  GiggleGitListFilesClass;

struct GiggleGitListFiles {
	GiggleJob parent;
};

struct GiggleGitListFilesClass {
	GiggleJobClass parent_class;
};

typedef enum {
	GIGGLE_GIT_FILE_STATUS_OTHER = 0,
	GIGGLE_GIT_FILE_STATUS_CACHED,
	GIGGLE_GIT_FILE_STATUS_UNMERGED,
	GIGGLE_GIT_FILE_STATUS_DELETED,
	GIGGLE_GIT_FILE_STATUS_CHANGED,
	GIGGLE_GIT_FILE_STATUS_KILLED,
}  GiggleGitListFilesStatus;

GType		         giggle_git_list_files_get_type   (void);
GiggleJob *              giggle_git_list_files_new        (void);

GiggleGitListFilesStatus giggle_git_list_files_get_file_status (GiggleGitListFiles *list_files,
								const gchar        *file);


G_END_DECLS

#endif /* __GIGGLE_GIT_LIST_FILES_H__ */
