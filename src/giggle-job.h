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

#ifndef __GIGGLE_JOB_H__
#define __GIGGLE_JOB_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GIGGLE_TYPE_JOB            (giggle_job_get_type ())
#define GIGGLE_JOB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_JOB, GiggleJob))
#define GIGGLE_JOB_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), GIGGLE_TYPE_JOB, GiggleJobClass))
#define GIGGLE_IS_JOB(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_JOB))
#define GIGGLE_IS_JOB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIGGLE_TYPE_JOB))
#define GIGGLE_JOB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIGGLE_TYPE_JOB, GiggleJobClass))

typedef struct GiggleJob      GiggleJob;
typedef struct GiggleJobClass GiggleJobClass;

struct GiggleJob {
	GObject parent;
};

struct GiggleJobClass {
	GObjectClass parent_class;

	/* < Vtable > */
	gboolean   (* get_command_line)  (GiggleJob    *job,
					  gchar       **command_line);
	void       (* handle_output)     (GiggleJob    *job,
					  const gchar  *output_str,
					  gsize         output_len);
};

GType        giggle_job_get_type         (void);

gboolean     giggle_job_get_command_line (GiggleJob    *job,
					  gchar       **command_line);
void         giggle_job_handle_output    (GiggleJob    *job,
					  const gchar  *output_str,
					  gsize         output_len);

G_END_DECLS

#endif /* __GIGGLE_JOB_H__ */
