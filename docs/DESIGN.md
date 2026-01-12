# booknote Architecture Design

This document outlines the technical architecture and design decisions for booknote.

---

## System Overview

booknote is a command-line application that manages a local library of books (PDFs) and associated notes. It uses SQLite for storage and provides a simple CLI interface.

### High-Level Architecture
```
┌─────────────────────────────────────────────────────────┐
│                     CLI Interface                        │
│                    (src/cli/)                           │
│  Commands: add, list, note, search, export, etc.       │
└──────────────────┬──────────────────────────────────────┘
                   │
                   v
┌─────────────────────────────────────────────────────────┐
│                   Core Library                           │
│                    (src/core/)                          │
│  - Book management   - Note management                  │
│  - Search engine     - Export functions                 │
└──────────────────┬──────────────────────────────────────┘
                   │
                   v
┌─────────────────────────────────────────────────────────┐
│                  Data Layer                              │
│                 (src/database/)                         │
│  - SQLite wrapper    - Schema management                │
│  - Query builders    - Migrations                       │
└──────────────────┬──────────────────────────────────────┘
                   │
                   v
┌─────────────────────────────────────────────────────────┐
│                External Services                         │
│                  (src/external/)                        │
│  - ISBN API client (OpenLibrary)                        │
│  - PDF text extraction (pdftotext)                      │
└─────────────────────────────────────────────────────────┘
```

---

## Directory Structure
```
booknote/
├── src/
│   ├── main.c                  # Entry point, CLI parsing
│   ├── core/
│   │   ├── book.h/c           # Book struct and operations
│   │   ├── note.h/c           # Note struct and operations
│   │   ├── search.h/c         # Search functionality
│   │   └── export.h/c         # Export to various formats
│   ├── database/
│   │   ├── db.h/c             # SQLite wrapper
│   │   ├── schema.h/c         # Schema definitions and migrations
│   │   └── queries.h/c        # Common SQL queries
│   ├── external/
│   │   ├── isbn.h/c           # ISBN API client
│   │   └── pdf.h/c            # PDF text extraction
│   └── utils/
│       ├── string.h/c         # String utilities
│       ├── error.h/c          # Error handling
│       └── config.h/c         # Configuration management
├── tests/
│   ├── test_book.c
│   ├── test_note.c
│   ├── test_database.c
│   └── fixtures/              # Test data
├── docs/
├── examples/
└── Makefile
```

---

## Database Schema

### Version 1 (v0.1)
```sql
-- Books table
CREATE TABLE books (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    isbn TEXT UNIQUE,              -- ISBN-10 or ISBN-13 (optional)
    title TEXT NOT NULL,
    author TEXT,
    year INTEGER,
    publisher TEXT,
    filepath TEXT NOT NULL UNIQUE, -- Path to PDF file
    added_at INTEGER NOT NULL,     -- Unix timestamp
    updated_at INTEGER NOT NULL
);

-- Notes table
CREATE TABLE notes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    book_id INTEGER NOT NULL,
    content TEXT NOT NULL,
    page_number INTEGER,           -- Optional page reference
    created_at INTEGER NOT NULL,
    updated_at INTEGER NOT NULL,
    FOREIGN KEY (book_id) REFERENCES books(id) ON DELETE CASCADE
);

-- Full-text search index
CREATE VIRTUAL TABLE notes_fts USING fts5(
    content,
    content=notes,
    content_rowid=id
);

-- Triggers to keep FTS in sync
CREATE TRIGGER notes_ai AFTER INSERT ON notes BEGIN
    INSERT INTO notes_fts(rowid, content) VALUES (new.id, new.content);
END;

CREATE TRIGGER notes_ad AFTER DELETE ON notes BEGIN
    DELETE FROM notes_fts WHERE rowid = old.id;
END;

CREATE TRIGGER notes_au AFTER UPDATE ON notes BEGIN
    UPDATE notes_fts SET content = new.content WHERE rowid = new.id;
END;

-- Metadata table for schema versioning
CREATE TABLE metadata (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL
);

INSERT INTO metadata (key, value) VALUES ('schema_version', '1');
```

### Future schema additions (v0.2+)
- Tags table for categorization
- PDF text cache table
- User preferences table

---

## Core Data Structures

### Book
```c
typedef struct {
    int id;
    char *isbn;        // Nullable
    char *title;
    char *author;      // Nullable
    int year;          // 0 if unknown
    char *publisher;   // Nullable
    char *filepath;
    time_t added_at;
    time_t updated_at;
} Book;
```

### Note
```c
typedef struct {
    int id;
    int book_id;
    char *content;
    int page_number;   // 0 if not page-specific
    time_t created_at;
    time_t updated_at;
} Note;
```

---

## CLI Interface Design

### Command Structure
```
booknote <command> [options] [arguments]
```

### Commands (v0.1)
```
booknote add <pdf_path> [--isbn ISBN] [--title TITLE] [--author AUTHOR]
  Add a new book to the library
  
booknote list [--sort title|author|date]
  List all books in the library
  
booknote show <book_id>
  Display book details and all notes
  
booknote remove <book_id>
  Remove a book and all its notes
  
booknote note <book_id> <"note text"> [--page N]
  Add a note to a book
  
booknote notes <book_id>
  List all notes for a book
  
booknote search <"query">
  Search notes by keyword
  
booknote export <book_id> [--format md|org|txt] [--output FILE]
  Export book and notes to a file
  
booknote help [command]
  Show help information
  
booknote version
  Show version information
```

### Exit Codes
```
0   - Success
1   - General error
2   - Invalid arguments
3   - Database error
4   - File not found
5   - Network error (ISBN lookup)
```

