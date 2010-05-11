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
 * write_config_file: write configuration to config file.
 */
static gboolean
write_config_file (EESettings *settings)
{
    GKeyFile *config;
    gchar *config_file = NULL;
    GError *error = NULL;
    GIOChannel *ioc;
    GIOStatus status;
    gchar *data, *curr;
    gsize len;
    gsize nwritten;
 
    /* load configuration file */
    config = g_key_file_new ();
    config_file = g_build_filename (settings->home, "config", NULL);
    g_key_file_load_from_file (config, config_file, 0, &error);
    if (error) {
        if (error->domain == G_FILE_ERROR && error->code == G_FILE_ERROR_NOENT) {
            g_error_free (error);
            error = NULL;
        }
        else {
            g_critical ("failed to open %s: %s", config_file, error->message);
            g_error_free (error);
            g_key_file_free (config);
            g_free (config_file);
            return FALSE;
        }
    }

    /* write settings to config */
    g_key_file_set_integer (config, "main", "cycle-time", settings->cycle_time);
    g_key_file_set_boolean (config, "main", "start-fullscreen", settings->start_fullscreen);
    g_key_file_set_boolean (config, "main", "disable-plugins", settings->disable_plugins);
    g_key_file_set_boolean (config, "main", "disable-scripts", settings->disable_scripts);
    g_key_file_set_boolean (config, "main", "small-toolbar", settings->small_toolbar);
    g_key_file_set_boolean (config, "main", "remember-geometry", settings->remember_geometry);

    /* write config to file */
    ioc = g_io_channel_new_file (config_file, "w", &error);
    if (error) {
        g_critical ("failed to open %s for writing: %s", config_file, error->message);
        g_error_free (error);
        g_key_file_free (config);
        g_free (config_file);
        if (ioc)
            g_io_channel_unref (ioc);
        return FALSE;
    }
    curr = data = g_key_file_to_data (config, &len, &error);
    again:
    status = g_io_channel_write_chars (ioc, curr, (gssize) len, &nwritten, &error);
    curr += nwritten;
    len -= nwritten;
    if (status == G_IO_STATUS_AGAIN)
        goto again;
    if (len > 0)
        goto again;
    g_free (data);
    if (status == G_IO_STATUS_ERROR) {
        g_critical ("error writing configuration to %s: %s", config_file, error->message);
        g_error_free (error);
        g_free (config_file);
        g_io_channel_unref (ioc);
        return FALSE;
    }
    else
        g_debug ("wrote configuration to %s", config_file);

    g_key_file_free (config);
    g_free (config_file);
    g_io_channel_unref (ioc);
    return TRUE;
}

/*
 * read_config_file: load configuration from config.
 */
static gboolean
read_config_file (EESettings *settings)
{
    gchar *config_file = NULL;
    GKeyFile *config;
    GError *error = NULL;
    gint cycle_time;
    gboolean start_fullscreen;
    gboolean disable_plugins;
    gboolean disable_scripts;
    gboolean small_toolbar;
    gboolean remember_geometry;

    config_file = g_build_filename (settings->home, "config", NULL);
    if (!g_file_test (config_file, G_FILE_TEST_IS_REGULAR))
        return write_config_file (settings);

    /* load configuration file */
    config = g_key_file_new ();
    g_key_file_load_from_file (config, config_file, 0, &error);
    if (error) {
        g_critical ("failed to open %s: %s", config_file, error->message);
        g_error_free (error);
        g_key_file_free (config);
        g_free (config_file);
        return FALSE;
    }
    g_debug ("loading configuration from %s", config_file);
    g_free (config_file);

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

    /* load small-toolbar parameter */
    small_toolbar = g_key_file_get_boolean (config, "main", "small-toolbar", &error);
    if (error) {
        if (error->code == G_KEY_FILE_ERROR_INVALID_VALUE)
            g_warning ("configuration error: failed to parse main::small-toolbar");
        g_error_free (error);
        error = NULL;
    }
    else
        settings->small_toolbar = small_toolbar;

    /* load remember-geometry parameter */
    remember_geometry = g_key_file_get_boolean (config, "main", "remember-geometry", &error);
    if (error) {
        if (error->code == G_KEY_FILE_ERROR_INVALID_VALUE)
            g_warning ("configuration error: failed to parse main::remember-geometry");
        g_error_free (error);
        error = NULL;
    }
    else
        settings->remember_geometry = remember_geometry;

    g_key_file_free (config);
    return TRUE;
}

/*
 * write_urls_file: write URLs to disk.
 */
