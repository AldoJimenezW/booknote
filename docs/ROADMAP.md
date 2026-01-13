# booknote Roadmap

This document outlines the development phases for booknote.

---

## Phase 1: Foundation (v0.1) - ✅ COMPLETED (January 2026)

### Goals
Establish core architecture and basic functionality. Users should be able to manage a library of books with notes.

### Features
- [x] SQLite database schema design
- [x] Book management commands
  - [x] `booknote add <pdf> [--title TITLE] [--author AUTHOR] [--isbn ISBN]`
  - [x] `booknote list`
  - [x] `booknote delete <book-id>`
  - [x] `booknote show <book-id>`
- [x] Note management
  - [x] `booknote note <book-id> "note text" [--page N]`
  - [x] `booknote show <book-id>` (list all notes for a book)
- [x] Search functionality
  - [x] `booknote search "keyword"`
  - [x] SQLite FTS5 full-text search
- [x] Help and version commands
- [x] User-friendly CLI with error messages
- [x] Confirmation prompts for destructive actions

### What Works
- Add books with complete metadata
- List books with authors and years
- Create page-specific notes
- Full-text search across all notes
- Delete books with cascade to notes
- Show book details with all notes
- Clean command-line interface
- Local SQLite storage

### Known Limitations
- No PDF text extraction yet
- No ISBN metadata lookup
- No export functionality
- No configuration file
- Limited input validation
- No automated tests

---

## Phase 2: Enhanced (v0.2) - Target: February 2026

### Goals
Improve usability and add practical features based on real usage. Make the tool more useful for daily workflows.

### Features
- [ ] Export functionality
  - [ ] `booknote export <book-id> [--format md|org|txt]`
  - [ ] Markdown format with book info + notes
  - [ ] Org-mode format
  - [ ] Plain text format
  - [ ] Optional: JSON format for tool integration
- [ ] ISBN metadata integration
  - [ ] OpenLibrary API client
  - [ ] Automatic title, author, year fetching
  - [ ] `booknote add <pdf> --isbn <ISBN>` auto-fills metadata
  - [ ] Manual override still possible
- [ ] Note editing
  - [ ] `booknote edit-note <note-id>`
  - [ ] Opens $EDITOR for modification
  - [ ] Update note content and page number
- [ ] Improved list command
  - [ ] `booknote list [--sort title|author|date]`
  - [ ] Better formatting
  - [ ] Show note count per book
- [ ] Configuration file
  - [ ] `~/.config/booknote/config`
  - [ ] Customize database path
  - [ ] Default export format
  - [ ] Editor preference

### Technical Improvements
- Input validation (file paths, ISBNs)
- Better error messages with suggestions
- Man page documentation
- Shell completion scripts (bash, zsh)

---

## Phase 3: Polish (v0.3) - Target: March 2026

### Goals
Refine UX and add quality-of-life features based on user feedback.

### Features
- [ ] PDF text extraction
  - [ ] Use `pdftotext` system command
  - [ ] Cache extracted text in database
  - [ ] Enable searching within PDF content
  - [ ] Show text context around search matches
- [ ] Tags and categories
  - [ ] `booknote tag <book-id> "tag1,tag2"`
  - [ ] `booknote tags` (list all tags)
  - [ ] Filter by tags in search and list
- [ ] Statistics
  - [ ] `booknote stats`
  - [ ] Total books, notes, pages referenced
  - [ ] Most annotated books
  - [ ] Recent activity
- [ ] Backup and restore
  - [ ] `booknote backup <path>`
  - [ ] `booknote restore <path>`
  - [ ] Simple SQLite database export/import
- [ ] Note deletion
  - [ ] `booknote delete-note <note-id>`
  - [ ] With confirmation
- [ ] Batch operations
  - [ ] Import multiple books from directory
  - [ ] Export all books at once

### Technical Improvements
- Comprehensive integration tests
- Memory leak auditing with valgrind
- Cross-platform testing (macOS, BSD)
- Performance optimization for large libraries (1000+ books)
- Better transaction management

---

## Phase 4: Advanced (v1.0) - Target: April-May 2026

### Goals
Feature-complete release with polished UX. Ready for broad adoption and daily use.

### Features
- [ ] Terminal UI (TUI) with ncurses
  - [ ] Interactive book browser
  - [ ] Note editor with live preview
  - [ ] Search interface with highlighting
  - [ ] Keyboard-driven navigation
- [ ] Enhanced search
  - [ ] Fuzzy matching
  - [ ] Search filters (by book, date range, page range)
  - [ ] Boolean operators (AND, OR, NOT)
  - [ ] Regex support
- [ ] Import from other tools
  - [ ] Zotero notes import
  - [ ] Calibre metadata import
  - [ ] Obsidian markdown import
- [ ] Advanced export
  - [ ] HTML with styling
  - [ ] LaTeX for academic papers
  - [ ] Anki flashcards format
- [ ] PDF annotations (experimental)
  - [ ] Generate annotated PDF with highlights
  - [ ] Requires external tool integration (pdftk)

### Technical Improvements
- CMake build system (in addition to Make)
- Automated test suite with >80% coverage
- CI/CD pipeline (GitHub Actions)
- Package distribution:
  - Homebrew formula (macOS)
  - AUR package (Arch Linux)
  - PPA or .deb (Ubuntu/Debian)
- Comprehensive documentation:
  - Man pages for all commands
  - Tutorial and examples
  - Architecture guide for contributors
  - Video demo

---

## Future Considerations (Post-1.0)

### Potential features (community-driven)

**Collaboration features:**
- Sync support (Git-based for power users)
- Documentation for Syncthing/Dropbox integration
- Optional self-hosted sync server
- Conflict resolution for concurrent edits

**Extended formats:**
- EPUB support
- MOBI support
- Web page archiving
- OCR for scanned books

**Advanced features:**
- Spaced repetition integration
- Reading progress tracking
- Visualization (reading heatmap, tag clouds)
- AI-powered summaries (optional, privacy-preserving)
- Mobile companion app (read-only view)
- Web interface for remote access

**Developer tools:**
- Plugin system for extensions
- REST API for integrations
- Library mode (embed in other applications)
- Language bindings (Python, JavaScript)

**Internationalization:**
- Multi-language support (i18n)
- Localized documentation
- RTL language support

---

## Release Philosophy

- **Early releases prioritize functionality over polish** - Get something working quickly
- **Breaking changes possible before v1.0** - Database schema and CLI may change
- **Migration tools always provided** - When breaking changes occur, we provide upgrade paths
- **Semantic versioning** - Once v1.0 is reached, semver strictly followed
- **User feedback drives priorities** - Community input shapes development direction

---

## Version History

### v0.1.0 (January 2026) - MVP Release
- Initial release with core functionality
- CLI commands: add, list, show, note, search, delete
- SQLite database with FTS5 search
- Book and note management
- ~2000 lines of C code
- Zero external dependencies except SQLite

---

## Contributing to Roadmap

Have ideas for booknote? Open an issue on GitHub with:
- Feature description
- Use case / problem it solves
- Proposed implementation (optional)
- Priority (nice-to-have vs essential)

Popular requests may be added to the roadmap.

---

Last updated: January 2026
