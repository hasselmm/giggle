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

#ifndef __GIGGLE_GIT_H__
#define __GIGGLE_GIT_H__

#include <glib-object.h>

#include "giggle-job.h"
#include "giggle-remote.h"

G_BEGIN_DECLS

#define GIGGLE_TYPE_GIT            (giggle_git_get_type ())
#define GIGGLE_GIT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_GIT, GiggleGit))
#define GIGGLE_GIT_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_GIT, GiggleGitClass))
#define GIGGLE_IS_GIT(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_GIT))
#define GIGGLE_IS_GIT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_GIT))
#define GIGGLE_GIT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_GIT, GiggleGitClass))

typedef struct GiggleGit      GiggleGit;
typedef struct GiggleGitClass GiggleGitClass;

struct GiggleGit {
	GObject parent;
};

struct GiggleGitClass {
	GObjectClass parent_class;

	void (* changed) (GiggleGit *git);
};

typedef void (*GiggleJobDoneCallback)   (GiggleGit *git,
					 GiggleJob *job,
					 GError    *error,
					 gpointer   user_data);

GType		 giggle_git_get_type         (void);
GiggleGit *      giggle_git_get              (void);
const gchar *    giggle_git_get_description  (GiggleGit    *git);
void             giggle_git_write_description(GiggleGit    *git,
					      const gchar  *description);
const gchar *    giggle_git_get_directory    (GiggleGit    *git);
gboolean         giggle_git_set_directory    (GiggleGit    *git,
					      const gchar  *directory,
					      GError      **error);
const gchar *    giggle_git_get_git_dir      (GiggleGit    *git);
const gchar *    giggle_git_get_project_dir  (GiggleGit    *git);
const gchar *    giggle_git_get_project_name (GiggleGit    *git);
GList *          giggle_git_get_remotes      (GiggleGit    *git);
void             giggle_git_save_remote      (GiggleGit    *git,
					      GiggleRemote *remote);

void             giggle_git_run_job_full     (GiggleGit             *git,
					      GiggleJob             *job,
					      GiggleJobDoneCallback  callback,
					      gpointer               user_data,
					      GDestroyNotify         destroy_notify);

void             giggle_git_run_job          (GiggleGit             *git,
					      GiggleJob             *job,
					      GiggleJobDoneCallback  callback,
					      gpointer               user_data);

void             giggle_git_cancel_job       (GiggleGit          *git,
					      GiggleJob          *job);
void             giggle_git_changed          (GiggleGit          *git);

gboolean         giggle_git_test_dir         (gchar const  * dir);


G_END_DECLS

#endif /* __GIGGLE_GIT_H__ */
