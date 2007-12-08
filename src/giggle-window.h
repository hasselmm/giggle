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

#ifndef __GIGGLE_WINDOW_H__
#define __GIGGLE_WINDOW_H__

#include <gtk/gtkwindow.h>
#include "libgiggle/giggle-git.h"

G_BEGIN_DECLS

#define GIGGLE_TYPE_WINDOW            (giggle_window_get_type ())
#define GIGGLE_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_WINDOW, GiggleWindow))
#define GIGGLE_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_WINDOW, GiggleWindowClass))
#define GIGGLE_IS_WINDOW(obj)	      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_WINDOW))
#define GIGGLE_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_WINDOW))
#define GIGGLE_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_WINDOW, GiggleWindowClass))

typedef struct GiggleWindow      GiggleWindow;
typedef struct GiggleWindowClass GiggleWindowClass;

struct GiggleWindow {
	GtkWindow parent_instance;
};

struct GiggleWindowClass {
	GtkWindowClass parent_class;
	void (*quitting) (GiggleWindow *window);
};

GType		   giggle_window_get_type      (void);
GtkWidget         *giggle_window_new           (void);
GiggleGit *        giggle_window_get_git       (GiggleWindow *self);
void               giggle_window_set_directory (GiggleWindow *self,
						gchar const  *dir);
void               giggle_window_show_diff_window (GiggleWindow *self);

G_END_DECLS

#endif /* __GIGGLE_WINDOW_H__ */
