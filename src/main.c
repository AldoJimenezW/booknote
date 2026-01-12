#include <stdio.h>
#include <stdlib.h>
#include "database/db.h"
#include "database/schema.h"
#include "core/book.h"
#include "utils/error.h"

int main(int argc, char **argv) {
    (void)argc;  // Suppress unused warning
    (void)argv;
    
    printf("booknote v0.1.0-dev\n");
    printf("Initializing database...\n");
    
    Database *db = NULL;
    BnError err = db_open(&db, NULL);
    
    if (err != BN_SUCCESS) {
        bn_print_error(err, "Failed to open database");
        return 1;
    }
    
    printf("Database opened successfully at: %s\n", db->path);
    
    // Test schema version
    int version = 0;
    err = schema_get_version(db->handle, &version);
    if (err == BN_SUCCESS) {
        printf("Schema version: %d\n", version);
    }
    
    // Test book creation
    Book *book = NULL;
    err = book_create(&book, "Test Book", "/tmp/test.pdf");
    if (err == BN_SUCCESS) {
        printf("Book created: %s\n", book->title);
        book_free(book);
    }
    
    db_close(db);
    printf("Database closed.\n");
    
    return 0;
}
