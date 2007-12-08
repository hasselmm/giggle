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

#include "giggle-searchable.h"

GType
giggle_searchable_get_type (void)
{
  static GType searchable_type = 0;

  if (G_UNLIKELY (!searchable_type)) {
	  const GTypeInfo giggle_searchable_info = {
		  sizeof (GiggleSearchableIface), /* class_size */
		  NULL,         /* base_init */
		  NULL,		/* base_finalize */
		  NULL,
		  NULL,		/* class_finalize */
		  NULL,		/* class_data */
		  0,
		  0,            /* n_preallocs */
		  NULL
	  };

	  searchable_type =
		  g_type_register_static (G_TYPE_INTERFACE, "GiggleSearchable",
					  &giggle_searchable_info, 0);

	  g_type_interface_add_prerequisite (searchable_type, G_TYPE_OBJECT);
  }

  return searchable_type;
}

gboolean
giggle_searchable_search (GiggleSearchable      *searchable,
			  const gchar           *search_term,
			  GiggleSearchDirection  direction,
			  gboolean               full_search)
{
	GiggleSearchableIface *iface;
	gboolean result = FALSE;

	g_return_val_if_fail (GIGGLE_IS_SEARCHABLE (searchable), FALSE);
	g_return_val_if_fail (direction == GIGGLE_SEARCH_DIRECTION_NEXT ||
			      direction == GIGGLE_SEARCH_DIRECTION_PREV, FALSE);

	iface = GIGGLE_SEARCHABLE_GET_IFACE (searchable);

	if (iface->search) {
		gchar *casefold_search_term;

		casefold_search_term = g_utf8_casefold (search_term, -1);
		result = (* iface->search) (searchable, casefold_search_term,
					    direction, full_search);

		g_free (casefold_search_term);
	}

	return result;
}

void
giggle_searchable_cancel (GiggleSearchable *searchable)
{
	GiggleSearchableIface *iface;

	g_return_if_fail (GIGGLE_IS_SEARCHABLE (searchable));

	iface = GIGGLE_SEARCHABLE_GET_IFACE (searchable);

	if (iface->cancel) {
		(* iface->cancel) (searchable);
	}
}
