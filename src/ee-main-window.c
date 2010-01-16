#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <libsoup/soup.h>
#include <ee-settings.h>

typedef struct {
    EESettings *settings;
    GtkWindow *window;
    WebKitWebView *webview;
    SoupSession *session;
    GtkLabel *status;
    guint timeout_id;
    GList *curr_url;
} EEWindowPrivate;

/*
 * on_window_destroy: callback when destroying the main window
 */
static void
on_window_destroy (GtkWindow *          window,
                   EEWindowPrivate *    private)
{
    g_free (private);
    gtk_main_quit ();
}

/*
 * on_load_started: callback when we start loading a new URL
 */
static void
on_load_started (WebKitWebView *        webview,
                 WebKitWebFrame *       frame,
                 EEWindowPrivate *      private)
{
    SoupURI *uri;
    gchar *uri_string;
    gchar *status;

    uri = (SoupURI *) private->curr_url->data;
    uri_string = soup_uri_to_string (uri, FALSE);
    status = g_strdup_printf ("loading %s", uri_string);
    gtk_label_set_text (private->status, status);
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
              EEWindowPrivate *     private)
{
    SoupURI *uri, *creds;

    /* retrying is pointless, we just return (which causes the load to fail) */
    if (retrying) {
        g_debug ("HTTP auth was rejected by server");
        return;
    }
    uri = soup_message_get_uri (message);
    creds = (SoupURI *) private->curr_url->data;
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
                  EEWindowPrivate *     private)
{
    gtk_label_set_text (private->status, "");
}

/*
 * on_title_changed: callback to change the main window title when
 *   the title of the URL resource changes
 */
static void
on_title_changed (WebKitWebView *       webview,
                  WebKitWebFrame *      frame,
                  gchar *               title,
                  EEWindowPrivate *     private)
{
    gchar *window_title;

    window_title = g_strdup_printf ("Eagle Eye - %s", title);
    gtk_window_set_title (private->window, window_title);
    g_free (window_title);
}

/*
 * load_url: loads private->curr_url into the webview widget
 */
static gboolean
load_url (EEWindowPrivate *private)
{
    SoupURI *url;
    gchar *s;

    if (private->curr_url == NULL)
        return FALSE;
    s = soup_uri_to_string ((SoupURI *)private->curr_url->data, FALSE);
    g_debug ("opening URL: %s", s);
    webkit_web_view_load_uri (private->webview, s);
    g_free (s);
    return TRUE;
}

/*
 * open_previous_url: jump to the previous URL and load it
 */
static void
open_previous_url (EEWindowPrivate *private)
{
    GList *prev;

    /* get the next URL in the list */
    prev = g_list_previous (private->curr_url);
    /* if NULL, then try to wrap around to the last URL in the list */
    if (prev == NULL)
        prev = g_list_last (private->settings->urls);
    /* if still NULL, then there are no URLS in the list, so return */
    if (prev == NULL)
        return;
    private->curr_url = prev;
    load_url (private);
}

/*
 * open_next_url: jump to the next URL and load it
 */
static void
open_next_url (EEWindowPrivate *private)
{
    GList *next;

    /* get the next URL in the list */
    next = g_list_next (private->curr_url);
    /* if NULL, then try to wrap around to the first URL in the list */
    if (next == NULL)
        next = g_list_first (private->settings->urls);
    /* if still NULL, then there are no URLS in the list, so return */
    if (next == NULL)
        return;
    private->curr_url = next;
    load_url (private);
}

/*
 * on_timeout: loads the next URL every time the cycle-time timeout expires
 */
static gboolean
on_timeout (EEWindowPrivate *private)
{
    g_debug ("cycling to next URL");
    open_next_url (private);
    return TRUE;
}

/*
 * on_clicked_back: load the previous URL when the user clicks the back button
 */
static void
on_clicked_back (GtkToolButton *        button,
                 EEWindowPrivate *      private)
{
    guint timeout_id = private->timeout_id;

    g_debug ("---- BACK ----");
    if (timeout_id > 0) {
        g_source_remove (timeout_id);
        private->timeout_id = 0;
    }
    open_previous_url (private);
    /* if we are not paused, then reschedule the cycle timeout */
    if (timeout_id > 0)
        private->timeout_id = g_timeout_add_seconds (private->settings->cycle_time,
            (GSourceFunc) on_timeout, private);
}

/*
 * on_clicked_forward: load the next URL when the user clicks the back button
 */
static void
on_clicked_forward (GtkToolButton *     button,
                    EEWindowPrivate *   private)
{
    guint timeout_id = private->timeout_id;

    g_debug ("---- FORWARD ----");
    if (timeout_id > 0) {
        g_source_remove (timeout_id);
        private->timeout_id = 0;
    }
    open_next_url (private);
    /* if we are not paused, then reschedule the cycle timeout */
    if (timeout_id > 0)
        private->timeout_id = g_timeout_add_seconds (private->settings->cycle_time,
            (GSourceFunc) on_timeout, private);
}

/*
 * on_toggled_pause: enable or disable page cycling when the user toggles the
 *   pause button
 */
