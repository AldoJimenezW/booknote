#define _POSIX_C_SOURCE 200809L
#include "commands.h"
#include "../database/queries.h"
#include "../core/book.h"
#include "../core/note.h"
#include "../utils/error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "0.1.0"

/* ============================================================================
 * Helper functions
 * ========================================================================= */

static void print_usage(void) {
    printf("Usage: booknote <command> [options]\n\n");
    printf("Commands:\n");
    printf("  add <filepath>           Add a book to your library\n");
    printf("  list                     List all books\n");
    printf("  show <book-id>           Show book details and notes\n");
    printf("  note <book-id> <text>    Add a note to a book\n");
    printf("  search <query>           Search notes\n");
    printf("  delete <book-id>         Delete a book\n");
    printf("  help [command]           Show help\n");
    printf("  version                  Show version\n\n");
    printf("Examples:\n");
    printf("  booknote add mybook.pdf --title \"My Book\" --author \"Author Name\"\n");
    printf("  booknote list\n");
    printf("  booknote note 1 \"This is an important point\"\n");
    printf("  booknote search \"machine learning\"\n\n");
    printf("For more information, visit: https://github.com/AldoJimenezW/booknote\n");
}

/* ============================================================================
 * Command implementations
 * ========================================================================= */

int cmd_help(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    print_usage();
    return 0;
}

int cmd_version(void) {
    printf("booknote version %s\n", VERSION);
    printf("SQLite version: %s\n", sqlite3_libversion());
    return 0;
}

int cmd_add(Database *db, int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Error: Missing filepath\n");
        fprintf(stderr, "Usage: booknote add <filepath> [--title TITLE] [--author AUTHOR] [--isbn ISBN]\n");
        return 1;
    }
    
    char *filepath = argv[2];
    char *title = NULL;
    char *author = NULL;
    char *isbn = NULL;
    
    // Parse optional arguments
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--title") == 0 && i + 1 < argc) {
            title = argv[++i];
        } else if (strcmp(argv[i], "--author") == 0 && i + 1 < argc) {
            author = argv[++i];
        } else if (strcmp(argv[i], "--isbn") == 0 && i + 1 < argc) {
            isbn = argv[++i];
        }
    }
    
    // Default title to filename if not provided
    if (!title) {
        title = strrchr(filepath, '/');
        title = title ? title + 1 : filepath;
    }
    
    // Create book
    Book *book = NULL;
    BnError err = book_create(&book, title, filepath);
    if (err != BN_SUCCESS) {
        bn_print_error(err, "creating book");
        return 1;
    }
    
    if (author) {
        book_set_author(book, author);
    }
    if (isbn) {
        book_set_isbn(book, isbn);
    }
    
    // Insert into database
    err = db_book_insert(db, book);
    if (err != BN_SUCCESS) {
        bn_print_error(err, "inserting book");
        book_free(book);
        return 1;
    }
    
    printf("Book added successfully!\n");
    printf("ID: %d\n", book->id);
    printf("Title: %s\n", book->title);
    if (book->author) {
        printf("Author: %s\n", book->author);
    }
    
    book_free(book);
    return 0;
}

int cmd_list(Database *db, int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    Book **books = NULL;
    int count = 0;
    
    BnError err = db_book_get_all(db, &books, &count);
    if (err != BN_SUCCESS) {
        bn_print_error(err, "listing books");
        return 1;
    }
    
    if (count == 0) {
        printf("No books in library.\n");
        printf("Add a book with: booknote add <filepath>\n");
        return 0;
    }
    
    printf("Books in library: %d\n\n", count);
    for (int i = 0; i < count; i++) {
        printf("[%d] %s", books[i]->id, books[i]->title);
        if (books[i]->author) {
            printf(" - %s", books[i]->author);
        }
        if (books[i]->year > 0) {
            printf(" (%d)", books[i]->year);
        }
        printf("\n");
        book_free(books[i]);
    }
    free(books);
    
    return 0;
}

int cmd_show(Database *db, int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Error: Missing book ID\n");
        fprintf(stderr, "Usage: booknote show <book-id>\n");
        return 1;
    }
    
    int book_id = atoi(argv[2]);
    if (book_id <= 0) {
        fprintf(stderr, "Error: Invalid book ID\n");
        return 1;
    }
    
    // Get book
    Book *book = NULL;
    BnError err = db_book_get_by_id(db, book_id, &book);
    if (err != BN_SUCCESS) {
        if (err == BN_ERROR_NOT_FOUND) {
            fprintf(stderr, "Error: Book not found (ID: %d)\n", book_id);
        } else {
            bn_print_error(err, "getting book");
        }
        return 1;
    }
    
    // Print book details
    printf("=== Book Details ===\n");
    printf("ID: %d\n", book->id);
    printf("Title: %s\n", book->title);
    if (book->author) {
        printf("Author: %s\n", book->author);
    }
    if (book->year > 0) {
        printf("Year: %d\n", book->year);
    }
    if (book->isbn) {
        printf("ISBN: %s\n", book->isbn);
    }
    if (book->publisher) {
        printf("Publisher: %s\n", book->publisher);
    }
    printf("File: %s\n", book->filepath);
    
    // Get notes
    printf("\n=== Notes ===\n");
    Note **notes = NULL;
    int note_count = 0;
    err = db_note_get_by_book(db, book_id, &notes, &note_count);
    
    if (err != BN_SUCCESS) {
        bn_print_error(err, "getting notes");
    } else if (note_count == 0) {
        printf("No notes yet. Add one with: booknote note %d \"your note\"\n", book_id);
    } else {
        printf("Total notes: %d\n\n", note_count);
        for (int i = 0; i < note_count; i++) {
            printf("[%d] ", notes[i]->id);
            if (notes[i]->page_number > 0) {
                printf("(page %d) ", notes[i]->page_number);
            }
            printf("%s\n", notes[i]->content);
            note_free(notes[i]);
        }
        free(notes);
    }
    
    book_free(book);
    return 0;
}

