# booknote GUI Design Document

Design specifications for the booknote graphical user interface.

---

## Vision

A distraction-free reading and note-taking environment optimized for studying technical books. The GUI prioritizes:
- **Focus**: Minimal UI chrome, content first
- **Productivity**: Keyboard-driven workflow
- **Simplicity**: Clean, intuitive interface
- **Performance**: Fast PDF rendering, responsive UI

---

## Technology Stack

**UI Framework:** GTK3 (v3.24+)
- Native Linux toolkit
- C API
- Mature and stable
- Good documentation

**PDF Rendering:** Poppler-glib (v25.03+)
- Fast rendering
- Good API
- Active development

**Database:** SQLite3 (shared with CLI)
- Same database as CLI
- Seamless switching between CLI/GUI

---

## Architecture

### Component Structure
```
src/gui/
├── main.c           # Entry point, GTK initialization
├── window.h/c       # Main window setup
├── booklist.h/c     # Sidebar book list (TreeView)
├── pdfviewer.h/c    # PDF display with Poppler
├── noteeditor.h/c   # Notes panel (TextView)
└── callbacks.h/c    # Event handlers
```

### Module Responsibilities

**main.c:**
- GTK initialization
- Database connection
- Window creation
- Main loop

**window.c:**
- Layout management (GtkPaned for split view)
- Menu bar
- Keyboard shortcuts
- Panel visibility toggling

**booklist.c:**
- Load books from database
- Display as TreeView
- Selection handling
- Book metadata display

**pdfviewer.c:**
- Render PDF pages with Poppler
- Page navigation
- Zoom controls
- Scroll handling

**noteeditor.c:**
- TextView for note editing
- Save notes to database
- Display existing notes
- Quick note creation (Ctrl+N)

---

## UI Layout

### Main Window (3-panel)
```
┌──────────────────────────────────────────────────────────┐
│ ☰ Menu                                      [_][□][X]    │
├──────────┬───────────────────────┬───────────────────────┤
│          │                       │                       │
│ 📚 Books │    PDF Viewer         │    📝 Notes           │
│          │                       │                       │
│ Book 1   │   ┌─────────────┐    │ • Note 1              │
│ Book 2   │   │             │    │ • Note 2              │
│ Book 3   │   │   PDF Page  │    │                       │
│          │   │   Content   │    │ ┌──────────────────┐  │
│ [+ Add]  │   │             │    │ │ Add new note...  │  │
│          │   └─────────────┘    │ └──────────────────┘  │
│          │   ◀ 45/500 ▶ [100%] │                       │
│          │                       │                       │
└──────────┴───────────────────────┴───────────────────────┘
```

**Panel widths (default):**
- Book list: 200px (resizable)
- PDF viewer: 60% of remaining space
- Notes panel: 40% of remaining space

---

## Keyboard Shortcuts

### Navigation
- `Ctrl+O` - Open book
- `Ctrl+W` - Close current book
- `Ctrl+Q` - Quit application
- `PgUp/PgDn` - Next/Previous PDF page
- `Home/End` - First/Last page

### View Control
- `Ctrl+B` - Toggle notes panel
- `Ctrl+L` - Toggle book list
- `F11` - Fullscreen PDF view
- `Escape` - Exit fullscreen
- `Ctrl++` - Zoom in PDF
- `Ctrl+-` - Zoom out PDF
- `Ctrl+0` - Reset zoom (100%)

### Notes
- `Ctrl+N` - New note (at current page)
- `Ctrl+S` - Save current note
- `Ctrl+F` - Search notes
- `Delete` - Delete selected note

### Quick Actions
- `Ctrl+1/2/3` - Switch between panels
- `Ctrl+Tab` - Next book
- `Ctrl+Shift+Tab` - Previous book

---

## Visual Design

### Color Scheme (Adaptive)

**Light Theme:**
- Background: #FFFFFF
- Sidebar: #F5F5F5
- Text: #2E3440
- Accent: #5E81AC
- Selection: #88C0D0

**Dark Theme:** (System preference)
- Background: #2E3440
- Sidebar: #3B4252
- Text: #ECEFF4
- Accent: #88C0D0
- Selection: #5E81AC

### Typography

**Books List:**
- Title: System default, 11pt, bold
- Author: System default, 9pt, italic

**PDF Viewer:**
- Rendered from PDF (no text overlay)

