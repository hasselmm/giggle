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

#ifndef __GIGGLE_CONFIGURATION_H__
#define __GIGGLE_CONFIGURATION_H__

#include <glib-object.h>

#include "giggle-job.h"

G_BEGIN_DECLS

#define GIGGLE_TYPE_CONFIGURATION            (giggle_configuration_get_type ())
#define GIGGLE_CONFIGURATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_CONFIGURATION, GiggleConfiguration))
#define GIGGLE_CONFIGURATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIGGLE_TYPE_CONFIGURATION, GiggleConfigurationClass))
#define GIGGLE_IS_CONFIGURATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_CONFIGURATION))
#define GIGGLE_IS_CONFIGURATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIGGLE_TYPE_CONFIGURATION))
#define GIGGLE_CONFIGURATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIGGLE_TYPE_CONFIGURATION, GiggleConfigurationClass))

typedef struct GiggleConfiguration      GiggleConfiguration;
typedef struct GiggleConfigurationClass GiggleConfigurationClass;
typedef enum   GiggleConfigurationField GiggleConfigurationField;

typedef void (*GiggleConfigurationFunc) (GiggleConfiguration *configuration,
					 gboolean             sucess,
					 gpointer             user_data);

struct GiggleConfiguration {
	GObject   parent_instance;
};

struct GiggleConfigurationClass {
	GObjectClass parent_class;

	void (*changed) (GiggleConfiguration *configuration);
};

enum GiggleConfigurationField {
	CONFIG_FIELD_NAME,
	CONFIG_FIELD_EMAIL,
	CONFIG_FIELD_MAIN_WINDOW_MAXIMIZED,
	CONFIG_FIELD_MAIN_WINDOW_GEOMETRY,
	CONFIG_FIELD_COMPACT_MODE
};


GType	                giggle_configuration_get_type      (void);
GiggleConfiguration *   giggle_configuration_new           (void);

void                    giggle_configuration_update        (GiggleConfiguration      *configuration,
							    GiggleConfigurationFunc   func,
							    gpointer                  data);

G_CONST_RETURN gchar *  giggle_configuration_get_field     (GiggleConfiguration      *configuration,
							    GiggleConfigurationField  field);
gboolean                giggle_configuration_get_boolean_field (GiggleConfiguration      *configuration,
								GiggleConfigurationField  field);

void                    giggle_configuration_set_field     (GiggleConfiguration      *configuration,
							    GiggleConfigurationField  field,
							    const gchar              *value);
void                    giggle_configuration_set_boolean_field (GiggleConfiguration      *configuration,
								GiggleConfigurationField  field,
								gboolean                  value);
void                    giggle_configuration_commit        (GiggleConfiguration      *configuration,
							    GiggleConfigurationFunc   func,
							    gpointer                  data);

G_END_DECLS

#endif /* __GIGGLE_CONFIGURATION_H__ */
