#ifndef BOOKNOTE_WINDOW_H
#define BOOKNOTE_WINDOW_H

#include <gtk/gtk.h>
#include "notesview.h"
#include "pdfviewer.h"
#include "libraryview.h"
#include "../database/db.h"

/**
 * Main window structure
 */
typedef struct {
    GtkWidget *window;

    // Navigation stack
    GtkWidget *stack;                // GtkStack containing "library" and "reading" views
    GtkWidget *library_container;    // Container for LibraryView
    GtkWidget *reading_container;    // Container for Reading view (PDF + Notes)
    GtkWidget *back_button;          // Back to Library button in reading header

    // Legacy panes (kept for backward compatibility during transition)
    GtkWidget *main_paned;           // Horizontal split
    GtkWidget *content_paned;        // Vertical split (PDF | Notes)

    // Left panel (book list)
    GtkWidget *book_sidebar;
    GtkWidget *book_list;
    GtkWidget *delete_book_button;   // Delete book button

    // Center panel (PDF viewer)
    PDFViewer *pdf_viewer;

    // Right panel (notes)
    GtkWidget *notes_container;
    NotesPanel *notes_panel;

    // Library view widget
    LibraryView *library_view;

    // State
    Database *db;
    gboolean notes_visible;

    // Current state
    int current_book_id;
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

// Navigation helpers
void window_show_library(MainWindow *win);
void window_show_reading(MainWindow *win, int book_id);

#endif // BOOKNOTE_WINDOW_H
