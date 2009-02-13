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

#include "config.h"
#include "giggle-plugin.h"

#include <string.h>
#include <glib/gi18n.h>

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_PLUGIN, GigglePluginPriv))

enum {
	PROP_NONE,
	PROP_NAME,
	PROP_BUILDER,
	PROP_FILENAME,
	PROP_DESCRIPTION,
};

typedef void (* GigglePluginCallback) (GtkAction    *action,
				       GigglePlugin *plugin);

typedef struct {
	char       *name;
	GtkBuilder *builder;
	char       *filename;
	char       *description;
	GPtrArray  *action_groups;
	GString    *ui_buffer;
	GModule    *module;
} GigglePluginPriv;

typedef struct {
	GString  *buffer;
	unsigned  element_tag_open : 1;
} GigglePluginParseContext;

GQuark
giggle_plugin_error_quark (void)
{
	static GQuark quark = 0;

	if (G_UNLIKELY (!quark))
		quark = g_quark_from_static_string ("giggle-plugin-error-quark");

	return quark;
}

static char *
plugin_get_callback_name (const char *action_name,
			  const char *signal_name)
{
	GError        *error = NULL;
	static GRegex *r1 = NULL;
	static GRegex *r2 = NULL;
	char          *a, *b;

	if (G_UNLIKELY (!r1)) {
		r1 = g_regex_new ("(.)([A-Z][a-z])", G_REGEX_OPTIMIZE, 0, &error);

		if (!r1) {
			g_warning ("%s: %s", G_STRFUNC, error->message);
			g_assert_not_reached ();
		}
	}

	if (G_UNLIKELY (!r2)) {
		r2 = g_regex_new ("[-_]+", G_REGEX_OPTIMIZE, 0, &error);

		if (!r2) {
			g_warning ("%s: %s", G_STRFUNC, error->message);
			g_assert_not_reached ();
		}
	}

	a = g_regex_replace (r1, action_name, -1, 0, "\\1_\\2", 0, NULL);
	b = g_strconcat (a, "_", signal_name, NULL);              g_free (a);
	a = g_regex_replace_literal (r2, b, -1, 0, "_", 0, NULL); g_free (b);
	b = g_ascii_strdown (a, -1);                              g_free (a);

	return b;
}

static gpointer
plugin_lookup_symbol (GigglePlugin *plugin,
		      const char   *symbol_name)
{
	GigglePluginPriv *priv = GET_PRIV (plugin);
	char             *plugin_dir, *path;
	const char       *plugin_name;
	gpointer          symbol;

	if (G_UNLIKELY (!priv->module)) {
		plugin_name = giggle_plugin_get_name (plugin);
		plugin_dir = g_path_get_dirname (priv->filename);
		path = g_module_build_path (plugin_dir, plugin_name);
		priv->module = g_module_open (path, G_MODULE_BIND_LAZY);

		if (!priv->module) {
			g_free (path); plugin_dir = g_build_filename (path = plugin_dir, ".libs", NULL);
			g_free (path); path = g_module_build_path (plugin_dir, plugin_name);
			priv->module = g_module_open (path, G_MODULE_BIND_LAZY);
		}

		if (!priv->module) {
			g_warning ("%s: Cannot find shared library for %s plugin",
				   G_STRFUNC, plugin_name);
		}

		g_free (plugin_dir);
		g_free (path);
	}

	if (priv->module && g_module_symbol (priv->module, symbol_name, &symbol))
		return symbol;

	return NULL;
}

static void
plugin_action_cb (GtkAction    *action,
		  GigglePlugin *plugin)
{
	char                        *callback_name;
	GigglePluginCallback         callback;
	const GSignalInvocationHint *hint;

	hint = g_signal_get_invocation_hint (action);
	g_signal_handlers_disconnect_by_func (action, plugin_action_cb, plugin);

	callback_name = plugin_get_callback_name (gtk_action_get_name (action),
						  g_signal_name (hint->signal_id));

	callback = plugin_lookup_symbol (plugin, callback_name);

	if (callback) {
		g_signal_connect (action, g_signal_name (hint->signal_id),
				  G_CALLBACK (callback), plugin);

		callback (action, plugin);
	}

	g_free (callback_name);
}

