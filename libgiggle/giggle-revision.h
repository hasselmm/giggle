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

#ifndef __GIGGLE_REVISION_H__
#define __GIGGLE_REVISION_H__

#include <glib-object.h>
#include <gdk/gdk.h>
#include <gtk/gtktreemodel.h>

#include "giggle-ref.h"
#include "giggle-branch.h"

G_BEGIN_DECLS

#define GIGGLE_TYPE_REVISION            (giggle_revision_get_type ())
#define GIGGLE_REVISION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_REVISION, GiggleRevision))
#define GIGGLE_REVISION_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_REVISION, GiggleRevisionClass))
#define GIGGLE_IS_REVISION(obj)	        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_REVISION))
#define GIGGLE_IS_REVISION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_REVISION))
#define GIGGLE_REVISION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_REVISION, GiggleRevisionClass))

typedef struct _GiggleRevision      GiggleRevision;
typedef struct _GiggleRevisionClass GiggleRevisionClass;

struct _GiggleRevision {
	GObject parent_instance;
};

struct _GiggleRevisionClass {
	GObjectClass parent_class;

	void (* descendent_branch_added) (GiggleRevision *revision,
					  GiggleBranch   *branch);
};

GType              giggle_revision_get_type          (void);
GiggleRevision *   giggle_revision_new               (const gchar *sha);

const gchar *      giggle_revision_get_sha           (GiggleRevision   *revision);
const gchar *      giggle_revision_get_author        (GiggleRevision   *revision);
const gchar *      giggle_revision_get_email         (GiggleRevision   *revision);
const struct tm *  giggle_revision_get_date          (GiggleRevision   *revision);
const gchar *      giggle_revision_get_short_log     (GiggleRevision   *revision);

GList *            giggle_revision_get_children      (GiggleRevision   *revision);
GList *            giggle_revision_get_parents       (GiggleRevision   *revision);
void               giggle_revision_add_parent        (GiggleRevision   *revision,
						      GiggleRevision   *parent);
void               giggle_revision_remove_parent     (GiggleRevision   *revision,
						      GiggleRevision   *parent);

GList *            giggle_revision_get_branch_heads  (GiggleRevision   *revision);
void               giggle_revision_add_branch_head   (GiggleRevision   *revision,
						      GiggleRef        *branch);

GList *            giggle_revision_get_tags          (GiggleRevision   *revision);
void               giggle_revision_add_tag           (GiggleRevision   *revision,
						      GiggleRef        *tag);

GList *            giggle_revision_get_remotes       (GiggleRevision   *revision);
void               giggle_revision_add_remote        (GiggleRevision   *revision,
						      GiggleRef        *tag);

GList *            giggle_revision_get_descendent_branches
						     (GiggleRevision   *revision);

int                giggle_revision_compare           (gconstpointer     a,
                                                      gconstpointer     b);

G_END_DECLS

#endif /* __GIGGLE_REVISION_H__ */
