#define _POSIX_C_SOURCE 200809L
#include "book.h"
#include <stdlib.h>
#include <string.h>

BnError book_create(Book **out_book, const char *title, const char *filepath) {
    if (!out_book || !title || !filepath) {
        return BN_ERROR_INVALID_ARG;
    }
    
    Book *book = calloc(1, sizeof(Book));
    if (!book) {
        return BN_ERROR_OUT_OF_MEMORY;
    }
    
    book->title = strdup(title);
    if (!book->title) {
        free(book);
        return BN_ERROR_OUT_OF_MEMORY;
    }
    
    book->filepath = strdup(filepath);
    if (!book->filepath) {
        free(book->title);
        free(book);
        return BN_ERROR_OUT_OF_MEMORY;
    }
    
    book->id = 0;  // Will be set by database
    book->isbn = NULL;
    book->author = NULL;
    book->year = 0;
    book->publisher = NULL;
    book->added_at = time(NULL);
    book->updated_at = book->added_at;
    
    *out_book = book;
    return BN_SUCCESS;
}

void book_free(Book *book) {
    if (!book) {
        return;
    }
    
    free(book->isbn);
    free(book->title);
    free(book->author);
    free(book->publisher);
    free(book->filepath);
    free(book);
}

BnError book_set_isbn(Book *book, const char *isbn) {
    if (!book) {
        return BN_ERROR_INVALID_ARG;
    }
    
    free(book->isbn);
    book->isbn = isbn ? strdup(isbn) : NULL;
    if (isbn && !book->isbn) {
        return BN_ERROR_OUT_OF_MEMORY;
    }
    
    book->updated_at = time(NULL);
    return BN_SUCCESS;
}

BnError book_set_author(Book *book, const char *author) {
    if (!book) {
        return BN_ERROR_INVALID_ARG;
    }
    
    free(book->author);
    book->author = author ? strdup(author) : NULL;
    if (author && !book->author) {
        return BN_ERROR_OUT_OF_MEMORY;
    }
    
    book->updated_at = time(NULL);
    return BN_SUCCESS;
}

BnError book_set_year(Book *book, int year) {
    if (!book) {
        return BN_ERROR_INVALID_ARG;
    }
    
    book->year = year;
    book->updated_at = time(NULL);
    return BN_SUCCESS;
}

BnError book_set_publisher(Book *book, const char *publisher) {
    if (!book) {
        return BN_ERROR_INVALID_ARG;
    }
    
    free(book->publisher);
    book->publisher = publisher ? strdup(publisher) : NULL;
    if (publisher && !book->publisher) {
        return BN_ERROR_OUT_OF_MEMORY;
    }
    
    book->updated_at = time(NULL);
    return BN_SUCCESS;
}