static void
plugin_connect_action (GigglePlugin *plugin,
		       GtkAction    *action)
{
	guint        *signals, n_signals, i;
	GSignalQuery  query;

	signals = g_signal_list_ids (G_OBJECT_TYPE (action), &n_signals);

	for (i = 0; i < n_signals; ++i) {
		g_signal_query (signals[i], &query);

		if (G_TYPE_NONE != query.return_type || query.n_params > 0)
			continue;

		g_signal_connect (action, query.signal_name,
				  G_CALLBACK (plugin_action_cb), plugin);
	}

	g_free (signals);
}

static void
plugin_add_child (GtkBuildable  *buildable,
		  GtkBuilder    *builder,
		  GObject       *child,
		  const gchar   *type)
{
	GigglePlugin     *plugin = GIGGLE_PLUGIN (buildable);
	GigglePluginPriv *priv = GET_PRIV (plugin);
	GList            *actions;

	g_return_if_fail (GTK_IS_ACTION_GROUP (child));

	g_ptr_array_add (priv->action_groups, child);
	actions = gtk_action_group_list_actions (GTK_ACTION_GROUP (child));

	while (actions) {
		plugin_connect_action (plugin, actions->data);
		actions = g_list_delete_link (actions, actions);
	}
}

static void
plugin_start_element (GMarkupParseContext  *markup,
		      const char           *tag_name,
		      const char          **attr_names,
		      const char          **attr_values,
		      gpointer              user_data,
		      GError              **error)
{
	GigglePluginParseContext *context = user_data;
	int                       i;

	if (context->element_tag_open)
		g_string_append_c (context->buffer, '>');

	g_string_append_c (context->buffer, '<');
	g_string_append   (context->buffer, tag_name);

	for (i = 0; attr_names[i]; ++i) {
		g_string_append_c (context->buffer, ' ');
		g_string_append   (context->buffer, attr_names[i]);
		g_string_append_c (context->buffer, '=');
		g_string_append_c (context->buffer, '"');
		g_string_append   (context->buffer, attr_values[i]);
		g_string_append_c (context->buffer, '"');
	}

	context->element_tag_open = TRUE;
}

static void
plugin_end_element (GMarkupParseContext  *ctxt,
		    const char           *tag_name,
		    gpointer              user_data,
		    GError              **error)
{
	GigglePluginParseContext *context = user_data;

	if (context->element_tag_open) {
		g_string_append_c (context->buffer, '/');
		g_string_append_c (context->buffer, '>');
	} else {
		g_string_append_c (context->buffer, '<');
		g_string_append_c (context->buffer, '/');
		g_string_append   (context->buffer, tag_name);
		g_string_append_c (context->buffer, '>');
	}

	context->element_tag_open = FALSE;
}

static void
plugin_text (GMarkupParseContext *ctxt,
	     const char          *text,
	     gsize                text_len,
	     gpointer             user_data,
	     GError             **error)
{
	GigglePluginParseContext *context = user_data;

	if (context->element_tag_open)
		g_string_append_c (context->buffer, '>');

	g_string_append_len (context->buffer, text, text_len);

	context->element_tag_open = FALSE;
}

static gboolean
plugin_custom_tag_start (GtkBuildable  *buildable,
			 GtkBuilder    *builder,
			 GObject       *child,
			 const gchar   *tagname,
			 GMarkupParser *parser,
			 gpointer      *data)
{
	GigglePluginPriv         *priv = GET_PRIV (buildable);
	GigglePluginParseContext *context;

	g_return_val_if_fail (NULL == child, FALSE);
	g_return_val_if_fail (!g_strcmp0 (tagname, "ui"), FALSE);

	parser->start_element = plugin_start_element;
	parser->end_element   = plugin_end_element;
	parser->text          = plugin_text;

	g_string_set_size (priv->ui_buffer, 0);

	context = *data = g_slice_new0 (GigglePluginParseContext);
	context->buffer = priv->ui_buffer;

	return TRUE;
}

