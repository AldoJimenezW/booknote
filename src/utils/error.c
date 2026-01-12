#include "error.h"
#include <stdio.h>

const char* bn_error_string(BnError err) {
    switch (err) {
        case BN_SUCCESS:
            return "Success";
        case BN_ERROR_INVALID_ARG:
            return "Invalid argument";
        case BN_ERROR_DATABASE:
            return "Database error";
        case BN_ERROR_FILE_NOT_FOUND:
            return "File not found";
        case BN_ERROR_NETWORK:
            return "Network error";
        case BN_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case BN_ERROR_NOT_FOUND:
            return "Resource not found";
        case BN_ERROR_DUPLICATE:
            return "Duplicate entry";
        case BN_ERROR_PERMISSION:
            return "Permission denied";
        case BN_ERROR_UNKNOWN:
        default:
            return "Unknown error";
    }
}

void bn_print_error(BnError err, const char *context) {
    if (err == BN_SUCCESS) {
        return;
    }
    
    fprintf(stderr, "Error: %s", bn_error_string(err));
    if (context) {
        fprintf(stderr, " (%s)", context);
    }
    fprintf(stderr, "\n");
}
