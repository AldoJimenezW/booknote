#ifndef BOOKNOTE_LIBRARYVIEW_H
#define BOOKNOTE_LIBRARYVIEW_H

#include <gtk/gtk.h>
#include "../database/db.h"

/**
 * Library view - Grid of books with covers
 */
typedef struct {
    GtkWidget *container;      // Main container
    GtkWidget *scrolled;       // Scrolled window
    GtkWidget *grid;           // Grid/FlowBox of books
    GtkWidget *add_button;     // Add book button (floating)
    GtkWidget *edit_button;    // Edit selected book
    GtkWidget *delete_button;  // Delete selected book
    int selected_book_id;      // Currently selected book id
    
    Database *db;
    
    // Callback when book is selected
    void (*on_book_selected)(int book_id, gpointer user_data);
    gpointer user_data;
} LibraryView;

/**
 * Create library view
 */
LibraryView* libraryview_create(Database *db);

/**
 * Load books into library
 */
void libraryview_load_books(LibraryView *view);

/**
 * Set callback for book selection
 */
void libraryview_set_callback(LibraryView *view, 
                              void (*callback)(int book_id, gpointer data),
                              gpointer user_data);

/**
 * Show Add Book dialog with ISBN lookup, fetch metadata and cover.
 * On success, inserts book into DB and triggers a refresh via the provided refresh callback.
 *
 * @param parent         Parent GtkWindow for modality
 * @param db             Database handle
 * @param on_refreshed   Callback to invoke after insertion (e.g., libraryview_load_books)
 * @param user_data      User data to pass to on_refreshed
 */
void libraryview_show_add_dialog(GtkWidget *parent,
                                 Database *db,
                                 void (*on_refreshed)(void *user_data),
                                 void *user_data);

/**
 * Destroy library view
 */
void libraryview_destroy(LibraryView *view);

#endif // BOOKNOTE_LIBRARYVIEW_H