static void
plugin_custom_finished (GtkBuildable  *buildable,
		        GtkBuilder    *builder,
		        GObject       *child,
		        const gchar   *tagname,
		        gpointer       data)
{
	g_slice_free (GigglePluginParseContext, data);
}

static void
giggle_plugin_buildable_init (GtkBuildableIface *iface)
{
	iface->add_child        = plugin_add_child;
	iface->custom_tag_start = plugin_custom_tag_start;
	iface->custom_finished  = plugin_custom_finished;
}

G_DEFINE_TYPE_WITH_CODE (GigglePlugin, giggle_plugin, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, giggle_plugin_buildable_init))

static void
giggle_plugin_init (GigglePlugin *plugin)
{
	GigglePluginPriv *priv = GET_PRIV (plugin);

	priv->action_groups = g_ptr_array_new ();
	priv->ui_buffer = g_string_new (NULL);
}

static void
plugin_set_property (GObject      *object,
		     guint         prop_id,
		     const GValue *value,
		     GParamSpec   *pspec)
{
	GigglePluginPriv *priv = GET_PRIV (object);

	switch (prop_id) {
	case PROP_BUILDER:
		if (priv->builder)
			g_object_unref (priv->builder);

		priv->builder = g_value_dup_object (value);
		break;

	case PROP_FILENAME:
		g_return_if_fail (g_str_has_suffix (g_value_get_string (value), ".xml"));

		g_free (priv->name);
		g_free (priv->filename);
		priv->filename = g_value_dup_string (value);
		priv->name = NULL;
		break;

	case PROP_DESCRIPTION:
		g_free (priv->description);
		priv->description = g_value_dup_string (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}

}

static void
plugin_get_property (GObject    *object,
		     guint       prop_id,
		     GValue     *value,
		     GParamSpec *pspec)
{
	GigglePlugin     *plugin = GIGGLE_PLUGIN (object);
	GigglePluginPriv *priv = GET_PRIV (plugin);

	switch (prop_id) {
	case PROP_NAME:
		g_value_set_string (value, giggle_plugin_get_name (plugin));
		break;

	case PROP_BUILDER:
		g_value_set_object (value, priv->builder);
		break;

	case PROP_FILENAME:
		g_value_set_string (value, priv->filename);
		break;

	case PROP_DESCRIPTION:
		g_value_set_string (value, priv->description);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}

}

static void
plugin_dispose (GObject *object)
{
	GigglePluginPriv *priv = GET_PRIV (object);

	if (priv->builder) {
		g_object_unref (priv->builder);
		priv->builder = NULL;
	}

	if (priv->module) {
		g_module_close (priv->module);
		priv->module = NULL;
	}

	g_ptr_array_foreach (priv->action_groups, (GFunc) g_object_unref, NULL);

	G_OBJECT_CLASS (giggle_plugin_parent_class)->dispose (object);
}

static void
plugin_finalize (GObject *object)
{
	GigglePluginPriv *priv = GET_PRIV (object);

	g_ptr_array_free (priv->action_groups, TRUE);
	g_string_free (priv->ui_buffer, TRUE);
	g_free (priv->description);
	g_free (priv->filename);
	g_free (priv->name);

	G_OBJECT_CLASS (giggle_plugin_parent_class)->finalize (object);
}

static void
giggle_plugin_class_init (GigglePluginClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->set_property = plugin_set_property;
	object_class->get_property = plugin_get_property;
	object_class->dispose      = plugin_dispose;
	object_class->finalize     = plugin_finalize;

	g_object_class_install_property
		(object_class, PROP_BUILDER,
		 g_param_spec_object
		 	("builder", "Builder",
			 "The GtkBuilder use for loading this plugin",
			 GTK_TYPE_BUILDER, G_PARAM_READWRITE));

	g_object_class_install_property
		(object_class, PROP_FILENAME,
		 g_param_spec_string
		 	("filename", "Filename",
			 "The filename of this plugin",
			 NULL, G_PARAM_READWRITE));

	g_object_class_install_property
		(object_class, PROP_DESCRIPTION,
		 g_param_spec_string
		 	("description", "Description",
			 "The description of this plugin",
			 NULL, G_PARAM_READWRITE));

	g_type_class_add_private (class, sizeof (GigglePluginPriv));
}

GigglePlugin *
giggle_plugin_new_from_file (const char  *filename,
			     GError     **error)
{
	GigglePlugin *plugin = NULL;
	GtkBuilder   *builder;
	GObject      *object;

	builder = gtk_builder_new ();
	gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);

	if (!gtk_builder_add_from_file (builder, filename, error))
		goto out;

	object = gtk_builder_get_object (builder, "plugin");

	if (object) {
		plugin = g_object_ref (object);
		giggle_plugin_set_builder (plugin, builder);
		giggle_plugin_set_filename (plugin, filename);
	} else {
		g_set_error (error, GIGGLE_PLUGIN_ERROR,
			     GIGGLE_PLUGIN_ERROR_MALFORMED,
			     _("Cannot find plugin description in '%s'"),
			     filename);
	}

out:
	if (builder)
		g_object_unref (builder);

	return plugin;
}

