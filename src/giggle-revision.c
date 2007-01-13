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
#include <gtk/gtk.h>

#include "giggle-revision.h"

typedef struct GiggleRevisionPriv GiggleRevisionPriv;

struct GiggleRevisionPriv {
	GiggleRevisionType  type;
	
	gchar              *sha;
	gchar              *author;
	gchar              *date; /* FIXME: shouldn't be a string. */
	gchar              *short_log;
	gchar              *long_log;
};

static GdkColor colors[] = {
	{ 0x0, 0x0000, 0x0000, 0x0000 }, /* black */
	{ 0x0, 0xffff, 0x0000, 0x0000 }, /* red   */
	{ 0x0, 0x0000, 0xffff, 0x0000 }, /* green */
	{ 0x0, 0x0000, 0x0000, 0xffff }, /* blue  */
	{ 0x0, 0xffff, 0xffff, 0x0000 }, /* yellow */
	{ 0x0, 0xffff, 0x0000, 0xffff }, /* pink?  */
	{ 0x0, 0x0000, 0xffff, 0xffff }, /* cyan   */
	{ 0x0, 0x6666, 0x6666, 0x6666 }, /* grey   */
};


static void revision_finalize           (GObject        *object);
static void revision_get_property       (GObject        *object,
					 guint           param_id,
					 GValue         *value,
					 GParamSpec     *pspec);
static void revision_set_property       (GObject        *object,
					 guint           param_id,
					 const GValue   *value,
					 GParamSpec     *pspec);
static void revision_copy_branch_status (gpointer        key,
					 gpointer        value,
					 GHashTable     *table);
static void revision_copy_status        (GiggleRevision *revision,
					 GHashTable     *branches_info);
static void revision_calculate_status   (GiggleRevision *revision,
					 GHashTable     *branches_info,
					 gint           *color);

G_DEFINE_TYPE (GiggleRevision, giggle_revision, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_TYPE,
	PROP_SHA,
	PROP_AUTHOR,
	PROP_DATE,
	PROP_SHORT_LOG,
	PROP_LONG_LOG
};

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_REVISION, GiggleRevisionPriv))

