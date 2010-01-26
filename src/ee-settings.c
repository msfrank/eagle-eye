#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libsoup/soup.h>
#include <ee-settings.h>

/*
 * read_config_file: load configuration from settings->config_file.
 */
static void
read_config_file (EESettings *settings)
{
    GKeyFile *config;
    GError *error = NULL;
    gint cycle_time;
    gint window_x;
    gint window_y;
    gboolean start_fullscreen;
    gboolean disable_plugins;
    gboolean disable_scripts;
    gint toolbar_size;
    gchar *cookies_file;

    /* load configuration file */
    config = g_key_file_new ();
    g_key_file_load_from_file (config, settings->config_file, 0, &error);
    if (error) {
        g_warning ("failed to open %s: %s", settings->config_file, error->message);
        g_error_free (error);
        g_key_file_free (config);
        return;
    }
    g_debug ("loading configuration from %s", settings->config_file);

    /* load start-fullscreen parameter */
    start_fullscreen = g_key_file_get_boolean (config, "main", "start-fullscreen", &error);
    if (error) {
        if (error->code == G_KEY_FILE_ERROR_INVALID_VALUE)
            g_warning ("configuration error: failed to parse main::start-fullscreen");
        g_error_free (error);
        error = NULL;
    }
    else
        settings->start_fullscreen = start_fullscreen;

    /* load window-x parameter */
    window_x = g_key_file_get_integer (config, "main", "window-x", &error);
    if (error) {
        if (error->code == G_KEY_FILE_ERROR_INVALID_VALUE)
            g_warning ("configuration error: failed to parse main::window-x");
        g_error_free (error);
        error = NULL;
    }
    else
        settings->window_x = window_x;

    /* load window-y parameter */
    window_y = g_key_file_get_integer (config, "main", "window-y", &error);
    if (error) {
        if (error->code == G_KEY_FILE_ERROR_INVALID_VALUE)
            g_warning ("configuration error: failed to parse main::window-y");
        g_error_free (error);
        error = NULL;
    }
    else
        settings->window_y = window_y;

    /* load cycle-time parameter */
    cycle_time = g_key_file_get_integer (config, "main", "cycle-time", &error);
    if (error) {
        if (error->code == G_KEY_FILE_ERROR_INVALID_VALUE)
            g_warning ("configuration error: failed to parse main::cycle-time");
        g_error_free (error);
        error = NULL;
    }
    else
        settings->cycle_time = cycle_time;

    /* load disable-plugins parameter */
    disable_plugins = g_key_file_get_boolean (config, "main", "disable-plugins", &error);
    if (error) {
        if (error->code == G_KEY_FILE_ERROR_INVALID_VALUE)
            g_warning ("configuration error: failed to parse main::disable-plugins");
        g_error_free (error);
        error = NULL;
    }
    else
        settings->disable_plugins = disable_plugins;

    /* load disable-scripts parameter */
    disable_scripts = g_key_file_get_boolean (config, "main", "disable-scripts", &error);
    if (error) {
        if (error->code == G_KEY_FILE_ERROR_INVALID_VALUE)
            g_warning ("configuration error: failed to parse main::disable-scripts");
        g_error_free (error);
        error = NULL;
    }
    else
        settings->disable_scripts = disable_scripts;

    /* load toolbar-size parameter */
    toolbar_size = g_key_file_get_integer (config, "main", "toolbar-size", &error);
    if (error) {
        if (error->code == G_KEY_FILE_ERROR_INVALID_VALUE)
            g_warning ("configuration error: failed to parse main::toolbar-size");
        g_error_free (error);
        error = NULL;
    }
    else if (toolbar_size < 0)
        g_warning ("configuration error: value for main::toolbar-size is < 0");
    else
        settings->toolbar_size = (guint) toolbar_size;

    /* load cookies-file parameter */
    cookies_file = g_key_file_get_string (config, "main", "cookies-file", &error);
    if (error) {
        if (error->code == G_KEY_FILE_ERROR_INVALID_VALUE)
            g_warning ("configuration error: failed to parse main::cookies-file");
        g_error_free (error);
        error = NULL;
    }
    else
        settings->cookies_file = cookies_file;

    g_key_file_free (config);
}

/*
 * write_config_file: write configuration to settings->config_file.
 */
