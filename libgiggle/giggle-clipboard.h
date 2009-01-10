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

#ifndef __GIGGLE_CLIPBOARD_H__
#define __GIGGLE_CLIPBOARD_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GIGGLE_TYPE_CLIPBOARD            (giggle_clipboard_get_type ())
#define GIGGLE_CLIPBOARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIGGLE_TYPE_CLIPBOARD, GiggleClipboard))
#define GIGGLE_IS_CLIPBOARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIGGLE_TYPE_CLIPBOARD))
#define GIGGLE_CLIPBOARD_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GIGGLE_TYPE_CLIPBOARD, GiggleClipboardIface))

typedef struct GiggleClipboardIface GiggleClipboardIface;
typedef struct GiggleClipboard      GiggleClipboard; /* dummy */

struct GiggleClipboardIface {
	GTypeInterface iface;

	/* vtable */
	void     (* do_cut)	(GiggleClipboard     *clipboard);
	gboolean (* can_cut)	(GiggleClipboard     *clipboard);

	void     (* do_copy)	(GiggleClipboard     *clipboard);
	gboolean (* can_copy)	(GiggleClipboard     *clipboard);

	void     (* do_paste)	(GiggleClipboard     *clipboard);
	gboolean (* can_paste)	(GiggleClipboard     *clipboard);

	void     (* do_delete)	(GiggleClipboard     *clipboard);
	gboolean (* can_delete)	(GiggleClipboard     *clipboard);

	/* signals */
	void     (* clipboard_changed) (GiggleClipboard     *clipboard);
};


GType      giggle_clipboard_get_type	(void);

void       giggle_clipboard_cut		(GiggleClipboard *clipboard);
gboolean   giggle_clipboard_can_cut	(GiggleClipboard *clipboard);

void       giggle_clipboard_copy	(GiggleClipboard *clipboard);
gboolean   giggle_clipboard_can_copy	(GiggleClipboard *clipboard);

void       giggle_clipboard_paste	(GiggleClipboard *clipboard);
gboolean   giggle_clipboard_can_paste	(GiggleClipboard *clipboard);

void       giggle_clipboard_delete	(GiggleClipboard *clipboard);
gboolean   giggle_clipboard_can_delete	(GiggleClipboard *clipboard);

void       giggle_clipboard_changed	(GiggleClipboard *clipboard);

G_END_DECLS

#endif /* __GIGGLE_CLIPBOARD_H__ */
