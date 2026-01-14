#define _POSIX_C_SOURCE 200809L
#include "note.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

BnError note_create(Note **out, int book_id, const char *title, const char *content, int page_number) {
    if (!out || book_id <= 0 || !content) {
        return BN_ERROR_INVALID_ARG;
    }
    
    Note *note = calloc(1, sizeof(Note));
    if (!note) {
        return BN_ERROR_OUT_OF_MEMORY;
    }
    
    note->book_id = book_id;
    note->page_number = page_number;
    
    // Set title (generate default if not provided)
    if (title && strlen(title) > 0) {
        note->title = strdup(title);
    } else {
        // Generate default title from first line of content (max 50 chars)
        char default_title[51];
        const char *first_line_end = strchr(content, '\n');
        if (first_line_end && (first_line_end - content) < 47) {
            snprintf(default_title, first_line_end - content + 1, "%s", content);
        } else {
            snprintf(default_title, sizeof(default_title), "%.47s...", content);
        }
        note->title = strdup(default_title);
    }
    
    note->content = strdup(content);
    
    if (!note->title || !note->content) {
        note_free(note);
        return BN_ERROR_OUT_OF_MEMORY;
    }
    
    note->created_at = time(NULL);
    note->updated_at = note->created_at;
    
    *out = note;
    return BN_SUCCESS;
}

void note_free(Note *note) {
    if (!note) {
        return;
    }
    
    free(note->content);
    free(note->title);
    free(note);
}

BnError note_set_content(Note *note, const char *content) {
    if (!note || !content) {
        return BN_ERROR_INVALID_ARG;
    }
    
    char *new_content = strdup(content);
    if (!new_content) {
        return BN_ERROR_OUT_OF_MEMORY;
    }
    
    free(note->content);
    note->content = new_content;
    note->updated_at = time(NULL);
    
    return BN_SUCCESS;
}

BnError note_set_page(Note *note, int page_number) {
    if (!note) {
        return BN_ERROR_INVALID_ARG;
    }
    
    note->page_number = page_number;
    note->updated_at = time(NULL);
    
    return BN_SUCCESS;
}

BnError note_set_title(Note *note, const char *title) {
    if (!note || !title) {
        return BN_ERROR_INVALID_ARG;
    }
    
    char *new_title = strdup(title);
    if (!new_title) {
        return BN_ERROR_OUT_OF_MEMORY;
    }
    
    free(note->title);
    note->title = new_title;
    note->updated_at = time(NULL);
    
    return BN_SUCCESS;
}
