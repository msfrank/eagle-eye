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
    SoupCookieJar *cookie_jar;
} EESettings;

EESettings *ee_settings_new (void);
void ee_settings_free (EESettings *settings);

#endif
