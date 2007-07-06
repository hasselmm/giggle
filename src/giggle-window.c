/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- *
 * vim: ts=8 sw=8 noet
 */
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
#include <string.h>
#include <stdlib.h>

#include "giggle-window.h"
#include "giggle-git.h"
#include "giggle-personal-details-window.h"
#include "giggle-view-summary.h"
#include "giggle-view-history.h"
#include "giggle-view-file.h"
#include "giggle-searchable.h"
#include "giggle-diff-window.h"
#include "giggle-configuration.h"
#include "giggle-history.h"
#include "eggfindbar.h"

typedef struct GiggleWindowPriv GiggleWindowPriv;

struct GiggleWindowPriv {
	GtkWidget           *content_vbox;
	GtkWidget           *main_notebook;

	/* Views */
	GtkWidget           *summary_view;
	GtkWidget           *history_view;
	GtkWidget           *file_view;

	/* Menu & toolbar */
	GtkUIManager        *ui_manager;
	GtkRecentManager    *recent_manager;
	GtkActionGroup      *recent_action_group;
	guint                recent_merge_id;

	GtkWidget           *find_bar;
	GtkToolItem         *full_search;

	GiggleGit           *git;

	GtkWidget           *personal_details_window;
	GtkWidget           *diff_current_window;

	GiggleConfiguration *configuration;

	gint                 width;
	gint                 height;
	gint                 x;
	gint                 y;
};

enum {
	SEARCH_NEXT,
	SEARCH_PREV
};


static void window_finalize                       (GObject           *object);
static gboolean window_configure_event            (GtkWidget           *widget,
						   GdkEventConfigure   *event);

static void window_add_widget_cb                  (GtkUIManager      *merge,
						   GtkWidget         *widget,
						   GiggleWindow      *window);

static void window_action_quit_cb                 (GtkAction         *action,
						   GiggleWindow      *window);
static void window_action_open_cb                 (GtkAction         *action,
						   GiggleWindow      *window);
static void window_action_save_patch_cb           (GtkAction         *action,
						   GiggleWindow      *window);
static void window_action_diff_cb                 (GtkAction         *action,
						   GiggleWindow      *window);
static void window_action_find_cb                 (GtkAction         *action,
						   GiggleWindow      *window);
static void window_action_find_next_cb            (GtkAction         *action,
						   GiggleWindow      *window);
static void window_action_find_prev_cb            (GtkAction         *action,
						   GiggleWindow      *window);
static void window_action_personal_details_cb     (GtkAction         *action,
						   GiggleWindow      *window);
static void window_action_about_cb                (GtkAction         *action,
						   GiggleWindow      *window);
static void window_action_compact_mode_cb         (GtkAction         *action,
						   GiggleWindow      *window);
static void window_action_view_file_list_cb       (GtkAction         *action,
						   GiggleWindow      *window);
static void window_action_view_graph_cb           (GtkAction         *action,
						   GiggleWindow      *window);
static void window_action_history_go_back         (GtkAction         *action,
						   GiggleWindow      *window);
static void window_action_history_go_forward      (GtkAction         *action,
						   GiggleWindow      *window);
static void window_action_history_refresh         (GtkAction         *action,
						   GiggleWindow      *window);
static void window_directory_changed_cb           (GiggleGit         *git,
						   GParamSpec        *arg,
						   GiggleWindow      *window);
static void window_recent_repositories_add        (GiggleWindow      *window);
static void window_recent_repositories_update     (GiggleWindow      *window);

static void window_notebook_switch_page_cb        (GtkNotebook       *notebook,
						   GtkNotebookPage   *page,
						   guint              page_num,
						   GiggleWindow      *window);

static void window_cancel_find                    (GtkWidget         *widget,
						   GiggleWindow      *window);
static void window_find_next                      (EggFindBar        *find_bar,
						   GiggleWindow      *window);
static void window_find_previous                  (EggFindBar        *find_bar,
						   GiggleWindow      *window);

static void window_update_toolbar_buttons         (GiggleWindow      *window);


