#define _POSIX_C_SOURCE 200809L
#include "queries.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Book operations
 * ========================================================================= */

BnError db_book_insert(Database *db, Book *book) {
    if (!db || !db->handle || !book) {
        return BN_ERROR_INVALID_ARG;
    }
    
    const char *sql = 
        "INSERT INTO books (isbn, title, author, year, publisher, filepath, cover_path, added_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return BN_ERROR_DATABASE;
    }
    
    // Bind parameters (1-indexed)
    sqlite3_bind_text(stmt, 1, book->isbn, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, book->title, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, book->author, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, book->year);
    sqlite3_bind_text(stmt, 5, book->publisher, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, book->filepath, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, book->cover_path, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 8, (sqlite3_int64)book->added_at);
    sqlite3_bind_int64(stmt, 9, (sqlite3_int64)book->updated_at);
    
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return BN_ERROR_DATABASE;
    }
    
    // Get auto-generated ID
    book->id = (int)sqlite3_last_insert_rowid(db->handle);
    
    sqlite3_finalize(stmt);
    return BN_SUCCESS;
}

BnError db_book_get_by_id(Database *db, int id, Book **out_book) {
    if (!db || !db->handle || !out_book || id <= 0) {
        return BN_ERROR_INVALID_ARG;
    }
    
    const char *sql = 
        "SELECT id, isbn, title, author, year, publisher, filepath, cover_path, added_at, updated_at "
        "FROM books WHERE id = ?;";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return BN_ERROR_DATABASE;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        Book *book = calloc(1, sizeof(Book));
        if (!book) {
            sqlite3_finalize(stmt);
            return BN_ERROR_OUT_OF_MEMORY;
        }
        
        book->id = sqlite3_column_int(stmt, 0);
        
        const char *isbn = (const char *)sqlite3_column_text(stmt, 1);
        book->isbn = isbn ? strdup(isbn) : NULL;
        
        const char *title = (const char *)sqlite3_column_text(stmt, 2);
        book->title = strdup(title);
        
        const char *author = (const char *)sqlite3_column_text(stmt, 3);
        book->author = author ? strdup(author) : NULL;
        
        book->year = sqlite3_column_int(stmt, 4);
        
        const char *publisher = (const char *)sqlite3_column_text(stmt, 5);
        book->publisher = publisher ? strdup(publisher) : NULL;
        
        const char *filepath = (const char *)sqlite3_column_text(stmt, 6);
        book->filepath = strdup(filepath);

        const char *cover_path = (const char *)sqlite3_column_text(stmt, 7);
        book->cover_path = cover_path ? strdup(cover_path) : NULL;
        
        book->added_at = (time_t)sqlite3_column_int64(stmt, 8);
        book->updated_at = (time_t)sqlite3_column_int64(stmt, 9);
        
        *out_book = book;
        sqlite3_finalize(stmt);
        return BN_SUCCESS;
    } else if (rc == SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return BN_ERROR_NOT_FOUND;
    } else {
        sqlite3_finalize(stmt);
        return BN_ERROR_DATABASE;
    }
}

BnError db_book_get_all(Database *db, Book ***out_books, int *out_count) {
    if (!db || !db->handle || !out_books || !out_count) {
        return BN_ERROR_INVALID_ARG;
    }
    
    const char *sql = 
        "SELECT id, isbn, title, author, year, publisher, filepath, cover_path, added_at, updated_at "
        "FROM books ORDER BY title;";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return BN_ERROR_DATABASE;
    }
    
    // First, count rows
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
    }
    sqlite3_reset(stmt);
    
    if (count == 0) {
        *out_books = NULL;
        *out_count = 0;
        sqlite3_finalize(stmt);
        return BN_SUCCESS;
    }
    
    // Allocate array
    Book **books = calloc(count, sizeof(Book *));
    if (!books) {
        sqlite3_finalize(stmt);
        return BN_ERROR_OUT_OF_MEMORY;
    }
    
    // Fetch rows
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < count) {
        Book *book = calloc(1, sizeof(Book));
        if (!book) {
            // Cleanup on error
            for (int j = 0; j < i; j++) {
                book_free(books[j]);
            }
            free(books);
            sqlite3_finalize(stmt);
            return BN_ERROR_OUT_OF_MEMORY;
        }
        
        book->id = sqlite3_column_int(stmt, 0);
        
        const char *isbn = (const char *)sqlite3_column_text(stmt, 1);
        book->isbn = isbn ? strdup(isbn) : NULL;
        
        const char *title = (const char *)sqlite3_column_text(stmt, 2);
        book->title = strdup(title);
        
        const char *author = (const char *)sqlite3_column_text(stmt, 3);
        book->author = author ? strdup(author) : NULL;
        
        book->year = sqlite3_column_int(stmt, 4);
        
        const char *publisher = (const char *)sqlite3_column_text(stmt, 5);
        book->publisher = publisher ? strdup(publisher) : NULL;
        
        const char *filepath = (const char *)sqlite3_column_text(stmt, 6);
        book->filepath = strdup(filepath);

        const char *cover_path = (const char *)sqlite3_column_text(stmt, 7);
        book->cover_path = cover_path ? strdup(cover_path) : NULL;
        
        book->added_at = (time_t)sqlite3_column_int64(stmt, 8);
        book->updated_at = (time_t)sqlite3_column_int64(stmt, 9);
        
        books[i++] = book;
    }
    
    *out_books = books;
    *out_count = count;
    
    sqlite3_finalize(stmt);
    return BN_SUCCESS;
}

