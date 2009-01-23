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

#include "giggle-history.h"

enum {
	HISTORY_CHANGED,
	HISTORY_RESET,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void
giggle_history_iface_init (gpointer iface,
			     gpointer iface_data)
{
	signals[HISTORY_CHANGED] =
		g_signal_new ("history-changed",
			      GIGGLE_TYPE_HISTORY,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GiggleHistoryIface, changed),
			      NULL, NULL, g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	signals[HISTORY_RESET] =
		g_signal_new ("history-reset",
			      GIGGLE_TYPE_HISTORY,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GiggleHistoryIface, reset),
			      NULL, NULL, g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

GType
giggle_history_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (!type)) {
		type = g_type_register_static_simple
			(G_TYPE_INTERFACE, "GiggleHistoryIface",
			 sizeof (GiggleHistoryIface),
			 giggle_history_iface_init, 0,
			 NULL, 0);

		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}

	return type;
}

GObject *
giggle_history_capture (GiggleHistory *history)
{
	GiggleHistoryIface *iface;

	g_return_val_if_fail (GIGGLE_IS_HISTORY (history), NULL);

	iface = GIGGLE_HISTORY_GET_IFACE (history);
	g_return_val_if_fail (NULL != iface->capture, NULL);

	return iface->capture (history);
}

void
giggle_history_restore (GiggleHistory *history,
			GObject       *snapshot)
{
	GiggleHistoryIface *iface;

	g_return_if_fail (GIGGLE_IS_HISTORY (history));

	iface = GIGGLE_HISTORY_GET_IFACE (history);
	g_return_if_fail (NULL != iface->restore);

	iface->restore (history, snapshot);
}

void
giggle_history_changed (GiggleHistory *history)
{
	g_return_if_fail (GIGGLE_IS_HISTORY (history));
	g_signal_emit (history, signals[HISTORY_CHANGED], 0);
}

void
giggle_history_reset (GiggleHistory *history)
{
	g_return_if_fail (GIGGLE_IS_HISTORY (history));
	g_signal_emit (history, signals[HISTORY_RESET], 0);
}
