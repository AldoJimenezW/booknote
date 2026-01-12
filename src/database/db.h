#ifndef BOOKNOTE_DB_H
#define BOOKNOTE_DB_H

#include <sqlite3.h>
#include "../utils/error.h"

/**
 * Database context
 */
typedef struct {
    sqlite3 *handle;
    char *path;
} Database;

/**
 * Initialize and open database
 * Creates database file if it doesn't exist
 * Initializes schema if needed
 * 
 * @param out_db Pointer to store database context
 * @param path Path to database file (NULL for default location)
 * @return BN_SUCCESS on success, error code otherwise
 */
BnError db_open(Database **out_db, const char *path);

/**
 * Close database connection
 */
void db_close(Database *db);

/**
 * Get default database path
 * Returns ~/.local/share/booknote/booknote.db
 * 
 * @param out_path Pointer to store allocated path string (caller must free)
 * @return BN_SUCCESS on success, error code otherwise
 */
BnError db_get_default_path(char **out_path);

/**
 * Begin transaction
 */
BnError db_begin_transaction(Database *db);

/**
 * Commit transaction
 */
BnError db_commit_transaction(Database *db);

/**
 * Rollback transaction
 */
BnError db_rollback_transaction(Database *db);

#endif // BOOKNOTE_DB_H
