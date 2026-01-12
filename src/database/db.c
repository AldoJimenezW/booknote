#define _POSIX_C_SOURCE 200809L
#include "db.h"
#include "schema.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>

static BnError ensure_directory_exists(const char *path) {
    char *dir_path = strdup(path);
    if (!dir_path) {
        return BN_ERROR_OUT_OF_MEMORY;
    }
    
    // Remove filename to get directory
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
    }
    
    // Create directory recursively
    char *p = dir_path;
    while (*p) {
        if (*p == '/') {
            *p = '\0';
            mkdir(dir_path, 0755);
            *p = '/';
        }
        p++;
    }
    mkdir(dir_path, 0755);
    
    free(dir_path);
    return BN_SUCCESS;
}

BnError db_get_default_path(char **out_path) {
    if (!out_path) {
        return BN_ERROR_INVALID_ARG;
    }
    
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) {
            home = pw->pw_dir;
        }
    }
    
    if (!home) {
        return BN_ERROR_NOT_FOUND;
    }
    
    const char *suffix = "/.local/share/booknote/booknote.db";
    size_t len = strlen(home) + strlen(suffix) + 1;
    
    char *path = malloc(len);
    if (!path) {
        return BN_ERROR_OUT_OF_MEMORY;
    }
    
    snprintf(path, len, "%s%s", home, suffix);
    *out_path = path;
    
    return BN_SUCCESS;
}

BnError db_open(Database **out_db, const char *path) {
    if (!out_db) {
        return BN_ERROR_INVALID_ARG;
    }
    
    BnError err;
    char *db_path = NULL;
    
    // Use default path if none provided
    if (!path) {
        err = db_get_default_path(&db_path);
        if (err != BN_SUCCESS) {
            return err;
        }
        path = db_path;
    }
    
    // Ensure directory exists
    err = ensure_directory_exists(path);
    if (err != BN_SUCCESS) {
        free(db_path);
        return err;
    }
    
    Database *db = calloc(1, sizeof(Database));
    if (!db) {
        free(db_path);
        return BN_ERROR_OUT_OF_MEMORY;
    }
    
    // Open SQLite database
    int rc = sqlite3_open(path, &db->handle);
    if (rc != SQLITE_OK) {
        free(db);
        free(db_path);
        return BN_ERROR_DATABASE;
    }
    
    // Enable foreign keys
    sqlite3_exec(db->handle, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
    
    // Store path
    db->path = db_path ? db_path : strdup(path);
    if (!db->path) {
        sqlite3_close(db->handle);
        free(db);
        return BN_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize schema
    err = schema_initialize(db->handle);
    if (err != BN_SUCCESS) {
        sqlite3_close(db->handle);
        free(db->path);
        free(db);
        return err;
    }
    
    *out_db = db;
    return BN_SUCCESS;
}

void db_close(Database *db) {
    if (!db) {
        return;
    }
    
    if (db->handle) {
        sqlite3_close(db->handle);
    }
    
    free(db->path);
    free(db);
}

BnError db_begin_transaction(Database *db) {
    if (!db || !db->handle) {
        return BN_ERROR_INVALID_ARG;
    }
    
    char *err_msg = NULL;
    int rc = sqlite3_exec(db->handle, "BEGIN TRANSACTION;", NULL, NULL, &err_msg);
    
    if (rc != SQLITE_OK) {
        if (err_msg) {
            sqlite3_free(err_msg);
        }
        return BN_ERROR_DATABASE;
    }
    
    return BN_SUCCESS;
}

BnError db_commit_transaction(Database *db) {
    if (!db || !db->handle) {
        return BN_ERROR_INVALID_ARG;
    }
    
    char *err_msg = NULL;
    int rc = sqlite3_exec(db->handle, "COMMIT;", NULL, NULL, &err_msg);
    
    if (rc != SQLITE_OK) {
        if (err_msg) {
            sqlite3_free(err_msg);
        }
        return BN_ERROR_DATABASE;
    }
    
    return BN_SUCCESS;
}

BnError db_rollback_transaction(Database *db) {
    if (!db || !db->handle) {
        return BN_ERROR_INVALID_ARG;
    }
    
    char *err_msg = NULL;
    int rc = sqlite3_exec(db->handle, "ROLLBACK;", NULL, NULL, &err_msg);
    
    if (rc != SQLITE_OK) {
        if (err_msg) {
            sqlite3_free(err_msg);
        }
        return BN_ERROR_DATABASE;
    }
    
    return BN_SUCCESS;
}
