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
#include "giggle-git-authors.h"

#include <libgiggle/giggle-author.h>

#include <string.h>

typedef struct GiggleGitAuthorsPriv GiggleGitAuthorsPriv;

struct GiggleGitAuthorsPriv {
	GList *authors;
};

/* START: GiggleFlexibleAuthor API */
typedef struct {
	guint  votes;
	gchar* ballot;
} GiggleVote;

typedef struct {
	GHashTable* known_names;
	GHashTable* known_emails;
} GiggleFlexibleAuthor;

static void
_giggle_flexible_author_add_ballot (GHashTable * hash_table,
				    gchar const* ballot)
{
	GiggleVote* vote = g_hash_table_lookup (hash_table, ballot);

	if (!vote) {
		vote = g_new0 (GiggleVote, 1);
		vote->ballot = g_strdup (ballot);
		g_hash_table_insert (hash_table, vote->ballot, vote);
	}
	vote->votes++;
}

static void
giggle_flexible_author_add_name (GiggleFlexibleAuthor* self,
				 gchar const * name)
{
	_giggle_flexible_author_add_ballot (self->known_names, name);
}

static void
giggle_flexible_author_add_email (GiggleFlexibleAuthor* self,
				  gchar const * email)
{
	_giggle_flexible_author_add_ballot (self->known_emails, email);
}

static void
find_popular (gchar const* key,
	      GiggleVote* vote,
	      GiggleVote**best)
{
	if (G_UNLIKELY (!*best)) {
		*best = vote;
	} else {
		if (vote->votes > (*best)->votes) {
			*best = vote;
		}
	}
}

static gchar const *
_giggle_flexible_author_get_voted (GHashTable* hash_table)
{
	GiggleVote* vote = NULL;
	g_hash_table_foreach (hash_table, (GHFunc)find_popular, &vote);
	g_return_val_if_fail (vote, "");
	return vote->ballot;
}

static gchar const *
giggle_flexible_author_get_voted_name (GiggleFlexibleAuthor *self)
{
	return _giggle_flexible_author_get_voted (self->known_names);
}

static gchar const *
giggle_flexible_author_get_voted_email (GiggleFlexibleAuthor *self)
{
	return _giggle_flexible_author_get_voted (self->known_emails);
}

static GiggleFlexibleAuthor*
giggle_flexible_author_new (gchar const* name,
			    gchar const* email)
{
	GiggleFlexibleAuthor* self = g_slice_new0 (GiggleFlexibleAuthor);
	self->known_names = g_hash_table_new_full  (g_str_hash, g_str_equal,
						    g_free, g_free);
	self->known_emails = g_hash_table_new_full (g_str_hash, g_str_equal,
						    g_free, g_free);
	giggle_flexible_author_add_name  (self, name);
	giggle_flexible_author_add_email (self, email);
	return self;
}
/* END: GiggleFlexibleAuthor API */

static void     git_authors_finalize      (GObject           *object);
static void     git_authors_get_property  (GObject           *object,
					   guint              param_id,
					   GValue            *value,
					   GParamSpec        *pspec);
static void     git_authors_set_property  (GObject           *object,
					   guint              param_id,
					   const GValue      *value,
					   GParamSpec        *pspec);
static gboolean authors_get_command_line  (GiggleJob         *job,
					   gchar            **command_line);
static void     authors_handle_output     (GiggleJob         *job,
					   const gchar       *output,
					   gsize              length);

G_DEFINE_TYPE (GiggleGitAuthors, giggle_git_authors, GIGGLE_TYPE_JOB)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_GIT_AUTHORS, GiggleGitAuthorsPriv))

static void
giggle_git_authors_class_init (GiggleGitAuthorsClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GiggleJobClass *job_class  = GIGGLE_JOB_CLASS (class);

	object_class->finalize     = git_authors_finalize;
	object_class->get_property = git_authors_get_property;
	object_class->set_property = git_authors_set_property;

	job_class->get_command_line = authors_get_command_line;
	job_class->handle_output    = authors_handle_output;

#if 0
	g_object_class_install_property (object_class,
					 PROP_MY_PROP,
					 g_param_spec_string ("my-prop",
							      "My Prop",
							      "Describe the property",
							      NULL,
							      G_PARAM_READABLE));
#endif

	g_type_class_add_private (object_class, sizeof (GiggleGitAuthorsPriv));
}

static void
giggle_git_authors_init (GiggleGitAuthors *git_authors)
{
	GiggleGitAuthorsPriv *priv;

	priv = GET_PRIV (git_authors);
}

static void
git_authors_finalize (GObject *object)
{
	GiggleGitAuthorsPriv *priv;

	priv = GET_PRIV (object);
	
	g_list_foreach (priv->authors, (GFunc) g_object_unref, NULL);
	g_list_free (priv->authors);
	priv->authors = NULL;

	G_OBJECT_CLASS (giggle_git_authors_parent_class)->finalize (object);
}

