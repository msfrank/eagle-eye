#include <glib.h>
#include <libsoup/soup.h>
#include <ee-settings.h>

static void
read_config_file (const gchar *path, EESettings *settings)
{
    GKeyFile *config;
    GError *error = NULL;
    gint cycle_time;
    gint window_x;
    gint window_y;
    gboolean start_fullscreen;

    config = g_key_file_new ();
    g_key_file_load_from_file (config, path, 0, &error);
    if (error) {
        g_warning ("failed to open %s: %s", path, error->message);
        g_error_free (error);
        g_key_file_free (config);
        return;
    }

    /* load start-fullscreen parameter */
    start_fullscreen = g_key_file_get_boolean (config, "main", "start-fullscreen", &error);
    if (error)
        g_error_free (error);
    else
        settings->start_fullscreen = start_fullscreen;
    error = NULL;

    /* load window-x parameter */
    window_x = g_key_file_get_integer (config, "main", "window-x", &error);
    if (error)
        g_error_free (error);
    else
        settings->window_x = window_x;
    error = NULL;

    /* load window-y parameter */
    window_y = g_key_file_get_integer (config, "main", "window-y", &error);
    if (error)
        g_error_free (error);
    else
        settings->window_y = window_y;
    error = NULL;

    /* load cycle-time parameter */
    cycle_time = g_key_file_get_integer (config, "main", "cycle-time", &error);
    if (error)
        g_error_free (error);
    else
        settings->cycle_time = cycle_time;
    error = NULL;

    g_key_file_free (config);
}

static void
read_urls_file (const gchar *path, EESettings *settings)
{
    GIOChannel *ioc;
    GError *error = NULL;
    GIOStatus status;
    gchar *s;
    gsize len;
    
    ioc = g_io_channel_new_file (path, "r", &error);
    if (error) {
        g_warning ("failed to open %s: %s", path, error->message);
        g_error_free (error);
        if (ioc)
            g_io_channel_unref (ioc);
        return;
    }

    while (1) {
        SoupURI *uri;
        gchar *id;

        status = g_io_channel_read_line (ioc, &s, &len, NULL, &error);
        if (status == G_IO_STATUS_AGAIN)
            continue;
        if (status == G_IO_STATUS_EOF)
            break;
        if (status == G_IO_STATUS_ERROR)
            break;
        s = g_strstrip (s);
        uri = soup_uri_new (s);
        if (uri && SOUP_URI_VALID_FOR_HTTP (uri)) {
            id = soup_uri_to_string (uri, FALSE);
            settings->urls = g_list_append (settings->urls, uri);  
            g_debug ("added URL %s", id);
            g_free (id);
        }
        else {
            if (uri)
                soup_uri_free (uri);
        }
    }

    g_io_channel_unref (ioc);
}

EESettings *
ee_settings_new (void)
{
    EESettings *settings;
    const gchar *home;
    gchar *path;

    /* alloc the settings object and set some defaults */
    settings = g_new0 (EESettings, 1);
    settings->urls = NULL;
    settings->cycle_time = 30;
    settings->window_x = 800;
    settings->window_y = 600;
    settings->start_fullscreen = FALSE;

    home = g_get_home_dir ();
  
    /* load config parameters */
    path = g_build_filename (home, ".eagle-eye", "config", NULL);
    read_config_file (path, settings);
    g_free (path);

    /* load the urls file */
    path = g_build_filename (home, ".eagle-eye", "urls", NULL);
    read_urls_file (path, settings);
    g_free (path);

    return settings;
}

void
ee_settings_free (EESettings *settings)
{
    GList *curr;

    /* free urls list */
    if (settings->urls) {
        for (curr = settings->urls; curr; curr = g_list_next (curr))
            soup_uri_free ((SoupURI *) curr->data);
        g_list_free (settings->urls);
    }

    g_free (settings);
}
