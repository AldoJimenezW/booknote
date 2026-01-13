#ifndef BOOKNOTE_COMMANDS_H
#define BOOKNOTE_COMMANDS_H

#include "../database/db.h"

/**
 * Command handler functions
 * Each returns 0 on success, non-zero on error
 */

/**
 * Add a book to the library
 * Usage: booknote add <filepath> [--title TITLE] [--author AUTHOR] [--isbn ISBN]
 */
int cmd_add(Database *db, int argc, char **argv);

/**
 * List all books
 * Usage: booknote list
 */
int cmd_list(Database *db, int argc, char **argv);

/**
 * Show book details and notes
 * Usage: booknote show <book-id>
 */
int cmd_show(Database *db, int argc, char **argv);

/**
 * Add a note to a book
 * Usage: booknote note <book-id> <"note text"> [--page N]
 */
int cmd_note(Database *db, int argc, char **argv);

/**
 * Search notes
 * Usage: booknote search <"query">
 */
int cmd_search(Database *db, int argc, char **argv);

/**
 * Delete a book
 * Usage: booknote delete <book-id>
 */
int cmd_delete(Database *db, int argc, char **argv);

/**
 * Show help
 * Usage: booknote help [command]
 */
int cmd_help(int argc, char **argv);

/**
 * Show version
 * Usage: booknote version
 */
int cmd_version(void);

#endif // BOOKNOTE_COMMANDS_H
