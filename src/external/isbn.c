/*
 * booknote/src/external/isbn.c
 *
 * ISBN client implementation using libcurl and json-c, plus cache helpers.
 *
 * This module implements:
 * - isbn_fetch_metadata()
 * - isbn_free_metadata()
 * - isbn_cover_url_medium()
 * - isbn_cache_dir()
 * - isbn_ensure_cache_dir()
 * - isbn_cached_cover_path()
 * - isbn_download_cover()
 *
 * Build dependencies (via pkg-config):
 * - libcurl
 * - json-c
 */

#include "isbn.h"

#include <curl/curl.h>
#include <json-c/json.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <glib.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

/* OpenLibrary base endpoints */
#define OPENLIBRARY_ISBN_JSON_FMT "https://openlibrary.org/isbn/%s.json"
#define OPENLIBRARY_COVER_MEDIUM_FMT "https://covers.openlibrary.org/b/isbn/%s-M.jpg"

/* Internal helpers */

typedef struct {
    char *data;
    size_t size;
} MemoryBuffer;

static void mem_buffer_init(MemoryBuffer *mb) {
    mb->data = NULL;
    mb->size = 0;
}

static void mem_buffer_free(MemoryBuffer *mb) {
    free(mb->data);
    mb->data = NULL;
    mb->size = 0;
}

static size_t curl_write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    MemoryBuffer *mb = (MemoryBuffer *)userp;

    char *new_data = (char *)realloc(mb->data, mb->size + realsize + 1);
    if (!new_data) {
        return 0; /* signal failure to libcurl */
    }
    mb->data = new_data;
    memcpy(mb->data + mb->size, contents, realsize);
    mb->size += realsize;
    mb->data[mb->size] = '\0';
    return realsize;
}

/* Trim ISBN string to digits only (remove hyphens/spaces). Returns newly allocated string. */
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

/* Fetch URL into memory buffer using libcurl */
static int http_get_to_buffer(const char *url, MemoryBuffer *out) {
    if (!url || !out) return ISBN_ERR_INVALID_ARG;

    CURL *curl = curl_easy_init();
    if (!curl) return ISBN_ERR_NETWORK;

    mem_buffer_init(out);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "booknote/0.4.0 (+https://github.com/AldoJimenezW/booknote)");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        mem_buffer_free(out);
        return ISBN_ERR_NETWORK;
    }

    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_cleanup(curl);

    if (status < 200 || status >= 300) {
        mem_buffer_free(out);
        return ISBN_ERR_HTTP_STATUS;
    }

    return ISBN_OK;
}

/* Create directory if not exists. Returns 0 on success, -1 on error. */
static int ensure_dir(const char *path, mode_t mode) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) return 0;
        return -1;
    }
    if (mkdir(path, mode) == 0) return 0;
    return -1;
}

/* Extract a reasonable author string from OpenLibrary JSON object */
static char *extract_author_best_effort(json_object *root) {
    /* Try "by_statement" first (often contains human-readable authors) */
    json_object *by_stmt = NULL;
    if (json_object_object_get_ex(root, "by_statement", &by_stmt) && by_stmt && json_object_is_type(by_stmt, json_type_string)) {
        const char *s = json_object_get_string(by_stmt);
        if (s && *s) return g_strdup(s);
    }

    /* Try "authors" array, but OpenLibrary returns keys that would need another request.
       We'll best-effort return "Author (key)" or just the key if name not available. */
    json_object *authors = NULL;
    if (json_object_object_get_ex(root, "authors", &authors) && authors && json_object_is_type(authors, json_type_array)) {
        if (json_object_array_length(authors) > 0) {
            json_object *first = json_object_array_get_idx(authors, 0);
            if (first && json_object_is_type(first, json_type_object)) {
                json_object *key = NULL;
                if (json_object_object_get_ex(first, "key", &key) && key && json_object_is_type(key, json_type_string)) {
                    const char *k = json_object_get_string(key);
                    if (k && *k) {
                        /* Return the key as a placeholder, e.g., "/authors/OL12345A" */
                        return g_strdup(k);
                    }
                }
            }
        }
    }

    /* Fallback: unknown */
    return NULL;
}

