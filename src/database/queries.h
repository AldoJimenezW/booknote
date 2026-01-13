#ifndef BOOKNOTE_QUERIES_H
#define BOOKNOTE_QUERIES_H

#include "db.h"
#include "../core/book.h"
#include "../core/note.h"

/**
 * Book database operations
 */

/**
 * Insert book into database
 * Updates book->id with the generated ID
 */
BnError db_book_insert(Database *db, Book *book);

/**
 * Get book by ID
 */
BnError db_book_get_by_id(Database *db, int id, Book **out_book);

/**
 * Get all books
 * Returns array of Book pointers, NULL-terminated
 */
BnError db_book_get_all(Database *db, Book ***out_books, int *out_count);

/**
 * Update book in database
 */
BnError db_book_update(Database *db, const Book *book);

/**
 * Delete book by ID
 * Cascades to delete all notes
 */
BnError db_book_delete(Database *db, int id);

/**
 * Note database operations
 */

/**
 * Insert note into database
 * Updates note->id with the generated ID
 */
BnError db_note_insert(Database *db, Note *note);

/**
 * Get note by ID
 */
BnError db_note_get_by_id(Database *db, int id, Note **out_note);

/**
 * Get all notes for a book
 */
BnError db_note_get_by_book(Database *db, int book_id, Note ***out_notes, int *out_count);

/**
 * Update note in database
 */
BnError db_note_update(Database *db, const Note *note);

/**
 * Delete note by ID
 */
BnError db_note_delete(Database *db, int id);

/**
 * Search notes by content (FTS)
 */
BnError db_note_search(Database *db, const char *query, Note ***out_notes, int *out_count);

#endif // BOOKNOTE_QUERIES_H
