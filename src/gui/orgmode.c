#include "orgmode.h"
#include <string.h>
#include <stdlib.h>
#include <pango/pango.h>

// Color palette (Dark Academia)
#define COLOR_TEXT           "#e0e0e0"
#define COLOR_ACCENT         "#d4af37"
#define COLOR_LINK           "#5ea0ff"
#define COLOR_TODO           "#ff4d4f"
#define COLOR_DONE           "#4caf50"
#define COLOR_CODE_BG        "#2a2a2a"

// Debounce default
#define DEFAULT_DEBOUNCE_MS 180

// Forward declarations
static void create_tags(OrgModeEditor *editor);
static gboolean debounce_timeout_cb(gpointer data);
static void buffer_changed_cb(GtkTextBuffer *buffer, gpointer user_data);
static void clear_all_tags(OrgModeEditor *editor);
static void apply_block_level_tags(OrgModeEditor *editor,
                                   const char *line_text,
                                   GtkTextIter *line_start,
                                   GtkTextIter *line_end);
static void apply_inline_tags(OrgModeEditor *editor,
                              const char *line_text,
                              GtkTextIter *line_start);

/**
 * Utility: Apply a tag to a character range given absolute character offsets
 */
static void apply_tag_range(GtkTextBuffer *buffer, GtkTextTag *tag,
                            gint start_offset, gint end_offset) {
    if (end_offset <= start_offset) return;
    GtkTextIter start_iter;
    GtkTextIter end_iter;
    gtk_text_buffer_get_iter_at_offset(buffer, &start_iter, start_offset);
    gtk_text_buffer_get_iter_at_offset(buffer, &end_iter, end_offset);
    gtk_text_buffer_apply_tag(buffer, tag, &start_iter, &end_iter);
}

/**
 * Utility: Convert byte index in line_text to character index safely (UTF-8)
 */
static gint line_byte_to_char_index(const char *line_text, gint byte_index) {
    if (byte_index < 0) return 0;
    const char *start = line_text;
    const char *pos = line_text + byte_index;
    if (pos < start) return 0;
    // Clamp if beyond end
    if ((gsize)byte_index > strlen(line_text)) {
        pos = line_text + strlen(line_text);
    }
    return g_utf8_pointer_to_offset(start, pos);
}

/**
 * Public API implementations
 */

OrgModeEditor* orgmode_editor_create(void) {
    OrgModeEditor *editor = g_malloc0(sizeof(OrgModeEditor));
    if (!editor) return NULL;

    // Container and text view
    editor->container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    editor->text_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(editor->text_view), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(editor->text_view), FALSE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(editor->text_view), 8);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(editor->text_view), 8);
    gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(editor->text_view), 2);
    gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(editor->text_view), 2);

    // Buffer
    editor->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor->text_view));

    // Scrolled window wrapping the text view
    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroller), editor->text_view);
    gtk_box_pack_start(GTK_BOX(editor->container), scroller, TRUE, TRUE, 0);

    // Create formatting tags
    create_tags(editor);

    // Initialize debounce state
    editor->debounce_source_id = 0;
    editor->last_change_serial = 0;

    // Enable live formatting with default delay
    orgmode_editor_enable_live_formatting(editor, DEFAULT_DEBOUNCE_MS);

    return editor;
}

void orgmode_editor_destroy(OrgModeEditor *editor) {
    if (!editor) return;
    orgmode_editor_disable_live_formatting(editor);
    // Widgets will be destroyed by GTK container ownership; free struct
    g_free(editor);
}

void orgmode_editor_update_formatting(OrgModeEditor *editor) {
    if (!editor || !editor->buffer) return;

    // Clear old tags
    clear_all_tags(editor);

    // Iterate line by line using GtkTextIter to get accurate character offsets
    GtkTextIter iter;
    gtk_text_buffer_get_start_iter(editor->buffer, &iter);

    while (!gtk_text_iter_is_end(&iter)) {
        GtkTextIter line_start = iter;
        GtkTextIter line_end = iter;
        gtk_text_iter_forward_to_line_end(&line_end);

        // Extract line text
        char *line_text = gtk_text_iter_get_text(&line_start, &line_end);

        // Apply block level tags (headers, bullets, TODO/DONE)
        apply_block_level_tags(editor, line_text, &line_start, &line_end);

        // Apply inline tags (*bold*, /italic/, etc.)
        apply_inline_tags(editor, line_text, &line_start);

        g_free(line_text);

        // Advance to next line (skip newline)
        if (!gtk_text_iter_is_end(&line_end)) {
            gtk_text_iter_forward_char(&line_end);
        }
        iter = line_end;
    }
}

