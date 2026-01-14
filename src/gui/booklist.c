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

static void on_browse_clicked(GtkWidget *button, gpointer entry) {
    (void)button;
    
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select PDF File",
        NULL,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Open", GTK_RESPONSE_ACCEPT,
        NULL);
    
    // Add PDF filter
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "PDF Files");
    gtk_file_filter_add_pattern(filter, "*.pdf");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_entry_set_text(GTK_ENTRY(entry), filename);
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
}

void booklist_show_add_dialog(GtkWidget *parent, Database *db, void (*on_success)(void *), void *data) {
    // Create dialog
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Add Book",
        GTK_WINDOW(parent),
        GTK_DIALOG_MODAL,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Add", GTK_RESPONSE_OK,
        NULL);
    
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 10);
    
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_add(GTK_CONTAINER(content_area), grid);
    
    // Filepath (required)
    GtkWidget *filepath_label = gtk_label_new("PDF Path:");
    GtkWidget *filepath_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(filepath_entry), "/path/to/book.pdf");
    gtk_grid_attach(GTK_GRID(grid), filepath_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), filepath_entry, 1, 0, 1, 1);
    
    // Browse button
    GtkWidget *browse_button = gtk_button_new_with_label("Browse...");
    gtk_grid_attach(GTK_GRID(grid), browse_button, 2, 0, 1, 1);
    
    // Title (required)
    GtkWidget *title_label = gtk_label_new("Title:");
    GtkWidget *title_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(title_entry), "Book title");
    gtk_grid_attach(GTK_GRID(grid), title_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), title_entry, 1, 1, 2, 1);
    
    // Author (optional)
    GtkWidget *author_label = gtk_label_new("Author:");
    GtkWidget *author_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(author_entry), "Optional");
    gtk_grid_attach(GTK_GRID(grid), author_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), author_entry, 1, 2, 2, 1);
    
    // Year (optional)
    GtkWidget *year_label = gtk_label_new("Year:");
    GtkWidget *year_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(year_entry), "Optional");
    gtk_grid_attach(GTK_GRID(grid), year_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), year_entry, 1, 3, 2, 1);
    
    // ISBN (optional)
    GtkWidget *isbn_label = gtk_label_new("ISBN:");
    GtkWidget *isbn_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(isbn_entry), "Optional");
    gtk_grid_attach(GTK_GRID(grid), isbn_label, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), isbn_entry, 1, 4, 2, 1);
    
    gtk_widget_show_all(content_area);
    
    // Browse button callback
    g_signal_connect(browse_button, "clicked", G_CALLBACK(on_browse_clicked), filepath_entry);
    
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (response == GTK_RESPONSE_OK) {
        const char *filepath = gtk_entry_get_text(GTK_ENTRY(filepath_entry));
        const char *title = gtk_entry_get_text(GTK_ENTRY(title_entry));
        const char *author = gtk_entry_get_text(GTK_ENTRY(author_entry));
        const char *year_str = gtk_entry_get_text(GTK_ENTRY(year_entry));
        const char *isbn = gtk_entry_get_text(GTK_ENTRY(isbn_entry));
        
        // Validate
        if (!filepath || strlen(filepath) == 0) {
            GtkWidget *error = gtk_message_dialog_new(GTK_WINDOW(parent),
                GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                "PDF path is required");
            gtk_dialog_run(GTK_DIALOG(error));
            gtk_widget_destroy(error);
            gtk_widget_destroy(dialog);
            return;
        }
        
        if (!title || strlen(title) == 0) {
            GtkWidget *error = gtk_message_dialog_new(GTK_WINDOW(parent),
                GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                "Title is required");
            gtk_dialog_run(GTK_DIALOG(error));
            gtk_widget_destroy(error);
            gtk_widget_destroy(dialog);
            return;
        }
        
        // Create book
        Book *book = NULL;
        BnError err = book_create(&book, title, filepath);
        
        if (err == BN_SUCCESS) {
            if (author && strlen(author) > 0) {
                book_set_author(book, author);
            }
            if (year_str && strlen(year_str) > 0) {
                int year = atoi(year_str);
                if (year > 0) {
                    book_set_year(book, year);
                }
            }
            if (isbn && strlen(isbn) > 0) {
                book_set_isbn(book, isbn);
            }
            
            err = db_book_insert(db, book);
            
            if (err == BN_SUCCESS) {
                GtkWidget *success = gtk_message_dialog_new(GTK_WINDOW(parent),
                    GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                    "Book added successfully!");
                gtk_dialog_run(GTK_DIALOG(success));
                gtk_widget_destroy(success);
                
                // Callback to refresh list
                if (on_success) {
                    on_success(data);
                }
            } else {
                GtkWidget *error = gtk_message_dialog_new(GTK_WINDOW(parent),
                    GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                    "Error adding book to database");
                gtk_dialog_run(GTK_DIALOG(error));
                gtk_widget_destroy(error);
            }
            
            book_free(book);
        }
    }
    
    gtk_widget_destroy(dialog);
}