---

## Error Handling Strategy

### Principles
- Always check return values
- Provide clear error messages to users
- Log errors for debugging
- Clean up resources on failure
- Never crash silently

### Error Reporting
```c
typedef enum {
    BN_SUCCESS = 0,
    BN_ERROR_INVALID_ARG,
    BN_ERROR_DATABASE,
    BN_ERROR_FILE_NOT_FOUND,
    BN_ERROR_NETWORK,
    BN_ERROR_OUT_OF_MEMORY,
    BN_ERROR_NOT_FOUND,
    BN_ERROR_DUPLICATE,
} BnError;

// All functions return BnError
// Use helper to print user-friendly message
void bn_print_error(BnError err, const char *context);
```

---

## Memory Management

### Rules
- All heap allocations must be paired with free
- Use valgrind to detect leaks
- Structs with pointers need explicit free functions
- Use RAII-style cleanup where possible (goto cleanup pattern)

### Example Pattern
```c
BnError book_create(Book **out_book, const char *title, const char *filepath) {
    BnError err = BN_SUCCESS;
    Book *book = NULL;
    
    book = calloc(1, sizeof(Book));
    if (!book) {
        err = BN_ERROR_OUT_OF_MEMORY;
        goto cleanup;
    }
    
    book->title = strdup(title);
    if (!book->title) {
        err = BN_ERROR_OUT_OF_MEMORY;
        goto cleanup;
    }
    
    book->filepath = strdup(filepath);
    if (!book->filepath) {
        err = BN_ERROR_OUT_OF_MEMORY;
        goto cleanup;
    }
    
    *out_book = book;
    return BN_SUCCESS;

cleanup:
    if (book) {
        free(book->title);
        free(book->filepath);
        free(book);
    }
    return err;
}

void book_free(Book *book) {
    if (!book) return;
    free(book->isbn);
    free(book->title);
    free(book->author);
    free(book->publisher);
    free(book->filepath);
    free(book);
}
```

---

## External Dependencies

### Required
- **SQLite3** - Database (ubiquitous, likely already installed)
- **libcurl** - HTTP requests for ISBN API
- **json-c** - JSON parsing for API responses

### Optional (v0.2+)
- **poppler-utils** - Provides `pdftotext` for text extraction
- **ncurses** - Terminal UI (v1.0)

### Rationale
- Keep dependencies minimal
- Prefer standard libraries over specialized ones
- All dependencies should be available in standard package managers

---

## Build System

### v0.1 - Simple Makefile
```makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
LIBS = -lsqlite3 -lcurl -ljson-c

SRC = src/main.c src/core/*.c src/database/*.c src/external/*.c src/utils/*.c
OBJ = $(SRC:.c=.o)
TARGET = booknote

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(OBJ) $(TARGET)

test:
	# TODO: Run tests

install:
	cp $(TARGET) /usr/local/bin/

.PHONY: all clean test install
```

### Future (v1.0)
- Migrate to CMake for better cross-platform support
- Add pkg-config for dependency detection
- Support for various install prefixes

---

## Testing Strategy

### Unit Tests
- Use Criterion framework
- Test each module in isolation
- Mock database and external calls where needed

### Integration Tests
- Test full command workflows
- Use temporary database for isolation
- Verify file operations

### Manual Testing Checklist (v0.1)
```
[ ] Add book without ISBN
[ ] Add book with ISBN
[ ] List empty library
[ ] List library with books
[ ] Add note to book
[ ] Search notes
[ ] Export book to markdown
[ ] Remove book
[ ] Handle invalid commands gracefully
```

---

## Performance Considerations

### v0.1 (not critical yet)
- Simple linear operations acceptable
- Optimize only if user-visible slowness

### v0.2+ (scale considerations)
- Index commonly searched fields
- Batch operations where possible
- Cache PDF text extraction
- Consider pagination for large result sets

---

## Security Considerations

### SQL Injection
- **Always use prepared statements**
- Never concatenate user input into SQL

### Path Traversal
- Validate file paths
- Resolve to absolute paths
- Check file exists and is readable

### API Keys
- ISBN APIs are public, no keys needed initially
- If keys needed later, use environment variables

---

## Configuration

### v0.1 - Hardcoded Defaults
```
Database: ~/.local/share/booknote/booknote.db
Config: (none yet)
```

### v0.2+ - Configuration File
```
~/.config/booknote/config
```

Example:
```ini
[database]
path = ~/.local/share/booknote/booknote.db

[export]
default_format = markdown

[api]
isbn_provider = openlibrary
```

---

## Design Decisions

### Why SQLite?
- Zero configuration
- Single file database
- Excellent FTS support
- Portable
- Reliable

### Why C?
- Performance
- Low-level control
- Demonstrates systems programming skills
- Portable across Unix-like systems
- Minimal runtime dependencies

### Why CLI first?
- Faster to develop than GUI
- Power users prefer CLI
- Easy to script and automate
- TUI/GUI can be added later

### Why local-first?
- Privacy
- No server infrastructure needed
- Works offline
- User owns their data

---

## Future Architecture Considerations

### Plugin System (post-1.0)
- Allow users to extend functionality
- Example: Custom export formats, additional APIs

### Sync Service (post-1.0)
- Git-based (user provides their own git repo)
- Documentation for Syncthing/Dropbox
- Optional self-hosted server for conflict resolution

### API/Library Mode (post-1.0)
- Build as library that can be embedded
- CLI becomes thin wrapper around library
- Enables GUI, mobile apps, etc.

---

Last updated: January 2025