gboolean parse_org_line(const char *line,
                        int *level_out,
                        gboolean *is_bullet_out,
                        gboolean *is_numbered_out,
                        gboolean *is_todo_out,
                        gboolean *is_done_out) {
    if (!line) return FALSE;

    // Initialize outputs
    if (level_out) *level_out = 0;
    if (is_bullet_out) *is_bullet_out = FALSE;
    if (is_numbered_out) *is_numbered_out = FALSE;
    if (is_todo_out) *is_todo_out = FALSE;
    if (is_done_out) *is_done_out = FALSE;

    gboolean detected = FALSE;

    // Trim leading spaces
    const char *p = line;
    while (*p == ' ' || *p == '\t') p++;

    // Headers: *, **, ***
    if (p[0] == '*') {
        int stars = 0;
        while (p[stars] == '*') stars++;
        if (stars >= 1 && stars <= 3 && (p[stars] == ' ' || p[stars] == '\t')) {
            if (level_out) *level_out = stars;
            detected = TRUE;
        }
    }

    // TODO/DONE near start
    if (g_str_has_prefix(p, "TODO ")) {
        if (is_todo_out) *is_todo_out = TRUE;
        detected = TRUE;
    } else if (g_str_has_prefix(p, "DONE ")) {
        if (is_done_out) *is_done_out = TRUE;
        detected = TRUE;
    }

    // Bullet list: "- ", "+ ", "* "
    if ((p[0] == '-' || p[0] == '+' || p[0] == '*') && p[1] == ' ') {
        if (is_bullet_out) *is_bullet_out = TRUE;
        detected = TRUE;
    }

    // Numbered list: "1. ", "2) "
    if (g_ascii_isdigit(p[0])) {
        int i = 0;
        while (g_ascii_isdigit(p[i])) i++;
        if ((p[i] == '.' || p[i] == ')') && p[i+1] == ' ') {
            if (is_numbered_out) *is_numbered_out = TRUE;
            detected = TRUE;
        }
    }

    return detected;
}

