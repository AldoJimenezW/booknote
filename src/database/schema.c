#include "schema.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const char *SQL_CREATE_BOOKS_TABLE =
    "CREATE TABLE IF NOT EXISTS books ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  isbn TEXT UNIQUE,"
    "  title TEXT NOT NULL,"
    "  author TEXT,"
    "  year INTEGER,"
    "  publisher TEXT,"
    "  filepath TEXT NOT NULL UNIQUE,"
    "  cover_path TEXT,"
    "  added_at INTEGER NOT NULL,"
    "  updated_at INTEGER NOT NULL"
    ");";

const char *SQL_CREATE_NOTES_TABLE =
    "CREATE TABLE IF NOT EXISTS notes ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  book_id INTEGER NOT NULL,"
    "  title TEXT NOT NULL DEFAULT 'Untitled',"
    "  content TEXT NOT NULL,"
    "  page_number INTEGER,"
    "  created_at INTEGER NOT NULL,"
    "  updated_at INTEGER NOT NULL,"
    "  FOREIGN KEY (book_id) REFERENCES books(id) ON DELETE CASCADE"
    ");";

const char *SQL_CREATE_NOTES_FTS =
    "CREATE VIRTUAL TABLE IF NOT EXISTS notes_fts USING fts5("
    "  content,"
    "  content=notes,"
    "  content_rowid=id"
    ");";

const char *SQL_CREATE_FTS_TRIGGERS =
    "CREATE TRIGGER IF NOT EXISTS notes_ai AFTER INSERT ON notes BEGIN "
    "  INSERT INTO notes_fts(rowid, content) VALUES (new.id, new.content);"
    "END;"
    "CREATE TRIGGER IF NOT EXISTS notes_ad AFTER DELETE ON notes BEGIN "
    "  DELETE FROM notes_fts WHERE rowid = old.id;"
    "END;"
    "CREATE TRIGGER IF NOT EXISTS notes_au AFTER UPDATE ON notes BEGIN "
    "  UPDATE notes_fts SET content = new.content WHERE rowid = new.id;"
    "END;";

const char *SQL_CREATE_METADATA_TABLE =
    "CREATE TABLE IF NOT EXISTS metadata ("
    "  key TEXT PRIMARY KEY,"
    "  value TEXT NOT NULL"
    ");";

static BnError execute_sql(sqlite3 *db, const char *sql) {
    char *err_msg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);

    if (rc != SQLITE_OK) {
        if (err_msg) {
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
        }
        return BN_ERROR_DATABASE;
    }

    return BN_SUCCESS;
}

static BnError migrate_to_v2(sqlite3 *db) {
    printf("Migrating database to version 2...\n");
    
    // Add title column to notes table
    const char *sql = "ALTER TABLE notes ADD COLUMN title TEXT NOT NULL DEFAULT 'Untitled';";
    
    BnError err = execute_sql(db, sql);
    if (err != BN_SUCCESS) {
        return err;
    }
    
    // Update schema version
    const char *update_version = "UPDATE metadata SET value = '2' WHERE key = 'schema_version';";
    err = execute_sql(db, update_version);
    if (err != BN_SUCCESS) {
        return err;
    }
    
    printf("Migration to v2 complete\n");
    return BN_SUCCESS;
}

BnError schema_initialize(sqlite3 *db) {
    if (!db) {
        return BN_ERROR_INVALID_ARG;
    }

    BnError err;

    // Create tables
    err = execute_sql(db, SQL_CREATE_BOOKS_TABLE);
    if (err != BN_SUCCESS) return err;

    err = execute_sql(db, SQL_CREATE_NOTES_TABLE);
    if (err != BN_SUCCESS) return err;

    err = execute_sql(db, SQL_CREATE_METADATA_TABLE);
    if (err != BN_SUCCESS) return err;

    err = execute_sql(db, SQL_CREATE_NOTES_FTS);
    if (err != BN_SUCCESS) return err;

    err = execute_sql(db, SQL_CREATE_FTS_TRIGGERS);
    if (err != BN_SUCCESS) return err;

    // Set schema version if not exists (for new databases)
    const char *insert_version =
        "INSERT OR IGNORE INTO metadata (key, value) VALUES ('schema_version', '3');";
    err = execute_sql(db, insert_version);
    if (err != BN_SUCCESS) return err;

    // Check if migration is needed for existing databases
    int version = 0;
    BnError ver_err = schema_get_version(db, &version);
    
    if (ver_err == BN_SUCCESS && version < 2) {
        printf("Old database detected (v%d). Migrating...\n", version);
        err = migrate_to_v2(db);
        if (err != BN_SUCCESS) {
            return err;
        }
        version = 2;
    }
    if (ver_err == BN_SUCCESS && version < 3) {
        printf("Migrating database to version 3...\n");
        // Add cover_path column to books table (backward-compatible)
        const char *sql = "ALTER TABLE books ADD COLUMN cover_path TEXT;";
        err = execute_sql(db, sql);
        if (err != BN_SUCCESS) {
            return err;
        }
        // Update schema version
        const char *update_version = "UPDATE metadata SET value = '3' WHERE key = 'schema_version';";
        err = execute_sql(db, update_version);
        if (err != BN_SUCCESS) {
            return err;
        }
        printf("Migration to v3 complete\n");
    }

    return BN_SUCCESS;
}

BnError schema_get_version(sqlite3 *db, int *out_version) {
    if (!db || !out_version) {
        return BN_ERROR_INVALID_ARG;
    }

    const char *sql = "SELECT value FROM metadata WHERE key = 'schema_version';";
    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return BN_ERROR_DATABASE;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const char *version_str = (const char *)sqlite3_column_text(stmt, 0);
        *out_version = atoi(version_str);
        sqlite3_finalize(stmt);
        return BN_SUCCESS;
    } else if (rc == SQLITE_DONE) {
        sqlite3_finalize(stmt);
        *out_version = 0;
        return BN_ERROR_NOT_FOUND;
    } else {
        sqlite3_finalize(stmt);
        return BN_ERROR_DATABASE;
    }
}
