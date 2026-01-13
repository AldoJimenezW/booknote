#include "booklist.h"
#include "../database/queries.h"
#include <stdlib.h>

GtkTreeModel* booklist_create_model(Database *db) {
    GtkListStore *store = gtk_list_store_new(NUM_COLS,
                                             G_TYPE_INT,      // ID
                                             G_TYPE_STRING,   // Title
                                             G_TYPE_STRING);  // Author
    
    // Load books from database
    Book **books = NULL;
    int count = 0;
    
    BnError err = db_book_get_all(db, &books, &count);
    if (err != BN_SUCCESS || count == 0) {
        return GTK_TREE_MODEL(store);
    }
    
    // Populate store
    for (int i = 0; i < count; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                          COL_ID, books[i]->id,
                          COL_TITLE, books[i]->title,
                          COL_AUTHOR, books[i]->author ? books[i]->author : "",
                          -1);
        book_free(books[i]);
    }
    free(books);
    
    return GTK_TREE_MODEL(store);
}

void booklist_setup_view(GtkTreeView *view) {
    // Title column
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
    
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
        "Book",
        renderer,
        "text", COL_TITLE,
        NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_append_column(view, column);
    
    // Hide headers for cleaner look
    gtk_tree_view_set_headers_visible(view, FALSE);
}

int booklist_get_selected_id(GtkTreeView *view) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(view);
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        return -1;
    }
    
    int id;
    gtk_tree_model_get(model, &iter, COL_ID, &id, -1);
    return id;
}
