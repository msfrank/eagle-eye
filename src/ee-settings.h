#ifndef EE_SETTINGS_H
#define EE_SETTINGS_H

#include <glib.h>
#include <libsoup/soup.h>

typedef struct {
    gchar *home;
    GList *urls;
    gint cycle_time;
    gboolean start_fullscreen;
    gboolean disable_plugins;
    gboolean disable_scripts;
    gboolean small_toolbar;
    gboolean remember_geometry;
    gchar *window_geometry;
    SoupCookieJar *cookie_jar;
} EESettings;

EESettings *ee_settings_load (int *argc, char ***argv);
gboolean ee_settings_insert_url (EESettings *settings, SoupURI *url, gint position);
gboolean ee_settings_insert_url_from_string (EESettings *settings, const gchar *url, gint position);
gboolean ee_settings_remove_url (EESettings *settings, guint index);
void ee_settings_save (EESettings *settings);
void ee_settings_free (EESettings *settings);

#endif
