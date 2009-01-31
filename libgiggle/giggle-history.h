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

#ifndef __GIGGLE_HISTORY_H__
#define __GIGGLE_HISTORY_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GIGGLE_TYPE_HISTORY            (giggle_history_get_type ())
#define GIGGLE_HISTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_HISTORY, GiggleHistory))
#define GIGGLE_IS_HISTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_HISTORY))
#define GIGGLE_HISTORY_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GIGGLE_TYPE_HISTORY, GiggleHistoryIface))

typedef struct _GiggleHistoryIface    GiggleHistoryIface;
typedef struct _GiggleHistory         GiggleHistory;          /* dummy */

struct _GiggleHistoryIface {
	GTypeInterface iface;

	/* vtable */
	GObject * (* capture)	(GiggleHistory *history);
	void      (* restore)	(GiggleHistory *history,
				 GObject       *snapshot);

	/* signals */
	void      (* changed)	(GiggleHistory *history);
	void      (* reset)	(GiggleHistory *history);
};

GType      giggle_history_get_type	(void) G_GNUC_CONST;

GObject *  giggle_history_capture	(GiggleHistory *history);

void       giggle_history_restore	(GiggleHistory *history,
					 GObject       *snapshot);

void       giggle_history_changed	(GiggleHistory *history);
void       giggle_history_reset		(GiggleHistory *history);

G_END_DECLS

#endif /* __GIGGLE_HISTORY_H__ */