const char *
giggle_plugin_get_name (GigglePlugin *plugin)
{
	GigglePluginPriv *priv;

	g_return_val_if_fail (GIGGLE_IS_PLUGIN (plugin), NULL);

	priv = GET_PRIV (plugin);

	if (!priv->name && priv->filename) {
		priv->name = g_path_get_basename (priv->filename);
		priv->name[strlen (priv->name) - 4] = '\0';
	}

	return priv->name;
}

void
giggle_plugin_set_builder (GigglePlugin *plugin,
			   GtkBuilder   *builder)
{
	g_return_if_fail (GIGGLE_IS_PLUGIN (plugin));
	g_return_if_fail (GTK_IS_BUILDER (builder) || !builder);
	g_object_set (plugin, "builder", builder, NULL);
}

GtkBuilder *
giggle_plugin_get_builder (GigglePlugin *plugin)
{
	g_return_val_if_fail (GIGGLE_IS_PLUGIN (plugin), NULL);
	return GET_PRIV (plugin)->builder;
}

void
giggle_plugin_set_filename (GigglePlugin *plugin,
			    const char   *filename)
{
	g_return_if_fail (GIGGLE_IS_PLUGIN (plugin));
	g_object_set (plugin, "filename", filename, NULL);
}

const char *
giggle_plugin_get_filename (GigglePlugin *plugin)
{
	g_return_val_if_fail (GIGGLE_IS_PLUGIN (plugin), NULL);
	return GET_PRIV (plugin)->filename;
}

void
giggle_plugin_set_description (GigglePlugin *plugin,
			       const char   *description)
{
	g_return_if_fail (GIGGLE_IS_PLUGIN (plugin));
	g_object_set (plugin, "description", description, NULL);
}

const char *
giggle_plugin_get_description (GigglePlugin *plugin)
{
	g_return_val_if_fail (GIGGLE_IS_PLUGIN (plugin), NULL);
	return GET_PRIV (plugin)->description;
}

guint
giggle_plugin_merge_ui (GigglePlugin *plugin,
			GtkUIManager *ui,
			GError      **error)
{
	GigglePluginPriv *priv = GET_PRIV (plugin);
	int               i;

	g_return_val_if_fail (GIGGLE_IS_PLUGIN (plugin), 0);
	g_return_val_if_fail (GTK_IS_UI_MANAGER (ui), 0);

	for (i = 0; i < priv->action_groups->len; ++i) {
		GtkActionGroup *group = priv->action_groups->pdata[i];
		gtk_ui_manager_insert_action_group (ui, group, 0);
	}

	return gtk_ui_manager_add_ui_from_string (ui, priv->ui_buffer->str,
						  priv->ui_buffer->len, error);
}

