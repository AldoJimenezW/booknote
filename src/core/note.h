#ifndef BOOKNOTE_NOTE_H
#define BOOKNOTE_NOTE_H

#include <time.h>
#include "../utils/error.h"

/**
 * Note structure representing a note for a book
 */
typedef struct {
    int id;
    int book_id;         // Foreign key to books table
    char *title;         // Note title
    char *content;       // Note text content
    int page_number;     // Page reference (0 if not page-specific)
    time_t created_at;
    time_t updated_at;
} Note;

/**
 * Create a new note object
 * 
 * @param out_note Pointer to store created note
 * @param book_id ID of the book this note belongs to
 * @param content Note text content (required)
 * @param page_number Page reference (0 for no specific page)
 * @return BN_SUCCESS on success, error code otherwise
 */
BnError note_create(Note **out, int book_id, const char *title, const char *content, int page_number);

/**
 * Free note memory
 */
void note_free(Note *note);

/**
 * Update note content
 */
BnError note_set_content(Note *note, const char *content);

/**
 * Update note page number
 */
BnError note_set_page(Note *note, int page_number);

/**
 * Set note title
 */
BnError note_set_title(Note *note, const char *title);

#endif // BOOKNOTE_NOTE_H