static void
write_config_file (EESettings *settings)
{
    GKeyFile *config;
    GError *error = NULL;
    GIOChannel *ioc;
    GIOStatus status;
    gchar *data, *curr;
    gsize len;
    gssize nwritten;
 
    if (settings->config_file == NULL)
        return;

    /* load configuration file */
    config = g_key_file_new ();
    g_key_file_load_from_file (config, settings->config_file, 0, &error);
    if (error) {
        g_warning ("failed to open %s: %s", settings->config_file, error->message);
        g_error_free (error);
        g_key_file_free (config);
        return;
    }

    /* write settings to config */
    g_key_file_set_integer (config, "main", "cycle-time", settings->cycle_time);
    g_key_file_set_boolean (config, "main", "start-fullscreen", settings->start_fullscreen);
    g_key_file_set_boolean (config, "main", "disable-plugins", settings->disable_plugins);
    g_key_file_set_boolean (config, "main", "disable-scripts", settings->disable_scripts);

    /* write config to file */
    ioc = g_io_channel_new_file (settings->config_file, "w", &error);
    if (error) {
        g_warning ("failed to open %s for writing: %s", settings->config_file, error->message);
        g_error_free (error);
        g_key_file_free (config);
        if (ioc)
            g_io_channel_unref (ioc);
        return;
    }
    curr = data = g_key_file_to_data (config, &len, &error);
    again:
    status = g_io_channel_write_chars (ioc, curr, len, &nwritten, &error);
    curr += nwritten;
    len -= nwritten;
    if (status == G_IO_STATUS_AGAIN)
        goto again;
    if (len > 0)
        goto again;
    g_free (data);
    if (status == G_IO_STATUS_ERROR) {
        g_warning ("error writing configuration to %s: %s", settings->config_file, error->message);
        g_error_free (error);
    }
    else
        g_debug ("wrote configuration to %s", settings->config_file);

    g_key_file_free (config);
    g_io_channel_unref (ioc);
}

/*
 * read_urls_file: load URLs from settings->urls_file.  the format of
 *   this file is one URL per line.  leading and trailing whitespace is
 *   removed before parsing the URL.  username and password can be
 *   specified using the normal URL syntax, and will be used for HTTP
 *   authentication.
 */
