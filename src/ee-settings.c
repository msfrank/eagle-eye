#include <glib.h>
#include <libsoup/soup.h>
#include <ee-settings.h>

/*
 * read_config_file: load configuration from $HOME/.eagle-eye/config.
 */
static void
read_config_file (const gchar *path, EESettings *settings)
{
    GKeyFile *config;
    GError *error = NULL;
    gint cycle_time;
    gint window_x;
    gint window_y;
    gboolean start_fullscreen;
    gboolean disable_plugins;
    gboolean disable_scripts;

    /* load configuration file */
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

    /* load disable-plugins parameter */
    disable_plugins = g_key_file_get_boolean (config, "main", "disable-plugins", &error);
    if (error)
        g_error_free (error);
    else
        settings->disable_plugins = disable_plugins;
    error = NULL;

    /* load disable-scripts parameter */
    disable_scripts = g_key_file_get_boolean (config, "main", "disable-scripts", &error);
    if (error)
        g_error_free (error);
    else
        settings->disable_scripts = disable_scripts;
    error = NULL;

    g_key_file_free (config);
}

/*
 * read_urls_file: load URLs from $HOME/.eagle-eye/urls.  the format of
 *   this file is one URL per line.  leading and trailing whitespace is
 *   removed before parsing the URL.  username and password can be
 *   specified using the normal URL syntax, and will be used for HTTP
 *   authentication.
 */
static void
read_urls_file (const gchar *path, EESettings *settings)
{
    GIOChannel *ioc;
    GError *error = NULL;
    GIOStatus status;
    gchar *s;
    gsize len;
    
    /* try to open the urls file */
    ioc = g_io_channel_new_file (path, "r", &error);
    if (error) {
        g_warning ("failed to open %s: %s", path, error->message);
        g_error_free (error);
        if (ioc)
            g_io_channel_unref (ioc);
        return;
    }

    /* loop reading each line of the file */
    while (1) {
        status = g_io_channel_read_line (ioc, &s, &len, NULL, &error);
        if (status == G_IO_STATUS_AGAIN)
            continue;
        if (status == G_IO_STATUS_EOF)
            break;
        if (status == G_IO_STATUS_ERROR) {
            g_warning ("error parsing urls file: %s", error->message);
            g_error_free (error);
            break;
        }
        /* remove leading and trailing whitespace */
        s = g_strstrip (s);
        /* if string is empty or starts with a '#', then ignore it */
        if (s[0] == '\0' || s[0] == '#')
            ;
        /* otherwise try to append the URL to the end of the urls list */
        else
            ee_settings_insert_url_from_string (settings, s, -1);
        g_free (s);
    }

    g_io_channel_unref (ioc);
}

/*
 * ee_settings_new: create a new settings object and populate it with data
 *   from the $HOME/.eagle-eye/ configuration directory.
 */
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
    settings->disable_plugins = FALSE;
    settings->disable_scripts = FALSE;

    home = g_get_home_dir ();
  
    /* load config parameters */
    path = g_build_filename (home, ".eagle-eye", "config", NULL);
    read_config_file (path, settings);
    g_free (path);

    /* load the urls file */
    path = g_build_filename (home, ".eagle-eye", "urls", NULL);
    read_urls_file (path, settings);
    g_free (path);

    /* open the cookie jar */
    path = g_build_filename (home, ".eagle-eye", "cookies", NULL);
    settings->cookie_jar = soup_cookie_jar_text_new (path, FALSE);
    g_free (path);

    return settings;
}

/*
 * ee_settings_insert_url: insert a URL at the specified position.  if the
 *   URL is not valid for HTTP, then returns FALSE, otherwise returns TRUE.
 */
gboolean
ee_settings_insert_url (EESettings *settings, SoupURI *url, gint position)
{
    SoupURI *copy;
    gchar *id;

    g_assert (settings != NULL);
    g_assert (url != NULL);

    if (!SOUP_URI_VALID_FOR_HTTP (url)) {
        id = soup_uri_to_string (url, FALSE);
        g_warning ("failed to insert URL %s: not a valid HTTP URL", id);
        g_free (id);
        return FALSE;
    }
    copy = soup_uri_copy (url);
    id = soup_uri_to_string (copy, FALSE);
    settings->urls = g_list_insert (settings->urls, copy, position);  
    g_debug ("inserted URL %s at position %i", id, position);
    g_free (id);
    return TRUE;
}

/*
 * ee_settings_insert_url_from_string: insert a URL at the specified 
 *   position.  if the URL is not valid for HTTP, then returns FALSE,
 *   otherwise returns TRUE.
 */
gboolean
ee_settings_insert_url_from_string (EESettings *settings, const gchar *url, gint position)
{
    SoupURI *u;
    gboolean retval = FALSE;

    g_assert (settings != NULL);
    g_assert (url != NULL);

    u = soup_uri_new (url);
    if (u != NULL) {
        retval = ee_settings_insert_url (settings, u, position);
        soup_uri_free (u);
    }
    else
        g_warning ("failed to insert URL %s: couldn't parse URL", url);
    return retval;
}

/*
 * ee_settings_remove_url:
 */
gboolean
ee_settings_remove_url (EESettings *settings, guint index)
{
    GList *item;

    item = g_list_nth (settings->urls, index);
    if (item == NULL)
        return FALSE;
    soup_uri_free ((SoupURI *) item->data);
    settings->urls = g_list_delete_link (settings->urls, item);
    return TRUE;
}

/*
 * ee_settings_free: free all memory associated with the settings object
 */
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

    /* unref the cookie_jar */
    if (settings->cookie_jar)
        g_object_unref (settings->cookie_jar);

    g_free (settings);
}
