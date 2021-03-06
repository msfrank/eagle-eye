#include <glib.h>
#include <gtk/gtk.h>
#include <libsoup/soup.h>
#include <ee-settings.h>

enum { URL_COLUMN, USER_COLUMN, PASSWORD_COLUMN, N_COLUMNS };

/*
 * 
 */
static void
on_row_inserted (GtkTreeModel *         model,
                 GtkTreePath *          path,
                 GtkTreeIter *          iter,
                 EESettings *           settings)
{
    gint *indices;
    gchar *url, *username, *password;
    SoupURI *uri;

    indices = gtk_tree_path_get_indices (path);
    if (!indices)
        return;
    gtk_tree_model_get (model, iter, URL_COLUMN, &url,
        USER_COLUMN, &username, PASSWORD_COLUMN, &password, -1);
    uri = soup_uri_new (url);
    if (username && username[0] != '\0')
        soup_uri_set_user (uri, username);
    if (password && password[0] != '\0')
    soup_uri_set_password (uri, password);
    if (ee_settings_insert_url (settings, uri, indices[0]))
        g_debug ("inserted row at position %i", indices[0]);
    soup_uri_free (uri);
    ee_settings_save(settings);
}

/*
 *
 */
static void
on_row_deleted (GtkTreeModel *          model,
                GtkTreePath *           path,
                EESettings *            settings)
{
    gint *indices;

    indices = gtk_tree_path_get_indices (path);
    if (!indices)
        return;
    if (indices[0] < 0) {
        g_warning ("invalid URL index %i", indices[0]);
        return;
    }
    ee_settings_remove_url (settings, (guint) indices[0]);
    g_debug ("deleted row at position %i", (guint) indices[0]);
    ee_settings_save(settings);
}

/*
 *
 */
static void
on_clicked_add (GtkToolButton *         button,
                GtkTreeView *           tree_view)
{
    GtkWidget *dialog;
    GtkWidget *align;
    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *url_entry;
    GtkWidget *user_entry;
    GtkWidget *pass_entry;

    g_debug ("---- ADD URL ----");

    /* create the toplevel dialog window */
    dialog = gtk_dialog_new_with_buttons ("Add a new URL ...",
        NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL,
        GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

    /* create 12px padding */
    align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (align), 12, 12, 12, 12);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog)->vbox), align, TRUE, TRUE, 0);

    table = gtk_table_new (3, 2, FALSE);
    gtk_container_add (GTK_CONTAINER (align), table);

    /* add the URL label and entry box */
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), "<b>URL</b>");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
        GTK_EXPAND | GTK_FILL, GTK_SHRINK, 6, 2);
    url_entry = gtk_entry_new ();
    gtk_entry_set_width_chars (GTK_ENTRY (url_entry), 40);
    gtk_entry_set_activates_default (GTK_ENTRY (url_entry), TRUE);
    gtk_table_attach (GTK_TABLE (table), url_entry, 1, 2, 0, 1,
        GTK_EXPAND | GTK_FILL, GTK_SHRINK, 6, 2);

    /* add the Username label and entry box */
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), "<b>Username</b> (optional)");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
        GTK_EXPAND | GTK_FILL, GTK_SHRINK, 6, 2);
    user_entry = gtk_entry_new ();
    gtk_entry_set_width_chars (GTK_ENTRY (user_entry), 32);
    gtk_entry_set_activates_default (GTK_ENTRY (user_entry), TRUE);
    gtk_table_attach (GTK_TABLE (table), user_entry, 1, 2, 1, 2,
        GTK_EXPAND | GTK_FILL, GTK_SHRINK, 6, 2);

    /* add the Password label and entry box */
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), "<b>Password</b> (optional)");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
        GTK_EXPAND | GTK_FILL, GTK_SHRINK, 6, 2);
    pass_entry = gtk_entry_new ();
    gtk_entry_set_width_chars (GTK_ENTRY (pass_entry), 32);
    gtk_entry_set_visibility (GTK_ENTRY (pass_entry), FALSE);
    gtk_entry_set_activates_default (GTK_ENTRY (pass_entry), TRUE);
    gtk_table_attach (GTK_TABLE (table), pass_entry, 1, 2, 2, 3,
        GTK_EXPAND | GTK_FILL, GTK_SHRINK, 6, 2);

    /* run the dialog */
    gtk_widget_show_all (dialog);
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
        const gchar *url = gtk_entry_get_text (GTK_ENTRY (url_entry));
        const gchar *user = gtk_entry_get_text (GTK_ENTRY (user_entry));
        const gchar *pass = gtk_entry_get_text (GTK_ENTRY (pass_entry));
        SoupURI *uri;

        uri = soup_uri_new (url);
        if (uri && SOUP_URI_VALID_FOR_HTTP (uri)) {
            GtkTreeModel *model;
            GtkTreeIter iter;

            if (user && user[0] != '\0')
                soup_uri_set_user (uri, user);
            if (pass && pass[0] != '\0')
                soup_uri_set_password (uri, pass);
            model = gtk_tree_view_get_model (tree_view);
            gtk_list_store_append (GTK_LIST_STORE (model), &iter);
            gtk_list_store_set (GTK_LIST_STORE (model), &iter, URL_COLUMN, url,
                USER_COLUMN, user, PASSWORD_COLUMN, pass, -1);
            soup_uri_free (uri);
        }
        else {
            GtkWidget *alert;
    
            alert = gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Failed to add URL: URL is invalid");
            gtk_dialog_run (GTK_DIALOG (alert));
            gtk_widget_destroy (alert);
        }
    }

    gtk_widget_destroy (dialog);
}