BnError db_book_update(Database *db, const Book *book) {
    if (!db || !db->handle || !book || book->id <= 0) {
        return BN_ERROR_INVALID_ARG;
    }
    
    const char *sql = 
        "UPDATE books SET isbn = ?, title = ?, author = ?, year = ?, "
        "publisher = ?, filepath = ?, cover_path = ?, updated_at = ? WHERE id = ?;";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return BN_ERROR_DATABASE;
    }
    
    sqlite3_bind_text(stmt, 1, book->isbn, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, book->title, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, book->author, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, book->year);
    sqlite3_bind_text(stmt, 5, book->publisher, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, book->filepath, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, book->cover_path, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 8, (sqlite3_int64)book->updated_at);
    sqlite3_bind_int(stmt, 9, book->id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        return BN_ERROR_DATABASE;
    }
    
    return BN_SUCCESS;
}

BnError db_book_delete(Database *db, int id) {
    if (!db || !db->handle || id <= 0) {
        return BN_ERROR_INVALID_ARG;
    }
    
    const char *sql = "DELETE FROM books WHERE id = ?;";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return BN_ERROR_DATABASE;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        return BN_ERROR_DATABASE;
    }
    
    return BN_SUCCESS;
}

/* ============================================================================
 * Note operations (simplified versions for now)
 * ========================================================================= */

BnError db_note_insert(Database *db, Note *note) {
    if (!db || !db->handle || !note) {
        return BN_ERROR_INVALID_ARG;
    }
    
    const char *sql = 
        "INSERT INTO notes (book_id, title, content, page_number, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?);";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return BN_ERROR_DATABASE;
    }
    
    sqlite3_bind_int(stmt, 1, note->book_id);
    sqlite3_bind_text(stmt, 2, note->title, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, note->content, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, note->page_number);
    sqlite3_bind_int64(stmt, 5, (sqlite3_int64)note->created_at);
    sqlite3_bind_int64(stmt, 6, (sqlite3_int64)note->updated_at);
    
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return BN_ERROR_DATABASE;
    }
    
    note->id = (int)sqlite3_last_insert_rowid(db->handle);
    
    sqlite3_finalize(stmt);
    return BN_SUCCESS;
}

BnError db_note_get_by_id(Database *db, int id, Note **out_note) {
    // TODO: Implement (similar to db_book_get_by_id)
    (void)db;
    (void)id;
    (void)out_note;
    return BN_ERROR_UNKNOWN;
}

