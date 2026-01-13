#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database/db.h"
#include "cli/commands.h"
#include "utils/error.h"

int main(int argc, char **argv) {
    // No command provided
    if (argc < 2) {
        cmd_help(0, NULL);
        return 1;
    }
    
    char *command = argv[1];
    
    // Commands that don't need database
    if (strcmp(command, "help") == 0 || strcmp(command, "--help") == 0 || strcmp(command, "-h") == 0) {
        return cmd_help(argc, argv);
    }
    
    if (strcmp(command, "version") == 0 || strcmp(command, "--version") == 0 || strcmp(command, "-v") == 0) {
        return cmd_version();
    }
    
    // Open database for all other commands
    Database *db = NULL;
    BnError err = db_open(&db, NULL);
    if (err != BN_SUCCESS) {
        bn_print_error(err, "opening database");
        return 1;
    }
    
    // Route to command
    int result = 0;
    
    if (strcmp(command, "add") == 0) {
        result = cmd_add(db, argc, argv);
    } else if (strcmp(command, "list") == 0) {
        result = cmd_list(db, argc, argv);
    } else if (strcmp(command, "show") == 0) {
        result = cmd_show(db, argc, argv);
    } else if (strcmp(command, "note") == 0) {
        result = cmd_note(db, argc, argv);
    } else if (strcmp(command, "search") == 0) {
        result = cmd_search(db, argc, argv);
    } else if (strcmp(command, "delete") == 0) {
        result = cmd_delete(db, argc, argv);
    } else {
        fprintf(stderr, "Error: Unknown command '%s'\n", command);
        fprintf(stderr, "Run 'booknote help' for usage information.\n");
        result = 1;
    }
    
    db_close(db);
    return result;
}