static gboolean
write_urls_file (EESettings *settings)
{
    gchar *urls_file = NULL;
    GIOChannel *ioc;
    GError *error = NULL;
    GIOStatus status;
    GList *item;
    SoupURI *uri;
    GString *str;
    gchar *data, *curr;
    gssize len;
    gsize nwritten;

    /* try to open the urls file */
    urls_file = g_build_filename (settings->home, "urls", NULL);
    ioc = g_io_channel_new_file (urls_file, "w", &error);
    if (error) {
        g_critical ("failed to open %s for writing: %s", urls_file, error->message);
        g_error_free (error);
        g_free (urls_file);
        if (ioc)
            g_io_channel_unref (ioc);
        return FALSE;
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
            g_critical ("error writing urls to %s: %s", urls_file, error->message);
            g_error_free (error);
            g_free (urls_file);
            g_io_channel_unref (ioc);
            return FALSE;
        }
    }

    g_debug ("wrote URLs to %s", urls_file);
    g_free (urls_file);
    g_io_channel_unref (ioc);
    return TRUE;
}

/*
 * read_urls_file: load URLs from urls file.  the format of this file
 *   is one URL per line.  leading and trailing whitespace is
 *   removed before parsing the URL.  username and password can be
 *   specified using the normal URL syntax, and will be used for HTTP
 *   authentication.
 */
