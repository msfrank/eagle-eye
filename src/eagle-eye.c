#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <ee-main-window.h>
#include <ee-settings.h>

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

static GOptionEntry entries[] = 
{
    { "version", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, on_parse_version_option, "Display program version", NULL },
    { NULL }
};

int
main (int argc, char *argv[])
{
    GOptionContext *ct;
    EESettings *settings;
    GtkWindow *window;
    GError *error = NULL;

    /* we need to initialize threading before using webkit */
    g_thread_init (NULL);

    /* initialize gtk */
    gtk_init (&argc, &argv);

    /* parse command line arguments */
    ct = g_option_context_new ("- iterate through web pages at regular intervals");
    g_option_context_add_group (ct, gtk_get_option_group (TRUE));
    g_option_context_add_main_entries (ct, entries, NULL);
    if (!g_option_context_parse (ct, &argc, &argv, &error)) {
        g_print ("failed to parse options: %s\n", error->message);
        exit (1);
    }
    g_option_context_free (ct);

    /* load settings */
    settings = ee_settings_new ();

    /* create the main window */
    window = ee_main_window_construct (settings);

    /* hand control over to gtk main loop */
    gtk_main ();

    ee_settings_free (settings);

    return 0;
}
