#include <gtk/gtk.h>
#include <ee-main-window.h>
#include <ee-settings.h>

int
main (int argc, char *argv[])
{
    EESettings *settings;
    GtkWindow *window;

    g_thread_init (NULL);
    gtk_init (&argc, &argv);

    settings = ee_settings_new ();
    window = ee_main_window_construct (settings);

    gtk_main ();

    ee_settings_free (settings);

    return 0;
}
