#ifndef BOOKNOTE_NOTESVIEW_H
#define BOOKNOTE_NOTESVIEW_H

#include <gtk/gtk.h>
#include "../database/db.h"

/**
 * Load and display notes for a book
 */
void notesview_load_notes(GtkTextView *view, Database *db, int book_id);

/**
 * Clear notes view
 */
void notesview_clear(GtkTextView *view);

#endif // BOOKNOTE_NOTESVIEW_H
