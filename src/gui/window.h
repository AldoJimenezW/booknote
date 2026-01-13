#ifndef BOOKNOTE_WINDOW_H
#define BOOKNOTE_WINDOW_H

#include <gtk/gtk.h>
#include "../database/db.h"

/**
 * Main window structure
 */
typedef struct {
    GtkWidget *window;
    GtkWidget *main_paned;      // Horizontal split
    GtkWidget *content_paned;   // Vertical split (PDF | Notes)
    
    // Left panel (book list)
    GtkWidget *book_sidebar;
    GtkWidget *book_list;
    
    // Center panel (PDF viewer)
    GtkWidget *pdf_container;
    GtkWidget *pdf_scrolled;
    
    // Right panel (notes)
    GtkWidget *notes_container;
    GtkWidget *notes_view;
    
    // State
    Database *db;
    gboolean notes_visible;
    gboolean sidebar_visible;
} MainWindow;

/**
 * Create main window
 */
MainWindow* window_create(Database *db);

/**
 * Destroy window and cleanup
 */
void window_destroy(MainWindow *win);

/**
 * Toggle notes panel visibility
 */
void window_toggle_notes(MainWindow *win);

/**
 * Toggle sidebar visibility
 */
void window_toggle_sidebar(MainWindow *win);

#endif // BOOKNOTE_WINDOW_H
