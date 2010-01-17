#include <glib.h>
#include <gtk/gtk.h>
#include <libsoup/soup.h>
#include <ee-settings.h>

/*
 *
 */
static void
on_clicked_add (GtkToolButton *         button,
                EESettings *            settings)
{
    g_debug ("---- ADD URL ----");
}

/*
 *
 */
static void
on_clicked_remove (GtkToolButton *      button,
                   EESettings *         settings)
{
    g_debug ("---- REMOVE URL ----");
}

/*
 *
 */
GtkWidget *
ee_url_manager_new (EESettings *settings)
{
    GtkWidget *dialog;
    GtkWidget *align;
    GtkWidget *frame;
    GtkWidget *vbox;
    GtkListStore *store;
    GList *curr;
    GtkWidget *tree_view;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkWidget *toolbar;
    GtkToolItem *add;
    GtkToolItem *remove;

    /* create the toplevel dialog window */
    dialog = gtk_dialog_new_with_buttons ("URL Manager",
        NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CLOSE,
        GTK_RESPONSE_CLOSE, NULL);
    gtk_window_set_default_size (GTK_WINDOW (dialog), 480, 320);

    /* create 12px padding */
    align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (align), 12, 12, 12, 12);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog)->vbox), align, TRUE, TRUE, 0);

    /* create the decorative frame */
    frame = gtk_frame_new (NULL);
    gtk_container_add (GTK_CONTAINER (align), frame);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (frame), vbox);

    /* create the URL list store */
    store = gtk_list_store_new (1, G_TYPE_STRING);

    /* load the list store from settings->urls */
    for (curr = settings->urls; curr; curr = g_list_next (curr)) {
        gchar *s;
        GtkTreeIter iter;

        s = soup_uri_to_string ((SoupURI *) curr->data, FALSE);
        gtk_list_store_append (GTK_LIST_STORE (store), &iter);
        gtk_list_store_set (GTK_LIST_STORE (store), &iter, 0, s, -1);
        g_free (s);
    }

    /* create the tree view and pack it into the vbox */
    tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
    gtk_box_pack_start (GTK_BOX (vbox), tree_view, TRUE, TRUE, 0);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("URL",
        renderer, "text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

    /* create the add/remove toolbar */
    toolbar = gtk_toolbar_new ();
    gtk_box_pack_start(GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
    g_object_set (toolbar, "icon-size", 1, NULL);

    /* add the toolbar items */
    add = gtk_tool_button_new_from_stock (GTK_STOCK_ADD);
    gtk_tool_item_set_tooltip_text (add, "Add URL");
    g_signal_connect (add, "clicked",
        G_CALLBACK (on_clicked_add), settings);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), add, -1);
    remove = gtk_tool_button_new_from_stock (GTK_STOCK_REMOVE);
    gtk_tool_item_set_tooltip_text (remove, "Remove URL");
    g_signal_connect (remove, "clicked",
        G_CALLBACK (on_clicked_remove), settings);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), remove, -1);
 
    gtk_widget_show_all (dialog);
    
    return dialog;
}