static void
giggle_revision_class_init (GiggleRevisionClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = revision_finalize;
	object_class->get_property = revision_get_property;
	object_class->set_property = revision_set_property;

	g_object_class_install_property (
		object_class,
		PROP_TYPE,
		g_param_spec_enum ("type",
				   "Type",
				   "Type of the revision",
				   GIGGLE_TYPE_REVISION_TYPE,
				   GIGGLE_REVISION_COMMIT,
				   G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	
	g_object_class_install_property (
		object_class,
		PROP_SHA,
		g_param_spec_string ("sha",
				     "SHA",
				     "SHA hash of the revision",
				     NULL,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (
		object_class,
		PROP_AUTHOR,
		g_param_spec_string ("author",
				     "Authur",
				     "Author of the revision",
				     NULL,
				     G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_DATE, /* FIXME: should not be a string... */
		g_param_spec_string ("date",
				     "Date",
				     "Date of the revision",
				     NULL,
				     G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_SHORT_LOG,
		g_param_spec_string ("short-log",
				     "Short log",
				     "Short log of the revision",
				     NULL,
				     G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_LONG_LOG,
		g_param_spec_string ("long-log",
				     "Long log",
				     "Long log of the revision",
				     NULL,
				     G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GiggleRevisionPriv));
}

static void
giggle_revision_init (GiggleRevision *revision)
{
}

static void
revision_finalize (GObject *object)
{
	GiggleRevision     *revision;
	GiggleRevisionPriv *priv;

	revision = GIGGLE_REVISION (object);

	priv = GET_PRIV (revision);

	if (revision->branch1) {
		giggle_branch_info_free (revision->branch1);
	}
	
	if (revision->branch2) {
		giggle_branch_info_free (revision->branch2);
	}
	
	if (revision->branches) {
		g_hash_table_destroy (revision->branches);
	}

	g_free (priv->sha);
	g_free (priv->author);
	g_free (priv->short_log);
	g_free (priv->long_log);
	
	G_OBJECT_CLASS (giggle_revision_parent_class)->finalize (object);
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
	case PROP_TYPE:
		g_value_set_enum (value, priv->type);
		break;
	case PROP_SHA:
		g_value_set_string (value, priv->sha);
		break;
	case PROP_AUTHOR:
		g_value_set_string (value, priv->author);
		break;
	case PROP_DATE:
		g_value_set_string (value, priv->date);
		break;
	case PROP_SHORT_LOG:
		g_value_set_string (value, priv->short_log);
		break;
	case PROP_LONG_LOG:
		g_value_set_string (value, priv->long_log);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
revision_set_property (GObject      *object,
		       guint         param_id,
		       const GValue *value,
		       GParamSpec   *pspec)
{
	GiggleRevisionPriv *priv;
	
	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_TYPE:
		priv->type = g_value_get_enum (value);
		break;
	case PROP_SHA:
		g_free (priv->sha);
		priv->sha = g_strdup (g_value_get_string (value));
		break;
	case PROP_AUTHOR:
		g_free (priv->author);
		priv->author = g_strdup (g_value_get_string (value));
		break;
	case PROP_DATE:
		g_free (priv->date);
		priv->date = g_strdup (g_value_get_string (value));
		break;
	case PROP_SHORT_LOG:
		g_free (priv->short_log);
		priv->short_log = g_strdup (g_value_get_string (value));
		break;
	case PROP_LONG_LOG:
		g_free (priv->long_log);
		priv->long_log = g_strdup (g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

GType
giggle_revision_type_get_type (void)
{
	static GType type = 0;

	if (!type) {
		const GEnumValue values[] = {
			{ GIGGLE_REVISION_BRANCH, "GIGGLE_REVISION_BRANCH", "branch" },
			{ GIGGLE_REVISION_MERGE,  "GIGGLE_REVISION_MERGE",  "merge" },
			{ GIGGLE_REVISION_COMMIT, "GIGGLE_REVISION_COMMIT", "commit" },
			{ 0, NULL, NULL }
		};

		type = g_enum_register_static ("GiggleRevisionType", values);
	}

	return type;
}

static void
revision_copy_branch_status (gpointer    key,
			     gpointer    value,
			     GHashTable *table)
{
	g_hash_table_insert (table, key, value);
}

static void
revision_copy_status (GiggleRevision *revision,
		      GHashTable     *branches_info)
{
	if (revision->branches) {
		g_hash_table_destroy (revision->branches);
	}
	
	revision->branches = g_hash_table_new (g_direct_hash, g_direct_equal);
	g_hash_table_foreach (branches_info,
			      (GHFunc) revision_copy_branch_status,
			      revision->branches);
}

static void
revision_calculate_status (GiggleRevision *revision,
			   GHashTable     *branches_info,
			   gint           *color)
{
	GiggleRevisionPriv *priv;
	GdkColor           *c;

	priv = GET_PRIV (revision);

	switch (priv->type) {
	case GIGGLE_REVISION_BRANCH:
		/* insert in the new branch the color from the first one */
		c = g_hash_table_lookup (branches_info, revision->branch1);

		if (!c) {
			g_critical ("model inconsistent, branches from an unexisting branch?");
		}
		
		g_hash_table_insert (branches_info, revision->branch2, c);
		revision_copy_status (revision, branches_info);
		break;

	case GIGGLE_REVISION_MERGE:
		revision_copy_status (revision, branches_info);
		/* remove merged branch */
		g_hash_table_remove (branches_info, revision->branch2);
		break;

	case GIGGLE_REVISION_COMMIT:
		/* increment color */
		*color = (*color + 1) % G_N_ELEMENTS (colors);

		/* change color for the changed branch */
		g_hash_table_insert (branches_info, revision->branch1, &colors[*color]);

		revision_copy_status (revision, branches_info);
		break;
	}
}

GiggleBranchInfo *
giggle_branch_info_new (const gchar *name)
{
	GiggleBranchInfo *info;

	info = g_new0 (GiggleBranchInfo, 1);
	info->name = g_strdup (name);

	return info;
}

void
giggle_branch_info_free (GiggleBranchInfo *info)
{
	g_free (info->name);
	g_free (info);
}

GiggleRevision *
giggle_revision_new_commit (const gchar      *sha,
			    GiggleBranchInfo *branch)
{
	GiggleRevision *revision;

	revision = g_object_new (GIGGLE_TYPE_REVISION,
				 "type", GIGGLE_REVISION_COMMIT,
				 "sha", sha,
				 NULL);

	revision->branch1 = branch;

	return revision;
}

GiggleRevision *
giggle_revision_new_branch (const gchar      *sha,
			    GiggleBranchInfo *old,
			    GiggleBranchInfo *new)
{
	GiggleRevision *revision;

	revision = g_object_new (GIGGLE_TYPE_REVISION,
				 "type", GIGGLE_REVISION_BRANCH,
				 "sha", sha,
				 NULL);

	revision->branch1 = old;
	revision->branch2 = new;

	return revision;
}

GiggleRevision *
giggle_revision_new_merge (const gchar      *sha,
			   GiggleBranchInfo *to,
			   GiggleBranchInfo *from)
{
	GiggleRevision *revision;

	revision = g_object_new (GIGGLE_TYPE_REVISION,
				 "type", GIGGLE_REVISION_MERGE,
				 "sha", sha,
				 NULL);

	revision->branch1 = to;
	revision->branch2 = from;

	return revision;
}

void
giggle_revision_validate (GtkTreeModel *model,
			  gint          n_column)
{
	GtkTreeIter     iter;
	gint            n_children;
	gint            color = 0;
	GHashTable     *branches_info;
	GiggleRevision *revision;
	
	g_return_if_fail (GTK_IS_TREE_MODEL (model));
	
	branches_info = g_hash_table_new (g_direct_hash, g_direct_equal);
	n_children = gtk_tree_model_iter_n_children (model, NULL);

	/* Need to calculate backwards from the end of the model. */
	while (n_children) {
		n_children--;
		gtk_tree_model_iter_nth_child (model, &iter, NULL, n_children);
		gtk_tree_model_get (model, &iter, n_column, &revision, -1);

		revision_calculate_status (revision, branches_info, &color);
	}
}

GiggleRevisionType
giggle_revision_get_revision_type (GiggleRevision *revision)
{
	GiggleRevisionPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), 0);

	priv = GET_PRIV (revision);
	
	return priv->type;
}

const gchar *
giggle_revision_get_sha (GiggleRevision *revision)
{
	GiggleRevisionPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);

	priv = GET_PRIV (revision);
	
	return priv->sha;
}

const gchar *
giggle_revision_get_author (GiggleRevision *revision)
{
	GiggleRevisionPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);

	priv = GET_PRIV (revision);
	
	return priv->author;
}

const gchar *
giggle_revision_get_date (GiggleRevision *revision)
{
	GiggleRevisionPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);

	priv = GET_PRIV (revision);
	
	return priv->date;
}

const gchar *
giggle_revision_get_short_log (GiggleRevision *revision)
{
	GiggleRevisionPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);

	priv = GET_PRIV (revision);
	
	return priv->short_log;
}

const gchar *
giggle_revision_get_long_log  (GiggleRevision   *revision)
{
	GiggleRevisionPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_REVISION (revision), NULL);

	priv = GET_PRIV (revision);
	
	return priv->long_log;
}

