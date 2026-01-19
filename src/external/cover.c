/*
 * booknote/src/external/cover.c
 *
 * Implement cover download and cache functions; PDF extraction placeholder.
 *
 * Responsibilities:
 * - Compute cache directory paths for covers under ~/.cache/booknote/covers
 * - Build cached cover file path for given ISBN
 * - Download cover image from URL and store in cache (reusing existing file)
 * - Provide a placeholder for extracting a cover from PDF (returns unsupported)
 *
 * Build dependencies (via pkg-config):
 * - libcurl
 *
 * Notes:
 * - This implementation normalizes ISBN strings to digits-only for file names.
 * - The PDF extraction is not implemented here and will return COVER_ERR_UNSUPPORTED.
 */

#include "cover.h"

#include <curl/curl.h>
#include <poppler/glib/poppler.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo/cairo.h>

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* -------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/* Normalize ISBN to digits-only; returns newly allocated string or NULL */
static char *normalize_isbn_digits(const char *isbn) {
    if (!isbn) return NULL;
    size_t len = strlen(isbn);
    char *out = (char *)malloc(len + 1);
    if (!out) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (isdigit((unsigned char)isbn[i])) {
            out[j++] = isbn[i];
        }
    }
    out[j] = '\0';

    if (j == 0) {
        free(out);
        return NULL;
    }
    return out;
}

/* Create directory if not exists (single level). Returns 0 on success, -1 on error. */
static int ensure_dir(const char *path, mode_t mode) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) return 0;
        return -1;
    }
    if (mkdir(path, mode) == 0) return 0;
    /* If mkdir failed because it already exists, treat as success */
    if (errno == EEXIST) return 0;
    return -1;
}

/* Write callback for libcurl to write binary data into a FILE* */
static size_t curl_write_file_cb(void *ptr, size_t size, size_t nmemb, void *stream) {
    FILE *fp = (FILE *)stream;
    return fwrite(ptr, size, nmemb, fp);
}

