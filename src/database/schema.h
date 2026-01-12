#ifndef BOOKNOTE_SCHEMA_H
#define BOOKNOTE_SCHEMA_H

#include <sqlite3.h>
#include "../utils/error.h"

/**
 * Current database schema version
 */
#define SCHEMA_VERSION 1

/**
 * SQL statement to create books table
 */
extern const char *SQL_CREATE_BOOKS_TABLE;

/**
 * SQL statement to create notes table
 */
extern const char *SQL_CREATE_NOTES_TABLE;

/**
 * SQL statement to create FTS index for notes
 */
extern const char *SQL_CREATE_NOTES_FTS;

/**
 * SQL statement to create FTS triggers
 */
extern const char *SQL_CREATE_FTS_TRIGGERS;

/**
 * SQL statement to create metadata table
 */
extern const char *SQL_CREATE_METADATA_TABLE;

/**
 * Initialize database schema
 * Creates all tables if they don't exist
 * 
 * @param db SQLite database handle
 * @return BN_SUCCESS on success, error code otherwise
 */
BnError schema_initialize(sqlite3 *db);

/**
 * Get current schema version from database
 * 
 * @param db SQLite database handle
 * @param out_version Pointer to store version number
 * @return BN_SUCCESS on success, error code otherwise
 */
BnError schema_get_version(sqlite3 *db, int *out_version);

#endif // BOOKNOTE_SCHEMA_H