static const GtkToggleActionEntry toggle_action_entries[] = {
	{ "CompactMode", NULL,
	  N_("_Compact Mode"), "F7", NULL,
	  G_CALLBACK (window_action_compact_mode_cb), FALSE
	},
	{ "ViewFileList", NULL,
	  N_("Show Project _Tree"), "F9", NULL,
	  G_CALLBACK (window_action_view_file_list_cb), TRUE
	},
	{ "ViewGraph", NULL,
	  N_("Show revision tree"), "F12", NULL,
	  G_CALLBACK (window_action_view_graph_cb), TRUE
	},
};

static const GtkActionEntry action_entries[] = {
	{ "ProjectMenu", NULL,
	  N_("_Project"), NULL, NULL,
	  NULL
	},
	{ "EditMenu", NULL,
	  N_("_Edit"), NULL, NULL,
	  NULL
	},
	{ "GoMenu", NULL,
	  N_("_Go"), NULL, NULL,
	  NULL
	},
	{ "ViewMenu", NULL,
	  N_("_View"), NULL, NULL,
	  NULL
	},
	{ "HelpMenu", NULL,
	  N_("_Help"), NULL, NULL,
	  NULL
	},
	{ "Open", GTK_STOCK_OPEN,
	  N_("_Open"), "<control>O", N_("Open a GIT repository"),
	  G_CALLBACK (window_action_open_cb)
	},
	{ "SavePatch", GTK_STOCK_SAVE,
	  N_("_Save patch"), "<control>S", N_("Save a patch"),
	  G_CALLBACK (window_action_save_patch_cb)
	},
	{ "Diff", NULL,
	  N_("_Diff current changes"), "<control>D", N_("Diff current changes"),
	  G_CALLBACK (window_action_diff_cb)
	},
	{ "Quit", GTK_STOCK_QUIT,
	  N_("_Quit"), "<control>Q", N_("Quit the application"),
	  G_CALLBACK (window_action_quit_cb)
	},
	{ "PersonalDetails", GTK_STOCK_PREFERENCES,
	  N_("Edit Personal _Details"), NULL, N_("Edit Personal details"),
	  G_CALLBACK (window_action_personal_details_cb)
	},
	{ "Find", GTK_STOCK_FIND,
	  N_("_Find..."), NULL, N_("Find..."),
	  G_CALLBACK (window_action_find_cb)
	},
	{ "FindNext", NULL,
	  N_("Find _Next"), "<control>G", N_("Find next"),
	  G_CALLBACK (window_action_find_next_cb)
	},
	{ "FindPrev", NULL,
	  N_("Find _Previous"), "<control><shift>G", N_("Find previous"),
	  G_CALLBACK (window_action_find_prev_cb)
	},
	{ "About", GTK_STOCK_ABOUT,
	  N_("_About"), NULL, N_("About this application"),
	  G_CALLBACK (window_action_about_cb)
	},

	/* Toolbar items */
	{ "BackHistory", GTK_STOCK_GO_BACK,
	  N_("_Back"), "<alt>Left", NULL,
	  G_CALLBACK (window_action_history_go_back)
	},
	{ "ForwardHistory", GTK_STOCK_GO_FORWARD,
	  N_("_Forward"), "<alt>Right", NULL,
	  G_CALLBACK (window_action_history_go_forward)
	},
	{ "RefreshHistory", GTK_STOCK_REFRESH,
	  N_("_Refresh"), "F5", NULL,
	  G_CALLBACK (window_action_history_refresh)
	},
};

static const gchar *ui_layout =
	"<ui>"
	"  <menubar name='MainMenubar'>"
	"    <menu action='ProjectMenu'>"
	"      <menuitem action='Open'/>"
/*
	"      <menuitem action='SavePatch'/>"
*/
	"      <menuitem action='Diff'/>"
	"      <separator/>"
	"      <placeholder name='RecentRepositories'/>"
	"      <separator/>"
	"      <menuitem action='Quit'/>"
	"    </menu>"
	"    <menu action='EditMenu'>"
	"      <menuitem action='PersonalDetails'/>"
	"      <separator/>"
	"      <menuitem action='Find'/>"
	"      <menuitem action='FindNext'/>"
	"      <menuitem action='FindPrev'/>"
	"    </menu>"
	"    <menu action='ViewMenu'>"
	"      <menuitem action='CompactMode'/>"
	"      <menuitem action='ViewFileList'/>"
	"      <menuitem action='ViewGraph'/>"
	"      <separator/>"
	"      <menuitem action='RefreshHistory'/>"
	"    </menu>"
	"    <menu action='GoMenu'>"
	"      <menuitem action='BackHistory'/>"
	"      <menuitem action='ForwardHistory'/>"
	"    </menu>"
	"    <menu action='HelpMenu'>"
	"      <menuitem action='About'/>"
	"    </menu>"
	"  </menubar>"
	"  <toolbar name='MainToolbar'>"
	"    <toolitem action='BackHistory'/>"
	"    <toolitem action='ForwardHistory'/>"
	"    <toolitem action='RefreshHistory'/>"
	"  </toolbar>"
	"</ui>";


