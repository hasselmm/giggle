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
#include "giggle-view-terminal.h"

#include <glib/gi18n.h>
#include <vte/vte.h>

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_VIEW_TERMINAL, GiggleViewTerminalPriv))

typedef struct {
	GtkWidget *notebook;
} GiggleViewTerminalPriv;

G_DEFINE_TYPE (GiggleViewTerminal, giggle_view_terminal, GIGGLE_TYPE_VIEW)

static void
view_terminal_tab_remove_cb (GtkNotebook *notebook,
			     GtkWidget   *child,
			     GiggleView  *view)
{
	if (0 == gtk_notebook_get_n_pages (notebook))
		gtk_action_set_visible (giggle_view_get_action (view), FALSE);
}

static void
view_terminal_dispose (GObject *object)
{
	GiggleViewTerminalPriv *priv = GET_PRIV (object);

	if (priv->notebook) {
		g_signal_handlers_disconnect_by_func (priv->notebook,
						      view_terminal_tab_remove_cb,
						      object);
		priv->notebook = NULL;
	}

	G_OBJECT_CLASS (giggle_view_terminal_parent_class)->dispose (object);
}

static void
giggle_view_terminal_class_init (GiggleViewTerminalClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->dispose = view_terminal_dispose;

	gtk_rc_parse_string
		("style \"giggle-terminal-tab-close-button-style\" {"
		 "	GtkButton::inner-border = { 0, 0, 0, 0 }"
		 "	GtkWidget::focus-padding = 0"
		 "	GtkWidget::focus-line-width = 0"
		 "	xthickness = 0"
		 "	ythickness = 0"
		 "}"
		 "widget \"*.giggle-terminal-tab-close-button\" "
		 "style \"giggle-terminal-tab-close-button-style\"");


	g_type_class_add_private (class, sizeof (GiggleViewTerminalPriv));
}

static void
giggle_view_terminal_init (GiggleViewTerminal *view)
{
	GiggleViewTerminalPriv *priv = GET_PRIV (view);

	priv->notebook = gtk_notebook_new ();
	/*FIXME:gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), FALSE);*/
	gtk_container_add (GTK_CONTAINER (view), priv->notebook);
	gtk_widget_show (priv->notebook);

	g_signal_connect (priv->notebook, "remove",
			  G_CALLBACK (view_terminal_tab_remove_cb), view);
}

GtkWidget *
giggle_view_terminal_new (void)
{
	GtkAction *action;

	action = g_object_new (GTK_TYPE_RADIO_ACTION,
			       "name", "TerminalView",
			       "label", _("_Terminal"),
			       "tooltip", _("Issue git commands via terminal"),
			       "icon-name", "gnome-terminal",
			       "is-important", TRUE,
			       "visible", FALSE, NULL);

	return g_object_new (GIGGLE_TYPE_VIEW_TERMINAL,
			     "action", action, NULL);
}

static void
view_terminal_close_tab (GtkWidget *terminal)
{
	GtkWidget *notebook = gtk_widget_get_parent (terminal);
	gtk_container_remove (GTK_CONTAINER (notebook), terminal);
}

static void
view_terminal_title_changed_cb (GtkWidget *terminal,
				GtkWidget *label)
{
	const char *title;

	title = vte_terminal_get_window_title (VTE_TERMINAL (terminal));
	gtk_label_set_text (GTK_LABEL (label), title);
}

static GtkWidget *
view_terminal_create_label (GiggleViewTerminal *view,
			    GtkWidget          *terminal,
			    const char         *title)
{
	GtkWidget *label, *button, *image, *hbox;

	label = gtk_label_new (title);
	gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_MIDDLE);

	button = gtk_button_new ();
	gtk_widget_set_name (button, "giggle-terminal-tab-close-button");
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);

	g_signal_connect_swapped (button, "clicked",
				  G_CALLBACK (view_terminal_close_tab),
				  terminal);

	image = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_container_add (GTK_CONTAINER (button), image);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_widget_show_all (hbox);

	g_signal_connect (terminal, "window-title-changed",
			  G_CALLBACK (view_terminal_title_changed_cb), label);

	return hbox;
}

void
giggle_view_terminal_append_tab (GiggleViewTerminal *view,
				 const char         *directory)
{
	GiggleViewTerminalPriv *priv = GET_PRIV (view);
	GtkWidget              *terminal, *label;
	const char             *shell;
	char                   *title;
	int                     i;

	g_return_if_fail (GIGGLE_IS_VIEW_TERMINAL (view));

	terminal = vte_terminal_new ();
	gtk_widget_grab_focus (terminal);
	gtk_widget_show (terminal);

	g_signal_connect (terminal, "child-exited",
			  G_CALLBACK (view_terminal_close_tab), NULL);

	shell = g_getenv ("SHELL");

	vte_terminal_fork_command (VTE_TERMINAL (terminal),
				   shell ? shell : "/bin/sh",
				   NULL, NULL, NULL, FALSE, FALSE, FALSE);

	title = g_filename_display_name (directory);
	label = view_terminal_create_label (view, terminal, title);
	g_free (title);

	i = gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), terminal, label);
	/*FIXME:gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), i > 0);*/
	gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), i);

	gtk_notebook_set_tab_label_packing (GTK_NOTEBOOK (priv->notebook),
                                            terminal, TRUE, TRUE, GTK_PACK_START);

	gtk_action_set_visible (giggle_view_get_action (GIGGLE_VIEW (view)), TRUE);
}

