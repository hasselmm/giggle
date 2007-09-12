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

#include <config.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "giggle-revision-tooltip.h"
#include "giggle-revision.h"
#include "giggle-ref.h"


typedef struct GiggleRevisionTooltipPriv GiggleRevisionTooltipPriv;

struct GiggleRevisionTooltipPriv {
	GiggleRevision *revision;
	GtkWidget      *label;
};

enum {
	PROP_0,
	PROP_REVISION,
};

static void revision_tooltip_finalize          (GObject *object);

static void revision_tooltip_get_property      (GObject          *object,
						guint             param_id,
						GValue           *value,
						GParamSpec       *pspec);
static void revision_tooltip_set_property      (GObject          *object,
						guint             param_id,
						const GValue     *value,
						GParamSpec       *pspec);

static gboolean revision_tooltip_expose        (GtkWidget        *widget,
						GdkEventExpose   *event);
static gboolean revision_tooltip_enter_notify  (GtkWidget        *widget,
						GdkEventCrossing *event);

G_DEFINE_TYPE (GiggleRevisionTooltip, giggle_revision_tooltip, GTK_TYPE_WINDOW)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REVISION_TOOLTIP, GiggleRevisionTooltipPriv))

static void
giggle_revision_tooltip_class_init (GiggleRevisionTooltipClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

	object_class->finalize = revision_tooltip_finalize;
	object_class->set_property = revision_tooltip_set_property;
	object_class->get_property = revision_tooltip_get_property;

	widget_class->expose_event = revision_tooltip_expose;
	widget_class->enter_notify_event = revision_tooltip_enter_notify;

	g_object_class_install_property (
		object_class,
		PROP_REVISION,
		g_param_spec_object ("revision",
				     "revision",
				     "Revision to show info about",
				     GIGGLE_TYPE_REVISION,
				     G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleRevisionTooltipPriv));
}

static void
giggle_revision_tooltip_init (GiggleRevisionTooltip *tooltip)
{
	GiggleRevisionTooltipPriv *priv;

	priv = GET_PRIV (tooltip);

	gtk_container_set_border_width (GTK_CONTAINER (tooltip), 3);
	gtk_window_set_resizable (GTK_WINDOW (tooltip), FALSE);

	gtk_window_set_type_hint (GTK_WINDOW (tooltip),
				  GDK_WINDOW_TYPE_HINT_TOOLTIP);

	gtk_container_set_reallocate_redraws (GTK_CONTAINER (tooltip), TRUE);

	priv->label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->label), 0., 0.);
	gtk_widget_show (priv->label);
	
	gtk_container_add (GTK_CONTAINER (tooltip), priv->label);
	gtk_widget_queue_resize (GTK_WIDGET (tooltip));
}

static void
revision_tooltip_finalize (GObject *object)
{
	GiggleRevisionTooltipPriv *priv;

	priv = GET_PRIV (object);

	if (priv->revision) {
		g_object_unref (priv->revision);
	}

	G_OBJECT_CLASS (giggle_revision_tooltip_parent_class)->finalize (object);
}

static void
revision_tooltip_get_property (GObject    *object,
			       guint       param_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
	GiggleRevisionTooltipPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_REVISION:
		g_value_set_object (value, priv->revision);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
revision_tooltip_set_property (GObject      *object,
			       guint         param_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
	GiggleRevisionTooltip     *tooltip;
	GiggleRevisionTooltipPriv *priv;

	tooltip = GIGGLE_REVISION_TOOLTIP (object);
	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_REVISION:
		giggle_revision_tooltip_set_revision (tooltip,
						      g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
revision_tooltip_expose (GtkWidget      *widget,
			 GdkEventExpose *event)
{
	GiggleRevisionTooltipPriv *priv;

	priv = GET_PRIV (widget);

	gtk_paint_flat_box (widget->style,
			    event->window,
			    GTK_STATE_NORMAL,
			    GTK_SHADOW_OUT,
			    NULL,
			    widget,
			    "tooltip",
			    0, 0,
			    widget->allocation.width,
			    widget->allocation.height);

	gtk_container_propagate_expose (GTK_CONTAINER (widget), priv->label, event);
	return FALSE;
}

static gboolean
revision_tooltip_enter_notify (GtkWidget        *widget,
			       GdkEventCrossing *event)
{
	GTK_WIDGET_CLASS (giggle_revision_tooltip_parent_class)->enter_notify_event (widget, event);
	gtk_widget_hide (widget);
	return FALSE;
}

GtkWidget *
giggle_revision_tooltip_new ()
{
	return g_object_new (GIGGLE_TYPE_REVISION_TOOLTIP,
			     "type", GTK_WINDOW_POPUP,
			     NULL);
}

static void
revision_tooltip_add_refs (GString *str,
			   gchar   *label,
			   GList   *list)
{
	GiggleRef *ref;

	while (list) {
		ref = list->data;
		list = list->next;

		g_string_append_printf (str,
					"<b>%s</b>: %s",
					label,
					giggle_ref_get_name (ref));

		if (list) {
			g_string_append (str, "\n");
		}
	}
}

void
giggle_revision_tooltip_set_revision (GiggleRevisionTooltip *tooltip,
				      GiggleRevision        *revision)
{
	GiggleRevisionTooltipPriv *priv;
	GList                     *branches, *tags, *remotes;
	GString                   *str;

	g_return_if_fail (GIGGLE_IS_REVISION_TOOLTIP (tooltip));

	priv = GET_PRIV (tooltip);

	if (priv->revision == revision) {
		return;
	}

	if (priv->revision) {
		g_object_unref (priv->revision);
	}

	priv->revision = g_object_ref (revision);
	str = g_string_new ("");

	branches = giggle_revision_get_branch_heads (revision);
	tags = giggle_revision_get_tags (revision);
	remotes = giggle_revision_get_remotes (revision);

	revision_tooltip_add_refs (str, _("Branch"), branches);

	if (branches && (tags || remotes)) {
		g_string_append (str, "\n");
	}

	revision_tooltip_add_refs (str, _("Tag"), tags);

	if ((branches || tags) && remotes) {
		g_string_append (str, "\n");
	}

	revision_tooltip_add_refs (str, _("Remote"), remotes);

	gtk_label_set_markup (GTK_LABEL (priv->label), str->str);
	g_object_notify (G_OBJECT (tooltip), "revision");

	g_string_free (str, TRUE);
}

GiggleRevision *
giggle_revision_tooltip_get_revision (GiggleRevisionTooltip *tooltip)
{
	GiggleRevisionTooltipPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION_TOOLTIP (tooltip), NULL);

	priv = GET_PRIV (tooltip);

	return priv->revision;
}