BnError db_note_get_by_book(Database *db, int book_id, Note ***out_notes, int *out_count) {
    if (!db || !db->handle || !out_notes || !out_count || book_id <= 0) {
        return BN_ERROR_INVALID_ARG;
    }
    
    const char *sql = 
        "SELECT id, book_id, title, content, page_number, created_at, updated_at "
        "FROM notes WHERE book_id = ? ORDER BY created_at;";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return BN_ERROR_DATABASE;
    }
    
    sqlite3_bind_int(stmt, 1, book_id);
    
    // Count rows
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
    }
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, book_id);
    
    if (count == 0) {
        *out_notes = NULL;
        *out_count = 0;
        sqlite3_finalize(stmt);
        return BN_SUCCESS;
    }
    
    // Allocate array
    Note **notes = calloc(count, sizeof(Note *));
    if (!notes) {
        sqlite3_finalize(stmt);
        return BN_ERROR_OUT_OF_MEMORY;
    }
    
    // Fetch rows
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < count) {
        Note *note = calloc(1, sizeof(Note));
        if (!note) {
            for (int j = 0; j < i; j++) {
                note_free(notes[j]);
            }
            free(notes);
            sqlite3_finalize(stmt);
            return BN_ERROR_OUT_OF_MEMORY;
        }
        
        note->id = sqlite3_column_int(stmt, 0);
        note->book_id = sqlite3_column_int(stmt, 1);
        
        const char *title = (const char *)sqlite3_column_text(stmt, 2);
        note->title = strdup(title);
        
        const char *content = (const char *)sqlite3_column_text(stmt, 3);
        note->content = strdup(content);
        
        note->page_number = sqlite3_column_int(stmt, 4);
        note->created_at = (time_t)sqlite3_column_int64(stmt, 5);
        note->updated_at = (time_t)sqlite3_column_int64(stmt, 6);
        
        notes[i++] = note;
    }
    
    *out_notes = notes;
    *out_count = count;
    
    sqlite3_finalize(stmt);
    return BN_SUCCESS;
}

BnError db_note_update(Database *db, const Note *note) {
    if (!db || !db->handle || !note || note->id <= 0) {
        return BN_ERROR_INVALID_ARG;
    }
    
    const char *sql = 
        "UPDATE notes SET title = ?, content = ?, page_number = ?, updated_at = ? "
        "WHERE id = ?;";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return BN_ERROR_DATABASE;
    }
    
    sqlite3_bind_text(stmt, 1, note->title, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, note->content, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, note->page_number);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)time(NULL));
    sqlite3_bind_int(stmt, 5, note->id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        return BN_ERROR_DATABASE;
    }
    
    return BN_SUCCESS;
}

BnError db_note_delete(Database *db, int id) {
    if (!db || !db->handle || id <= 0) {
        return BN_ERROR_INVALID_ARG;
    }
    
    const char *sql = "DELETE FROM notes WHERE id = ?;";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return BN_ERROR_DATABASE;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        return BN_ERROR_DATABASE;
    }
    
    return BN_SUCCESS;
}

BnError db_note_search(Database *db, const char *query, Note ***out_notes, int *out_count) {
    if (!db || !db->handle || !query || !out_notes || !out_count) {
        return BN_ERROR_INVALID_ARG;
    }
    
    const char *sql = 
        "SELECT n.id, n.book_id, n.title, n.content, n.page_number, n.created_at, n.updated_at "
        "FROM notes n "
        "JOIN notes_fts fts ON n.id = fts.rowid "
        "WHERE fts.content MATCH ? "
        "ORDER BY n.created_at DESC;";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return BN_ERROR_DATABASE;
    }
    
    sqlite3_bind_text(stmt, 1, query, -1, SQLITE_TRANSIENT);
    
    // Count rows
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
    }
    sqlite3_reset(stmt);
    sqlite3_bind_text(stmt, 1, query, -1, SQLITE_TRANSIENT);
    
    if (count == 0) {
        *out_notes = NULL;
        *out_count = 0;
        sqlite3_finalize(stmt);
        return BN_SUCCESS;
    }
    
    // Allocate array
    Note **notes = calloc(count, sizeof(Note *));
    if (!notes) {
        sqlite3_finalize(stmt);
        return BN_ERROR_OUT_OF_MEMORY;
    }
    
    // Fetch rows
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < count) {
        Note *note = calloc(1, sizeof(Note));
        if (!note) {
            for (int j = 0; j < i; j++) {
                note_free(notes[j]);
            }
            free(notes);
            sqlite3_finalize(stmt);
            return BN_ERROR_OUT_OF_MEMORY;
        }
        
        note->id = sqlite3_column_int(stmt, 0);
        note->book_id = sqlite3_column_int(stmt, 1);
        
        const char *title = (const char *)sqlite3_column_text(stmt, 2);
        note->title = strdup(title);
        
        const char *content = (const char *)sqlite3_column_text(stmt, 3);
        note->content = strdup(content);
        
        note->page_number = sqlite3_column_int(stmt, 4);
        note->created_at = (time_t)sqlite3_column_int64(stmt, 5);
        note->updated_at = (time_t)sqlite3_column_int64(stmt, 6);
        
        notes[i++] = note;
    }
    
    *out_notes = notes;
    *out_count = count;
    
    sqlite3_finalize(stmt);
    return BN_SUCCESS;
}
