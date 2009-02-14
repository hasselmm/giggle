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

#include "config.h"
#include "giggle-git-log.h"

#include <libgiggle/giggle-revision.h>

#include <string.h>

typedef struct GiggleGitLogPriv GiggleGitLogPriv;

struct GiggleGitLogPriv {
	GiggleRevision *revision;
	gchar          *log;
};

static void     git_log_finalize            (GObject           *object);
static void     git_log_get_property        (GObject           *object,
					     guint              param_id,
					     GValue            *value,
					     GParamSpec        *pspec);
static void     git_log_set_property        (GObject           *object,
					     guint              param_id,
					     const GValue      *value,
					     GParamSpec        *pspec);

static gboolean git_log_get_command_line    (GiggleJob         *job,
					     gchar            **command_line);
static void     git_log_handle_output       (GiggleJob         *job,
					     const gchar       *output_str,
					     gsize              output_len);

enum {
	PROP_0,
	PROP_REVISION,
};

G_DEFINE_TYPE (GiggleGitLog, giggle_git_log, GIGGLE_TYPE_JOB)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_LOG, GiggleGitLogPriv))

static void
giggle_git_log_class_init (GiggleGitLogClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GiggleJobClass *job_class    = GIGGLE_JOB_CLASS (class);

	object_class->finalize     = git_log_finalize;
	object_class->get_property = git_log_get_property;
	object_class->set_property = git_log_set_property;

	job_class->get_command_line = git_log_get_command_line;
	job_class->handle_output    = git_log_handle_output;

	g_object_class_install_property (object_class,
					 PROP_REVISION,
					 g_param_spec_object ("revision",
							      "revision",
							      "Revision",
							      GIGGLE_TYPE_REVISION,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private (object_class, sizeof (GiggleGitLogPriv));
}

static void
giggle_git_log_init (GiggleGitLog *log)
{
}

static void
git_log_finalize (GObject *object)
{
	GiggleGitLogPriv *priv;

	priv = GET_PRIV (object);
	g_object_unref (priv->revision);
	g_free (priv->log);

	G_OBJECT_CLASS (giggle_git_log_parent_class)->finalize (object);
}

static void
git_log_get_property (GObject    *object,
		      guint       param_id,
		      GValue     *value,
		      GParamSpec *pspec)
{
	GiggleGitLogPriv *priv;

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
git_log_set_property (GObject      *object,
		      guint         param_id,
		      const GValue *value,
		      GParamSpec   *pspec)
{
	GiggleGitLogPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_REVISION:
		priv->revision = g_value_dup_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
git_log_get_command_line (GiggleJob  *job,
			  gchar     **command_line)
{
	GiggleGitLogPriv *priv;
	GString          *str;
	const gchar      *sha;

	priv = GET_PRIV (job);
	sha = giggle_revision_get_sha (priv->revision);
	str = g_string_new (GIT_COMMAND);
	g_string_append_printf (str, " rev-list --pretty=raw %s^..%s", sha, sha);

	*command_line = g_string_free (str, FALSE);
	return TRUE;
}

static gchar *
git_log_parse_log (const gchar *output)
{
	gint      i = 0;
	gchar   **lines;
	GString  *long_log;

	lines = g_strsplit (output, "\n", -1);
	long_log = g_string_new ("");

	while (lines[i]) {
		gchar* converted = NULL;

		if (g_utf8_validate (lines[i], -1, NULL)) {
			converted = g_strdup (lines[i]);
		}

		if (!converted) {
			converted = g_locale_to_utf8 (lines[i], -1,
						      NULL, NULL,
						      NULL); // FIXME: add GError
		}

		if (!converted) {
			converted = g_filename_to_utf8 (lines[i], -1,
							NULL, NULL,
							NULL); // FIXME: add GError
		}

		if (!converted) {
			converted = g_convert (lines[i], -1,
					       "UTF-8", "ISO-8859-15",
					       NULL, NULL, NULL); // FIXME: add GError
		}

		if (!converted) {
			converted = g_strescape (lines[i], "\n\r\\\"\'");
		}

		if (!converted) {
			g_warning ("Error while converting string");
			// FIXME: fallback
		}

		if (g_str_has_prefix (converted, " ")) {
			g_strstrip (converted);
			g_string_append_printf (long_log, "%s\n", converted);
		}

		g_free (converted);

		i++;
	}

	return g_string_free (long_log, FALSE);
}

static void
git_log_handle_output (GiggleJob   *job,
		       const gchar *output_str,
		       gsize        output_len)
{
	GiggleGitLogPriv  *priv;
	gchar            **lines;

	priv = GET_PRIV (job);

	lines = g_strsplit (output_str, "\n", -1);
	priv->log = git_log_parse_log (output_str);
	g_strfreev (lines);
}

GiggleJob *
giggle_git_log_new (GiggleRevision *revision)
{
	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);

	return g_object_new (GIGGLE_TYPE_GIT_LOG,
			     "revision", revision,
			     NULL);
}

G_CONST_RETURN gchar *
giggle_git_log_get_log (GiggleGitLog *log)
{
	GiggleGitLogPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_LOG (log), NULL);

	priv = GET_PRIV (log);

	return priv->log;
}