G_DEFINE_TYPE (GiggleWindow, giggle_window, GTK_TYPE_WINDOW)

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_WINDOW, GiggleWindowPriv))

#define RECENT_FILES_GROUP "giggle"
#define SAVE_PATCH_UI_PATH "/ui/MainMenubar/ProjectMenu/SavePatch"
#define FIND_PATH "/ui/MainMenubar/EditMenu/Find"
#define COMPACT_MODE "/ui/MainMenubar/ViewMenu/CompactMode"
#define RECENT_REPOS_PLACEHOLDER_PATH "/ui/MainMenubar/ProjectMenu/RecentRepositories"
#define BACK_HISTORY_PATH "/ui/MainToolbar/BackHistory"
#define FORWARD_HISTORY_PATH "/ui/MainToolbar/ForwardHistory"

static void
giggle_window_class_init (GiggleWindowClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

	object_class->finalize = window_finalize;
	widget_class->configure_event = window_configure_event;

	g_type_class_add_private (object_class, sizeof (GiggleWindowPriv));
}

static void
window_create_menu (GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GtkActionGroup   *action_group;
	GError           *error = NULL;

	priv = GET_PRIV (window);
	priv->ui_manager = gtk_ui_manager_new ();
	g_signal_connect (priv->ui_manager,
			  "add_widget",
			  G_CALLBACK (window_add_widget_cb),
			  window);

	action_group = gtk_action_group_new ("MainActions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (action_group,
				      action_entries,
				      G_N_ELEMENTS (action_entries),
				      window);
	gtk_action_group_add_toggle_actions (action_group,
					     toggle_action_entries,
					     G_N_ELEMENTS (toggle_action_entries),
					     window);
	gtk_ui_manager_insert_action_group (priv->ui_manager, action_group, 0);

	gtk_window_add_accel_group (GTK_WINDOW (window),
				    gtk_ui_manager_get_accel_group (priv->ui_manager));

	g_object_unref (action_group);

	gtk_ui_manager_add_ui_from_string (priv->ui_manager, ui_layout, -1, &error);
	if (error) {
		g_error ("Couldn't create UI: %s\n", error->message);
	}

	gtk_ui_manager_ensure_update (priv->ui_manager);

	/* create recent repositories resources */
	priv->recent_action_group = gtk_action_group_new ("RecentRepositories");
	gtk_ui_manager_insert_action_group (priv->ui_manager, priv->recent_action_group, 0);

	priv->recent_manager = gtk_recent_manager_get_default ();
	g_signal_connect_swapped (priv->recent_manager, "changed",
				  G_CALLBACK (window_recent_repositories_update), window);

	window_recent_repositories_update (window);
}

static void
window_create_find_bar (GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GtkToolItem      *separator;

	priv = GET_PRIV (window);

	priv->find_bar = egg_find_bar_new ();

	separator = gtk_separator_tool_item_new ();
	gtk_widget_show (GTK_WIDGET (separator));
	gtk_toolbar_insert (GTK_TOOLBAR (priv->find_bar), separator, -1);

	priv->full_search = gtk_toggle_tool_button_new ();
	gtk_tool_button_set_label (GTK_TOOL_BUTTON (priv->full_search), _("Search inside _diffs"));
	gtk_tool_button_set_use_underline (GTK_TOOL_BUTTON (priv->full_search), TRUE);
	gtk_tool_item_set_is_important (priv->full_search, TRUE);
	gtk_widget_show (GTK_WIDGET (priv->full_search));

	gtk_toolbar_insert (GTK_TOOLBAR (priv->find_bar), priv->full_search, -1);

	gtk_box_pack_end (GTK_BOX (priv->content_vbox), priv->find_bar, FALSE, FALSE, 0);

	g_signal_connect (priv->find_bar, "close",
			  G_CALLBACK (window_cancel_find), window);
	g_signal_connect (priv->find_bar, "next",
			  G_CALLBACK (window_find_next), window);
	g_signal_connect (priv->find_bar, "previous",
			  G_CALLBACK (window_find_previous), window);
}

