#ifndef BOOKNOTE_NOTESVIEW_H
#define BOOKNOTE_NOTESVIEW_H

#include <gtk/gtk.h>
#include "../database/db.h"
#include "orgmode.h"
#include "../core/note.h"

/**
 * Notes panel columns
 */
enum {
    NOTE_COL_ID = 0,
    NOTE_COL_TITLE,
    NOTE_COL_PAGE,
    NOTE_COL_NUM
};

/**
 * Notes panel widget structure
 */
typedef struct {
    GtkWidget *container;       // Main container
    GtkWidget *notes_list;      // TreeView for note list
    GtkWidget *editor;          // TextView for editing
    GtkWidget *save_button;     // Save button
    GtkWidget *delete_button;   // Delete button
    
    Database *db;
    int current_book_id;
    int current_note_id;        // -1 if no note selected
} NotesPanel;

/**
 * Create notes panel
 */
NotesPanel* notespanel_create(Database *db);

/**
 * Load notes for a book
 */
void notespanel_load_book(NotesPanel *panel, int book_id);

/**
 * Clear panel (no book selected)
 */
void notespanel_clear(NotesPanel *panel);

/**
 * Destroy panel
 */
void notespanel_destroy(NotesPanel *panel);

#endif // BOOKNOTE_NOTESVIEW_H
