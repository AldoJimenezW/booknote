#ifndef BOOKNOTE_BOOKLIST_H
#define BOOKNOTE_BOOKLIST_H

#include <gtk/gtk.h>
#include "../database/db.h"
#include "../core/book.h"

/**
 * Book list columns
 */
enum {
    COL_ID = 0,
    COL_TITLE,
    COL_AUTHOR,
    NUM_COLS
};

/**
 * Create and populate book list model
 */
GtkTreeModel* booklist_create_model(Database *db);

/**
 * Setup tree view columns
 */
void booklist_setup_view(GtkTreeView *view);

/**
 * Get book ID from selected row
 */
int booklist_get_selected_id(GtkTreeView *view);

/**
 * Show add book dialog
 */
void booklist_show_add_dialog(GtkWidget *parent, Database *db, void (*on_success)(void *), void *data);

#endif // BOOKNOTE_BOOKLIST_H
