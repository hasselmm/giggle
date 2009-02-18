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

#include <gio/gio.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

gboolean            giggle_list_view_delete_selection	  (GtkWidget         *treeview,
							   GdkEventKey       *event);

gboolean            giggle_tree_view_select_row_by_string (GtkWidget         *treeview,
							   int                column,
							   const char        *pattern);

GtkActionGroup *    giggle_ui_manager_get_action_group    (GtkUIManager      *manager,
							   const char        *group_name);

GAppLaunchContext * giggle_create_app_launch_context      (GtkWidget         *widget);

void                giggle_open_file_with_context         (GAppLaunchContext *context,
							   const char        *directory,
							   const char        *filename);

void                giggle_open_file                      (GtkWidget         *widget,
							   const char        *directory,
							   const char        *filename);

G_END_DECLS

#endif /* __GIGGLE_DUMMY_H__ */