void parse_inline_formatting(const char *line,
                             gint line_offset,
                             GtkTextBuffer *buffer,
                             OrgModeEditor *tags) {
    if (!line || !buffer || !tags) return;

    gint base_char_offset = line_offset;

    // C helper to find paired markers: returns TRUE and sets byte indices on success
    gboolean find_pair_in_line(const char *s, char marker, gint start_from_bytes, gint *out_start_b, gint *out_end_b) {
        const char *start = s + start_from_bytes;
        const char *open = strchr(start, marker);
        if (!open) return FALSE;
        const char *close = strchr(open + 1, marker);
        if (!close) return FALSE;
        *out_start_b = (gint)(open - s);
        *out_end_b = (gint)(close - s);
        return TRUE;
    }

    // Scan through the line for multiple pairs, advancing after each match
    gint cursor_b = 0;
    while (line[cursor_b] != '\0') {
        gboolean matched = FALSE;

        // Bold: *text*
        {
            gint sb, eb;
            if (find_pair_in_line(line, '*', cursor_b, &sb, &eb)) {
                gint start_char = line_byte_to_char_index(line, sb);
                gint end_char = line_byte_to_char_index(line, eb);
                // Apply tag for inside content only
                apply_tag_range(buffer, tags->tag_bold,
                                base_char_offset + start_char + 1,
                                base_char_offset + end_char);
                cursor_b = eb + 1;
                matched = TRUE;
            }
        }
        if (matched) continue;

        // Italic: /text/
        {
            gint sb, eb;
            if (find_pair_in_line(line, '/', cursor_b, &sb, &eb)) {
                gint start_char = line_byte_to_char_index(line, sb);
                gint end_char = line_byte_to_char_index(line, eb);
                apply_tag_range(buffer, tags->tag_italic,
                                base_char_offset + start_char + 1,
                                base_char_offset + end_char);
                cursor_b = eb + 1;
                matched = TRUE;
            }
        }
        if (matched) continue;

        // Underline: _text_
        {
            gint sb, eb;
            if (find_pair_in_line(line, '_', cursor_b, &sb, &eb)) {
                gint start_char = line_byte_to_char_index(line, sb);
                gint end_char = line_byte_to_char_index(line, eb);
                apply_tag_range(buffer, tags->tag_underline,
                                base_char_offset + start_char + 1,
                                base_char_offset + end_char);
                cursor_b = eb + 1;
                matched = TRUE;
            }
        }
        if (matched) continue;

        // Code: =text=
        {
            gint sb, eb;
            if (find_pair_in_line(line, '=', cursor_b, &sb, &eb)) {
                gint start_char = line_byte_to_char_index(line, sb);
                gint end_char = line_byte_to_char_index(line, eb);
                apply_tag_range(buffer, tags->tag_code,
                                base_char_offset + start_char + 1,
                                base_char_offset + end_char);
                cursor_b = eb + 1;
                matched = TRUE;
            }
        }
        if (matched) continue;

        // Verbatim: ~text~
        {
            gint sb, eb;
            if (find_pair_in_line(line, '~', cursor_b, &sb, &eb)) {
                gint start_char = line_byte_to_char_index(line, sb);
                gint end_char = line_byte_to_char_index(line, eb);
                apply_tag_range(buffer, tags->tag_code,
                                base_char_offset + start_char + 1,
                                base_char_offset + end_char);
                cursor_b = eb + 1;
                matched = TRUE;
            }
        }
        if (matched) continue;

        // Links: [[url][text]]
        if (line[cursor_b] == '[' && line[cursor_b+1] == '[') {
            const char *start = line + cursor_b;
            const char *mid = strstr(start, "][");
            const char *end = strstr(start, "]]");
            if (mid && end && mid < end) {
                gint url_start_b = cursor_b + 2; // after [[
                gint url_end_b = (gint)(mid - line) - 1;
                gint text_start_b = (gint)(mid - line) + 2;
                gint text_end_b = (gint)(end - line) - 1;

                // Apply link tag to text portion
                gint text_start_char = line_byte_to_char_index(line, text_start_b);
                gint text_end_char = line_byte_to_char_index(line, text_end_b + 1);
                apply_tag_range(buffer, tags->tag_link,
                                base_char_offset + text_start_char,
                                base_char_offset + text_end_char);

                cursor_b = (gint)(end - line) + 2; // after ]]
                matched = TRUE;
            }
        }
        if (matched) continue;

        // Advance
        cursor_b++;
    }
}

void orgmode_editor_enable_live_formatting(OrgModeEditor *editor, guint delay_ms) {
    if (!editor || !editor->buffer) return;

    // Connect changed signal
    g_signal_connect(editor->buffer, "changed", G_CALLBACK(buffer_changed_cb), editor);

    // Set default debounce if zero
    if (delay_ms == 0) delay_ms = DEFAULT_DEBOUNCE_MS;

    // Store debounce delay by scheduling the first pass; further changes will reset it
    editor->debounce_source_id = g_timeout_add(delay_ms, debounce_timeout_cb, editor);
}

void orgmode_editor_disable_live_formatting(OrgModeEditor *editor) {
    if (!editor) return;

    // Disconnect signal handlers of type "changed"
    // Note: A robust approach would store the handler ID; for simplicity, rely on GTK to clean up on destroy.
    if (editor->debounce_source_id != 0) {
        g_source_remove(editor->debounce_source_id);
        editor->debounce_source_id = 0;
    }
}

/**
 * Internal implementations
 */