**Notes:**
- Editor: Monospace, 10pt
- Display: System default, 10pt

---

## State Management

### Application State
```c
typedef struct {
    Database *db;              // Database connection
    GtkWidget *window;         // Main window
    GtkWidget *book_list;      // Book list widget
    GtkWidget *pdf_viewer;     // PDF viewer widget
    GtkWidget *note_editor;    // Note editor widget
    
    // Current state
    Book *current_book;        // Currently open book
    PopplerDocument *pdf_doc;  // Loaded PDF
    int current_page;          // Current PDF page
    double zoom_level;         // PDF zoom (1.0 = 100%)
    
    // UI state
    gboolean notes_visible;    // Notes panel visible?
    gboolean booklist_visible; // Book list visible?
    gboolean fullscreen;       // Fullscreen mode?
} AppState;
```

### Persistence

**Settings saved to:** `~/.config/booknote/gui.conf`
```ini
[window]
width=1200
height=800
maximized=false

[panels]
booklist_width=200
notes_width=400
notes_visible=true
booklist_visible=true

[pdf]
zoom=1.0
last_book_id=5
last_page=45
```

---

## Implementation Roadmap

### Phase 1: Basic Structure (v0.2-alpha)
- [x] Install dependencies (GTK3, Poppler)
- [ ] Main window with menu bar
- [ ] 3-panel layout (empty)
- [ ] Window state persistence
- [ ] Keyboard shortcuts framework

### Phase 2: Book List (v0.2-beta)
- [ ] Load books from database
- [ ] Display in TreeView
- [ ] Book selection
- [ ] Add book dialog
- [ ] Delete book (with confirmation)

### Phase 3: PDF Viewer (v0.2-rc1)
- [ ] Load PDF with Poppler
- [ ] Render current page
- [ ] Page navigation
- [ ] Zoom in/out
- [ ] Scroll handling

### Phase 4: Notes Panel (v0.2-rc2)
- [ ] Display notes for current book
- [ ] Add new note
- [ ] Edit existing note
- [ ] Delete note
- [ ] Page number association

### Phase 5: Polish (v0.2 Release)
- [ ] Panel toggle (Ctrl+B, Ctrl+L)
- [ ] Fullscreen mode (F11)
- [ ] Search functionality
- [ ] Settings dialog
- [ ] About dialog
- [ ] Icon and .desktop file

### Phase 6: Advanced (v0.3+)
- [ ] PDF text selection
- [ ] Copy text to note
- [ ] Highlight annotations
- [ ] Note linking to PDF selection
- [ ] PDF bookmarks sidebar
- [ ] Recent books menu
- [ ] Export notes from GUI

---

## Build System

### Makefile Targets
```makefile
# CLI binary (existing)
booknote: ...

# GUI binary (new)
booknote-gui: ...

# Install both
install: booknote booknote-gui
	install -m 755 booknote /usr/local/bin/
	install -m 755 booknote-gui /usr/local/bin/
	install -m 644 booknote.desktop /usr/share/applications/
```

### Dependencies
```makefile
GUI_CFLAGS = $(shell pkg-config --cflags gtk+-3.0 poppler-glib)
GUI_LIBS = $(shell pkg-config --libs gtk+-3.0 poppler-glib)
```

---

## Testing Strategy

### Manual Testing Checklist

**Basic Functionality:**
- [ ] Window opens without errors
- [ ] Books load from database
- [ ] Book selection works
- [ ] PDF loads and displays
- [ ] Page navigation works
- [ ] Notes save to database
- [ ] Search finds notes
- [ ] Keyboard shortcuts work

**Edge Cases:**
- [ ] Large PDFs (500+ pages)
- [ ] Books without PDFs
- [ ] Empty library
- [ ] Corrupted PDFs
- [ ] Very long notes
- [ ] Special characters in filenames

**Performance:**
- [ ] PDF rendering < 100ms per page
- [ ] UI remains responsive during PDF load
- [ ] Database queries < 50ms
- [ ] No memory leaks (valgrind)

---

## Future Considerations

### v0.3+ Features
- PDF text selection and copy
- Highlight annotations in PDF
- Note templates
- Split screen (multiple PDFs)
- Tablet/stylus support
- OCR for scanned PDFs

### v1.0+ Features
- Plugin system
- Custom themes
- Cloud sync UI
- Collaboration features
- Mobile companion app

---

