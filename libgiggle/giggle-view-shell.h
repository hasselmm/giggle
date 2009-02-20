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

#ifndef __GIGGLE_VIEW_SHELL_H__
#define __GIGGLE_VIEW_SHELL_H__

#include "giggle-view.h"

G_BEGIN_DECLS

#define GIGGLE_TYPE_VIEW_SHELL            (giggle_view_shell_get_type ())
#define GIGGLE_VIEW_SHELL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_VIEW_SHELL, GiggleViewShell))
#define GIGGLE_VIEW_SHELL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_VIEW_SHELL, GiggleViewShellClass))
#define GIGGLE_IS_VIEW_SHELL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_VIEW_SHELL))
#define GIGGLE_IS_VIEW_SHELL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_VIEW_SHELL))
#define GIGGLE_VIEW_SHELL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_VIEW_SHELL, GiggleViewShellClass))

typedef struct GiggleViewShell       GiggleViewShell;
typedef struct GiggleViewShellClass  GiggleViewShellClass;

struct GiggleViewShell {
	GtkNotebook parent;
};

struct GiggleViewShellClass {
	GtkNotebookClass parent_class;
};

GType          giggle_view_shell_get_type        (void);

GtkWidget *    giggle_view_shell_new             (void);
GtkWidget *    giggle_view_shell_new_with_ui     (GtkUIManager    *manager,
						  const char      *group_name);

void           giggle_view_shell_add_placeholder (GiggleViewShell *shell,
						  const char      *path);

void           giggle_view_shell_append_view     (GiggleViewShell *shell,
						  GiggleView      *view);

void           giggle_view_shell_set_view_name   (GiggleViewShell *shell,
						  const char      *name);
const char *   giggle_view_shell_get_view_name   (GiggleViewShell *shell);

void           giggle_view_shell_set_group_name   (GiggleViewShell *shell,
						   const char      *name);
const char *   giggle_view_shell_get_group_name  (GiggleViewShell *shell);

void           giggle_view_shell_set_ui_manager  (GiggleViewShell *shell,
						  GtkUIManager    *ui_manager);
GtkUIManager * giggle_view_shell_get_ui_manager  (GiggleViewShell *shell);

GiggleView *   giggle_view_shell_find_view       (GiggleViewShell *shell,
						  GType            type);
gboolean       giggle_view_shell_select          (GiggleViewShell *shell,
						  GiggleView      *view);
GiggleView *   giggle_view_shell_get_selected    (GiggleViewShell *shell);

G_END_DECLS

#endif /* __GIGGLE_VIEW_SHELL_H__ */