static void create_tags(OrgModeEditor *editor) {
    GtkTextBuffer *buffer = editor->buffer;
    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);

    // Header tags
    editor->tag_header1 = gtk_text_tag_new("header1");
    g_object_set(editor->tag_header1,
                 "weight", PANGO_WEIGHT_BOLD,
                 "foreground", COLOR_ACCENT,
                 "size-points", 16.0,
                 NULL);

    editor->tag_header2 = gtk_text_tag_new("header2");
    g_object_set(editor->tag_header2,
                 "weight", PANGO_WEIGHT_BOLD,
                 "foreground", COLOR_ACCENT,
                 "size-points", 14.0,
                 NULL);

    editor->tag_header3 = gtk_text_tag_new("header3");
    g_object_set(editor->tag_header3,
                 "weight", PANGO_WEIGHT_BOLD,
                 "foreground", COLOR_ACCENT,
                 "size-points", 13.0,
                 NULL);

    // Inline tags
    editor->tag_bold = gtk_text_tag_new("bold");
    g_object_set(editor->tag_bold,
                 "weight", PANGO_WEIGHT_BOLD,
                 NULL);

    editor->tag_italic = gtk_text_tag_new("italic");
    g_object_set(editor->tag_italic,
                 "style", PANGO_STYLE_ITALIC,
                 NULL);

    editor->tag_underline = gtk_text_tag_new("underline");
    g_object_set(editor->tag_underline,
                 "underline", PANGO_UNDERLINE_SINGLE,
                 NULL);

    editor->tag_code = gtk_text_tag_new("code");
    g_object_set(editor->tag_code,
                 "family", "Monospace",
                 "background", COLOR_CODE_BG,
                 NULL);

    editor->tag_link = gtk_text_tag_new("link");
    g_object_set(editor->tag_link,
                 "foreground", COLOR_LINK,
                 "underline", PANGO_UNDERLINE_SINGLE,
                 NULL);

    editor->tag_bullet = gtk_text_tag_new("bullet");
    g_object_set(editor->tag_bullet,
                 "foreground", COLOR_ACCENT,
                 NULL);

    editor->tag_todo = gtk_text_tag_new("todo");
    g_object_set(editor->tag_todo,
                 "foreground", COLOR_TODO,
                 "weight", PANGO_WEIGHT_BOLD,
                 NULL);

    editor->tag_done = gtk_text_tag_new("done");
    g_object_set(editor->tag_done,
                 "foreground", COLOR_DONE,
                 "strikethrough", TRUE,
                 NULL);

    // Add tags to table
    gtk_text_tag_table_add(table, editor->tag_header1);
    gtk_text_tag_table_add(table, editor->tag_header2);
    gtk_text_tag_table_add(table, editor->tag_header3);
    gtk_text_tag_table_add(table, editor->tag_bold);
    gtk_text_tag_table_add(table, editor->tag_italic);
    gtk_text_tag_table_add(table, editor->tag_underline);
    gtk_text_tag_table_add(table, editor->tag_code);
    gtk_text_tag_table_add(table, editor->tag_link);
    gtk_text_tag_table_add(table, editor->tag_bullet);
    gtk_text_tag_table_add(table, editor->tag_todo);
    gtk_text_tag_table_add(table, editor->tag_done);
}

static gboolean debounce_timeout_cb(gpointer data) {
    OrgModeEditor *editor = (OrgModeEditor *)data;
    if (!editor) return G_SOURCE_REMOVE;

    // Perform formatting update
    orgmode_editor_update_formatting(editor);

    // Reset debounce id
    editor->debounce_source_id = 0;

    return G_SOURCE_REMOVE; // one-shot timeout
}

static void buffer_changed_cb(GtkTextBuffer *buffer, gpointer user_data) {
    OrgModeEditor *editor = (OrgModeEditor *)user_data;
    if (!editor) return;

    // Cancel previous debounce if pending
    if (editor->debounce_source_id != 0) {
        g_source_remove(editor->debounce_source_id);
        editor->debounce_source_id = 0;
    }

    // Schedule a new debounce timeout
    editor->debounce_source_id = g_timeout_add(DEFAULT_DEBOUNCE_MS, debounce_timeout_cb, editor);
}

