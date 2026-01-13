#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include "database/db.h"
#include "database/schema.h"
#include "database/queries.h"
#include "core/book.h"
#include "core/note.h"
#include "utils/error.h"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    printf("=== booknote v0.1.0-dev ===\n\n");
    
    // Open database
    printf("[1] Opening database...\n");
    Database *db = NULL;
    BnError err = db_open(&db, NULL);
    if (err != BN_SUCCESS) {
        bn_print_error(err, "Failed to open database");
        return 1;
    }
    printf("    Database: %s\n", db->path);
    
    // Check schema version
    int version = 0;
    err = schema_get_version(db->handle, &version);
    if (err == BN_SUCCESS) {
        printf("    Schema version: %d\n\n", version);
    }
    
    // Create and insert a book
    printf("[2] Creating book...\n");
    Book *book = NULL;
    err = book_create(&book, "The C Programming Language", "/home/user/books/k&r.pdf");
    if (err != BN_SUCCESS) {
        bn_print_error(err, "book_create");
        db_close(db);
        return 1;
    }
    
    book_set_author(book, "Brian Kernighan & Dennis Ritchie");
    book_set_year(book, 1978);
    book_set_isbn(book, "0131103628");
    
    printf("    Title: %s\n", book->title);
    printf("    Author: %s\n", book->author);
    printf("    Year: %d\n", book->year);
    
    printf("[3] Inserting book into database...\n");
    err = db_book_insert(db, book);
    if (err != BN_SUCCESS) {
        bn_print_error(err, "db_book_insert");
        book_free(book);
        db_close(db);
        return 1;
    }
    printf("    Book ID: %d\n\n", book->id);
    
    // Create and insert notes
    printf("[4] Creating notes...\n");
    Note *note1 = NULL;
    err = note_create(&note1, book->id, "Pointers are powerful but need careful handling", 45);
    if (err != BN_SUCCESS) {
        bn_print_error(err, "note_create");
        book_free(book);
        db_close(db);
        return 1;
    }
    
    err = db_note_insert(db, note1);
    if (err != BN_SUCCESS) {
        bn_print_error(err, "db_note_insert");
        note_free(note1);
        book_free(book);
        db_close(db);
        return 1;
    }
    printf("    Note 1 ID: %d (page %d)\n", note1->id, note1->page_number);
    
    Note *note2 = NULL;
    err = note_create(&note2, book->id, "Always use const for read-only parameters", 67);
    if (err == BN_SUCCESS) {
        err = db_note_insert(db, note2);
        if (err == BN_SUCCESS) {
            printf("    Note 2 ID: %d (page %d)\n\n", note2->id, note2->page_number);
        }
    }
    
    // Retrieve book from database
    printf("[5] Retrieving book from database...\n");
    Book *retrieved_book = NULL;
    err = db_book_get_by_id(db, book->id, &retrieved_book);
    if (err != BN_SUCCESS) {
        bn_print_error(err, "db_book_get_by_id");
    } else {
        printf("    Retrieved: %s by %s\n", retrieved_book->title, retrieved_book->author);
        book_free(retrieved_book);
    }
    
    // List all books
    printf("\n[6] Listing all books...\n");
    Book **books = NULL;
    int count = 0;
    err = db_book_get_all(db, &books, &count);
    if (err != BN_SUCCESS) {
        bn_print_error(err, "db_book_get_all");
    } else {
        printf("    Total books: %d\n", count);
        for (int i = 0; i < count; i++) {
            printf("    [%d] %s\n", books[i]->id, books[i]->title);
            book_free(books[i]);
        }
        free(books);
    }
    
    // Cleanup
    printf("\n[7] Cleaning up...\n");
    note_free(note1);
    note_free(note2);
    book_free(book);
    db_close(db);
    
    printf("    Done!\n");
    return 0;
}
