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

G_BEGIN_DECLS

#define GIGGLE_TYPE_REVISION            (giggle_revision_get_type ())
#define GIGGLE_REVISION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_REVISION, GiggleRevision))
#define GIGGLE_REVISION_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_REVISION, GiggleRevisionClass))
#define GIGGLE_IS_REVISION(obj)	        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_REVISION))
#define GIGGLE_IS_REVISION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_REVISION))
#define GIGGLE_REVISION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_REVISION, GiggleRevisionClass))

typedef struct GiggleRevision      GiggleRevision;
typedef struct GiggleRevisionClass GiggleRevisionClass;
typedef enum   GiggleRevisionType  GiggleRevisionType;
typedef struct GiggleBranchInfo    GiggleBranchInfo;

struct GiggleRevisionClass {
	GObjectClass parent_class;
};

struct GiggleBranchInfo {
	gchar *name;
};

enum GiggleRevisionType {
	GIGGLE_REVISION_BRANCH,
	GIGGLE_REVISION_MERGE,
	GIGGLE_REVISION_COMMIT
};

struct GiggleRevision {
	GObject parent_instance;

	/* All this should be priv... */
	GiggleRevisionType  type;
	GiggleBranchInfo   *branch1; /* Only this one will be used in COMMIT. */
	GiggleBranchInfo   *branch2;

	/* Stuff that will be filled out in the validation process. */
	GHashTable         *branches;

};


GType             giggle_revision_get_type   (void);
GiggleRevision *  giggle_revision_new_commit (GiggleBranchInfo *branch);
GiggleRevision *  giggle_revision_new_branch (GiggleBranchInfo *old,
					      GiggleBranchInfo *new);
GiggleRevision *  giggle_revision_new_merge  (GiggleBranchInfo *to,
					      GiggleBranchInfo *from);
void              giggle_revision_validate   (GtkTreeModel     *model,
					      gint              n_column);

GiggleBranchInfo *giggle_branch_info_new     (const gchar      *name);
void              giggle_branch_info_free    (GiggleBranchInfo *info);

G_END_DECLS

#endif /* __GIGGLE_REVISION_H__ */
