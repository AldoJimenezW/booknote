#ifndef BOOKNOTE_ORGMODE_H
#define BOOKNOTE_ORGMODE_H

#include <gtk/gtk.h>

/**
 * OrgModeEditor - Rich text editor for Org-mode syntax with live rendering
 *
 * This widget wraps a GtkTextView and manages GtkTextTags to render Org-mode
 * syntax in real-time as the user types. A debounced update is scheduled
 * using GLib timeouts to avoid lag on each keystroke.
 */
typedef struct {
    // Public container to embed in layouts
    GtkWidget *container;

    // Internal editor components
    GtkWidget *text_view;
    GtkTextBuffer *buffer;

    // Text tags for formatting
    GtkTextTag *tag_header1;
    GtkTextTag *tag_header2;
    GtkTextTag *tag_header3;
    GtkTextTag *tag_bold;
    GtkTextTag *tag_italic;
    GtkTextTag *tag_underline;
    GtkTextTag *tag_code;
    GtkTextTag *tag_link;
    GtkTextTag *tag_bullet;
    GtkTextTag *tag_todo;
    GtkTextTag *tag_done;

    // Debounce source id for scheduled formatting updates
    guint debounce_source_id;

    // Optional: last known version stamp to avoid redundant passes
    guint64 last_change_serial;
} OrgModeEditor;

/**
 * Create a new OrgModeEditor.
 *
 * Returns:
 * - A newly allocated OrgModeEditor instance with a ready-to-use container.
 *
 * The caller should pack `editor->container` into the UI and manage its lifetime
 * by calling `orgmode_editor_destroy()` when done.
 */
OrgModeEditor* orgmode_editor_create(void);

/**
 * Destroy the OrgModeEditor and free resources.
 */
void orgmode_editor_destroy(OrgModeEditor *editor);

/**
 * Update formatting for the entire buffer.
 *
 * This function parses the buffer content and applies GtkTextTags to render
 * Org-mode syntax. It is intended to be called from a debounced timeout or
 * after bulk operations. It will clear existing tag ranges and re-apply them.
 */
void orgmode_editor_update_formatting(OrgModeEditor *editor);

/**
 * Parse a single line for Org-mode block-level syntax.
 *
 * Parameters:
 * - line: a NUL-terminated string of the line content.
 * - level_out: optional, receives header level (1..3) if a header is detected, 0 otherwise.
 * - is_bullet_out: optional, TRUE if the line is a bullet list item.
 * - is_numbered_out: optional, TRUE if the line is a numbered list item.
 * - is_todo_out: optional, TRUE if the line starts with a TODO item.
 * - is_done_out: optional, TRUE if the line starts with a DONE item.
 *
 * Returns:
 * - TRUE if any block-level syntax was detected, FALSE otherwise.
 *
 * Notes:
 * - Supported headers: "*", "**", "***" at the start of the line.
 * - Bullets: "-", "+", "* " at the start of the line (space after marker).
 * - Numbered: "1. ", "2) " patterns.
 * - TODO/DONE: "TODO " / "DONE " markers near the start of the line.
 */
gboolean parse_org_line(const char *line,
                        int *level_out,
                        gboolean *is_bullet_out,
                        gboolean *is_numbered_out,
                        gboolean *is_todo_out,
                        gboolean *is_done_out);

/**
 * Parse inline formatting markers within a line and apply tags.
 *
 * This function scans a line to find:
 * - *bold*
 * - /italic/
 * - _underline_
 * - =code=
 * - ~verbatim~
 * - [[link][text]] (applies link tag to 'text', optional handling for URL)
 *
 * It should be used by `orgmode_editor_update_formatting()` to apply tags
 * to specific ranges. The function does not modify the buffer directly; it
 * reports the ranges found so the caller can apply tags.
 *
 * Parameters:
 * - line: the NUL-terminated line content
 * - line_offset: the start offset of the line in the buffer (byte index)
 * - buffer: the GtkTextBuffer to apply tags to
 * - tags: the editor struct (to access tag instances)
 *
 * Implementation note:
 * - The function will iterate over the line and detect paired markers,
 *   validating that pairs are properly closed and non-overlapping. Invalid
 *   or unclosed markers are ignored gracefully.
 */
void parse_inline_formatting(const char *line,
                             gint line_offset,
                             GtkTextBuffer *buffer,
                             OrgModeEditor *tags);

/**
 * Enable debounced live formatting.
 *
 * Registers a "changed" signal handler on the buffer to schedule a deferred
 * call to `orgmode_editor_update_formatting()` using g_timeout_add(). This
 * avoids applying formatting on every keystroke.
 *
 * Parameters:
 * - editor: the editor instance
 * - delay_ms: debounce delay in milliseconds (recommended: 120-250ms)
 */
void orgmode_editor_enable_live_formatting(OrgModeEditor *editor, guint delay_ms);

/**
 * Disable live formatting and cancel any pending debounce timers.
 */
void orgmode_editor_disable_live_formatting(OrgModeEditor *editor);

#endif // BOOKNOTE_ORGMODE_H