static gboolean
read_urls_file (EESettings *settings)
{
    gchar *urls_file = NULL;
    GIOChannel *ioc;
    GError *error = NULL;
    GIOStatus status;
    gchar *s;
    gsize len;
    
    urls_file = g_build_filename (settings->home, "urls", NULL);
    if (!g_file_test (urls_file, G_FILE_TEST_IS_REGULAR))
        return write_urls_file (settings);

    /* try to open the urls file */
    ioc = g_io_channel_new_file (urls_file, "r", &error);
    if (error) {
        g_critical ("failed to open %s: %s", urls_file, error->message);
        g_error_free (error);
        g_free (urls_file);
        if (ioc)
            g_io_channel_unref (ioc);
        return FALSE;
    }
    g_debug ("loading URLs from %s", urls_file);
    g_free (urls_file);

    /* loop reading each line of the file */
    while (1) {
        status = g_io_channel_read_line (ioc, &s, &len, NULL, &error);
        if (status == G_IO_STATUS_AGAIN)
            continue;
        if (status == G_IO_STATUS_EOF)
            break;
        if (status == G_IO_STATUS_ERROR) {
            g_critical ("error parsing URLs file: %s", error->message);
            g_error_free (error);
            g_io_channel_unref (ioc);
            return FALSE;
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
    return TRUE;
}

/*
 * write_geometry_file: write window geometry to disk.
 */
static gboolean
write_geometry_file (EESettings *settings)
{
    gchar *geometry_file = NULL;
    GError *error = NULL;
    GIOChannel *ioc;
    GIOStatus status;
    gchar *curr = NULL;
    gssize len = 0;
    gsize nwritten = 0;
 
    if (settings->remember_geometry == FALSE || settings->window_geometry == NULL)
        return TRUE;

    /* write geometry to file */
    geometry_file = g_build_filename (settings->home, "geometry", NULL);
    ioc = g_io_channel_new_file (geometry_file, "w", &error);
    if (error) {
        g_critical ("failed to open %s for writing: %s", geometry_file, error->message);
        g_error_free (error);
        g_free (geometry_file);
        if (ioc)
            g_io_channel_unref (ioc);
        return FALSE;
    }
    curr = settings->window_geometry;
    len = strlen(settings->window_geometry);
    again:
    status = g_io_channel_write_chars (ioc, curr, len, &nwritten, &error);
    curr += nwritten;
    len -= nwritten;
    if (status == G_IO_STATUS_AGAIN)
        goto again;
    if (len > 0)
        goto again;
    if (status == G_IO_STATUS_ERROR) {
        g_critical ("error writing geometry to %s: %s", geometry_file, error->message);
        g_error_free (error);
        g_free (geometry_file);
        g_io_channel_unref (ioc);
        return FALSE;
    }
    else
        g_debug ("wrote geometry to %s", geometry_file);

    g_free (geometry_file);
    g_io_channel_unref (ioc);
    return TRUE;
}

/*
 * read_geometry_file: load window geometry settings.
 */
static gboolean
read_geometry_file (EESettings *settings)
{
    gchar *geometry_file = NULL;
    GIOChannel *ioc;
    GError *error = NULL;
    GIOStatus status;
    gchar *s;
    gsize len;
    
    if (settings->remember_geometry == FALSE)
        return TRUE;

    geometry_file = g_build_filename (settings->home, "geometry", NULL);
    if (!g_file_test (geometry_file, G_FILE_TEST_IS_REGULAR))
        return write_geometry_file (settings);

    /* try to open the geometry file */
    ioc = g_io_channel_new_file (geometry_file, "r", &error);
    if (error) {
        g_critical ("failed to open %s: %s", geometry_file, error->message);
        g_error_free (error);
        g_free (geometry_file);
        if (ioc)
            g_io_channel_unref (ioc);
        return FALSE;
    }
    g_debug ("loading geometry from %s", geometry_file);
    g_free (geometry_file);

    /* read the first line of the file */
    status = g_io_channel_read_line (ioc, &s, &len, NULL, &error);
    if (status == G_IO_STATUS_ERROR) {
        g_critical ("error parsing geometry file: %s", error->message);
        g_error_free (error);
        g_io_channel_unref (ioc);
        return FALSE;
    }

    /* remove leading and trailing whitespace */
    if (s != NULL)
        settings->window_geometry = g_strstrip (s);
    else
        settings->window_geometry = NULL;
    g_io_channel_unref (ioc);
    return TRUE;
}

/*
 * display the version and exit
 */
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
 *   data will be loaded from the directory specified by --config, otherwise
 *   the data will be retrieved from $HOME/.eagle-eye/.  Returns a new
 *   EESettings object if loading succeeded, otherwise returns NULL.
 */
EESettings *
ee_settings_load (int *argc, char ***argv)
{
    GOptionContext *ct;
    EESettings *settings;
    GError *error = NULL;
    gchar *home = NULL;
    gchar *cookies_file = NULL;
    gchar *geometry = NULL;

    GOptionEntry entries[] = 
    {
        { "config", 'c', 0, G_OPTION_ARG_FILENAME, &home, "Use DIR for storing configuration files", "DIR" },
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
        g_print ("\n");
        g_print ("Try '%s --help' for command line usage.\n", g_get_prgname ());
        g_option_context_free (ct);
        return NULL;
    }
    g_option_context_free (ct);

    /* alloc the settings object and set some defaults */
    settings = g_new0 (EESettings, 1);
    settings->home = NULL;
    settings->urls = NULL;
    settings->cycle_time = 30;
    settings->start_fullscreen = FALSE;
    settings->disable_plugins = FALSE;
    settings->disable_scripts = FALSE;
    settings->small_toolbar = FALSE;
    settings->remember_geometry = FALSE;

    /* if --config wasn't specified, then define it as $HOME/.eagle-eye */
    if (home)
        settings->home = g_strdup (home);
    else
        settings->home = g_build_filename (g_get_home_dir (), ".eagle-eye", NULL);

    /* create home if it doesn't exist */
    if (mkdir (settings->home, 0755) < 0 && errno != EEXIST) {
        g_warning ("failed to create %s: %s", settings->home, g_strerror (errno));
        g_free (settings);
        return NULL;
    }
  
    /* load config parameters */
    if (!read_config_file (settings)) {
        ee_settings_free (settings);
        return NULL;
    }

    /* load the urls file */
    if (!read_urls_file (settings)) {
        ee_settings_free (settings);
        return NULL;
    }

    /* load saved window geometry if specified in the config */
    if (!read_geometry_file (settings)) {
        ee_settings_free (settings);
        return NULL;
    }

    /* load geometry from the command line, if specified */
    if (geometry != NULL) {
        if (settings->window_geometry)
            g_free (settings->window_geometry);
        settings->window_geometry = g_strdup (geometry);
    }

    /* open the cookie jar */
    cookies_file = g_build_filename (settings->home, "cookies", NULL);
    settings->cookie_jar = soup_cookie_jar_text_new (cookies_file, FALSE);
    g_free (cookies_file);

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
 * ee_settings_remove_url: remove the URL at the specified index.
 *   returns TRUE if removal succeeded, otherwise FALSE if there was
 *   an error.
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
 * ee_settings_save: save settings to disk.
 */
gboolean
ee_settings_save (EESettings *settings)
{
    write_config_file (settings);
    write_urls_file (settings);
    write_geometry_file (settings);
    return TRUE;
}

/*
 * ee_settings_free: free all memory associated with the settings object.
 */
void
ee_settings_free (EESettings *settings)
{
    GList *item;

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
