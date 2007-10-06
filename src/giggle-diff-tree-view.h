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

#ifndef __GIGGLE_DIFF_TREE_VIEW_H__
#define __GIGGLE_DIFF_TREE_VIEW_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#include "giggle-revision.h"

G_BEGIN_DECLS

#define GIGGLE_TYPE_DIFF_TREE_VIEW            (giggle_diff_tree_view_get_type ())
#define GIGGLE_DIFF_TREE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_DIFF_TREE_VIEW, GiggleDiffTreeView))
#define GIGGLE_DIFF_TREE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_DIFF_TREE_VIEW, GiggleDiffTreeViewClass))
#define GIGGLE_IS_DIFF_TREE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_DIFF_TREE_VIEW))
#define GIGGLE_IS_DIFF_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_DIFF_TREE_VIEW))
#define GIGGLE_DIFF_TREE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_DIFF_TREE_VIEW, GiggleDiffTreeViewClass))

typedef struct _GiggleDiffTreeView      GiggleDiffTreeView;
typedef struct _GiggleDiffTreeViewClass GiggleDiffTreeViewClass;

struct _GiggleDiffTreeView {
	GtkTreeView parent_instance;
};

struct _GiggleDiffTreeViewClass {
	GtkTreeViewClass parent_class;
	void (* path_selected) (GiggleDiffTreeView *view, const gchar *path);
};

GType              giggle_diff_tree_view_get_type          (void);
GtkWidget *        giggle_diff_tree_view_new               (void);

void               giggle_diff_tree_view_set_revisions     (GiggleDiffTreeView *view,
							    GiggleRevision     *from,
							    GiggleRevision     *to);

gchar *            giggle_diff_tree_view_get_selection     (GiggleDiffTreeView *view);
gboolean           giggle_diff_tree_view_get_compact_mode  (GiggleDiffTreeView *view);
void               giggle_diff_tree_view_set_compact_mode  (GiggleDiffTreeView *view,
							    gboolean            compact_mode);

G_END_DECLS

#endif /* __GIGGLE_DIFF_TREE_VIEW_H__ */
