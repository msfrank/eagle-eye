#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <libsoup/soup.h>
#include <ee-main-window.h>
#include <ee-settings.h>
#include <ee-prefs-dialog.h>
#include <ee-url-manager.h>

/*
 * on_window_destroy: callback when destroying the main window
 */
static void
on_window_destroy (GtkWindow *          window,
                   EEMainWindow *       mainwin)
{
    g_free (mainwin);
    gtk_main_quit ();
}

/*
 * on_load_started: callback when we start loading a new URL
 */
static void
on_load_started (WebKitWebView *        webview,
                 WebKitWebFrame *       frame,
                 EEMainWindow *         mainwin)
{
    SoupURI *uri;
    gchar *uri_string;
    gchar *status;

    uri = (SoupURI *) mainwin->curr_url->data;
    uri_string = soup_uri_to_string (uri, FALSE);
    status = g_strdup_printf ("loading %s", uri_string);
    gtk_label_set_text (mainwin->status, status);
    g_free (status);
    g_free (uri_string);
}

/*
 * on_http_auth: callback when HTTP authorization is requested
 */
static void
on_http_auth (SoupSession *         session,
              SoupMessage *         message,
              SoupAuth *            auth,
              gboolean              retrying,
              EEMainWindow *        mainwin)
{
    SoupURI *uri, *creds;

    /* retrying is pointless, we just return (which causes the load to fail) */
    if (retrying) {
        g_debug ("HTTP auth was rejected by server");
        return;
    }
    uri = soup_message_get_uri (message);
    creds = (SoupURI *) mainwin->curr_url->data;
    if (creds->user && creds->password)
        soup_auth_authenticate (auth, creds->user, creds->password);
    else
        g_debug ("HTTP auth missing credentials");
    /* note that uri is *not* freed, we don't own that memory */
}

/*
 * on_load_finished: callback when we've finished loading a URL
 */
static void
on_load_finished (WebKitWebView *       webview,
                  WebKitWebFrame *      frame,
                  EEMainWindow *        mainwin)
{
    gtk_label_set_text (mainwin->status, "");
}

/*
 * on_title_changed: callback to change the main window title when
 *   the title of the URL resource changes
 */
static void
on_title_changed (WebKitWebView *       webview,
                  WebKitWebFrame *      frame,
                  gchar *               title,
                  EEMainWindow *        mainwin)
{
    gchar *window_title;

    window_title = g_strdup_printf ("Eagle Eye - %s", title);
    gtk_window_set_title (mainwin->window, window_title);
    g_free (window_title);
}

/*
 * load_url: loads mainwin->curr_url into the webview widget
 */
static gboolean
load_url (EEMainWindow *mainwin)
{
    SoupURI *url;
    gchar *s;

    if (mainwin->curr_url == NULL)
        return FALSE;
    s = soup_uri_to_string ((SoupURI *)mainwin->curr_url->data, FALSE);
    g_debug ("opening URL: %s", s);
    webkit_web_view_load_uri (mainwin->webview, s);
    g_free (s);
    return TRUE;
}

/*
 * open_previous_url: jump to the previous URL and load it
 */
static void
open_previous_url (EEMainWindow *mainwin)
{
    GList *prev;

    /* get the next URL in the list */
    prev = g_list_previous (mainwin->curr_url);
    /* if NULL, then try to wrap around to the last URL in the list */
    if (prev == NULL)
        prev = g_list_last (mainwin->settings->urls);
    /* if still NULL, then there are no URLS in the list, so return */
    if (prev == NULL)
        return;
    mainwin->curr_url = prev;
    load_url (mainwin);
}

/*
 * open_next_url: jump to the next URL and load it
 */
static void
open_next_url (EEMainWindow *mainwin)
{
    GList *next;

    /* get the next URL in the list */
    next = g_list_next (mainwin->curr_url);
    /* if NULL, then try to wrap around to the first URL in the list */
    if (next == NULL)
        next = g_list_first (mainwin->settings->urls);
    /* if still NULL, then there are no URLS in the list, so return */
    if (next == NULL)
        return;
    mainwin->curr_url = next;
    load_url (mainwin);
}