Last updated: January 2025
EOFcat > docs/GUI-DESIGN.md << 'EOF'
# booknote GUI Design Document

Design specifications for the booknote graphical user interface.

---

## Vision

A distraction-free reading and note-taking environment optimized for studying technical books. The GUI prioritizes:
- **Focus**: Minimal UI chrome, content first
- **Productivity**: Keyboard-driven workflow
- **Simplicity**: Clean, intuitive interface
- **Performance**: Fast PDF rendering, responsive UI

---

## Technology Stack

**UI Framework:** GTK3 (v3.24+)
- Native Linux toolkit
- C API
- Mature and stable
- Good documentation

**PDF Rendering:** Poppler-glib (v25.03+)
- Fast rendering
- Good API
- Active development

**Database:** SQLite3 (shared with CLI)
- Same database as CLI
- Seamless switching between CLI/GUI

---

## Architecture

### Component Structure
```
src/gui/
├── main.c           # Entry point, GTK initialization
├── window.h/c       # Main window setup
├── booklist.h/c     # Sidebar book list (TreeView)
├── pdfviewer.h/c    # PDF display with Poppler
├── noteeditor.h/c   # Notes panel (TextView)
└── callbacks.h/c    # Event handlers
```

### Module Responsibilities

**main.c:**
- GTK initialization
- Database connection
- Window creation
- Main loop

**window.c:**
- Layout management (GtkPaned for split view)
- Menu bar
- Keyboard shortcuts
- Panel visibility toggling

**booklist.c:**
- Load books from database
- Display as TreeView
- Selection handling
- Book metadata display

**pdfviewer.c:**
- Render PDF pages with Poppler
- Page navigation
- Zoom controls
- Scroll handling

**noteeditor.c:**
- TextView for note editing
- Save notes to database
- Display existing notes
- Quick note creation (Ctrl+N)

---

## UI Layout

### Main Window (3-panel)
```
┌──────────────────────────────────────────────────────────┐
│ ☰ Menu                                      [_][□][X]    │
├──────────┬───────────────────────┬───────────────────────┤
│          │                       │                       │
│ 📚 Books │    PDF Viewer         │    📝 Notes           │
│          │                       │                       │
│ Book 1   │   ┌─────────────┐    │ • Note 1              │
│ Book 2   │   │             │    │ • Note 2              │
│ Book 3   │   │   PDF Page  │    │                       │
│          │   │   Content   │    │ ┌──────────────────┐  │
│ [+ Add]  │   │             │    │ │ Add new note...  │  │
│          │   └─────────────┘    │ └──────────────────┘  │
│          │   ◀ 45/500 ▶ [100%] │                       │
│          │                       │                       │
└──────────┴───────────────────────┴───────────────────────┘
```

**Panel widths (default):**
- Book list: 200px (resizable)
- PDF viewer: 60% of remaining space
- Notes panel: 40% of remaining space

---

## Keyboard Shortcuts

### Navigation
- `Ctrl+O` - Open book
- `Ctrl+W` - Close current book
- `Ctrl+Q` - Quit application
- `PgUp/PgDn` - Next/Previous PDF page
- `Home/End` - First/Last page

### View Control
- `Ctrl+B` - Toggle notes panel
- `Ctrl+L` - Toggle book list
- `F11` - Fullscreen PDF view
- `Escape` - Exit fullscreen
- `Ctrl++` - Zoom in PDF
- `Ctrl+-` - Zoom out PDF
- `Ctrl+0` - Reset zoom (100%)

### Notes
- `Ctrl+N` - New note (at current page)
- `Ctrl+S` - Save current note
- `Ctrl+F` - Search notes
- `Delete` - Delete selected note

### Quick Actions
- `Ctrl+1/2/3` - Switch between panels
- `Ctrl+Tab` - Next book
- `Ctrl+Shift+Tab` - Previous book

---

## Visual Design

### Color Scheme (Adaptive)

**Light Theme:**
- Background: #FFFFFF
- Sidebar: #F5F5F5
- Text: #2E3440
- Accent: #5E81AC
- Selection: #88C0D0

**Dark Theme:** (System preference)
- Background: #2E3440
- Sidebar: #3B4252
- Text: #ECEFF4
- Accent: #88C0D0
- Selection: #5E81AC

### Typography

**Books List:**
- Title: System default, 11pt, bold
- Author: System default, 9pt, italic

**PDF Viewer:**
- Rendered from PDF (no text overlay)