/* Extract publisher best-effort (from "publishers" array first element, or "publisher") */
static char *extract_publisher_best_effort(json_object *root) {
    json_object *publishers = NULL;
    if (json_object_object_get_ex(root, "publishers", &publishers) && publishers && json_object_is_type(publishers, json_type_array)) {
        if (json_object_array_length(publishers) > 0) {
            json_object *first = json_object_array_get_idx(publishers, 0);
            if (first && json_object_is_type(first, json_type_string)) {
                const char *s = json_object_get_string(first);
                if (s && *s) return g_strdup(s);
            }
        }
    }
    json_object *publisher = NULL;
    if (json_object_object_get_ex(root, "publisher", &publisher) && publisher && json_object_is_type(publisher, json_type_string)) {
        const char *s = json_object_get_string(publisher);
        if (s && *s) return g_strdup(s);
    }
    return NULL;
}

/* Extract year best-effort from "publish_date" (e.g., "April 2001") or "publish_year" array */
static int extract_year_best_effort(json_object *root) {
    json_object *publish_year = NULL;
    if (json_object_object_get_ex(root, "publish_year", &publish_year) && publish_year && json_object_is_type(publish_year, json_type_array)) {
        if (json_object_array_length(publish_year) > 0) {
            json_object *first = json_object_array_get_idx(publish_year, 0);
            if (first && json_object_is_type(first, json_type_int)) {
                return json_object_get_int(first);
            }
        }
    }
    json_object *publish_date = NULL;
    if (json_object_object_get_ex(root, "publish_date", &publish_date) && publish_date && json_object_is_type(publish_date, json_type_string)) {
        const char *s = json_object_get_string(publish_date);
        if (s && *s) {
            /* Parse last 4 consecutive digits in the string as year */
            int year = 0;
            int digits[4] = {0, 0, 0, 0};
            int idx = 0;
            for (size_t i = 0; i < strlen(s); i++) {
                if (isdigit((unsigned char)s[i])) {
                    if (idx < 4) digits[idx++] = s[i] - '0';
                    else {
                        /* Shift window */
                        digits[0] = digits[1];
                        digits[1] = digits[2];
                        digits[2] = digits[3];
                        digits[3] = s[i] - '0';
                    }
                } else {
                    /* reset if interrupted; keep last 4-digit run only */
                }
            }
            if (idx == 4) {
                year = digits[0]*1000 + digits[1]*100 + digits[2]*10 + digits[3];
                return year;
            } else if (idx > 0) {
                /* if fewer digits, attempt parse at end */
                char buf[8] = {0};
                int j = 0;
                for (int i = 0; i < idx && j < 7; i++) buf[j++] = (char)('0' + digits[i]);
                year = atoi(buf);
                if (year > 0) return year;
            }
        }
    }
    return 0;
}

/* Public API implementations */

int isbn_fetch_metadata(const char *isbn, IsbnMetadata **out_meta) {
    if (!isbn || !out_meta) return ISBN_ERR_INVALID_ARG;

    *out_meta = NULL;

    char *normalized = normalize_isbn_digits(isbn);
    if (!normalized) return ISBN_ERR_INVALID_ARG;

    char url[256];
    snprintf(url, sizeof(url), OPENLIBRARY_ISBN_JSON_FMT, normalized);

    MemoryBuffer mb;
    int rc = http_get_to_buffer(url, &mb);
    if (rc != ISBN_OK) {
        free(normalized);
        return rc;
    }

    /* Parse JSON */
    json_object *root = json_tokener_parse(mb.data);
    mem_buffer_free(&mb);
    if (!root || !json_object_is_type(root, json_type_object)) {
        free(normalized);
        if (root) json_object_put(root);
        return ISBN_ERR_PARSE;
    }

    IsbnMetadata *meta = (IsbnMetadata *)calloc(1, sizeof(IsbnMetadata));
    if (!meta) {
        json_object_put(root);
        free(normalized);
        return ISBN_ERR_MEMORY;
    }

    meta->isbn = normalized;

    /* Title */
    json_object *title = NULL;
    if (json_object_object_get_ex(root, "title", &title) && title && json_object_is_type(title, json_type_string)) {
        const char *s = json_object_get_string(title);
        if (s && *s) meta->title = g_strdup(s);
    }

    /* Author best-effort */
    meta->author = extract_author_best_effort(root);

    /* Publisher best-effort */
    meta->publisher = extract_publisher_best_effort(root);

    /* Year best-effort */
    meta->year = extract_year_best_effort(root);

    json_object_put(root);

    *out_meta = meta;
    return ISBN_OK;
}

void isbn_free_metadata(IsbnMetadata *meta) {
    if (!meta) return;
    free(meta->isbn);
    free(meta->title);
    free(meta->author);
    free(meta->publisher);
    free(meta);
}

