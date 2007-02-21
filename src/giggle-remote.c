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

#include <string.h>
#include "giggle-remote.h"

typedef struct GiggleRemotePriv GiggleRemotePriv;

struct GiggleRemotePriv {
	gchar *name;
	gchar *url;
};

enum {
	PROP_0,
	PROP_NAME,
	PROP_URL
};

static void     remote_finalize            (GObject           *object);
static void     remote_get_property        (GObject           *object,
					   guint              param_id,
					   GValue            *value,
					   GParamSpec        *pspec);
static void     remote_set_property        (GObject           *object,
					   guint              param_id,
					   const GValue      *value,
					   GParamSpec        *pspec);

G_DEFINE_TYPE (GiggleRemote, giggle_remote, G_TYPE_OBJECT)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REMOTE, GiggleRemotePriv))

static void
giggle_remote_class_init (GiggleRemoteClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize     = remote_finalize;
	object_class->get_property = remote_get_property;
	object_class->set_property = remote_set_property;

	g_object_class_install_property (object_class,
					 PROP_NAME,
					 g_param_spec_string ("name", "Name",
							      "This remote's name",
							      NULL, G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_URL,
					 g_param_spec_string ("url", "URL",
							      "This remote's URL",
							      NULL, G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleRemotePriv));
}

static void
giggle_remote_init (GiggleRemote *remote)
{
	GiggleRemotePriv *priv;

	priv = GET_PRIV (remote);
}

static void
remote_finalize (GObject *object)
{
	GiggleRemotePriv *priv;

	priv = GET_PRIV (object);
	
	g_free (priv->name);
	g_free (priv->url);

	G_OBJECT_CLASS (giggle_remote_parent_class)->finalize (object);
}

static void
remote_get_property (GObject    *object,
		    guint       param_id,
		    GValue     *value,
		    GParamSpec *pspec)
{
	GiggleRemotePriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_NAME:
		g_value_set_string (value, priv->name);
		break;
	case PROP_URL:
		g_value_set_string (value, priv->url);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
remote_set_property (GObject      *object,
		    guint         param_id,
		    const GValue *value,
		    GParamSpec   *pspec)
{
	GiggleRemotePriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_NAME:
		priv->name = g_value_dup_string (value);
		break;
	case PROP_URL:
		giggle_remote_set_url (GIGGLE_REMOTE (object), g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

GiggleRemote *
giggle_remote_new (gchar const *name)
{
	return g_object_new (GIGGLE_TYPE_REMOTE, "name", name, NULL);
}

static void
remote_add_branch (GiggleRemote       *remote,
		   GiggleRemoteBranch *branch)
{
	g_warning ("FIXME: implement %s", __PRETTY_FUNCTION__);
}

GiggleRemote *
giggle_remote_new_from_file (gchar const *filename)
{
	GiggleRemote *remote;
	gchar        *content;

	content = g_path_get_basename (filename);
	remote = giggle_remote_new (content);
	g_free (content);
	content = NULL;

	if(g_file_get_contents (filename, &content, NULL, NULL)) {
		gchar**lines;
		gchar**step;
		lines = g_strsplit (content, "\n", -1);
		for (step = lines; step && *step; step++) {
			GiggleRemoteBranch* branch = NULL;
			if(!**step) {
				/* empty string */
				continue;
			} else if(g_str_has_prefix(*step, "URL: ")) {
				giggle_remote_set_url (remote, *step + strlen ("URL: "));
			} else if(g_str_has_prefix(*step, "Push: ")) {
				branch = giggle_remote_branch_new (GIGGLE_REMOTE_DIRECTION_PUSH,
								   *step + strlen ("Push: "));
			} else if(g_str_has_prefix(*step, "Pull: ")) {
				branch = giggle_remote_branch_new (GIGGLE_REMOTE_DIRECTION_PULL,
								   *step + strlen ("Pull: "));
			} else {
				gchar* escaped = g_strescape (*step, NULL);
				g_warning ("Read unexpected line at %s:%d\n\"%s\"",
					   filename, step - lines, escaped);
				g_free (escaped);
			}

			if(GIGGLE_IS_REMOTE_BRANCH (branch)) {
				remote_add_branch (remote, branch);
				g_object_unref (branch);
				branch = NULL;
			}
		}
		g_strfreev (lines);
	}
	g_free (content);

	return remote;
}

const gchar *
giggle_remote_get_name (GiggleRemote *remote)
{
	g_return_val_if_fail (GIGGLE_IS_REMOTE (remote), NULL);

	return GET_PRIV (remote)->name;
}

const gchar *
giggle_remote_get_url (GiggleRemote *remote)
{
	g_return_val_if_fail (GIGGLE_IS_REMOTE (remote), NULL);

	return GET_PRIV (remote)->url;
}

void
giggle_remote_set_url (GiggleRemote *remote,
		       const gchar  *url)
{
	GiggleRemotePriv *priv;

	g_return_if_fail (GIGGLE_IS_REMOTE (remote));
	priv = GET_PRIV (remote);

	if(priv->url == url) {
		return;
	}

	g_free (priv->url);
	priv->url = g_strdup (url);

	g_object_notify (G_OBJECT (remote), "url");
}

