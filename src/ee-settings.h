#ifndef EE_SETTINGS_H
#define EE_SETTINGS_H

#include <glib.h>
#include <libsoup/soup.h>

typedef struct {
    GList *urls;
    gint cycle_time;
    gint window_x;
    gint window_y;
    gboolean start_fullscreen;
    gboolean disable_plugins;
    gboolean disable_scripts;
    SoupCookieJar *cookie_jar;
} EESettings;

EESettings *ee_settings_new (void);
gboolean ee_settings_insert_url (EESettings *settings, const gchar *url, gint position);
void ee_settings_free (EESettings *settings);

#endif
