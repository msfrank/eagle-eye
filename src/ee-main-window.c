#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <ee-settings.h>

typedef struct {
    EESettings *settings;
    GtkWindow *window;
    WebKitWebView *webview;
    GtkLabel *status;
    guint timeout_id;
    GList *curr_url;
} EEWindowPrivate;

static void
on_window_destroy (GtkWindow *          window,
                   EEWindowPrivate *    private)
{
    g_free (private);
    gtk_main_quit ();
}

static void
on_load_started (WebKitWebView *        webview,
                 WebKitWebFrame *       frame,
                 EEWindowPrivate *      private)
{
    return;
}

static void
on_load_finished (WebKitWebView *       webview,
                  WebKitWebFrame *      frame,
                  EEWindowPrivate *     private)
{
    return;
}

static void
on_title_changed (WebKitWebView *       webview,
                  WebKitWebFrame *      frame,
                  gchar *               title,
                  EEWindowPrivate *     private)
{
    return;
}

static void
on_clicked_back (GtkToolButton *        button,
                 EEWindowPrivate *      private)
{
    g_debug ("---- BACK ----");
}

static void
on_clicked_forward (GtkToolButton *     button,
                    EEWindowPrivate *   private)
{
    g_debug ("---- FORWARD ----");
}

static void
on_toggled_pause (GtkToggleToolButton *         button,
                  EEWindowPrivate *             private)
{
    if (gtk_toggle_tool_button_get_active (button))
        g_debug ("---- PAUSE ----");
    else
        g_debug ("---- UNPAUSE ----");
}

static void
on_toggled_fullscreen (GtkToggleToolButton *    button,
                       EEWindowPrivate *        private)
{
    if (gtk_toggle_tool_button_get_active (button))
        g_debug ("---- FULLSCREEN ON ----");
    else
        g_debug ("---- FULLSCREEN OFF ----");
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
    GtkToolItem *status_item;

    private = g_new0 (EEWindowPrivate, 1);

    private->settings = settings;
    private->timeout_id = 0;
    private->curr_url = NULL;

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
    status_item = gtk_tool_item_new ();
    gtk_container_add (GTK_CONTAINER (status_item), status);
    gtk_tool_item_set_expand(status_item, TRUE);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), status_item, -1);

    gtk_widget_show_all (GTK_WIDGET (window));

    /* if start-fullscreen is true, then make the window fullscreen */
    if (settings->start_fullscreen == TRUE)
        gtk_window_fullscreen (window);

    return window;
}