/*
 * on_timeout: loads the next URL every time the cycle-time timeout expires
 */
static gboolean
on_timeout (EEMainWindow *mainwin)
{
    g_debug ("cycling to next URL");
    open_next_url (mainwin);
    g_debug ("next cycle is scheduled in %i seconds", mainwin->settings->cycle_time);
    return TRUE;
}

/*
 * on_clicked_back: load the previous URL when the user clicks the back button
 */
static void
on_clicked_back (GtkToolButton *        button,
                 EEMainWindow *         mainwin)
{
    guint timeout_id = mainwin->timeout_id;

    g_debug ("---- BACK ----");
    if (timeout_id > 0) {
        g_source_remove (timeout_id);
        mainwin->timeout_id = 0;
    }
    open_previous_url (mainwin);
    /* if we are not paused, then reschedule the cycle timeout */
    if (timeout_id > 0) {
        mainwin->timeout_id = g_timeout_add_seconds (mainwin->settings->cycle_time,
            (GSourceFunc) on_timeout, mainwin);
        g_debug ("next cycle is scheduled in %i seconds", mainwin->settings->cycle_time);
    }
}

/*
 * on_clicked_forward: load the next URL when the user clicks the back button
 */
static void
on_clicked_forward (GtkToolButton *     button,
                    EEMainWindow *      mainwin)
{
    guint timeout_id = mainwin->timeout_id;

    g_debug ("---- FORWARD ----");
    if (timeout_id > 0) {
        g_source_remove (timeout_id);
        mainwin->timeout_id = 0;
    }
    open_next_url (mainwin);
    /* if we are not paused, then reschedule the cycle timeout */
    if (timeout_id > 0) {
        mainwin->timeout_id = g_timeout_add_seconds (mainwin->settings->cycle_time,
            (GSourceFunc) on_timeout, mainwin);
        g_debug ("next cycle is scheduled in %i seconds", mainwin->settings->cycle_time);
    }
}

/*
 * on_toggled_pause: enable or disable page cycling when the user toggles the
 *   pause button
 */
static void
on_toggled_pause (GtkToggleToolButton *         button,
                  EEMainWindow *                mainwin)
{
    if (gtk_toggle_tool_button_get_active (button)) {
        g_source_remove (mainwin->timeout_id);
        mainwin->timeout_id = 0;
        g_debug ("---- PAUSE ----");
    }
    else {
        mainwin->timeout_id = g_timeout_add_seconds (mainwin->settings->cycle_time,
            (GSourceFunc) on_timeout, mainwin);
        g_debug ("---- UNPAUSE ----");
        g_debug ("next cycle is scheduled in %i seconds", mainwin->settings->cycle_time);
    }
}

/*
 * on_toggled_fullscreen: enable or disable fullscreen mode when the user
 *   toggles the fullscreen button
 */
static void
on_toggled_fullscreen (GtkToggleToolButton *    button,
                       EEMainWindow *           mainwin)
{
    if (gtk_toggle_tool_button_get_active (button)) {
        gtk_window_fullscreen (mainwin->window);
        g_debug ("---- FULLSCREEN ON ----");
    }
    else {
        gtk_window_unfullscreen (mainwin->window);
        g_debug ("---- FULLSCREEN OFF ----");
    }
}

/*
 * on_clicked_edit: display the URL manager
 */