void
giggle_window_set_directory (GiggleWindow *window,
		      const gchar  *directory)
{
	GiggleWindowPriv *priv;
	GError           *error = NULL;

	priv = GET_PRIV (window);

	if (!giggle_git_set_directory (priv->git, directory, &error)) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (window),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("The directory '%s' does not look like a "
						   "GIT repository."), directory);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
}

static void
window_configuration_committed (GiggleConfiguration *configuration,
				gboolean             success,
				gpointer             user_data)
{
	gtk_main_quit ();
}

static void
save_state (GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	gboolean          compact;
	gchar             buf[25];
	gboolean          maximized;

	priv = GET_PRIV (window);

	g_snprintf (buf, sizeof (buf), "%dx%d+%d+%d", priv->width, priv->height, priv->x, priv->y);
	giggle_configuration_set_field (priv->configuration,
					CONFIG_FIELD_MAIN_WINDOW_GEOMETRY, buf);

	maximized = gdk_window_get_state (GTK_WIDGET (window)->window) & GDK_WINDOW_STATE_MAXIMIZED;
	giggle_configuration_set_boolean_field (priv->configuration,
						CONFIG_FIELD_MAIN_WINDOW_MAXIMIZED,
						maximized);

	compact = giggle_view_history_get_compact_mode (GIGGLE_VIEW_HISTORY (priv->history_view));
	giggle_configuration_set_boolean_field (priv->configuration,
						CONFIG_FIELD_COMPACT_MODE,
						compact);

	giggle_configuration_commit (priv->configuration,
				     window_configuration_committed,
				     window);
}

static gboolean
window_delete_event_cb (GtkWidget *widget,
			GdkEvent  *event,
			gpointer   user_data)
{
	save_state (GIGGLE_WINDOW (widget));
	gtk_widget_hide (widget);
	return TRUE;
}

static void
window_bind_state (GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GtkAction *action;
	const char *geom_str;
	gboolean compact, found_geometry = FALSE;

	priv = GET_PRIV (window);
	g_assert (priv->configuration);

	/* set the toggle menu item to indicate the current compact setting */
	compact = giggle_configuration_get_boolean_field (priv->configuration, CONFIG_FIELD_COMPACT_MODE);
	action = gtk_ui_manager_get_action (priv->ui_manager, COMPACT_MODE);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), compact);
	giggle_view_history_set_compact_mode (GIGGLE_VIEW_HISTORY (priv->history_view), compact);

	geom_str = giggle_configuration_get_field (priv->configuration, CONFIG_FIELD_MAIN_WINDOW_GEOMETRY);
	if (geom_str) {
		found_geometry = gtk_window_parse_geometry (GTK_WINDOW (window), geom_str);
	}

	if (!found_geometry) {
		gtk_window_set_default_size (GTK_WINDOW (window), 700, 550);
	}

	if (giggle_configuration_get_boolean_field (priv->configuration, CONFIG_FIELD_MAIN_WINDOW_MAXIMIZED)) {
		gtk_window_maximize (GTK_WINDOW (window));
	}

	gtk_widget_show (GTK_WIDGET (window));

	/* set up a callback to save the new UI state on application exit */
	g_signal_connect (GTK_WINDOW (window), "delete-event",
			  G_CALLBACK (window_delete_event_cb), NULL);
}

static void on_configuration_updated (GiggleConfiguration *configuration,
				      gboolean             success,
				      gpointer             user_data)
{
	if (success) {
		GiggleWindow* win = GIGGLE_WINDOW (user_data);
		window_bind_state (win);
	}
}

