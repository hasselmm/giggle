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

#ifndef __GIGGLE_GIT_IGNORE_H__
#define __GIGGLE_GIT_IGNORE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GIGGLE_TYPE_GIT_IGNORE            (giggle_git_ignore_get_type ())
#define GIGGLE_GIT_IGNORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_GIT_IGNORE, GiggleGitIgnore))
#define GIGGLE_GIT_IGNORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIGGLE_TYPE_GIT_IGNORE, GiggleGitIgnoreClass))
#define GIGGLE_IS_GIT_IGNORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_GIT_IGNORE))
#define GIGGLE_IS_GIT_IGNORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIGGLE_TYPE_GIT_IGNORE))
#define GIGGLE_GIT_IGNORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIGGLE_TYPE_GIT_IGNORE, GiggleGitIgnoreClass))

typedef struct _GiggleGitIgnore      GiggleGitIgnore;
typedef struct _GiggleGitIgnoreClass GiggleGitIgnoreClass;

struct _GiggleGitIgnore {
	GObject parent_instance;
};

struct _GiggleGitIgnoreClass {
	GObjectClass parent_class;
};

GType              giggle_git_ignore_get_type               (void);
GiggleGitIgnore *  giggle_git_ignore_new                    (const gchar     *directory_path);

gboolean           giggle_git_ignore_path_matches           (GiggleGitIgnore *git_ignore,
							     const gchar     *name);
void               giggle_git_ignore_add_glob               (GiggleGitIgnore *git_ignore,
							     const gchar     *glob);
void               giggle_git_ignore_add_glob_for_path      (GiggleGitIgnore *git_ignore,
							     const gchar     *path);
gboolean           giggle_git_ignore_remove_glob_for_path   (GiggleGitIgnore *git_ignore,
							     const gchar     *name,
							     gboolean         perfect_match);


G_END_DECLS

#endif /* __GIGGLE_GIT_IGNORE_H__ */