char *isbn_cover_url_medium(const char *isbn) {
    if (!isbn || !*isbn) return NULL;
    char *normalized = normalize_isbn_digits(isbn);
    if (!normalized) return NULL;

    size_t need = strlen(OPENLIBRARY_COVER_MEDIUM_FMT) + strlen(normalized) + 8;
    char *url = (char *)malloc(need);
    if (!url) {
        free(normalized);
        return NULL;
    }
    snprintf(url, need, OPENLIBRARY_COVER_MEDIUM_FMT, normalized);
    free(normalized);
    return url;
}

/* Build ~/.cache/booknote/covers path */
char *isbn_cache_dir(void) {
    const char *home = getenv("HOME");
    if (!home || !*home) {
        /* As a fallback, try getpwuid? Keep simple: fail if HOME missing */
        return NULL;
    }
    const char *suffix = "/.cache/booknote/covers";
    size_t need = strlen(home) + strlen(suffix) + 1;
    char *path = (char *)malloc(need);
    if (!path) return NULL;
    snprintf(path, need, "%s%s", home, suffix);
    return path;
}

/* Ensure ~/.cache, ~/.cache/booknote, ~/.cache/booknote/covers exist */
int isbn_ensure_cache_dir(void) {
    const char *home = getenv("HOME");
    if (!home || !*home) return ISBN_ERR_IO;

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/.cache", home);
    if (ensure_dir(path, 0755) != 0 && errno != EEXIST) return ISBN_ERR_IO;

    snprintf(path, sizeof(path), "%s/.cache/booknote", home);
    if (ensure_dir(path, 0755) != 0 && errno != EEXIST) return ISBN_ERR_IO;

    snprintf(path, sizeof(path), "%s/.cache/booknote/covers", home);
    if (ensure_dir(path, 0755) != 0 && errno != EEXIST) return ISBN_ERR_IO;

    return ISBN_OK;
}

char *isbn_cached_cover_path(const char *isbn) {
    if (!isbn || !*isbn) return NULL;
    char *normalized = normalize_isbn_digits(isbn);
    if (!normalized) return NULL;

    char *dir = isbn_cache_dir();
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

/* Download binary content to file */
static int http_download_to_file(const char *url, const char *filepath) {
    if (!url || !filepath) return ISBN_ERR_INVALID_ARG;

    int rc = isbn_ensure_cache_dir();
    if (rc != ISBN_OK) return rc;

    CURL *curl = curl_easy_init();
    if (!curl) return ISBN_ERR_NETWORK;

    FILE *fp = fopen(filepath, "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return ISBN_ERR_IO;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "booknote/0.4.0 (+https://github.com/AldoJimenezW/booknote)");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL); /* default fwrite */
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

    curl_easy_cleanup(curl);
    fclose(fp);

    if (res != CURLE_OK) {
        /* Remove incomplete file */
        unlink(filepath);
        return ISBN_ERR_NETWORK;
    }
    if (status < 200 || status >= 300) {
        unlink(filepath);
        return ISBN_ERR_HTTP_STATUS;
    }

    return ISBN_OK;
}

/* Extract ISBN from covers URL of form covers.openlibrary.org/b/isbn/{ISBN}-M.jpg */
static char *isbn_from_cover_url(const char *cover_url) {
    if (!cover_url) return NULL;
    const char *marker = "/isbn/";
    const char *p = strstr(cover_url, marker);
    if (!p) return NULL;
    p += strlen(marker);
    const char *dash = strchr(p, '-');
    if (!dash) return NULL;
    size_t len = (size_t)(dash - p);
    if (len == 0) return NULL;
    char *isbn_raw = (char *)malloc(len + 1);
    if (!isbn_raw) return NULL;
    memcpy(isbn_raw, p, len);
    isbn_raw[len] = '\0';
    /* Normalize digits */
    char *normalized = normalize_isbn_digits(isbn_raw);
    free(isbn_raw);
    return normalized;
}

int isbn_download_cover(const char *cover_url, char **out_path) {
    if (!cover_url || !out_path) return ISBN_ERR_INVALID_ARG;
    *out_path = NULL;

    /* Determine ISBN from URL to compute cache path */
    char *isbn_norm = isbn_from_cover_url(cover_url);
    if (!isbn_norm) return ISBN_ERR_INVALID_ARG;

    char *path = isbn_cached_cover_path(isbn_norm);
    free(isbn_norm);
    if (!path) return ISBN_ERR_MEMORY;

    /* If already exists, return as is */
    struct stat st;
    if (stat(path, &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0) {
        *out_path = path;
        return ISBN_OK;
    }

    /* Download */
    int rc = http_download_to_file(cover_url, path);
    if (rc != ISBN_OK) {
        free(path);
        return rc;
    }

    *out_path = path;
    return ISBN_OK;
}