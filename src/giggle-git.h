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

#include "giggle-revision.h"

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
};

typedef void (*GiggleDiffCallback) (GiggleGit      *git,
				    guint           id,
				    GError         *error,
				    GiggleRevision *rev1,
				    GiggleRevision *rev2,
				    const gchar    *diff,
				    gpointer        user_data);

GType		 giggle_git_get_type         (void);
GiggleGit *      giggle_git_new              (void);
const gchar *    giggle_git_get_directory    (GiggleGit    *git);
gboolean         giggle_git_set_directory    (GiggleGit    *git,
					      const gchar  *directory,
					      GError      **error);

guint            giggle_git_get_diff         (GiggleGit          *git,
					      GiggleRevision     *rev1,
					      GiggleRevision     *rev2,
					      GiggleDiffCallback  callback,
					      gpointer            user_data);

void             giggle_git_cancel           (GiggleGit          *git,
					      guint               id);

G_END_DECLS

#endif /* __GIGGLE_GIT_H__ */
