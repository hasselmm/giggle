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
#include "giggle-revision.h"

#include <time.h>

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REVISION, GiggleRevisionPriv))

enum {
	PROP_0,
	PROP_SHA,
	PROP_AUTHOR,
	PROP_COMMITTER,
	PROP_DATE,
	PROP_SHORT_LOG
};

typedef struct {
	char               *sha;
	struct tm          *date;
	GiggleAuthor       *author;
	GiggleAuthor       *committer;
	char               *short_log;

	GList              *descendent_branches;

	GList              *branch_heads;
	GList              *tags;
	GList              *remotes;

	GList              *parents;
	GList              *children;
}  GiggleRevisionPriv;

G_DEFINE_TYPE (GiggleRevision, giggle_revision, G_TYPE_OBJECT)

static void
revision_add_descendent_branch (GiggleRevision *revision,
				GiggleBranch   *branch);

static void
revision_set_property (GObject      *object,
		       guint         param_id,
		       const GValue *value,
		       GParamSpec   *pspec)
{
	GiggleRevisionPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_SHA:
		g_free (priv->sha);
		priv->sha = g_value_dup_string (value);
		break;

	case PROP_AUTHOR:
		if (priv->author)
			g_object_unref (priv->author);

		priv->author = g_value_dup_object (value);
		break;

	case PROP_COMMITTER:
		if (priv->committer)
			g_object_unref (priv->committer);

		priv->committer = g_value_dup_object (value);
		break;

	case PROP_DATE:
		g_free (priv->date);
		priv->date = g_value_get_pointer (value);
		break;

	case PROP_SHORT_LOG:
		g_free (priv->short_log);
		priv->short_log = g_value_dup_string (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
revision_get_property (GObject    *object,
		       guint       param_id,
		       GValue     *value,
		       GParamSpec *pspec)
{
	GiggleRevisionPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_SHA:
		g_value_set_string (value, priv->sha);
		break;

	case PROP_AUTHOR:
		g_value_set_object (value, priv->author);
		break;

	case PROP_COMMITTER:
		g_value_set_object (value, priv->committer);
		break;

	case PROP_DATE:
		g_value_set_pointer (value, priv->date);
		break;

	case PROP_SHORT_LOG:
		g_value_set_string (value, priv->short_log);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
revision_finalize (GObject *object)
{
	GiggleRevision     *revision;
	GiggleRevisionPriv *priv;

	revision = GIGGLE_REVISION (object);
	priv = GET_PRIV (revision);

	g_free (priv->sha);
	g_free (priv->short_log);

	if (priv->author)
		g_object_unref (priv->author);
	if (priv->committer)
		g_object_unref (priv->committer);
	if (priv->date)
		g_free (priv->date);

	g_list_free (priv->parents);
	g_list_free (priv->children);

	g_list_foreach (priv->branch_heads, (GFunc) g_object_unref, NULL);
	g_list_free (priv->branch_heads);

	g_list_foreach (priv->tags, (GFunc) g_object_unref, NULL);
	g_list_free (priv->tags);

	g_list_foreach (priv->remotes, (GFunc) g_object_unref, NULL);
	g_list_free (priv->remotes);

	g_list_free (priv->descendent_branches);

	G_OBJECT_CLASS (giggle_revision_parent_class)->finalize (object);
}

static void
giggle_revision_class_init (GiggleRevisionClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->set_property = revision_set_property;
	object_class->get_property = revision_get_property;
	object_class->finalize     = revision_finalize;

	g_object_class_install_property
		(object_class, PROP_SHA,
		 g_param_spec_string ("sha", "SHA",
				      "SHA hash of the revision",
				      NULL,
				      G_PARAM_READWRITE |
				      G_PARAM_STATIC_STRINGS |
				      G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property
		(object_class, PROP_AUTHOR,
		 g_param_spec_object ("author", "Author",
				      "Author of the revision",
				      GIGGLE_TYPE_AUTHOR,
				      G_PARAM_READWRITE |
				      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property
		(object_class, PROP_COMMITTER,
		 g_param_spec_object ("committer", "Committer",
				      "Committer of the revision",
				      GIGGLE_TYPE_AUTHOR,
				      G_PARAM_READWRITE |
				      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property
		(object_class, PROP_DATE,
		 g_param_spec_pointer ("date", "Date",
				       "Date of the revision",
				       G_PARAM_READWRITE |
				       G_PARAM_STATIC_STRINGS));

	g_object_class_install_property
		(object_class, PROP_SHORT_LOG,
		 g_param_spec_string ("short-log", "Short log",
				      "Short log of the revision",
				      NULL,
				      G_PARAM_READWRITE |
				      G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (object_class, sizeof (GiggleRevisionPriv));
}

static void
giggle_revision_init (GiggleRevision *revision)
{
}

GiggleRevision *
giggle_revision_new (const gchar *sha)
{
	return g_object_new (GIGGLE_TYPE_REVISION, "sha", sha, NULL);
}

const gchar *
giggle_revision_get_sha (GiggleRevision *revision)
{
	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);
	return GET_PRIV (revision)->sha;
}

GiggleAuthor *
giggle_revision_get_author (GiggleRevision *revision)
{
	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);
	return GET_PRIV (revision)->author;
}

void
giggle_revision_set_author (GiggleRevision *revision,
			    GiggleAuthor   *author)
{
	g_return_if_fail (GIGGLE_IS_REVISION (revision));
	g_return_if_fail (GIGGLE_IS_AUTHOR (author) | !author);
	g_object_set (revision, "author", author, NULL);
}

GiggleAuthor *
giggle_revision_get_committer (GiggleRevision *revision)
{
	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);
	return GET_PRIV (revision)->committer;
}

void
giggle_revision_set_committer (GiggleRevision *revision,
			       GiggleAuthor   *committer)
{
	g_return_if_fail (GIGGLE_IS_REVISION (revision));
	g_return_if_fail (GIGGLE_IS_AUTHOR (committer) | !committer);
	g_object_set (revision, "committer", committer, NULL);
}

const struct tm *
giggle_revision_get_date (GiggleRevision *revision)
{
	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);
	return GET_PRIV (revision)->date;
}

void
giggle_revision_set_date (GiggleRevision  *revision,
			  const struct tm *date)
{
	g_return_if_fail (GIGGLE_IS_REVISION (revision));
	g_return_if_fail (NULL != date);
	g_object_set (revision, "date", date, NULL);
}

const gchar *
giggle_revision_get_short_log (GiggleRevision *revision)
{
	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);
	return GET_PRIV (revision)->short_log;
}

void
giggle_revision_set_short_log (GiggleRevision *revision,
			       const char     *short_log)
{
	g_return_if_fail (GIGGLE_IS_REVISION (revision));
	g_object_set (revision, "short-log", short_log, NULL);
}

static void
revision_propagate_descendent_branches (GiggleRevision *revision,
					GiggleBranch   *branch)
{
	GList *parents;

	parents = giggle_revision_get_parents (revision);

	while (parents) {
		revision_add_descendent_branch (parents->data, branch);
		parents = parents->next;
	}
}

static void
revision_add_descendent_branch (GiggleRevision *revision,
				GiggleBranch   *branch)
{
	GiggleRevisionPriv *priv;

	g_return_if_fail (GIGGLE_IS_REVISION (revision));
	g_return_if_fail (GIGGLE_IS_REF (branch));

	priv = GET_PRIV (revision);

	if (!g_list_find (priv->descendent_branches, branch)) {
		priv->descendent_branches = g_list_prepend (priv->descendent_branches, branch);
		revision_propagate_descendent_branches (revision, branch);
	}
}

static void
giggle_revision_add_child (GiggleRevision *revision,
			   GiggleRevision *child)
{
	GiggleRevisionPriv *priv;
	GList              *branches;

	g_return_if_fail (GIGGLE_IS_REVISION (revision));
	g_return_if_fail (GIGGLE_IS_REVISION (child));

	priv = GET_PRIV (revision);

	priv->children = g_list_prepend (priv->children, child);
	branches = priv->descendent_branches;

	/* propagate descendent branches to new child */
	while (branches) {
		revision_add_descendent_branch (child, branches->data);
		branches = branches->next;
	}
}

static void
giggle_revision_remove_child (GiggleRevision *revision,
			      GiggleRevision *child)
{
	GiggleRevisionPriv *priv;

	g_return_if_fail (GIGGLE_IS_REVISION (revision));
	g_return_if_fail (GIGGLE_IS_REVISION (child));

	priv = GET_PRIV (revision);

	/* the child could have been added several times? */
	priv->children = g_list_remove_all (priv->children, child);
}

GList*
giggle_revision_get_parents (GiggleRevision *revision)
{
	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);
	return GET_PRIV (revision)->parents;
}

GList*
giggle_revision_get_children (GiggleRevision *revision)
{
	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);
	return GET_PRIV (revision)->children;
}

void
giggle_revision_add_parent (GiggleRevision *revision,
			    GiggleRevision *parent)
{
	GiggleRevisionPriv *priv;

	g_return_if_fail (GIGGLE_IS_REVISION (revision));
	g_return_if_fail (GIGGLE_IS_REVISION (parent));

	priv = GET_PRIV (revision);

	priv->parents = g_list_prepend (priv->parents, parent);
	giggle_revision_add_child (parent, revision);
}

void
giggle_revision_remove_parent (GiggleRevision *revision,
			       GiggleRevision *parent)
{
	GiggleRevisionPriv *priv;

	g_return_if_fail (GIGGLE_IS_REVISION (revision));
	g_return_if_fail (GIGGLE_IS_REVISION (parent));

	priv = GET_PRIV (revision);

	/* the parent could have been added several times? */
	priv->parents = g_list_remove_all (priv->parents, parent);
	giggle_revision_remove_child (parent, revision);
}

GList*
giggle_revision_get_branch_heads (GiggleRevision *revision)
{
	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);
	return GET_PRIV (revision)->branch_heads;
}

void
giggle_revision_add_branch_head (GiggleRevision *revision,
				 GiggleRef      *branch)
{
	GiggleRevisionPriv *priv;

	g_return_if_fail (GIGGLE_IS_REVISION (revision));
	g_return_if_fail (GIGGLE_IS_REF (branch));

	priv = GET_PRIV (revision);

	priv->branch_heads = g_list_prepend (priv->branch_heads,
					     g_object_ref (branch));

	revision_add_descendent_branch (revision, GIGGLE_BRANCH (branch));
}

GList *
giggle_revision_get_tags (GiggleRevision *revision)
{
	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);
	return GET_PRIV (revision)->tags;
}

void
giggle_revision_add_tag (GiggleRevision *revision,
			 GiggleRef      *tag)
{
	GiggleRevisionPriv *priv;

	g_return_if_fail (GIGGLE_IS_REVISION (revision));
	g_return_if_fail (GIGGLE_IS_REF (tag));

	priv = GET_PRIV (revision);

	priv->tags = g_list_prepend (priv->tags,
				     g_object_ref (tag));
}

GList *
giggle_revision_get_remotes (GiggleRevision *revision)
{
	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);
	return GET_PRIV (revision)->remotes;
}

void
giggle_revision_add_remote (GiggleRevision *revision,
			    GiggleRef      *remote)
{
	GiggleRevisionPriv *priv;

	g_return_if_fail (GIGGLE_IS_REVISION (revision));
	g_return_if_fail (GIGGLE_IS_REF (remote));

	priv = GET_PRIV (revision);

	priv->remotes = g_list_prepend (priv->remotes,
					g_object_ref (remote));
}

GList*
giggle_revision_get_descendent_branches (GiggleRevision *revision)
{
	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);
	return GET_PRIV (revision)->descendent_branches;
}

int
giggle_revision_compare (gconstpointer a,
                         gconstpointer b)
{
	if (!GIGGLE_IS_REVISION (a))
		return GIGGLE_IS_REVISION (b) ? -1 : 0;

	if (!GIGGLE_IS_REVISION (b))
		return 1;

	return g_strcmp0 (giggle_revision_get_sha (GIGGLE_REVISION (a)),
			  giggle_revision_get_sha (GIGGLE_REVISION (b)));
}