/* Download URL to file path using libcurl; returns 0 on success, <0 on error */
static int http_download_to_file(const char *url, const char *filepath) {
    if (!url || !filepath) return COVER_ERR_INVALID_ARG;

    CURL *curl = curl_easy_init();
    if (!curl) return COVER_ERR_NETWORK;

    FILE *fp = fopen(filepath, "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return COVER_ERR_IO;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "booknote/0.4.0 (+https://github.com/AldoJimenezW/booknote)");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_file_cb);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);

    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

    curl_easy_cleanup(curl);
    fclose(fp);

    if (res != CURLE_OK) {
        /* Remove incomplete file */
        unlink(filepath);
        return COVER_ERR_NETWORK;
    }
    if (status < 200 || status >= 300) {
        unlink(filepath);
        return COVER_ERR_HTTP_STATUS;
    }

    return COVER_OK;
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

char *cover_cache_dir(void) {
    const char *home = getenv("HOME");
    if (!home || !*home) {
        return NULL;
    }
    const char *suffix = "/.cache/booknote/covers";
    size_t need = strlen(home) + strlen(suffix) + 1;
    char *path = (char *)malloc(need);
    if (!path) return NULL;
    snprintf(path, need, "%s%s", home, suffix);
    return path;
}

int cover_ensure_cache_dir(void) {
    const char *home = getenv("HOME");
    if (!home || !*home) return COVER_ERR_IO;

    char path[PATH_MAX];

    /* ~/.cache */
    snprintf(path, sizeof(path), "%s/.cache", home);
    if (ensure_dir(path, 0755) != 0 && errno != EEXIST) return COVER_ERR_IO;

    /* ~/.cache/booknote */
    snprintf(path, sizeof(path), "%s/.cache/booknote", home);
    if (ensure_dir(path, 0755) != 0 && errno != EEXIST) return COVER_ERR_IO;

    /* ~/.cache/booknote/covers */
    snprintf(path, sizeof(path), "%s/.cache/booknote/covers", home);
    if (ensure_dir(path, 0755) != 0 && errno != EEXIST) return COVER_ERR_IO;

    return COVER_OK;
}

char *cover_path_for_isbn(const char *isbn) {
    if (!isbn || !*isbn) return NULL;
    char *normalized = normalize_isbn_digits(isbn);
    if (!normalized) return NULL;

    char *dir = cover_cache_dir();
    if (!dir) {
        free(normalized);
        return NULL;
    }

    size_t need = strlen(dir) + 1 /* slash */ + strlen(normalized) + strlen(".jpg") + 1;
    char *path = (char *)malloc(need);
    if (!path) {
        free(dir);
        free(normalized);
        return NULL;
    }
    snprintf(path, need, "%s/%s.jpg", dir, normalized);

    free(dir);
    free(normalized);
    return path;
}

int cover_download_for_isbn(const char *isbn, const char *cover_url, char **out_path) {
    if (!isbn || !cover_url || !out_path) return COVER_ERR_INVALID_ARG;
    *out_path = NULL;

    /* Ensure cache directory exists */
    int rc = cover_ensure_cache_dir();
    if (rc != COVER_OK) return rc;

    /* Build target path */
    char *path = cover_path_for_isbn(isbn);
    if (!path) return COVER_ERR_MEMORY;

    /* If already cached and non-empty, reuse */
    struct stat st;
    if (stat(path, &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0) {
        *out_path = path;
        return COVER_OK;
    }

    /* Download */
    rc = http_download_to_file(cover_url, path);
    if (rc != COVER_OK) {
        free(path);
        return rc;
    }

    *out_path = path;
    return COVER_OK;
}

/* Placeholder: PDF cover extraction not implemented here. */
int cover_extract_from_pdf(const char *pdf_path,
                           const char *target_jpg,
                           char **out_path,
                           int width_px) {
    if (!pdf_path || !target_jpg || !out_path || width_px <= 0) {
        return COVER_ERR_INVALID_ARG;
    }
    *out_path = NULL;

    /* Build file:// URI for Poppler */
    char *uri = NULL;
    {
        GFile *file = g_file_new_for_path(pdf_path);
        if (!file) {
            return COVER_ERR_IO;
        }
        uri = g_file_get_uri(file);
        g_object_unref(file);
        if (!uri) {
            return COVER_ERR_IO;
        }
    }

    /* Load document */
    GError *error = NULL;
    PopplerDocument *doc = poppler_document_new_from_file(uri, NULL, &error);
    g_free(uri);
    if (!doc) {
        if (error) g_error_free(error);
        return COVER_ERR_IO;
    }

    /* Get first page */
    PopplerPage *page = poppler_document_get_page(doc, 0);
    if (!page) {
        g_object_unref(doc);
        return COVER_ERR_IO;
    }

    /* Page size and scale */
    double page_w = 0.0, page_h = 0.0;
    poppler_page_get_size(page, &page_w, &page_h);
    if (page_w <= 0.0 || page_h <= 0.0) {
        g_object_unref(page);
        g_object_unref(doc);
        return COVER_ERR_IO;
    }
    double scale = (double)width_px / page_w;
    int out_w = width_px;
    int out_h = (int)(page_h * scale);

    /* Create pixbuf */
    /* Render first page to Cairo image surface and convert to GdkPixbuf */
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, out_w, out_h);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        g_object_unref(page);
        g_object_unref(doc);
        return COVER_ERR_MEMORY;
    }
    cairo_t *cr = cairo_create(surface);
    cairo_scale(cr, scale, scale);
    poppler_page_render(page, cr);
    cairo_destroy(cr);

    unsigned char *src_data = cairo_image_surface_get_data(surface);
    int src_stride = cairo_image_surface_get_stride(surface);
    if (!src_data || src_stride <= 0) {
        cairo_surface_destroy(surface);
        g_object_unref(page);
        g_object_unref(doc);
        return COVER_ERR_IO;
    }

    int dst_stride = out_w * 3; /* RGB */
    unsigned char *dst_data = g_malloc(out_h * dst_stride);
    if (!dst_data) {
        cairo_surface_destroy(surface);
        g_object_unref(page);
        g_object_unref(doc);
        return COVER_ERR_MEMORY;
    }

    /* Convert ARGB32 (premultiplied) to RGB by ignoring alpha (Dark theme, no transparency needed) */
    for (int y = 0; y < out_h; y++) {
        const unsigned char *src_row = src_data + y * src_stride;
        unsigned char *dst_row = dst_data + y * dst_stride;
        for (int x = 0; x < out_w; x++) {
            /* ARGB32 in Cairo is native-endian; on little-endian it's BGRA */
            unsigned char b = src_row[x * 4 + 0];
            unsigned char g = src_row[x * 4 + 1];
            unsigned char r = src_row[x * 4 + 2];
            /* alpha = src_row[x * 4 + 3]; */
            dst_row[x * 3 + 0] = r;
            dst_row[x * 3 + 1] = g;
            dst_row[x * 3 + 2] = b;
        }
    }

    /* Free callback for pixbuf data */
    void destroy_notify(guchar *pixels, gpointer data) {
        (void)data;
        g_free(pixels);
    }

    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data(
        dst_data,
        GDK_COLORSPACE_RGB,
        FALSE, /* no alpha */
        8,
        out_w,
        out_h,
        dst_stride,
        destroy_notify,
        NULL
    );
    if (!pixbuf) {
        g_free(dst_data);
        cairo_surface_destroy(surface);
        g_object_unref(page);
        g_object_unref(doc);
        return COVER_ERR_MEMORY;
    }

    /* Save as JPEG */
    gboolean saved = FALSE;
    error = NULL;
    saved = gdk_pixbuf_save(pixbuf, target_jpg, "jpeg", &error, "quality", "90", NULL);

    g_object_unref(pixbuf);
    cairo_surface_destroy(surface);
    g_object_unref(page);
    g_object_unref(doc);

    if (!saved) {
        if (error) g_error_free(error);
        return COVER_ERR_IO;
    }

    /* Return path (caller takes ownership) */
    *out_path = g_strdup(target_jpg);
    return COVER_OK;
}