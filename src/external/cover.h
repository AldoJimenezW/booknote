#ifndef BOOKNOTE_EXTERNAL_COVER_H
#define BOOKNOTE_EXTERNAL_COVER_H

/*
 * booknote/src/external/cover.h
 *
 * Cover download and caching utilities for Booknote.
 *
 * Responsibilities:
 * - Compute the local cache directory for covers:
 *     ~/.cache/booknote/covers
 * - Compute the cached cover file path for a given ISBN:
 *     ~/.cache/booknote/covers/{isbn}.jpg
 * - Download cover images from URLs (e.g., OpenLibrary cover endpoint) and
 *   store them in the cache, reusing existing files when present.
 * - Optional helper to extract a cover image from a PDF file when ISBN
 *   metadata is unavailable (first page thumbnail).
 *
 * Dependencies (expected to be linked via pkg-config):
 * - libcurl (for HTTP downloads)
 * - poppler-glib (optional, for PDF cover extraction)
 *
 * Error codes:
 *   COVER_OK                = 0
 *   COVER_ERR_INVALID_ARG   = -1
 *   COVER_ERR_MEMORY        = -2
 *   COVER_ERR_IO            = -3
 *   COVER_ERR_NETWORK       = -4
 *   COVER_ERR_HTTP_STATUS   = -5
 *   COVER_ERR_UNSUPPORTED   = -6
 */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum CoverError {
    COVER_OK               = 0,
    COVER_ERR_INVALID_ARG  = -1,
    COVER_ERR_MEMORY       = -2,
    COVER_ERR_IO           = -3,
    COVER_ERR_NETWORK      = -4,
    COVER_ERR_HTTP_STATUS  = -5,
    COVER_ERR_UNSUPPORTED  = -6
} CoverError;

/*
 * cover_cache_dir
 *
 * Returns the absolute path to the covers cache directory:
 *   ~/.cache/booknote/covers
 *
 * The returned string is heap-allocated and must be freed by the caller
 * using free().
 *
 * Returns:
 * - Newly allocated string on success
 * - NULL on memory error or if HOME is not set
 */
char *cover_cache_dir(void);

/*
 * cover_ensure_cache_dir
 *
 * Ensures the covers cache directory exists, creating missing parents if
 * necessary. Safe to call multiple times.
 *
 * Returns:
 * - COVER_OK on success
 * - COVER_ERR_IO if directory creation failed
 */
int cover_ensure_cache_dir(void);

/*
 * cover_path_for_isbn
 *
 * Computes the absolute cached cover path for the given ISBN:
 *   ~/.cache/booknote/covers/{isbn}.jpg
 *
 * The ISBN is expected to contain only digits; callers should normalize any
 * input (strip dashes/spaces) before calling, or pass the raw ISBN and allow
 * this function to best-effort normalize digits.
 *
 * The returned string is heap-allocated and must be freed by the caller.
 *
 * Parameters:
 * - isbn: NUL-terminated ISBN string (may include hyphens/spaces)
 *
 * Returns:
 * - Newly allocated absolute path on success
 * - NULL on invalid argument or memory error
 */
char *cover_path_for_isbn(const char *isbn);

/*
 * cover_download_for_isbn
 *
 * Downloads a cover image for a given ISBN from the provided URL and stores it
 * in the cache under {isbn}.jpg. If a cached file already exists and is non-empty,
 * the function returns its path without re-downloading.
 *
 * The returned path is heap-allocated and must be freed by the caller.
 *
 * Parameters:
 * - isbn:      NUL-terminated ISBN string (digits/hyphens accepted)
 * - cover_url: NUL-terminated URL pointing to the cover image (e.g., OpenLibrary)
 * - out_path:  Output pointer to receive the absolute cached file path
 *
 * Returns:
 * - COVER_OK on success (out_path set)
 * - COVER_ERR_INVALID_ARG for bad inputs
 * - COVER_ERR_IO for filesystem errors
 * - COVER_ERR_NETWORK / COVER_ERR_HTTP_STATUS for HTTP failures
 * - COVER_ERR_MEMORY if allocation fails
 */
int cover_download_for_isbn(const char *isbn, const char *cover_url, char **out_path);

/*
 * cover_extract_from_pdf
 *
 * Optional helper to generate a cover image from the first page of a local PDF
 * when ISBN metadata and network covers are unavailable. Produces a JPEG file
 * in the cache directory under a provided target path (or default naming).
 *
 * Note: Implementation may depend on poppler-glib and gdk-pixbuf. If not
 * available at build time, this function should return COVER_ERR_UNSUPPORTED.
 *
 * Parameters:
 * - pdf_path:   Absolute or relative path to the PDF file
 * - target_jpg: If non-NULL, absolute path for the output JPEG; if NULL, the
 *               function will create a file under the covers cache directory
 *               using a hashed filename. The returned path (via out_path)
 *               will reflect the actual location.
 * - out_path:   Output pointer to receive the absolute path to the generated JPEG
 * - width_px:   Desired width in pixels for the cover image (height is scaled)
 *
 * Returns:
 * - COVER_OK on success (out_path set)
 * - COVER_ERR_INVALID_ARG for bad inputs
 * - COVER_ERR_UNSUPPORTED if PDF rendering dependencies are unavailable
 * - COVER_ERR_IO on filesystem or encoding errors
 * - COVER_ERR_MEMORY if allocation fails
 */
int cover_extract_from_pdf(const char *pdf_path,
                           const char *target_jpg,
                           char **out_path,
                           int width_px);

#ifdef __cplusplus
}
#endif

#endif /* BOOKNOTE_EXTERNAL_COVER_H */