static void clear_all_tags(OrgModeEditor *editor) {
    GtkTextBuffer *buffer = editor->buffer;
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);

    gtk_text_buffer_remove_tag(buffer, editor->tag_header1, &start, &end);
    gtk_text_buffer_remove_tag(buffer, editor->tag_header2, &start, &end);
    gtk_text_buffer_remove_tag(buffer, editor->tag_header3, &start, &end);
    gtk_text_buffer_remove_tag(buffer, editor->tag_bold, &start, &end);
    gtk_text_buffer_remove_tag(buffer, editor->tag_italic, &start, &end);
    gtk_text_buffer_remove_tag(buffer, editor->tag_underline, &start, &end);
    gtk_text_buffer_remove_tag(buffer, editor->tag_code, &start, &end);
    gtk_text_buffer_remove_tag(buffer, editor->tag_link, &start, &end);
    gtk_text_buffer_remove_tag(buffer, editor->tag_bullet, &start, &end);
    gtk_text_buffer_remove_tag(buffer, editor->tag_todo, &start, &end);
    gtk_text_buffer_remove_tag(buffer, editor->tag_done, &start, &end);
}

static void apply_block_level_tags(OrgModeEditor *editor,
                                   const char *line_text,
                                   GtkTextIter *line_start,
                                   GtkTextIter *line_end) {
    if (!line_text || !line_start || !line_end) return;

    int level = 0;
    gboolean is_bullet = FALSE, is_numbered = FALSE, is_todo = FALSE, is_done = FALSE;
    gboolean any = parse_org_line(line_text, &level, &is_bullet, &is_numbered, &is_todo, &is_done);
    if (!any) return;

    GtkTextBuffer *buffer = editor->buffer;
    gint start_offset = gtk_text_iter_get_offset(line_start);
    gint end_offset = gtk_text_iter_get_offset(line_end);

    // Apply header tags: apply to entire line excluding leading stars
    if (level >= 1 && level <= 3) {
        // Compute where header text starts: after stars and following space
        const char *p = line_text;
        while (*p == ' ' || *p == '\t') p++;
        int stars = 0;
        while (p[stars] == '*') stars++;
        int title_byte_index = stars;
        // Skip the separating space after stars
        if (p[title_byte_index] == ' ' || p[title_byte_index] == '\t') title_byte_index++;

        gint title_char_index = line_byte_to_char_index(p, title_byte_index);
        GtkTextTag *tag = (level == 1) ? editor->tag_header1 :
                          (level == 2) ? editor->tag_header2 : editor->tag_header3;

        apply_tag_range(buffer, tag,
                        start_offset + (gtk_text_iter_get_char(line_start) ? 0 : 0) + title_char_index,
                        end_offset);
    }

    // Bullets and numbered lists: accent the marker
    if (is_bullet || is_numbered) {
        const char *p = line_text;
        while (*p == ' ' || *p == '\t') p++;

        gint marker_len_chars = 0;
        if (is_bullet) {
            // "- " or "+ " or "* "
            marker_len_chars = 2;
        } else {
            // Numbered e.g., "12. "
            int i = 0;
            while (g_ascii_isdigit(p[i])) i++;
            if (p[i] == '.' || p[i] == ')') i++;
            if (p[i] == ' ') i++;
            marker_len_chars = g_utf8_strlen(p, i);
        }

        apply_tag_range(buffer, editor->tag_bullet,
                        start_offset,
                        start_offset + marker_len_chars);
    }

    // TODO / DONE markers: apply tags to those words near start
    if (is_todo || is_done) {
        const char *p = line_text;
        while (*p == ' ' || *p == '\t') p++;

        const char *kw = is_todo ? "TODO" : "DONE";
        size_t kwlen = strlen(kw);

        gint kw_start_char = g_utf8_pointer_to_offset(line_text, p);
        gint kw_end_char = kw_start_char + (gint)g_utf8_strlen(kw, -1);

        GtkTextTag *tag = is_todo ? editor->tag_todo : editor->tag_done;
        apply_tag_range(buffer, tag, start_offset + kw_start_char, start_offset + kw_end_char);
    }
}

static void apply_inline_tags(OrgModeEditor *editor,
                              const char *line_text,
                              GtkTextIter *line_start) {
    if (!line_text || !line_start) return;

    gint base_char_offset = gtk_text_iter_get_offset(line_start);

    // Delegate to parse_inline_formatting for detailed scanning
    parse_inline_formatting(line_text, base_char_offset, editor->buffer, editor);
}