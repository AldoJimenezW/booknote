#ifndef BOOKNOTE_ERROR_H
#define BOOKNOTE_ERROR_H

/**
 * Error codes for booknote operations
 */
typedef enum {
    BN_SUCCESS = 0,
    BN_ERROR_INVALID_ARG,
    BN_ERROR_DATABASE,
    BN_ERROR_FILE_NOT_FOUND,
    BN_ERROR_NETWORK,
    BN_ERROR_OUT_OF_MEMORY,
    BN_ERROR_NOT_FOUND,
    BN_ERROR_DUPLICATE,
    BN_ERROR_PERMISSION,
    BN_ERROR_UNKNOWN
} BnError;

/**
 * Get human-readable error message
 */
const char* bn_error_string(BnError err);

/**
 * Print error message to stderr with context
 */
void bn_print_error(BnError err, const char *context);

#endif // BOOKNOTE_ERROR_H
