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

#include "giggle-clipboard.h"

enum {
	CLIPBOARD_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void
giggle_clipboard_iface_init (gpointer iface,
			     gpointer iface_data)
{
	signals[CLIPBOARD_CHANGED] =
		g_signal_new ("clipboard-changed",
			      GIGGLE_TYPE_CLIPBOARD,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GiggleClipboardIface, clipboard_changed),
			      NULL, NULL, g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

GType
giggle_clipboard_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (!type)) {
		type = g_type_register_static_simple
			(G_TYPE_INTERFACE, "GiggleClipboardIface",
			 sizeof (GiggleClipboardIface),
			 giggle_clipboard_iface_init, 0,
			 NULL, 0);

		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}

	return type;
}

void
giggle_clipboard_cut (GiggleClipboard *clipboard)
{
	GiggleClipboardIface *iface;

	g_return_if_fail (GIGGLE_IS_CLIPBOARD (clipboard));
	iface = GIGGLE_CLIPBOARD_GET_IFACE (clipboard);

	if (iface->do_cut)
		iface->do_cut (clipboard);
}

gboolean
giggle_clipboard_can_cut (GiggleClipboard *clipboard)
{
	GiggleClipboardIface *iface;

	g_return_val_if_fail (GIGGLE_IS_CLIPBOARD (clipboard), FALSE);
	iface = GIGGLE_CLIPBOARD_GET_IFACE (clipboard);

	if (iface->can_cut)
		return iface->can_cut (clipboard);

	return FALSE;
}

void
giggle_clipboard_copy (GiggleClipboard *clipboard)
{
	GiggleClipboardIface *iface;

	g_return_if_fail (GIGGLE_IS_CLIPBOARD (clipboard));
	iface = GIGGLE_CLIPBOARD_GET_IFACE (clipboard);

	if (iface->do_copy)
		iface->do_copy (clipboard);
}

gboolean
giggle_clipboard_can_copy (GiggleClipboard *clipboard)
{
	GiggleClipboardIface *iface;

	g_return_val_if_fail (GIGGLE_IS_CLIPBOARD (clipboard), FALSE);
	iface = GIGGLE_CLIPBOARD_GET_IFACE (clipboard);

	if (iface->can_copy)
		return iface->can_copy (clipboard);

	return FALSE;
}

void
giggle_clipboard_paste (GiggleClipboard *clipboard)
{
	GiggleClipboardIface *iface;

	g_return_if_fail (GIGGLE_IS_CLIPBOARD (clipboard));
	iface = GIGGLE_CLIPBOARD_GET_IFACE (clipboard);

	if (iface->do_paste)
		iface->do_paste (clipboard);
}

gboolean
giggle_clipboard_can_paste (GiggleClipboard *clipboard)
{
	GiggleClipboardIface *iface;

	g_return_val_if_fail (GIGGLE_IS_CLIPBOARD (clipboard), FALSE);
	iface = GIGGLE_CLIPBOARD_GET_IFACE (clipboard);

	if (iface->can_paste)
		return iface->can_paste (clipboard);

	return FALSE;
}

void
giggle_clipboard_delete (GiggleClipboard *clipboard)
{
	GiggleClipboardIface *iface;

	g_return_if_fail (GIGGLE_IS_CLIPBOARD (clipboard));
	iface = GIGGLE_CLIPBOARD_GET_IFACE (clipboard);

	if (iface->do_delete)
		iface->do_delete (clipboard);
}

gboolean
giggle_clipboard_can_delete (GiggleClipboard *clipboard)
{
	GiggleClipboardIface *iface;

	g_return_val_if_fail (GIGGLE_IS_CLIPBOARD (clipboard), FALSE);
	iface = GIGGLE_CLIPBOARD_GET_IFACE (clipboard);

	if (iface->can_delete)
		return iface->can_delete (clipboard);

	return FALSE;
}

void
giggle_clipboard_changed (GiggleClipboard *clipboard)
{
	g_return_if_fail (GIGGLE_IS_CLIPBOARD (clipboard));
	g_signal_emit (clipboard, signals[CLIPBOARD_CHANGED], 0);
}
