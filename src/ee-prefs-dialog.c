#include <glib.h>
#include <gtk/gtk.h>
#include <ee-main-window.h>
#include <ee-settings.h>

void
on_cycle_time_changed (GtkSpinButton *      button,
                       EEMainWindow *       mainwin)
{
    mainwin->settings->cycle_time =
        (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON (button));
    ee_settings_save (mainwin->settings);
}

void
on_start_fullscreen (GtkToggleButton *      button,
                     EEMainWindow *         mainwin)
{
    mainwin->settings->start_fullscreen = 
        gtk_toggle_button_get_active (button);
    ee_settings_save (mainwin->settings);
}

void
on_disable_plugins (GtkToggleButton *       button,
                    EEMainWindow *          mainwin)
{
    gboolean disable_plugins = gtk_toggle_button_get_active (button);
    WebKitWebSettings *websettings = webkit_web_view_get_settings (mainwin->webview);
    mainwin->settings->disable_plugins = disable_plugins;
    g_object_set (websettings, "enable-plugins", !disable_plugins, NULL);
    ee_settings_save (mainwin->settings);
}

void
on_disable_scripts (GtkToggleButton *       button,
                    EEMainWindow *          mainwin)
{
    gboolean disable_scripts = gtk_toggle_button_get_active (button);
    WebKitWebSettings *websettings = webkit_web_view_get_settings (mainwin->webview);
    mainwin->settings->disable_scripts = disable_scripts;
    g_object_set (websettings, "enable-scripts", !disable_scripts, NULL);
    ee_settings_save (mainwin->settings);
}

/*
 *
 */
void
ee_prefs_dialog_run (EEMainWindow *mainwin)
{
    GtkWidget *dialog;
    GtkWidget *align;
    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *cycle_time;
    GtkWidget *start_fullscreen;
    GtkWidget *disable_plugins;
    GtkWidget *disable_scripts;

    /* create the toplevel dialog window */
    dialog = gtk_dialog_new_with_buttons ("Preferences", NULL,
        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

    /* create 12px padding */
    align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (align), 12, 12, 12, 12);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog)->vbox), align, TRUE, TRUE, 0);

    table = gtk_table_new (4, 2, FALSE);
    gtk_container_add (GTK_CONTAINER (align), table);

    /* add the cycle time spinner */
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), "<b>Cycle Time</b>");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
        GTK_EXPAND | GTK_FILL, GTK_SHRINK, 6, 2);
    cycle_time = gtk_spin_button_new_with_range (10.0, 86400.0, 10.0);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (cycle_time),
        (gdouble) mainwin->settings->cycle_time);
    g_signal_connect (cycle_time, "value-changed",
        G_CALLBACK (on_cycle_time_changed), mainwin);
    gtk_table_attach (GTK_TABLE (table), cycle_time, 1, 2, 0, 1,
        GTK_EXPAND | GTK_FILL, GTK_SHRINK, 6, 2);

    /* add the start-fullscreen checkbox */
    start_fullscreen = gtk_check_button_new_with_label ("Start in fullscreen mode");
    if (mainwin->settings->start_fullscreen)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (start_fullscreen), TRUE);
    g_signal_connect (start_fullscreen, "toggled",
        G_CALLBACK (on_start_fullscreen), mainwin);
    gtk_table_attach (GTK_TABLE (table), start_fullscreen, 0, 2, 1, 2,
        GTK_EXPAND | GTK_FILL, GTK_SHRINK, 6, 2);

    /* add the disable-plugins checkbox */
    disable_plugins = gtk_check_button_new_with_label ("Disable browser plugins");
    if (mainwin->settings->disable_plugins)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (disable_plugins), TRUE);
    g_signal_connect (disable_plugins, "toggled",
        G_CALLBACK (on_disable_plugins), mainwin);
    gtk_table_attach (GTK_TABLE (table), disable_plugins, 0, 2, 2, 3,
        GTK_EXPAND | GTK_FILL, GTK_SHRINK, 6, 2);

    /* add the disable-scripts checkbox */
    disable_scripts = gtk_check_button_new_with_label ("Disable browser scripts");
    if (mainwin->settings->disable_scripts)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (disable_scripts), TRUE);
    g_signal_connect (disable_scripts, "toggled",
        G_CALLBACK (on_disable_scripts), mainwin);
    gtk_table_attach (GTK_TABLE (table), disable_scripts, 0, 2, 3, 4,
        GTK_EXPAND | GTK_FILL, GTK_SHRINK, 6, 2);

    /* run the dialog */
    gtk_widget_show_all (dialog);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}