static void
giggle_window_init (GiggleWindow *window)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (window);

	priv->git = giggle_git_get ();
	priv->configuration = giggle_configuration_new ();

	g_signal_connect (priv->git,
			  "notify::directory",
			  G_CALLBACK (window_directory_changed_cb),
			  window);
	g_signal_connect_swapped (priv->git,
				  "notify::project-dir",
				  G_CALLBACK (window_recent_repositories_add),
				  window);

	priv->content_vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (priv->content_vbox);
	gtk_container_add (GTK_CONTAINER (window), priv->content_vbox);

	window_create_menu (window);

	priv->main_notebook = gtk_notebook_new ();
	gtk_widget_show (priv->main_notebook);
	gtk_box_pack_start_defaults (GTK_BOX (priv->content_vbox), priv->main_notebook);

	g_signal_connect_after (priv->main_notebook, "switch-page",
				G_CALLBACK (window_notebook_switch_page_cb), window);

	/* setup find bar */
	window_create_find_bar (window);

	/* append history view */
	priv->history_view = giggle_view_history_new ();
	gtk_widget_show (priv->history_view);

	g_signal_connect_swapped (priv->history_view, "history-changed",
				  G_CALLBACK (window_update_toolbar_buttons), window);

	gtk_notebook_append_page (GTK_NOTEBOOK (priv->main_notebook),
				  priv->history_view,
				  gtk_label_new (_("History")));

	/* append file view */
	/*
	priv->file_view = giggle_view_file_new ();
	gtk_widget_show (priv->file_view);

	gtk_notebook_append_page (GTK_NOTEBOOK (priv->main_notebook),
				  priv->file_view,
				  gtk_label_new (_("Files")));
	*/

	/* append summary view */
	priv->summary_view = giggle_view_summary_new ();
	gtk_widget_show (priv->summary_view);

	gtk_notebook_append_page (GTK_NOTEBOOK (priv->main_notebook),
				  priv->summary_view,
				  gtk_label_new (_("Summary")));

	giggle_configuration_update (priv->configuration, on_configuration_updated, window);
}

static void
window_finalize (GObject *object)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (object);

	g_object_unref (priv->ui_manager);
	g_object_unref (priv->git);
	g_object_unref (priv->recent_manager);
	g_object_unref (priv->recent_action_group);
	g_object_unref (priv->configuration);

	G_OBJECT_CLASS (giggle_window_parent_class)->finalize (object);
}

static gboolean
window_configure_event (GtkWidget         *widget,
			GdkEventConfigure *event)
{
	GiggleWindowPriv *priv = GET_PRIV (widget);

	if (! (gdk_window_get_state (widget->window) & GDK_WINDOW_STATE_MAXIMIZED)) {
		gtk_window_get_size (GTK_WINDOW (widget), &priv->width, &priv->height);
		gtk_window_get_position (GTK_WINDOW (widget), &priv->x, &priv->y);
	}

	return GTK_WIDGET_CLASS (giggle_window_parent_class)->configure_event (widget, event);;
}

static void
window_recent_repositories_add (GiggleWindow *window)
{
	static gchar     *groups[] = { RECENT_FILES_GROUP, NULL };
	GiggleWindowPriv *priv;
	GtkRecentData     data = { 0, };
	const gchar      *repository;
	gchar            *tmp_string;

	priv = GET_PRIV (window);

	repository = giggle_git_get_project_dir (priv->git);
	if(!repository) {
		repository = giggle_git_get_git_dir (priv->git);
	}

	g_return_if_fail (repository != NULL);

	data.display_name = (gchar *) giggle_git_get_project_name (priv->git);
	data.groups = groups;
	data.mime_type = "x-directory/normal";
	data.app_name = (gchar *) g_get_application_name ();
	data.app_exec = g_strjoin (" ", g_get_prgname (), "%u", NULL);

	tmp_string = g_filename_to_uri (repository, NULL, NULL);
	gtk_recent_manager_add_full (priv->recent_manager,
                                     tmp_string, &data);
	g_free (tmp_string);
	g_free (data.app_exec);
}

static void
window_recent_repository_activate (GtkAction    *action,
				   GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	const gchar      *directory;

	priv = GET_PRIV (window);

	directory = g_object_get_data (G_OBJECT (action), "recent-action-path");
	giggle_window_set_directory (window, directory);
}

static void
window_recent_repositories_clear (GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GList            *actions, *l;

	priv = GET_PRIV (window);
	actions = l = gtk_action_group_list_actions (priv->recent_action_group);

	if (priv->recent_merge_id) {
		gtk_ui_manager_remove_ui (priv->ui_manager, priv->recent_merge_id);
	}

	for (l = actions; l != NULL; l = l->next) {
		g_signal_handlers_disconnect_by_func (GTK_ACTION (l->data),
                                                      G_CALLBACK (window_recent_repository_activate),
                                                      window);

		gtk_action_group_remove_action (priv->recent_action_group, l->data);
	}

	g_list_free (actions);
}

