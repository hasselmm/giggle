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

#ifndef __GIGGLE_GIT_CONFIG_H__
#define __GIGGLE_GIT_CONFIG_H__

#include <libgiggle/giggle-job.h>

G_BEGIN_DECLS

#define GIGGLE_TYPE_GIT_CONFIG            (giggle_git_config_get_type ())
#define GIGGLE_GIT_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_GIT_CONFIG, GiggleGitConfig))
#define GIGGLE_GIT_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIGGLE_TYPE_GIT_CONFIG, GiggleGitConfigClass))
#define GIGGLE_IS_GIT_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_GIT_CONFIG))
#define GIGGLE_IS_GIT_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIGGLE_TYPE_GIT_CONFIG))
#define GIGGLE_GIT_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIGGLE_TYPE_GIT_CONFIG, GiggleGitConfigClass))

typedef struct GiggleGitConfig      GiggleGitConfig;
typedef struct GiggleGitConfigClass GiggleGitConfigClass;

typedef void (*GiggleGitConfigFunc) (GiggleGitConfig *config,
				     gboolean         sucess,
				     gpointer         user_data);

struct GiggleGitConfig {
	GObject   parent_instance;
};

struct GiggleGitConfigClass {
	GObjectClass parent_class;

	void (*changed) (GiggleGitConfig *config);
};

typedef enum {
	GIGGLE_GIT_CONFIG_FIELD_NAME,
	GIGGLE_GIT_CONFIG_FIELD_EMAIL,
	GIGGLE_GIT_CONFIG_FIELD_MAIN_WINDOW_MAXIMIZED,
	GIGGLE_GIT_CONFIG_FIELD_MAIN_WINDOW_GEOMETRY,
	GIGGLE_GIT_CONFIG_FIELD_MAIN_WINDOW_VIEW,
	GIGGLE_GIT_CONFIG_FIELD_SHOW_GRAPH,
	GIGGLE_GIT_CONFIG_FIELD_FILE_VIEW_PATH,
	GIGGLE_GIT_CONFIG_FIELD_FILE_VIEW_HPANE_POSITION,
	GIGGLE_GIT_CONFIG_FIELD_FILE_VIEW_VPANE_POSITION,
	GIGGLE_GIT_CONFIG_FIELD_HISTORY_VIEW_VPANE_POSITION,
} GiggleGitConfigField;

GType                   giggle_git_config_get_type          (void);
GiggleGitConfig *       giggle_git_config_new               (void);

void                    giggle_git_config_update            (GiggleGitConfig      *config,
							     GiggleGitConfigFunc   func,
							     gpointer              data);
void                    giggle_git_config_commit            (GiggleGitConfig      *config,
							     GiggleGitConfigFunc   func,
							     gpointer              data);

G_CONST_RETURN gchar *  giggle_git_config_get_field         (GiggleGitConfig      *config,
							     GiggleGitConfigField  field);
int                     giggle_git_config_get_int_field     (GiggleGitConfig      *config,
							     GiggleGitConfigField  field);
gboolean                giggle_git_config_get_boolean_field (GiggleGitConfig      *config,
							     GiggleGitConfigField  field);
void                    giggle_git_config_set_field         (GiggleGitConfig      *config,
							     GiggleGitConfigField  field,
							     const gchar          *value);
void                    giggle_git_config_set_int_field     (GiggleGitConfig      *config,
							     GiggleGitConfigField  field,
							     int                   value);
void                    giggle_git_config_set_boolean_field (GiggleGitConfig      *config,
							     GiggleGitConfigField  field,
							     gboolean              value);

void                    giggle_git_config_bind              (GiggleGitConfig      *config,
							     GiggleGitConfigField  field,
							     GObject              *object,
							     const char           *property);

G_END_DECLS

#endif /* __GIGGLE_GIT_CONFIG_H__ */
