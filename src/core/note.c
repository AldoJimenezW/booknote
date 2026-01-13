#define _POSIX_C_SOURCE 200809L
#include "note.h"
#include <stdlib.h>
#include <string.h>

BnError note_create(Note **out_note, int book_id, const char *content, int page_number) {
    if (!out_note || !content || book_id <= 0) {
        return BN_ERROR_INVALID_ARG;
    }
    
    Note *note = calloc(1, sizeof(Note));
    if (!note) {
        return BN_ERROR_OUT_OF_MEMORY;
    }
    
    note->content = strdup(content);
    if (!note->content) {
        free(note);
        return BN_ERROR_OUT_OF_MEMORY;
    }
    
    note->id = 0;  // Will be set by database
    note->book_id = book_id;
    note->page_number = page_number;
    note->created_at = time(NULL);
    note->updated_at = note->created_at;
    
    *out_note = note;
    return BN_SUCCESS;
}

void note_free(Note *note) {
    if (!note) {
        return;
    }
    
    free(note->content);
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
