#include "notesview.h"
#include "../database/queries.h"
#include "../core/note.h"
#include <stdio.h>

void notesview_load_notes(GtkTextView *view, Database *db, int book_id) {
    if (!view || !db || book_id <= 0) return;
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);
    
    // Load notes from database
    Note **notes = NULL;
    int count = 0;
    
    BnError err = db_note_get_by_book(db, book_id, &notes, &count);
    if (err != BN_SUCCESS) {
        gtk_text_buffer_set_text(buffer, "Error loading notes", -1);
        return;
    }
    
    if (count == 0) {
        gtk_text_buffer_set_text(buffer, 
            "No notes yet.\n\n"
            "Click '+ Add Note' to create your first note.", -1);
        return;
    }
    
    // Build notes text
    GString *text = g_string_new("");
    
    for (int i = 0; i < count; i++) {
        g_string_append_printf(text, "[%d] ", notes[i]->id);
        
        if (notes[i]->page_number > 0) {
            g_string_append_printf(text, "(page %d) ", notes[i]->page_number);
        }
        
        g_string_append_printf(text, "%s\n\n", notes[i]->content);
        
        note_free(notes[i]);
    }
    free(notes);
    
    gtk_text_buffer_set_text(buffer, text->str, -1);
    g_string_free(text, TRUE);
}

void notesview_clear(GtkTextView *view) {
    if (!view) return;
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);
    gtk_text_buffer_set_text(buffer, "Select a book to view notes", -1);
}