static void
on_clicked_edit (GtkToolButton *        button,
                 EEMainWindow *         mainwin)
{
    GtkWidget *dialog;
    gint result;

    g_debug ("---- EDIT ----");
    dialog = ee_url_manager_new (mainwin->settings);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

/*
 * on_clicked_prefs: display the preferences
 */
static void
on_clicked_prefs (GtkToolButton *       button,
                  EEMainWindow *        mainwin)
{
    GtkWidget *dialog;

    g_debug ("---- PREFERENCES ----");
    ee_prefs_dialog_run (mainwin);
}

/*
 *
 */
static void
on_add_url (GtkMenuItem *               menu_item,
            EESettings *                settings)
{
    g_debug ("---- POPUP ADD URL ----");
}

/*
 *
 */
static void
on_populate_popup (WebKitWebView *      webview,
                   GtkMenu *            menu,
                   EESettings *         settings)
{
    GtkWidget *add_item;
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());
    add_item = gtk_menu_item_new_with_label ("Add URL...");
    g_signal_connect (add_item, "activate", G_CALLBACK (on_add_url), settings);
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), add_item);
    gtk_widget_show_all (GTK_WIDGET (menu));
}

/*
 * ee_main_window_construct: create the main window
 */
GtkWindow *
ee_main_window_construct(EESettings *settings)
{
    EEMainWindow *mainwin;
    GtkWindow *window;
    GtkWidget *vbox;
    GtkWidget *webview;
    WebKitWebSettings *websettings;
    GtkWidget *sw;
    GtkWidget *toolbar;
    GtkToolItem *back;
    GtkToolItem *forward;
    GtkToolItem *pause;
    GtkToolItem *fullscreen;
    GtkToolItem *edit;
    GtkToolItem *prefs;
    GtkWidget *status;
    GtkWidget *align;
    GtkToolItem *status_item;

    mainwin = g_new0 (EEMainWindow, 1);

    /* set the mainwin data for the main window */
    mainwin->settings = settings;
    mainwin->timeout_id = 0;
    mainwin->curr_url = settings->urls;

    /* create the toplevel window */ 
    window = (GtkWindow *) gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (window, "Eagle Eye");
    gtk_window_set_default_size (window, settings->window_x, settings->window_y);
    mainwin->window = window;
    g_signal_connect (window, "destroy",
        G_CALLBACK (on_window_destroy), mainwin);
    
    /* create the container for the webview and toolbar */
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add(GTK_CONTAINER (window), vbox);

    /* create the webview widget */
    webview = webkit_web_view_new ();
    webkit_web_view_set_full_content_zoom(WEBKIT_WEB_VIEW (webview), TRUE);
    mainwin->webview = WEBKIT_WEB_VIEW (webview);
    g_signal_connect(webview, "load-started",
        G_CALLBACK (on_load_started), mainwin);
    g_signal_connect(WEBKIT_WEB_VIEW (webview), "load-finished",
        G_CALLBACK (on_load_finished), mainwin);
    g_signal_connect(WEBKIT_WEB_VIEW (webview), "title-changed",
        G_CALLBACK (on_title_changed), mainwin);
    g_signal_connect(WEBKIT_WEB_VIEW (webview), "populate-popup",
        G_CALLBACK (on_populate_popup), mainwin);

    /* configure the web settings object */
    websettings = webkit_web_view_get_settings (mainwin->webview);
    if (settings->disable_plugins)
        g_object_set (websettings, "enable-plugins", FALSE, NULL);
    if (settings->disable_scripts)
        g_object_set (websettings, "enable-scripts", FALSE, NULL);
        
    /* configure the SoupSession */
    mainwin->session = webkit_get_default_session ();
    g_signal_connect (mainwin->session, "authenticate",
        G_CALLBACK (on_http_auth), mainwin);
    soup_session_remove_feature_by_type (mainwin->session, WEBKIT_TYPE_SOUP_AUTH_DIALOG);
    if (settings->cookie_jar)
        soup_session_add_feature (mainwin->session, SOUP_SESSION_FEATURE (settings->cookie_jar));

    /* put the webview in a scrolled window and put that in the vbox */
    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (sw),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER (sw), webview);
    gtk_box_pack_start(GTK_BOX (vbox), sw, TRUE, TRUE, 0);

    /* add a separator to look nice :) */
    gtk_box_pack_start(GTK_BOX (vbox), gtk_hseparator_new (), FALSE, FALSE, 0);

    /* create the toolbar */
    toolbar = gtk_toolbar_new ();
    gtk_box_pack_start(GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
    g_object_set (toolbar, "icon-size", settings->toolbar_size, NULL);

    /* add the toolbar items */
    back = gtk_tool_button_new_from_stock (GTK_STOCK_MEDIA_PREVIOUS);
    gtk_tool_item_set_tooltip_text (back, "Previous URL");
    g_signal_connect (back, "clicked",
        G_CALLBACK (on_clicked_back), mainwin);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), back, -1);
    forward = gtk_tool_button_new_from_stock (GTK_STOCK_MEDIA_NEXT);
    gtk_tool_item_set_tooltip_text (forward, "Next URL");
    g_signal_connect (forward, "clicked",
        G_CALLBACK (on_clicked_forward), mainwin);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), forward, -1);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gtk_separator_tool_item_new (), -1);
    pause = gtk_toggle_tool_button_new_from_stock (GTK_STOCK_MEDIA_PAUSE);
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (pause), FALSE);
    gtk_tool_item_set_tooltip_text (pause, "Pause URL cycling");
    g_signal_connect (pause, "toggled",
        G_CALLBACK (on_toggled_pause), mainwin);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), pause, -1);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gtk_separator_tool_item_new (), -1);
    fullscreen = gtk_toggle_tool_button_new_from_stock (GTK_STOCK_FULLSCREEN);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON (fullscreen), FALSE);
    gtk_tool_item_set_tooltip_text (fullscreen, "Toggle fullscreen mode");
    g_signal_connect (fullscreen, "toggled",
        G_CALLBACK (on_toggled_fullscreen), mainwin);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), fullscreen, -1);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gtk_separator_tool_item_new (), -1);
    edit = gtk_tool_button_new_from_stock (GTK_STOCK_EDIT);
    gtk_tool_item_set_tooltip_text (edit, "Edit URLs");
    g_signal_connect (edit, "clicked",
        G_CALLBACK (on_clicked_edit), mainwin);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), edit, -1);
    prefs = gtk_tool_button_new_from_stock (GTK_STOCK_PREFERENCES);
    gtk_tool_item_set_tooltip_text (edit, "Preferences");
    g_signal_connect (prefs, "clicked",
        G_CALLBACK (on_clicked_prefs), mainwin);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), prefs, -1);

    /* create the status area and add it to the toolbar */
    status = gtk_label_new (NULL);
    mainwin->status = GTK_LABEL (status);
    align = gtk_alignment_new (0.0, 0.5, 1.0, 1.0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 12, 12);
    gtk_container_add (GTK_CONTAINER (align), status);
    status_item = gtk_tool_item_new ();
    gtk_container_add (GTK_CONTAINER (status_item), align);
    gtk_tool_item_set_expand(status_item, TRUE);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), status_item, -1);

    /* if window geometry was specified, then set it now */
    gtk_widget_show_all (GTK_WIDGET (vbox));
    if (settings->window_geometry) {
        if (!gtk_window_parse_geometry (window, settings->window_geometry))
            g_warning ("failed to parse window geometry '%s'", settings->window_geometry);
    }

    gtk_widget_show_all (GTK_WIDGET (window));

    /* if enabled, make the window fullscreen and toggle the fullscreen button */
    if (settings->start_fullscreen == TRUE) {
        gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (fullscreen), TRUE);
        gtk_window_fullscreen (window);
    }

    /* load the first URL */
    load_url (mainwin);

    /* start running the timeout function */
    mainwin->timeout_id = g_timeout_add_seconds (mainwin->settings->cycle_time,
        (GSourceFunc) on_timeout, mainwin);
    g_debug ("next cycle is scheduled in %i seconds", mainwin->settings->cycle_time);

    return window;
}
