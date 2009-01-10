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

#include <config.h>
#include <glib/gi18n.h>

#include "giggle-revision-info.h"

typedef struct GiggleRevisionInfoPriv GiggleRevisionInfoPriv;

struct GiggleRevisionInfoPriv {
	GiggleRevision *revision;

	GtkWidget      *label;
	GtkWidget      *sha1;
	GtkWidget      *date;
	GtkWidget      *summary;
};

enum {
	PROP_0,
	PROP_REVISION,
};

G_DEFINE_TYPE (GiggleRevisionInfo, giggle_revision_info, GTK_TYPE_VBOX)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REVISION_INFO, GiggleRevisionInfoPriv))

static void
revision_info_real_set_revision (GiggleRevisionInfo *info,
				 GiggleRevision     *revision)
{
	GiggleRevisionInfoPriv *priv;
	char			date[256] = "";
	const struct tm        *tm = NULL;


	priv = GET_PRIV (info);

	if (priv->revision) {
		g_object_unref (priv->revision);
		priv->revision = NULL;
	}

	if (revision) {
		priv->revision = g_object_ref (revision);
		tm = giggle_revision_get_date (priv->revision);

		gtk_label_set_text (GTK_LABEL (priv->summary),
				    giggle_revision_get_short_log (priv->revision));
		gtk_label_set_text (GTK_LABEL (priv->sha1),
				    giggle_revision_get_sha (priv->revision));

		if (tm)
			strftime (date, sizeof (date), "%c", tm);

		if (*date) {
			gtk_label_set_text (GTK_LABEL (priv->date), date);
			gtk_widget_show (priv->date);
		} else {
			gtk_widget_hide (priv->date);
		}

		gtk_widget_show (priv->sha1);
	} else {
		gtk_label_set_text (GTK_LABEL (priv->summary),
				    _("Uncommitted changes"));

		gtk_widget_hide (priv->sha1);
		gtk_widget_hide (priv->date);
	}
}

static void
revision_info_get_property (GObject    *object,
			   guint       param_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
	GiggleRevisionInfoPriv *priv;

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
revision_info_set_property (GObject      *object,
			   guint         param_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
	switch (param_id) {
	case PROP_REVISION:
		revision_info_real_set_revision (GIGGLE_REVISION_INFO (object),
						 g_value_get_object (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		return;
	}
}

static void
revision_info_dispose (GObject *object)
{
	GiggleRevisionInfoPriv *priv;

	priv = GET_PRIV (object);

	if (priv->revision) {
		g_object_unref (priv->revision);
		priv->revision = NULL;
	}

	G_OBJECT_CLASS (giggle_revision_info_parent_class)->dispose (object);
}

static void
giggle_revision_info_class_init (GiggleRevisionInfoClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->get_property = revision_info_get_property;
	object_class->set_property = revision_info_set_property;
	object_class->dispose      = revision_info_dispose;

	g_object_class_install_property (object_class,
					 PROP_REVISION,
					 g_param_spec_object ("revision",
							      "Revision",
							      "The git revsion to show",
							      GIGGLE_TYPE_REVISION,
							      G_PARAM_READWRITE));

	g_type_class_add_private (class, sizeof (GiggleRevisionInfoPriv));
}

static void
giggle_revision_info_init (GiggleRevisionInfo *info)
{
	GiggleRevisionInfoPriv *priv;
	GtkWidget *hbox;

	priv = GET_PRIV (info);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (info), hbox, FALSE, FALSE, 0);

	priv->label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->label), 0.0, 0.5);
	gtk_label_set_ellipsize (GTK_LABEL (priv->label), PANGO_ELLIPSIZE_END);
	gtk_box_pack_start (GTK_BOX (hbox), priv->label, TRUE, TRUE, 0);

	priv->sha1 = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->sha1), 1.0, 0.5);
	gtk_label_set_selectable (GTK_LABEL (priv->sha1), TRUE);
	gtk_box_pack_start (GTK_BOX (hbox), priv->sha1, FALSE, FALSE, 0);

	gtk_widget_show_all (hbox);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (info), hbox, FALSE, FALSE, 0);

	priv->summary = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->summary), 0.0, 0.5);
	gtk_label_set_selectable (GTK_LABEL (priv->summary), TRUE);
	gtk_label_set_ellipsize (GTK_LABEL (priv->summary), PANGO_ELLIPSIZE_END);
	gtk_box_pack_start (GTK_BOX (hbox), priv->summary, TRUE, TRUE, 0);

	priv->date = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->date), 1.0, 0.5);
	gtk_label_set_selectable (GTK_LABEL (priv->date), TRUE);
	gtk_box_pack_start (GTK_BOX (hbox), priv->date, FALSE, FALSE, 0);

	gtk_widget_show_all (hbox);
}

GtkWidget *
giggle_revision_info_new (void)
{
	return g_object_new (GIGGLE_TYPE_REVISION_INFO, NULL);
}

void
giggle_revision_info_set_revision (GiggleRevisionInfo *info,
				   GiggleRevision     *revision)
{
	g_return_if_fail (GIGGLE_IS_REVISION_INFO (info));
	g_return_if_fail (GIGGLE_IS_REVISION (revision) || !revision);
	g_object_set (info, "revision", revision, NULL);
}

GiggleRevision *
giggle_revision_info_get_revision (GiggleRevisionInfo *info)
{
	g_return_val_if_fail (GIGGLE_IS_REVISION_INFO (info), NULL);
	return GET_PRIV (info)->revision;
}

GtkWidget *
giggle_revision_info_get_label (GiggleRevisionInfo *info)
{
	g_return_val_if_fail (GIGGLE_IS_REVISION_INFO (info), NULL);
	return GET_PRIV (info)->label;
}

