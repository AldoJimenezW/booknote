# booknote

Note-taking for technical books. Simple, fast, local-first.

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Status](https://img.shields.io/badge/status-in%20development-orange)]()

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

## Current Status

This project is in active development. The first usable release (v0.1) is targeted for February 2025.

**Progress:**
- [x] Repository structure
- [ ] Core architecture design
- [ ] Database schema
- [ ] CLI commands
- [ ] ISBN metadata integration
- [ ] Note management system
- [ ] Export functionality

---

## Planned Features

### Core functionality
- Add books to your library (PDF files)
- Create structured notes per book
- Search across all your notes
- Automatic metadata from ISBN
- Export to Markdown, Org-mode, or plain text

### Technical details
- Written in C11 for performance and portability
- SQLite for local storage
- Full-text search with FTS5
- Minimal dependencies
- Cross-platform (Linux first, macOS/BSD support planned)

---

## Roadmap

**v0.1 - Minimum Viable Product**
- Basic book management (add, list, remove)
- Note creation and retrieval
- ISBN metadata lookup via OpenLibrary API
- Simple keyword search
- Markdown export

**v0.2 - Enhanced**
- PDF text extraction
- Page-level note anchoring
- Improved search (fuzzy matching, filters)
- Multiple export formats

**v1.0 - Full Release**
- Terminal UI (ncurses)
- Tags and categories
- Backup and restore
- Comprehensive documentation

See [ROADMAP.md](docs/ROADMAP.md) for detailed feature planning.

---

## Why Contribute?

This project needs help. If you care about:
- Open-source tools for learning and research
- Clean C code and systems programming
- Building practical CLI applications
- Privacy-focused software

Then this might interest you.

**What we need:**
- Code review and feedback on architecture decisions
- Testing on different platforms (especially macOS/BSD)
- Documentation improvements
- Feature ideas from real workflows
- Bug reports once we have a testable version

**What you'll learn:**
- Working with SQLite in C
- Building user-friendly CLI tools
- HTTP APIs and JSON parsing in C
- Full-text search implementation
- Open-source project maintenance

No contribution is too small. Even fixing typos or improving error messages helps.

---

## Getting Involved

The project is in early stages. Here's how you can help:

1. **Star the repository** - Shows interest and helps visibility
2. **Open issues** - Share feature ideas or report problems
3. **Discuss design** - Comment on architectural decisions
4. **Write code** - Contributions welcome once v0.1 foundation is complete
5. **Spread the word** - Tell others who might benefit

Formal contribution guidelines will be added as the project matures.

---

## Technical Stack

- **Language:** C11 (ISO/IEC 9899:2011)
- **Database:** SQLite3
- **HTTP client:** libcurl
- **JSON parser:** json-c
- **Build system:** Make (CMake support planned)
- **Testing:** Criterion framework (planned)

---

## License

GNU General Public License v3.0 - see [LICENSE](LICENSE) for details.

This means you can use, modify, and distribute this software, but any derivative work must also be open-source under GPL-3.0.

---

## Author

Aldo Jiménez Wiehoff

- GitHub: [@AldoJimenezW](https://github.com/AldoJimenezW)
- Email: aldo@jimenezwiehoff.lat
- LinkedIn: [aldo-jimenez-wiehoff](https://linkedin.com/in/aldo-jimenez-wiehoff)

---

## Acknowledgments

Inspired by the limitations of existing tools like Obsidian (poor PDF support), Zotero (academic-focused), and Calibre (library management, not note-taking).

Built for developers and researchers who want better workflows for technical reading.
