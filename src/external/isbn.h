#ifndef BOOKNOTE_EXTERNAL_ISBN_H
#define BOOKNOTE_EXTERNAL_ISBN_H

#include <stddef.h>

/*
 * ISBN client header using libcurl and json-c
 *
 * This module provides a small client for querying OpenLibrary by ISBN and
 * obtaining metadata and cover URLs, along with a helper to download and cache
 * the cover image to the local filesystem.
 *
 * API endpoints:
 * - Metadata: https://openlibrary.org/isbn/{ISBN}.json
 * - Covers:   https://covers.openlibrary.org/b/isbn/{ISBN}-M.jpg
 *
 * Dependencies (link via pkg-config):
 * - libcurl
 * - json-c
 *
 * Typical usage:
 *   IsbnMetadata *meta = NULL;
 *   if (isbn_fetch_metadata("9781492052203", &meta) == 0 && meta) {
 *       const char *cover_url = isbn_cover_url_medium(meta->isbn);
 *       char *cached_path = NULL;
 *       isbn_download_cover(cover_url, &cached_path);
 *       // Use meta fields and cached_path...
 *       isbn_free_metadata(meta);
 *       free(cached_path);
 *   }
 */

/*
 * IsbnMetadata
 *
 * Minimal metadata fields extracted from OpenLibrary. All strings are heap
 * allocated and owned by the struct. Use isbn_free_metadata() to release.
 *
 * Fields that may be absent in the API response will be NULL.
 */
typedef struct IsbnMetadata {
    char *isbn;        /* normalized ISBN (as provided), e.g., "9781492052203" */
    char *title;       /* book title */
    char *author;      /* primary author (best-effort extraction) */
    char *publisher;   /* primary publisher (best-effort extraction) */
    int   year;        /* publication year if available; 0 if unknown */
} IsbnMetadata;

/*
 * Error codes
 *
 * 0  => success
 * <0 => error
 */
typedef enum IsbnError {
    ISBN_OK                = 0,
    ISBN_ERR_INVALID_ARG   = -1,
    ISBN_ERR_NETWORK       = -2,
    ISBN_ERR_HTTP_STATUS   = -3,
    ISBN_ERR_PARSE         = -4,
    ISBN_ERR_MEMORY        = -5,
    ISBN_ERR_IO            = -6
} IsbnError;

/*
 * isbn_fetch_metadata
 *
 * Fetches metadata from OpenLibrary for the given ISBN. On success, allocates
 * and returns an IsbnMetadata instance via out_meta. The caller owns the result
 * and must free it with isbn_free_metadata().
 *
 * Parameters:
 * - isbn:        NUL-terminated ISBN string (digits and possibly dashes)
 * - out_meta:    pointer to receive IsbnMetadata* on success
 *
 * Returns:
 * - ISBN_OK on success
 * - One of the ISBN_ERR_* codes on failure
 */
int isbn_fetch_metadata(const char *isbn, IsbnMetadata **out_meta);

/*
 * isbn_free_metadata
 *
 * Frees an IsbnMetadata instance allocated by isbn_fetch_metadata.
 * Safe to call with NULL.
 */
void isbn_free_metadata(IsbnMetadata *meta);

/*
 * isbn_cover_url_medium
 *
 * Builds the OpenLibrary cover URL for the given ISBN (medium size).
 * The returned pointer is a static string owned by the function only if
 * the isbn is NULL or empty (returns NULL). Otherwise, this returns a newly
 * allocated string that must be freed by the caller with free().
 *
 * Example: "https://covers.openlibrary.org/b/isbn/{ISBN}-M.jpg"
 *
 * Parameters:
 * - isbn: NUL-terminated ISBN string
 *
 * Returns:
 * - Newly allocated string with the URL on success
 * - NULL on invalid argument or memory error
 */
char *isbn_cover_url_medium(const char *isbn);

/*
 * isbn_cache_dir
 *
 * Returns the cache directory path for covers:
 *   ~/.cache/booknote/covers
 *
 * The returned string is newly allocated and must be freed by the caller.
 * The directory may not exist; isbn_ensure_cache_dir() can be used to create it.
 *
 * Returns:
 * - Newly allocated string with the absolute path
 * - NULL on memory error
 */
char *isbn_cache_dir(void);

/*
 * isbn_ensure_cache_dir
 *
 * Ensures the covers cache directory exists, creating it (and parents) when
 * necessary. Uses standard POSIX APIs.
 *
 * Returns:
 * - 0 on success
 * - <0 on failure (e.g., permissions)
 */
int isbn_ensure_cache_dir(void);

/*
 * isbn_cached_cover_path
 *
 * Computes the absolute path for a cached cover file:
 *   ~/.cache/booknote/covers/{isbn}.jpg
 *
 * The returned string is newly allocated and must be freed by the caller.
 *
 * Parameters:
 * - isbn: NUL-terminated ISBN string
 *
 * Returns:
 * - Newly allocated string with the absolute path on success
 * - NULL on invalid argument or memory error
 */
char *isbn_cached_cover_path(const char *isbn);

/*
 * isbn_download_cover
 *
 * Downloads a cover image from the provided URL and stores it in the cache
 * directory using the filename derived from the ISBN (extracted from the URL).
 * If the file already exists, this function can return its path without
 * re-downloading depending on implementation choices.
 *
 * Parameters:
 * - cover_url:  NUL-terminated URL to the cover image
 * - out_path:   pointer to receive newly allocated absolute file path on success
 *
 * Returns:
 * - ISBN_OK on success
 * - ISBN_ERR_INVALID_ARG for bad inputs
 * - ISBN_ERR_NETWORK / ISBN_ERR_HTTP_STATUS for network/HTTP failures
 * - ISBN_ERR_IO for filesystem errors
 * - ISBN_ERR_MEMORY if allocation fails
 */
int isbn_download_cover(const char *cover_url, char **out_path);

#endif /* BOOKNOTE_EXTERNAL_ISBN_H */