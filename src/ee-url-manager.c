#include <glib.h>
#include <gtk/gtk.h>
#include <libsoup/soup.h>
#include <ee-settings.h>

static GtkWidget *
construct_url_tree_view (GList *urls)
{

}

/*
 *
 */
GtkWidget *
ee_url_manager_new (EESettings *settings)
{
    GtkWidget *dialog;
    GtkWidget *align;
    GtkListStore *store;
    GList *curr;
    GtkWidget *tree_view;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    dialog = gtk_dialog_new_with_buttons ("URL Manager",
        NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CLOSE,
        GTK_RESPONSE_CLOSE, NULL);

    align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (align), 12, 12, 12, 12);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog)->vbox), align, TRUE, TRUE, 0);

    store = gtk_list_store_new (1, G_TYPE_STRING);

    for (curr = settings->urls; curr; curr = g_list_next (curr)) {
        gchar *s;
        GtkTreeIter iter;

        s = soup_uri_to_string ((SoupURI *) curr->data, FALSE);
        gtk_list_store_append (GTK_LIST_STORE (store), &iter);
        gtk_list_store_set (GTK_LIST_STORE (store), &iter, 0, s, -1);
        g_free (s);
    }

    tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
    gtk_container_add (GTK_CONTAINER (align), tree_view);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("URL",
        renderer, "text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

    gtk_widget_show_all (dialog);
    
    return dialog;
}
