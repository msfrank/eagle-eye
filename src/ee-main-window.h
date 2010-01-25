#ifndef EE_MAIN_WINDOW_H
#define EE_MAIN_WINDOW_H

#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <ee-settings.h>

typedef struct {
    EESettings *settings;
    GtkWindow *window;
    WebKitWebView *webview;
    SoupSession *session;
    GtkLabel *status;
    guint timeout_id;
    GList *curr_url;
} EEMainWindow;

GtkWindow *ee_main_window_construct (EESettings *settings);

#endif
