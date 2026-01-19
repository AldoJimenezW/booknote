# Booknote v1.0.0

A minimalist personal library manager with a clean Dark Academia-inspired UI. This release introduces a simplified Markdown notes editor and full CRUD actions for books directly in the Library view.

## What’s new in v1.0.0
- Notes:
  - Replaced the previous Org-mode renderer with a well-supported Markdown editor for reliability and clarity.
  - Live formatting with common Markdown inline styles and code blocks.
- Library:
  - Added Edit and Delete actions for books from the Library header and/or card actions.
  - Improved “Add Book” flow with a file chooser for PDFs and metadata autofill via ISBN.
  - Fallback cover generation from the first page of the PDF using Poppler + Cairo.
- UI/UX:
  - Polished Dark Academia CSS with consistent styles for headers, buttons, cards, and focus rings.
  - Navigation via GtkStack between Library and Reading views with smooth transitions.

## Features
- Library view with book cards (cover, title, author).
- Reading view with a PDF viewer and a Markdown notes panel.
- ISBN metadata fetch and cover download from OpenLibrary (with caching).
- Cover fallback rendering from PDFs when ISBN covers are unavailable.
- CRUD:
  - Add Book (with ISBN fetch and PDF chooser).
  - Edit Book (title, author, publisher, year, filepath, ISBN).
  - Delete Book (with confirmation).
- Keyboard shortcuts:
  - Ctrl+Q: Quit
  - Ctrl+L: Go to Library
  - Ctrl+B: Toggle notes panel

## Build and Run
Dependencies (Debian/Ubuntu):
- GTK3: libgtk-3-dev
- Poppler GLib: libpoppler-glib-dev
- SQLite3: libsqlite3-dev
- cURL: libcurl4-openssl-dev
- JSON-C: libjson-c-dev
- pkg-config, build-essential

Build:
- make booknote-gui

Run:
- ./booknote-gui

## Usage Guide

### Library View
- Add Book:
  - Click “+ Add Book”.
  - Optionally enter an ISBN and click “Fetch from ISBN” to auto-fill metadata and get a cover.
  - Select the PDF with the file chooser.
  - Click “Add” to insert it into your library.
- Edit Selected:
  - Select a book card to focus it.
  - Click “Edit Selected” to open the edit dialog.
  - Modify metadata and/or change the PDF.
  - Save to update the database.
- Delete Selected:
  - Select a book card.
  - Click “Delete Selected”.
  - Confirm deletion.

### Reading View
- Shows the PDF on the left and the Markdown notes panel on the right.
- Use Ctrl+B to toggle the notes panel.
- Notes support:
  - Inline: bold, italic, code.
  - Links: standard Markdown links.
  - Blocks: fenced code blocks.
  - Lists: ordered and unordered.
- Notes are saved per book and persist across sessions.

