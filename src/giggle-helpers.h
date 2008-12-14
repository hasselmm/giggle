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

#ifndef __GIGGLE_HELPERS_H__
#define __GIGGLE_HELPERS_H__

#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

/* returns TRUE if the key press was delete and at least one row has been
 * deleted */
gboolean tree_view_delete_selection_on_list_store (GtkWidget   *treeview,
						   GdkEventKey *event);

gboolean tree_view_select_row_by_string           (GtkWidget   *treeview,
						   int          column,
						   const char  *pattern);

G_END_DECLS

#endif /* __GIGGLE_DUMMY_H__ */