static void
read_urls_file (EESettings *settings)
{
    GIOChannel *ioc;
    GError *error = NULL;
    GIOStatus status;
    gchar *s;
    gsize len;
    
    /* try to open the urls file */
    ioc = g_io_channel_new_file (settings->urls_file, "r", &error);
    if (error) {
        g_warning ("failed to open %s: %s", settings->urls_file, error->message);
        g_error_free (error);
        if (ioc)
            g_io_channel_unref (ioc);
        return;
    }
    g_debug ("loading URLs from %s", settings->urls_file);

    /* loop reading each line of the file */
    while (1) {
        status = g_io_channel_read_line (ioc, &s, &len, NULL, &error);
        if (status == G_IO_STATUS_AGAIN)
            continue;
        if (status == G_IO_STATUS_EOF)
            break;
        if (status == G_IO_STATUS_ERROR) {
            g_warning ("error parsing URLs file: %s", error->message);
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
 * write_urls_file: write URLs to settings->urls_file.
 */
static void
write_urls_file (EESettings *settings)
{
    GIOChannel *ioc;
    GError *error = NULL;
    GIOStatus status;
    GList *item;
    SoupURI *uri;
    GString *str;
    gchar *data, *curr;
    gssize len;
    gsize nwritten;

    if (settings->urls_file == NULL)
        return;

    /* try to open the urls file */
    ioc = g_io_channel_new_file (settings->urls_file, "w", &error);
    if (error) {
        g_warning ("failed to open %s for writing: %s", settings->urls_file, error->message);
        g_error_free (error);
        if (ioc)
            g_io_channel_unref (ioc);
        return;
    }

    /* loop reading each line of the file */
    for (item = settings->urls; item; item = g_list_next (item)) {
        uri = (SoupURI *) item->data;
        str = g_string_new (NULL);
        /* append the scheme */
        if (uri->scheme == SOUP_URI_SCHEME_HTTP)
            g_string_append_printf (str, "%s://", SOUP_URI_SCHEME_HTTP);
        else if (uri->scheme == SOUP_URI_SCHEME_HTTPS)
            g_string_append_printf (str, "%s://", SOUP_URI_SCHEME_HTTPS);
        else {
                g_string_free (str, TRUE);
                continue;
        }
        /* append the username and password */
        if (uri->user) {
            str = g_string_append (str, uri->user);
            if (uri->password)
                g_string_append_printf (str, ":%s", uri->password);
            str = g_string_append (str, "@");
        }
        /* append the host */
        g_string_append (str, uri->host);
        /* append the port */
        if (uri->port != 80)
            g_string_append_printf (str, ":%i", uri->port);
        /* append the path */
        g_string_append (str, uri->path);
        if (uri->query)
            g_string_append_printf (str, "?%s", uri->query);
        if (uri->fragment)
            g_string_append_printf (str, "#%s", uri->fragment);
        g_string_append (str, "\n");
        /* write out the string */
        curr = data = g_string_free (str, FALSE);
        len = (gssize) strlen (data);
        /* write string to URLs file */
        again:
        status = g_io_channel_write_chars (ioc, curr, len, &nwritten, &error);
        curr += nwritten;
        len -= nwritten;
        if (status == G_IO_STATUS_AGAIN)
            goto again;
        if (len > 0)
            goto again;
        g_free (data);
        if (status == G_IO_STATUS_ERROR) {
            g_warning ("error writing urls to %s: %s", settings->urls_file, error->message);
            g_error_free (error);
            break;
        }
    }

    g_debug ("wrote URLs to %s", settings->urls_file);
    g_io_channel_unref (ioc);
}

static gboolean
on_parse_version_option (const gchar *      name,
                         const gchar *      value,
                         gpointer           data,
                         GError **          error)
{
    g_print ("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
    exit (0);
    /* we never get here, but gets rid of the compiler warning */
    return TRUE;
}

/*
 * ee_settings_load: create and load a new settings object.  configuration 
 *   data will be loaded from --config and --urls if they are specified,
 *   otherwise the data will be retrieved from $HOME/.eagle-eye/config and
 *   $HOME/.eagle-eye/urls, respectively.
 */
EESettings *
ee_settings_load (int *argc, char ***argv)
{
    GOptionContext *ct;
    EESettings *settings;
    GError *error = NULL;
    gchar *home;
    gchar *config_file = NULL;
    gchar *urls_file = NULL;
    gchar *geometry = NULL;

    GOptionEntry entries[] = 
    {
        { "config", 'c', 0, G_OPTION_ARG_FILENAME, &config_file, "Use specified configuration file", "PATH" },
        { "urls", 'u', 0, G_OPTION_ARG_FILENAME, &urls_file, "Use specified urls file", "PATH" },
        { "geometry", 0, 0, G_OPTION_ARG_STRING, &geometry, "Set the window geometry from the provided X geometry specification", "GEOMETRY" },
        { "version", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, on_parse_version_option, "Display program version", NULL },
        { NULL }
    };

    /* parse command line arguments */
    ct = g_option_context_new ("[URL...]");
    g_option_context_set_summary (ct, "Iterate through web pages at regular intervals");
    g_option_context_add_group (ct, gtk_get_option_group (TRUE));
    g_option_context_add_main_entries (ct, entries, NULL);
    if (!g_option_context_parse (ct, argc, argv, &error)) {
        g_print ("failed to parse options: %s\n", error->message);
        exit (1);
    }
    g_option_context_free (ct);

    /* alloc the settings object and set some defaults */
    settings = g_new0 (EESettings, 1);
    settings->urls = NULL;
    settings->cycle_time = 30;
    settings->window_x = 800;
    settings->window_y = 600;
    settings->start_fullscreen = FALSE;
    settings->disable_plugins = FALSE;
    settings->disable_scripts = FALSE;
    settings->toolbar_size = 3;
    settings->config_file = NULL;
    settings->urls_file = NULL;
    settings->cookies_file = NULL;

    home = g_build_filename (g_get_home_dir (), ".eagle-eye", NULL);

    /* create $HOME/.eagle-eye if it doesn't exist */
    if (mkdir (home, 0755) < 0 && errno != EEXIST) {
        g_warning ("failed to create %s: %s", home, g_strerror (errno));
        g_free (home);
        home = NULL;
    }
  
    /* load config parameters */
    if (config_file)
        settings->config_file = g_strdup (config_file);
    if (settings->config_file == NULL && home)
        settings->config_file = g_build_filename (home, "config", NULL);
    read_config_file (settings);

    /* load the urls file */
    if (urls_file)
        settings->urls_file = g_strdup (urls_file);
    if (settings->urls_file == NULL && home)
        settings->urls_file = g_build_filename (home, "urls", NULL);
    read_urls_file (settings);

    /* load geometry from the command line, if specified */
    if (geometry != NULL)
        settings->window_geometry = g_strdup (geometry);

    /* open the cookie jar */
    if (settings->cookies_file == NULL && home)
        settings->cookies_file = g_build_filename (home, "cookies", NULL);
    if (settings->cookies_file)
        settings->cookie_jar = soup_cookie_jar_text_new (settings->cookies_file, FALSE);

    g_free (home);

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
 * ee_settings_close: free all memory associated with the settings object
 */
void
ee_settings_save (EESettings *settings)
{
    GList *item;

    /* write config to file */
    write_config_file (settings);

    /* write urls to file */
    write_urls_file (settings);

    /* free urls list */
    if (settings->urls) {
        for (item = settings->urls; item; item = g_list_next (item))
            soup_uri_free ((SoupURI *) item->data);
        g_list_free (settings->urls);
    }

    /* unref the cookie_jar */
    if (settings->cookie_jar)
        g_object_unref (settings->cookie_jar);

    g_free (settings);
}