**Notes:**
- Editor: Monospace, 10pt
- Display: System default, 10pt

---

## State Management

### Application State
```c
typedef struct {
    Database *db;              // Database connection
    GtkWidget *window;         // Main window
    GtkWidget *book_list;      // Book list widget
    GtkWidget *pdf_viewer;     // PDF viewer widget
    GtkWidget *note_editor;    // Note editor widget
    
    // Current state
    Book *current_book;        // Currently open book
    PopplerDocument *pdf_doc;  // Loaded PDF
    int current_page;          // Current PDF page
    double zoom_level;         // PDF zoom (1.0 = 100%)
    
    // UI state
    gboolean notes_visible;    // Notes panel visible?
    gboolean booklist_visible; // Book list visible?
    gboolean fullscreen;       // Fullscreen mode?
} AppState;
```

### Persistence

**Settings saved to:** `~/.config/booknote/gui.conf`
```ini
[window]
width=1200
height=800
maximized=false

[panels]
booklist_width=200
notes_width=400
notes_visible=true
booklist_visible=true

[pdf]
zoom=1.0
last_book_id=5
last_page=45
```

---

## Implementation Roadmap

### Phase 1: Basic Structure (v0.2-alpha)
- [x] Install dependencies (GTK3, Poppler)
- [ ] Main window with menu bar
- [ ] 3-panel layout (empty)
- [ ] Window state persistence
- [ ] Keyboard shortcuts framework

### Phase 2: Book List (v0.2-beta)
- [ ] Load books from database
- [ ] Display in TreeView
- [ ] Book selection
- [ ] Add book dialog
- [ ] Delete book (with confirmation)

### Phase 3: PDF Viewer (v0.2-rc1)
- [ ] Load PDF with Poppler
- [ ] Render current page
- [ ] Page navigation
- [ ] Zoom in/out
- [ ] Scroll handling

### Phase 4: Notes Panel (v0.2-rc2)
- [ ] Display notes for current book
- [ ] Add new note
- [ ] Edit existing note
- [ ] Delete note
- [ ] Page number association

### Phase 5: Polish (v0.2 Release)
- [ ] Panel toggle (Ctrl+B, Ctrl+L)
- [ ] Fullscreen mode (F11)
- [ ] Search functionality
- [ ] Settings dialog
- [ ] About dialog
- [ ] Icon and .desktop file

### Phase 6: Advanced (v0.3+)
- [ ] PDF text selection
- [ ] Copy text to note
- [ ] Highlight annotations
- [ ] Note linking to PDF selection
- [ ] PDF bookmarks sidebar
- [ ] Recent books menu
- [ ] Export notes from GUI

---

## Build System

### Makefile Targets
```makefile
# CLI binary (existing)
booknote: ...

# GUI binary (new)
booknote-gui: ...

# Install both
install: booknote booknote-gui
	install -m 755 booknote /usr/local/bin/
	install -m 755 booknote-gui /usr/local/bin/
	install -m 644 booknote.desktop /usr/share/applications/
```

### Dependencies
```makefile
GUI_CFLAGS = $(shell pkg-config --cflags gtk+-3.0 poppler-glib)
GUI_LIBS = $(shell pkg-config --libs gtk+-3.0 poppler-glib)
```

---

## Testing Strategy

### Manual Testing Checklist

**Basic Functionality:**
- [ ] Window opens without errors
- [ ] Books load from database
- [ ] Book selection works
- [ ] PDF loads and displays
- [ ] Page navigation works
- [ ] Notes save to database
- [ ] Search finds notes
- [ ] Keyboard shortcuts work

**Edge Cases:**
- [ ] Large PDFs (500+ pages)
- [ ] Books without PDFs
- [ ] Empty library
- [ ] Corrupted PDFs
- [ ] Very long notes
- [ ] Special characters in filenames

**Performance:**
- [ ] PDF rendering < 100ms per page
- [ ] UI remains responsive during PDF load
- [ ] Database queries < 50ms
- [ ] No memory leaks (valgrind)

---

## Future Considerations

### v0.3+ Features
- PDF text selection and copy
- Highlight annotations in PDF
- Note templates
- Split screen (multiple PDFs)
- Tablet/stylus support
- OCR for scanned PDFs

### v1.0+ Features
- Plugin system
- Custom themes
- Cloud sync UI
- Collaboration features
- Mobile companion app

---

Last updated: January 2025