/* this should not be necessary when there's
 * GtkRecentManager/GtkUIManager integration
 */
static void
window_recent_repositories_reload (GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GList            *recent_items, *l;
	GtkRecentInfo    *info;
	GtkAction        *action;
	gchar            *action_name, *label;
	gint              count = 0;

	priv = GET_PRIV (window);

	recent_items = gtk_recent_manager_get_items (priv->recent_manager);
	priv->recent_merge_id = gtk_ui_manager_new_merge_id (priv->ui_manager);
	l = recent_items = g_list_reverse (recent_items);

	/* FIXME: the max count is hardcoded */
	while (l && count < 10) {
		info = l->data;

		if (gtk_recent_info_has_group (info, RECENT_FILES_GROUP)) {
			action_name = g_strdup_printf ("recent-repository-%d", count);
			label = gtk_recent_info_get_uri_display (info);

			/* FIXME: add accel? */

			action = gtk_action_new (action_name,
						 label,
						 NULL,
						 NULL);

			g_object_set_data_full (G_OBJECT (action), "recent-action-path",
						gtk_recent_info_get_uri_display (info),
						(GDestroyNotify) g_free);

			g_signal_connect (action,
					  "activate",
					  G_CALLBACK (window_recent_repository_activate),
					  window);

			gtk_action_group_add_action (priv->recent_action_group, action);

			gtk_ui_manager_add_ui (priv->ui_manager,
					       priv->recent_merge_id,
					       RECENT_REPOS_PLACEHOLDER_PATH,
					       action_name,
					       action_name,
					       GTK_UI_MANAGER_MENUITEM,
					       FALSE);

			g_object_unref (action);
			g_free (action_name);
			g_free (label);
			count++;
		}

		l = l->next;
	}

	g_list_foreach (recent_items, (GFunc) gtk_recent_info_unref, NULL);
	g_list_free (recent_items);
}

static void
window_recent_repositories_update (GiggleWindow *window)
{
	window_recent_repositories_clear (window);
	window_recent_repositories_reload (window);
}

/* Update revision info. If previous_revision is not NULL, a diff between it and
 * the current revision will be shown.
 */

static void
window_add_widget_cb (GtkUIManager *merge, 
		      GtkWidget    *widget, 
		      GiggleWindow *window)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (window);

	gtk_box_pack_start (GTK_BOX (priv->content_vbox), widget, FALSE, FALSE, 0);
}

static void
window_action_quit_cb (GtkAction    *action,
		       GiggleWindow *window)
{
	save_state (window);
	gtk_widget_hide (GTK_WIDGET (window));
}

static void
window_action_open_cb (GtkAction    *action,
		       GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GtkWidget        *file_chooser;
	const gchar      *directory;

	priv = GET_PRIV (window);

	file_chooser = gtk_file_chooser_dialog_new (
		_("Select GIT repository"),
		GTK_WINDOW (window),
		GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		NULL);

	gtk_file_chooser_set_current_folder (
		GTK_FILE_CHOOSER (file_chooser),
		giggle_git_get_directory (priv->git));

	if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_OK) {
		directory = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (file_chooser));
		giggle_window_set_directory (window, directory);
	}

	gtk_widget_destroy (file_chooser);
}

static void
window_action_save_patch_cb (GtkAction    *action,
			     GiggleWindow *window)
{
/* FIXME: implement this again with GiggleView */
#if 0
	GiggleWindowPriv *priv;
	GtkWidget        *file_chooser;
	GtkTextBuffer    *text_buffer;
	GtkTextIter       iter_start, iter_end;
	gchar            *text, *path;
	GError           *error = NULL;

	priv = GET_PRIV (window);

	file_chooser = gtk_file_chooser_dialog_new (
		_("Save patch file"),
		GTK_WINDOW (window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		NULL);

	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (file_chooser), TRUE);

	/* FIXME: remember the last selected folder */

	if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_OK) {
		text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->diff_textview));
		gtk_text_buffer_get_bounds (text_buffer, &iter_start, &iter_end);

		text = gtk_text_buffer_get_text (text_buffer, &iter_start, &iter_end, TRUE);
		path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));

		if (!g_file_set_contents (path, text, strlen (text), &error)) {
			window_show_error (window,
					   N_("An error ocurred when saving to file:\n%s"),
					   error);
			g_error_free (error);
		}
	}

	gtk_widget_destroy (file_chooser);