int cmd_note(Database *db, int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Error: Missing arguments\n");
        fprintf(stderr, "Usage: booknote note <book-id> <\"note text\"> [--page N]\n");
        return 1;
    }
    
    int book_id = atoi(argv[2]);
    if (book_id <= 0) {
        fprintf(stderr, "Error: Invalid book ID\n");
        return 1;
    }
    
    char *content = argv[3];
    int page_number = 0;
    
    // Parse optional page number
    for (int i = 4; i < argc; i++) {
        if (strcmp(argv[i], "--page") == 0 && i + 1 < argc) {
            page_number = atoi(argv[++i]);
        }
    }
    
    // Verify book exists
    Book *book = NULL;
    BnError err = db_book_get_by_id(db, book_id, &book);
    if (err != BN_SUCCESS) {
        if (err == BN_ERROR_NOT_FOUND) {
            fprintf(stderr, "Error: Book not found (ID: %d)\n", book_id);
        } else {
            bn_print_error(err, "verifying book");
        }
        return 1;
    }
    book_free(book);
    
    // Create note
    Note *note = NULL;
    err = note_create(&note, book_id, content, page_number);
    if (err != BN_SUCCESS) {
        bn_print_error(err, "creating note");
        return 1;
    }
    
    // Insert into database
    err = db_note_insert(db, note);
    if (err != BN_SUCCESS) {
        bn_print_error(err, "inserting note");
        note_free(note);
        return 1;
    }
    
    printf("Note added successfully!\n");
    printf("Note ID: %d\n", note->id);
    if (page_number > 0) {
        printf("Page: %d\n", page_number);
    }
    
    note_free(note);
    return 0;
}

int cmd_search(Database *db, int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Error: Missing search query\n");
        fprintf(stderr, "Usage: booknote search <\"query\">\n");
        return 1;
    }
    
    char *query = argv[2];
    
    Note **notes = NULL;
    int count = 0;
    
    BnError err = db_note_search(db, query, &notes, &count);
    if (err != BN_SUCCESS) {
        bn_print_error(err, "searching notes");
        return 1;
    }
    
    if (count == 0) {
        printf("No notes found matching: \"%s\"\n", query);
        return 0;
    }
    
    printf("Found %d note(s) matching: \"%s\"\n\n", count, query);
    
    for (int i = 0; i < count; i++) {
        // Get book info for context
        Book *book = NULL;
        err = db_book_get_by_id(db, notes[i]->book_id, &book);
        
        printf("[%d] ", notes[i]->id);
        if (book) {
            printf("(%s) ", book->title);
            book_free(book);
        }
        if (notes[i]->page_number > 0) {
            printf("page %d: ", notes[i]->page_number);
        }
        printf("%s\n", notes[i]->content);
        
        note_free(notes[i]);
    }
    free(notes);
    
    return 0;
}

int cmd_delete(Database *db, int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Error: Missing book ID\n");
        fprintf(stderr, "Usage: booknote delete <book-id>\n");
        return 1;
    }
    
    int book_id = atoi(argv[2]);
    if (book_id <= 0) {
        fprintf(stderr, "Error: Invalid book ID\n");
        return 1;
    }
    
    // Verify book exists
    Book *book = NULL;
    BnError err = db_book_get_by_id(db, book_id, &book);
    if (err != BN_SUCCESS) {
        if (err == BN_ERROR_NOT_FOUND) {
            fprintf(stderr, "Error: Book not found (ID: %d)\n", book_id);
        } else {
            bn_print_error(err, "verifying book");
        }
        return 1;
    }
    
    printf("Delete book: %s? (y/N): ", book->title);
    char confirm[10];
    if (!fgets(confirm, sizeof(confirm), stdin) || confirm[0] != 'y') {
        printf("Cancelled.\n");
        book_free(book);
        return 0;
    }
    
    book_free(book);
    
    // Delete
    err = db_book_delete(db, book_id);
    if (err != BN_SUCCESS) {
        bn_print_error(err, "deleting book");
        return 1;
    }
    
    printf("Book deleted (including all notes).\n");
    return 0;
}
