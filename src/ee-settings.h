#ifndef EE_SETTINGS_H
#define EE_SETTINGS_H

#include <glib.h>
#include <libsoup/soup.h>

typedef struct {
    gchar *config_file;
    gchar *urls_file;
    GList *urls;
    gint cycle_time;
    gint window_x;
    gint window_y;
    gboolean start_fullscreen;
    gchar *window_geometry;
    gboolean disable_plugins;
    gboolean disable_scripts;
    guint toolbar_size;
    gchar *cookies_file;
    SoupCookieJar *cookie_jar;
} EESettings;

EESettings *ee_settings_load (int *argc, char ***argv);
gboolean ee_settings_insert_url (EESettings *settings, SoupURI *url, gint position);
gboolean ee_settings_insert_url_from_string (EESettings *settings, const gchar *url, gint position);
gboolean ee_settings_remove_url (EESettings *settings, guint index);
void ee_settings_save (EESettings *settings);
void ee_settings_free (EESettings *settings);

#endif
