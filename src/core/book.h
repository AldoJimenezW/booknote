#ifndef BOOKNOTE_BOOK_H
#define BOOKNOTE_BOOK_H

#include <time.h>
#include "../utils/error.h"

/**
 * Book structure representing a book in the library
 */
typedef struct {
    int id;
    char *isbn;        // ISBN-10 or ISBN-13, nullable
    char *title;
    char *author;      // Nullable
    int year;          // 0 if unknown
    char *publisher;   // Nullable
    char *filepath;    // Path to PDF file
    char *cover_path;  // Optional path to cached cover image (e.g., ~/.cache/booknote/covers/{isbn}.jpg)
    time_t added_at;
    time_t updated_at;
} Book;

/**
 * Create a new book object
 * 
 * @param out_book Pointer to store created book
 * @param title Book title (required)
 * @param filepath Path to PDF file (required)
 * @return BN_SUCCESS on success, error code otherwise
 */
BnError book_create(Book **out_book, const char *title, const char *filepath);

/**
 * Free book memory
 */
void book_free(Book *book);

/**
 * Set optional book metadata
 */
BnError book_set_isbn(Book *book, const char *isbn);
BnError book_set_author(Book *book, const char *author);
BnError book_set_year(Book *book, int year);
BnError book_set_publisher(Book *book, const char *publisher);

#endif // BOOKNOTE_BOOK_H
