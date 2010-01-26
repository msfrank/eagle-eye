#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <ee-main-window.h>
#include <ee-settings.h>

int
main (int argc, char *argv[])
{
    EESettings *settings;
    GtkWindow *window;

    /* we need to initialize threading before using webkit */
    g_thread_init (NULL);

    /* initialize gtk */
    gtk_init (&argc, &argv);

    /* load settings */
    settings = ee_settings_load (&argc, &argv);

    /* prepend URLs listed on the command line in reverse order */
    for (argc--; argc > 0; argc--)
        ee_settings_insert_url_from_string (settings, argv[argc], 0);

    /* create the main window */
    window = ee_main_window_construct (settings);

    /* hand control over to gtk main loop */
    gtk_main ();

    /* save settings to disk*/
    ee_settings_save (settings);

    return 0;
}
