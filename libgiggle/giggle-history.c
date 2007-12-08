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

#include "giggle-history.h"

enum {
	HISTORY_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void  giggle_history_base_init    (gpointer  g_class);

GType
giggle_history_get_type (void)
{
  static GType history_type = 0;

  if (G_UNLIKELY (!history_type)) {
	  const GTypeInfo giggle_history_info = {
		  sizeof (GiggleHistoryIface), /* class_size */
		  giggle_history_base_init,    /* base_init */
		  NULL,		/* base_finalize */
		  NULL,
		  NULL,		/* class_finalize */
		  NULL,		/* class_data */
		  0,
		  0,            /* n_preallocs */
		  NULL
	  };

	  history_type =
		  g_type_register_static (G_TYPE_INTERFACE, "GiggleHistory",
					  &giggle_history_info, 0);

	  g_type_interface_add_prerequisite (history_type, G_TYPE_OBJECT);
  }

  return history_type;
}

static void
giggle_history_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (initialized) {
		return;
	}

	signals[HISTORY_CHANGED] =
		g_signal_new ("history-changed",
			      GIGGLE_TYPE_HISTORY,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GiggleHistoryIface, history_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	initialized = TRUE;
}

void
giggle_history_go_back (GiggleHistory *history)
{
	GiggleHistoryIface *iface;

	g_return_if_fail (GIGGLE_IS_HISTORY (history));

	iface = GIGGLE_HISTORY_GET_IFACE (history);

	if (iface->go_back) {
		(iface->go_back) (history);
	}
}

gboolean
giggle_history_can_go_back (GiggleHistory *history)
{
	GiggleHistoryIface *iface;

	g_return_val_if_fail (GIGGLE_IS_HISTORY (history), FALSE);

	iface = GIGGLE_HISTORY_GET_IFACE (history);

	if (iface->can_go_back) {
		return (iface->can_go_back) (history);
	}

	return FALSE;
}

void
giggle_history_go_forward (GiggleHistory *history)
{
	GiggleHistoryIface *iface;

	g_return_if_fail (GIGGLE_IS_HISTORY (history));

	iface = GIGGLE_HISTORY_GET_IFACE (history);

	if (iface->go_forward) {
		(iface->go_forward) (history);
	}
}

gboolean
giggle_history_can_go_forward (GiggleHistory *history)
{
	GiggleHistoryIface *iface;

	g_return_val_if_fail (GIGGLE_IS_HISTORY (history), FALSE);

	iface = GIGGLE_HISTORY_GET_IFACE (history);

	if (iface->can_go_forward) {
		return (iface->can_go_forward) (history);
	}

	return FALSE;
}

void
giggle_history_changed (GiggleHistory *history)
{
	g_return_if_fail (GIGGLE_IS_HISTORY (history));

	g_signal_emit (history, signals[HISTORY_CHANGED], 0);
}