static void
git_authors_get_property (GObject    *object,
		    guint       param_id,
		    GValue     *value,
		    GParamSpec *pspec)
{
	GiggleGitAuthorsPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
git_authors_set_property (GObject      *object,
		    guint         param_id,
		    const GValue *value,
		    GParamSpec   *pspec)
{
	GiggleGitAuthorsPriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static gboolean
authors_get_command_line (GiggleJob *job,
			  gchar    **command_line)
{
	*command_line = g_strdup (GIT_COMMAND " log");
	return TRUE;
}

static void
add_author (gchar const          *key,
	    GiggleFlexibleAuthor *value,
	    GiggleGitAuthorsPriv *priv)
{
	/* FIXME: giggle_author_new (name, email) */
	gchar const* popular_name  = giggle_flexible_author_get_voted_name (value);
	gchar const* popular_email = giggle_flexible_author_get_voted_email (value);
	gchar* string = NULL;

	if (strcmp (popular_name, key)) {
		/* this is not the most popular one */
		return;
	}

	if (popular_email && *popular_email) {
		string = g_strdup_printf ("%s <%s>", popular_name, popular_email);
	} else {
		string = g_strdup (popular_name);
	}
	priv->authors = g_list_prepend (priv->authors, giggle_author_new_from_string (string));
	g_free (string);
}

static void
authors_handle_output (GiggleJob   *job,
		       const gchar *output,
		       gsize        length)
{
	GiggleGitAuthorsPriv *priv;
	GHashTable           *authors_by_name;
	GHashTable           *authors_by_email;
	GList                *authors;
	gchar               **lines;
	gchar               **line;

	priv = GET_PRIV (job);

	lines = g_strsplit (output, "\n", -1);

	authors_by_name  = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	authors_by_email = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

	authors = NULL;
	for (line = lines; line && *line; line++) {
		if (g_str_has_prefix (*line, "Author: ")) {
			GiggleAuthor* author = giggle_author_new_from_string (*line + strlen ("Author: "));
			gchar const * email = giggle_author_get_email (author);
			gchar const * name  = giggle_author_get_name  (author);

			/* try to find this author */
			GiggleFlexibleAuthor* by_name;
			GiggleFlexibleAuthor* by_email;
			by_name  = name  ? g_hash_table_lookup (authors_by_name,  name)  : NULL;
			by_email = email ? g_hash_table_lookup (authors_by_email, email) : NULL;

			if (!by_name && !by_email) {
				/* create a new flexible author */
				by_name = giggle_flexible_author_new (giggle_author_get_name (author),
								      giggle_author_get_email (author));
				g_hash_table_insert (authors_by_name,
						     (gpointer) g_strdup (giggle_author_get_name (author)),
						     by_name);
				g_hash_table_insert (authors_by_email,
						     (gpointer) g_strdup (giggle_author_get_email (author)),
						     by_name);
			} else if (!by_name) {
				/* add the name to by_email */
				giggle_flexible_author_add_name (by_email,
								 giggle_author_get_name (author));
				giggle_flexible_author_add_email (by_email,
								  giggle_author_get_email (author));
				g_hash_table_insert (authors_by_name,
						     (gpointer) g_strdup (giggle_author_get_name (author)),
						     by_email);
			} else if (!by_email) {
				/* add the email to by_name */
				giggle_flexible_author_add_email (by_name,
								  giggle_author_get_email (author));
				giggle_flexible_author_add_name (by_name,
								 giggle_author_get_name (author));
				g_hash_table_insert (authors_by_email,
						     (gpointer) g_strdup (giggle_author_get_email (author)),
						     by_name);
			} else if (by_name == by_email) {
				/* just increase the counters */
				giggle_flexible_author_add_email (by_name,
								  giggle_author_get_email (author));
				giggle_flexible_author_add_name (by_name,
								 giggle_author_get_name (author));
			} else {
				/* both are different and real flexible authors, merge them */
				g_warning ("FIXME: implement merging; ask sven@imendio.com for an implementation and give him your git repository as a test case");
			}
			g_object_unref (author);
		}
	}

	g_list_foreach (priv->authors, (GFunc)g_object_unref, NULL);
	g_list_free (priv->authors);

	priv->authors = NULL;
	g_hash_table_foreach (authors_by_name, (GHFunc)add_author, priv);

	g_strfreev (lines);
}

GiggleJob *
giggle_git_authors_new (void)
{
	return g_object_new (GIGGLE_TYPE_GIT_AUTHORS, NULL);
}

GList *
giggle_git_authors_get_list (GiggleGitAuthors *authors)
{
	GiggleGitAuthorsPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_GIT_AUTHORS (authors), NULL);

	priv = GET_PRIV (authors);

	return priv->authors;
}