## Markdown Examples
Inline:
- Bold: **bold text**
- Italic: _italic text_
- Code: `inline_code()`
- Link: [Org-mode Docs](https://orgmode.org)

Lists:
- Unordered:
  - Item A
  - Item B
- Ordered:
  1. First
  2. Second

Code blocks:
``` 
```c
#include <stdio.h>
int main(void) {
    printf("Hello, Markdown!\n");
    return 0;
}
```
```

## Covers and Caching
- ISBN covers are downloaded to ~/.cache/booknote/covers/{isbn}.jpg
- If no ISBN cover is found, Booknote renders the first page of the PDF at a suitable thumbnail size and caches it.

## Roadmap
- Preferences for notes (debounce, theme).
- Improved accessibility and keyboard navigation.
- Export notes to Markdown files.
- Better error feedback for network/IO.

## License
See LICENSE for details.

Note-taking for technical books. Simple, fast, local-first.

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Version](https://img.shields.io/badge/version-0.1.0-green)]()

---

## What is booknote?

booknote is a command-line tool for managing notes on technical books and PDFs. It solves a simple problem: existing note-taking apps either don't handle PDFs well, or they're overly complex for focused reading workflows.

booknote keeps your book annotations organized, searchable, and exportable—without getting in your way.

**Design principles:**
- Fast and lightweight (written in C)
- Local-first (your data stays on your machine)
- Simple CLI interface
- Zero configuration required

---

## Installation

### From Source
```bash
git clone https://github.com/AldoJimenezW/booknote.git
cd booknote
make
sudo make install
```

### Requirements

- GCC or Clang
- SQLite3 development libraries
- Make

**Ubuntu/Debian:**
```bash
sudo apt install build-essential libsqlite3-dev
```

**Arch Linux:**
```bash
sudo pacman -S base-devel sqlite
```

**macOS:**
```bash
brew install sqlite
```

---

## Quick Start
```bash
# Add a book
booknote add ~/books/sicp.pdf --title "SICP" --author "Abelson & Sussman"

# List your library
booknote list

# Add a note
booknote note 1 "Recursion explained beautifully" --page 45

# Show book with notes
booknote show 1

# Search notes
booknote search "recursion"

# Get help
booknote help
```

---

## Usage

### Add a Book
```bash
booknote add <filepath> [options]

Options:
  --title TITLE      Book title (default: filename)
  --author AUTHOR    Author name
  --isbn ISBN        ISBN-10 or ISBN-13
  --year YEAR        Publication year
  --publisher PUB    Publisher name

Examples:
  booknote add mybook.pdf
  booknote add ~/books/linux.pdf --title "Understanding the Linux Kernel"
  booknote add book.pdf --title "My Book" --author "John Doe" --isbn "1234567890"
```

### List Books
```bash
booknote list

Output:
  Books in library: 3
  [1] Structure and Interpretation of Computer Programs - Abelson & Sussman
  [2] The C Programming Language - Kernighan & Ritchie (1978)
  [3] Understanding the Linux Kernel - Bovet & Cesati
```

### Show Book Details
```bash
booknote show <book-id>

Example:
  booknote show 1
  
Output:
  === Book Details ===
  ID: 1
  Title: SICP
  Author: Abelson & Sussman
  File: /home/user/books/sicp.pdf
  
  === Notes ===
  Total notes: 2
  [1] (page 45) Recursion explained beautifully
  [2] Chapter 1 introduces computation fundamentals
```

### Add Notes
```bash
booknote note <book-id> "note text" [--page N]

Examples:
  booknote note 1 "Important concept to remember"
  booknote note 1 "Pointers require careful handling" --page 67
```

### Search Notes
```bash
booknote search "query"

Example:
  booknote search "recursion"
  
Output:
  Found 1 note(s) matching: "recursion"
  [1] (SICP) page 45: Recursion explained beautifully
```

### Delete a Book
```bash
booknote delete <book-id>

Example:
  booknote delete 2
  Delete book: The C Programming Language? (y/N): y
  Book deleted (including all notes).
```

---

## Features

### Current (v0.1.0)

- Add books with metadata (title, author, ISBN, year, publisher)
- Create page-specific notes
- Full-text search across all notes (powered by SQLite FTS5)
- List and browse your library
- Delete books (cascades to notes)
- Local SQLite database
- Zero configuration

### Planned (v0.2+)

- PDF text extraction and caching
- Export to Markdown, Org-mode, plain text
- Tags and categories
- ISBN metadata lookup (OpenLibrary API)
- Statistics and reading progress
- Backup and restore
- Terminal UI (ncurses)

See [ROADMAP.md](docs/ROADMAP.md) for detailed planning.

---

## Technical Details

**Stack:**
- Language: C11 (ISO/IEC 9899:2011)
- Database: SQLite3 with FTS5 full-text search
- Build: Make
- Testing: Manual (automated tests planned)

**Architecture:**
- Modular design (core, database, CLI, utils)
- Prepared statements (SQL injection safe)
- Memory-safe (valgrind clean)
- Error handling throughout

See [DESIGN.md](docs/DESIGN.md) for architecture details.

---

## Database Location

By default, booknote stores data at:
```
~/.local/share/booknote/booknote.db
```

This follows the XDG Base Directory specification.

---

## Development

### Building
```bash
make              # Build
make clean        # Clean build artifacts
make run          # Build and run
make install      # Install to /usr/local/bin
```

### Project Structure
```
booknote/
├── src/
│   ├── main.c           # Entry point
│   ├── cli/             # Command handlers
│   ├── core/            # Book and Note models
│   ├── database/        # SQLite wrapper and queries
│   └── utils/           # Error handling, utilities
├── docs/                # Documentation
├── tests/               # Tests (planned)
└── Makefile
```

---

## Contributing

Contributions are welcome! This project is in active development.

**Areas that need help:**
- Automated testing (unit and integration tests)
- Cross-platform testing (macOS, BSD)
- PDF text extraction integration
- Export functionality
- Documentation improvements
- Bug reports and feature requests

Please open an issue before starting work on major features.

---

## License

GNU General Public License v3.0 - see [LICENSE](LICENSE) for details.

This means you can use, modify, and distribute this software, but any derivative work must also be open-source under GPL-3.0.

---

## Author

**Aldo Jiménez Wiehoff**

- GitHub: [@AldoJimenezW](https://github.com/AldoJimenezW)
- Email: aldo@jimenezwiehoff.lat
- LinkedIn: [aldo-jimenez-wiehoff](https://linkedin.com/in/aldo-jimenez-wiehoff)

---

## Acknowledgments

Built with passion for technical reading and learning.

Inspired by the limitations of existing tools like Obsidian (poor PDF support), Zotero (academic-focused), and Calibre (library management, not note-taking).

Special thanks to the SQLite team for FTS5.

---

**Star this repo if you find it useful!**