#endif
}

static void
window_action_diff_cb (GtkAction    *action,
		       GiggleWindow *window)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (window);

	if (!priv->diff_current_window) {
		priv->diff_current_window = giggle_diff_window_new ();

		gtk_window_set_transient_for (GTK_WINDOW (priv->diff_current_window),
					      GTK_WINDOW (window));
		g_signal_connect (priv->diff_current_window, "delete-event",
				  G_CALLBACK (gtk_widget_hide_on_delete), NULL);
		g_signal_connect_after (priv->diff_current_window, "response",
					G_CALLBACK (gtk_widget_hide), NULL);
	}

	gtk_widget_show (priv->diff_current_window);
}

static void
window_notebook_switch_page_cb (GtkNotebook     *notebook,
				GtkNotebookPage *page,
				guint            page_num,
				GiggleWindow    *window)
{
	GiggleWindowPriv *priv;
	GtkWidget        *page_widget;
	GtkAction        *action;

	priv = GET_PRIV (window);
	page_widget = gtk_notebook_get_nth_page (notebook, page_num);

	/* Update find */
	action = gtk_ui_manager_get_action (priv->ui_manager, FIND_PATH);
	gtk_action_set_sensitive (action, GIGGLE_IS_SEARCHABLE (page_widget));

	/* Update history search */
	window_update_toolbar_buttons (window);
}

static void
window_action_find_cb (GtkAction    *action,
		       GiggleWindow *window)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (window);

	gtk_widget_show (priv->find_bar);
	gtk_widget_grab_focus (priv->find_bar);
}

static void
window_action_find_next_cb (GtkAction    *action,
			    GiggleWindow *window)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (window);
	window_find_next (EGG_FIND_BAR (priv->find_bar), window);
}

static void
window_action_find_prev_cb (GtkAction    *action,
			    GiggleWindow *window)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (window);
	window_find_previous (EGG_FIND_BAR (priv->find_bar), window);
}

static void
window_action_compact_mode_cb (GtkAction    *action,
			       GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	gboolean          active;

	priv = GET_PRIV (window);
	active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	giggle_view_history_set_compact_mode (GIGGLE_VIEW_HISTORY (priv->history_view), active);
}

static void
window_action_view_file_list_cb (GtkAction    *action,
				 GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	gboolean          active;

	priv = GET_PRIV (window);
	active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	giggle_view_history_set_file_list_visible (GIGGLE_VIEW_HISTORY (priv->history_view), active);
}

static void
window_action_view_graph_cb (GtkAction    *action,
			     GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	gboolean          active;

	priv = GET_PRIV (window);
	active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	giggle_view_history_set_graph_visible (GIGGLE_VIEW_HISTORY (priv->history_view), active);
}

static void
window_cancel_find (GtkWidget    *widget,
		    GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GtkWidget        *page;
	guint             page_num;

	priv = GET_PRIV (window);
	page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->main_notebook));
	page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->main_notebook), page_num);

	g_return_if_fail (GIGGLE_IS_SEARCHABLE (page));

	giggle_searchable_cancel (GIGGLE_SEARCHABLE (page));
	gtk_widget_hide (widget);
}

static void
window_find (EggFindBar            *find_bar,
	     GiggleWindow          *window,
	     GiggleSearchDirection  direction)
{
	GiggleWindowPriv *priv;
	GtkWidget        *page;
	guint             page_num;
	const gchar      *search_string;
	gboolean          full_search;

	priv = GET_PRIV (window);
	page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->main_notebook));
	page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->main_notebook), page_num);

	g_return_if_fail (GIGGLE_IS_SEARCHABLE (page));

	search_string = egg_find_bar_get_search_string (find_bar);

	if (search_string && *search_string) {
		full_search = gtk_toggle_tool_button_get_active (
			GTK_TOGGLE_TOOL_BUTTON (priv->full_search));

		giggle_searchable_search (GIGGLE_SEARCHABLE (page),
					  search_string, direction, full_search);
	}
}

