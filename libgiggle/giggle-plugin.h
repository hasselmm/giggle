/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2009 Mathias Hasselmann
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

#ifndef __GIGGLE_PLUGIN_H__
#define __GIGGLE_PLUGIN_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GIGGLE_TYPE_PLUGIN            (giggle_plugin_get_type ())
#define GIGGLE_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_PLUGIN, GigglePlugin))
#define GIGGLE_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_PLUGIN, GigglePluginClass))
#define GIGGLE_IS_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_PLUGIN))
#define GIGGLE_IS_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_PLUGIN))
#define GIGGLE_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_PLUGIN, GigglePluginClass))

#define GIGGLE_PLUGIN_ERROR           (giggle_plugin_error_quark ())

typedef struct GigglePlugin      	GigglePlugin;
typedef struct GigglePluginClass	GigglePluginClass;
typedef struct GigglePluginManager      GigglePluginManager;

typedef enum {
	GIGGLE_PLUGIN_ERROR_NONE,
	GIGGLE_PLUGIN_ERROR_MALFORMED,
} GigglePluginError;

struct GigglePlugin {
	GObject parent_instance;
};

struct GigglePluginClass {
	GObjectClass parent_class;
};

GQuark			giggle_plugin_error_quark	(void) G_GNUC_CONST;

GType			giggle_plugin_get_type		(void) G_GNUC_CONST;

GigglePlugin *		giggle_plugin_new_from_file	(const char         *filename,
						 	 GError            **error);

const char *		giggle_plugin_get_name		(GigglePlugin        *plugin);

void			giggle_plugin_set_builder	(GigglePlugin        *plugin,
							 GtkBuilder          *builder);
GtkBuilder *		giggle_plugin_get_builder	(GigglePlugin        *plugin);

void			giggle_plugin_set_filename	(GigglePlugin        *plugin,
							 const char          *filename);
const char *		giggle_plugin_get_filename	(GigglePlugin        *plugin);

void			giggle_plugin_set_description	(GigglePlugin        *plugin,
							 const char          *description);
const char *		giggle_plugin_get_description	(GigglePlugin        *plugin);

void			giggle_plugin_set_manager	(GigglePlugin        *plugin,
							 GigglePluginManager *description);
GigglePluginManager *	giggle_plugin_get_manager 	(GigglePlugin        *plugin);

guint			giggle_plugin_merge_ui		(GigglePlugin        *plugin,
							 GtkUIManager        *ui,
							 GError             **error);

G_END_DECLS

#endif /* __GIGGLE_PLUGIN_H__ */