/*
 *
 */
static void
on_clicked_remove (GtkToolButton *      button,
                   GtkTreeView *        tree_view)
{
    GtkTreeSelection *select;
    GtkTreeModel *model;
    GtkTreeIter iter;

    g_debug ("---- REMOVE URL ----");
    select = gtk_tree_view_get_selection (tree_view);
    /* if nothing is selected, then return */
    if (!gtk_tree_selection_get_selected (select, &model, &iter))
        return;
    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
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
    store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

    /* load the list store from settings->urls */
    for (curr = settings->urls; curr; curr = g_list_next (curr)) {
        SoupURI *uri;
        gchar *url, *user, *password;
        GtkTreeIter iter;

        uri = (SoupURI *) curr->data;
        url = soup_uri_to_string (uri, FALSE);
        user = uri->user ? g_strdup (uri->user) : NULL;
        password = uri->password ? g_strdup (uri->password) : NULL;

        gtk_list_store_append (GTK_LIST_STORE (store), &iter);
        gtk_list_store_set (GTK_LIST_STORE (store), &iter, URL_COLUMN, url,
            USER_COLUMN, user, PASSWORD_COLUMN, password, -1);
        if (url)
            g_free (url);
        if (user)
            g_free (user);
        if (password)
            g_free (password);
    }

    /*
     * connect to store modification signals.  we do this *after* loading
     * the store with initial data, otherwise the on_row_inserted callback
     * will be called for each row.
     */
    g_signal_connect (store, "row-changed",
        G_CALLBACK (on_row_inserted), settings);
    g_signal_connect (store, "row-deleted",
        G_CALLBACK (on_row_deleted), settings);

    /* create the tree view and pack it into the vbox */
    tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
    gtk_tree_view_set_reorderable (GTK_TREE_VIEW (tree_view), TRUE);
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
        G_CALLBACK (on_clicked_add), tree_view);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), add, -1);
    remove = gtk_tool_button_new_from_stock (GTK_STOCK_REMOVE);
    gtk_tool_item_set_tooltip_text (remove, "Remove URL");
    g_signal_connect (remove, "clicked",
        G_CALLBACK (on_clicked_remove), tree_view);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), remove, -1);
 
    gtk_widget_show_all (dialog);
    
    return dialog;
}