static void
window_find_next (EggFindBar   *find_bar,
		  GiggleWindow *window)
{
	window_find (find_bar, window, GIGGLE_SEARCH_DIRECTION_NEXT);
}

static void
window_find_previous (EggFindBar   *find_bar,
		      GiggleWindow *window)
{
	window_find (find_bar, window, GIGGLE_SEARCH_DIRECTION_PREV);
}

static void
window_action_personal_details_cb (GtkAction    *action,
				   GiggleWindow *window)
{
	GiggleWindowPriv *priv;

	priv = GET_PRIV (window);

	if (!priv->personal_details_window) {
		priv->personal_details_window = giggle_personal_details_window_new ();

		gtk_window_set_transient_for (GTK_WINDOW (priv->personal_details_window),
					      GTK_WINDOW (window));
		g_signal_connect (priv->personal_details_window, "delete-event",
				  G_CALLBACK (gtk_widget_hide_on_delete), NULL);
		g_signal_connect_after (priv->personal_details_window, "response",
					G_CALLBACK (gtk_widget_hide), NULL);
	}

	gtk_widget_show (priv->personal_details_window);
}

static void
window_action_about_cb (GtkAction    *action,
			GiggleWindow *window)
{
	const gchar *authors[] = {
		"Sven Herzberg",
		"Mikael Hallendal",
		"Richard Hult",
		"Carlos Garnacho",
		NULL
	};

	gtk_show_about_dialog (GTK_WINDOW (window),
			       "name", "Giggle",
			       "copyright", "Copyright \xc2\xa9 2007 Imendio AB",
			       "translator-credits", _("translator-credits"),
			       "version", VERSION,
			       "authors", authors,
			       NULL);
}

static void
window_directory_changed_cb (GiggleGit    *git,
			     GParamSpec   *arg,
			     GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	gchar            *title;
	const gchar      *directory;

	priv = GET_PRIV (window);

	directory = giggle_git_get_directory (git);
	title = g_strdup_printf ("%s - Giggle", directory);
	gtk_window_set_title (GTK_WINDOW (window), title);
	g_free (title);
}

static void
window_update_toolbar_buttons (GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GtkAction        *action;
	GtkWidget        *page_widget;
	gboolean          back, forward;
	gint              page_num;

	priv = GET_PRIV (window);
	page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->main_notebook));
	page_widget = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->main_notebook), page_num);
	back = forward = FALSE;

	if (GIGGLE_IS_HISTORY (page_widget)) {
		back = giggle_history_can_go_back (GIGGLE_HISTORY (page_widget));
		forward = giggle_history_can_go_forward (GIGGLE_HISTORY (page_widget));
	}

	action = gtk_ui_manager_get_action (priv->ui_manager, BACK_HISTORY_PATH);
	gtk_action_set_sensitive (action, back);

	action = gtk_ui_manager_get_action (priv->ui_manager, FORWARD_HISTORY_PATH);
	gtk_action_set_sensitive (action, forward);
}

static void
window_action_history_go_back (GtkAction    *action,
			       GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GtkWidget        *page;
	gint              page_num;

	priv = GET_PRIV (window);
	page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->main_notebook));
	page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->main_notebook), page_num);

	giggle_history_go_back (GIGGLE_HISTORY (page));
}

static void
window_action_history_go_forward (GtkAction    *action,
				  GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	GtkWidget        *page;
	gint              page_num;

	priv = GET_PRIV (window);
	page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->main_notebook));
	page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->main_notebook), page_num);

	giggle_history_go_forward (GIGGLE_HISTORY (page));
}

static void
window_action_history_refresh (GtkAction    *action,
			       GiggleWindow *window)
{
	GiggleWindowPriv *priv;
	const gchar      *directory;

	priv = GET_PRIV (window);
	directory = giggle_git_get_directory (priv->git);
	giggle_window_set_directory (window, g_strdup(directory));
}

GtkWidget *
giggle_window_new (void)
{
	GtkWidget *window;

	window = g_object_new (GIGGLE_TYPE_WINDOW, NULL);

	return window;
}

GiggleGit *
giggle_window_get_git (GiggleWindow *self)
{
	g_return_val_if_fail (GIGGLE_IS_WINDOW (self), NULL);

	return GET_PRIV (self)->git;
}