static void
on_toggled_pause (GtkToggleToolButton *         button,
                  EEWindowPrivate *             private)
{
    if (gtk_toggle_tool_button_get_active (button)) {
        g_source_remove (private->timeout_id);
        private->timeout_id = 0;
        g_debug ("---- PAUSE ----");
    }
    else {
        private->timeout_id = g_timeout_add_seconds (private->settings->cycle_time,
            (GSourceFunc) on_timeout, private);
        g_debug ("---- UNPAUSE ----");
    }
}

/*
 * on_toggled_fullscreen: enable or disable fullscreen mode when the user
 *   toggles the fullscreen button
 */
static void
on_toggled_fullscreen (GtkToggleToolButton *    button,
                       EEWindowPrivate *        private)
{
    if (gtk_toggle_tool_button_get_active (button)) {
        gtk_window_fullscreen (private->window);
        g_debug ("---- FULLSCREEN ON ----");
    }
    else {
        gtk_window_unfullscreen (private->window);
        g_debug ("---- FULLSCREEN OFF ----");
    }
}
 
/*
 * ee_main_window_construct: create the main window
 */
GtkWindow *
ee_main_window_construct(EESettings *settings)
{
    EEWindowPrivate *private;
    GtkWindow *window;
    GtkWidget *vbox;
    GtkWidget *webview;
    GtkWidget *sw;
    GtkWidget *toolbar;
    GtkToolItem *back;
    GtkToolItem *forward;
    GtkToolItem *pause;
    GtkToolItem *fullscreen;
    GtkWidget *status;
    GtkWidget *align;
    GtkToolItem *status_item;

    private = g_new0 (EEWindowPrivate, 1);

    /* set the private data for the main window */
    private->settings = settings;
    private->timeout_id = 0;
    private->curr_url = settings->urls;

    /* create the toplevel window */ 
    window = (GtkWindow *) gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (window, "Eagle Eye");
    gtk_window_set_default_size (window, settings->window_x, settings->window_y);
    private->window = window;
    g_signal_connect (window, "destroy",
        G_CALLBACK (on_window_destroy), private);
    
    /* create the container for the webview and toolbar */
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add(GTK_CONTAINER (window), vbox);

    /* create the webview widget */
    webview = webkit_web_view_new ();
    webkit_web_view_set_full_content_zoom(WEBKIT_WEB_VIEW (webview), TRUE);
    private->webview = WEBKIT_WEB_VIEW (webview);
    g_signal_connect(webview, "load-started",
        G_CALLBACK (on_load_started), private);
    g_signal_connect(WEBKIT_WEB_VIEW (webview), "load-finished",
        G_CALLBACK (on_load_finished), private);
    g_signal_connect(WEBKIT_WEB_VIEW (webview), "title-changed",
        G_CALLBACK (on_title_changed), private);

    /* configure the SoupSession */
    private->session = webkit_get_default_session ();
    g_signal_connect (private->session, "authenticate",
        G_CALLBACK (on_http_auth), private);
    soup_session_remove_feature_by_type (private->session, WEBKIT_TYPE_SOUP_AUTH_DIALOG);

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

    /* add the toolbar items */
    back = gtk_tool_button_new_from_stock (GTK_STOCK_MEDIA_PREVIOUS);
    g_signal_connect (back, "clicked",
        G_CALLBACK (on_clicked_back), private);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), back, -1);
    forward = gtk_tool_button_new_from_stock (GTK_STOCK_MEDIA_NEXT);
    g_signal_connect (forward, "clicked",
        G_CALLBACK (on_clicked_forward), private);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), forward, -1);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gtk_separator_tool_item_new (), -1);
    pause = gtk_toggle_tool_button_new_from_stock (GTK_STOCK_MEDIA_PAUSE);
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (pause), FALSE);
    g_signal_connect (pause, "toggled",
        G_CALLBACK (on_toggled_pause), private);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), pause, -1);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gtk_separator_tool_item_new (), -1);
    fullscreen = gtk_toggle_tool_button_new_from_stock (GTK_STOCK_FULLSCREEN);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON (fullscreen), FALSE);
    g_signal_connect (fullscreen, "toggled",
        G_CALLBACK (on_toggled_fullscreen), private);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), fullscreen, -1);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), gtk_separator_tool_item_new (), -1);

    /* create the status area and add it to the toolbar */
    status = gtk_label_new (NULL);
    private->status = GTK_LABEL (status);
    align = gtk_alignment_new (0.0, 0.5, 1.0, 1.0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 12, 12);
    gtk_container_add (GTK_CONTAINER (align), status);
    status_item = gtk_tool_item_new ();
    gtk_container_add (GTK_CONTAINER (status_item), align);
    gtk_tool_item_set_expand(status_item, TRUE);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), status_item, -1);

    gtk_widget_show_all (GTK_WIDGET (window));

    /* if start-fullscreen is true, then make the window fullscreen */
    if (settings->start_fullscreen == TRUE)
        gtk_window_fullscreen (window);

    /* load the first URL */
    load_url (private);

    /* start running the timeout function */
    private->timeout_id = g_timeout_add_seconds (private->settings->cycle_time,
        (GSourceFunc) on_timeout, private);

    return window;
}
