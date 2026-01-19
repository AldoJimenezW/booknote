#include "notesview.h"
#include "../database/queries.h"
#include <stdio.h>
#include <string.h>

static void on_note_selected(GtkTreeView *view, gpointer data);
static void on_save_clicked(GtkWidget *widget, gpointer data);
static void on_delete_clicked(GtkWidget *widget, gpointer data);
static void on_new_note_clicked(GtkWidget *widget, gpointer data);

NotesPanel* notespanel_create(Database *db) {
    NotesPanel *panel = calloc(1, sizeof(NotesPanel));
    if (!panel) return NULL;
    
    panel->db = db;
    panel->current_book_id = -1;
    panel->current_note_id = -1;
    
    // Main container (vertical split)
    panel->container = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_paned_set_position(GTK_PANED(panel->container), 200);
    
    // Top section: Notes list
    GtkWidget *top_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    
    GtkWidget *header = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header), "<b>Notes</b>");
    gtk_box_pack_start(GTK_BOX(top_box), header, FALSE, FALSE, 5);
    
    // Scrolled window for list
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    // TreeView for notes
    panel->notes_list = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(panel->notes_list), TRUE);
    
    // Columns
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    
    GtkTreeViewColumn *col_title = gtk_tree_view_column_new_with_attributes(
        "Title", renderer, "text", NOTE_COL_TITLE, NULL);
    gtk_tree_view_column_set_expand(col_title, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(panel->notes_list), col_title);
    
    GtkTreeViewColumn *col_page = gtk_tree_view_column_new_with_attributes(
        "Page", renderer, "text", NOTE_COL_PAGE, NULL);
    gtk_tree_view_column_set_min_width(col_page, 60);
    gtk_tree_view_append_column(GTK_TREE_VIEW(panel->notes_list), col_page);
    
    // Connect selection signal
    GtkTreeSelection *selection = gtk_tree_view_get_selection(
        GTK_TREE_VIEW(panel->notes_list));
    g_signal_connect(selection, "changed", G_CALLBACK(on_note_selected), panel);
    
    gtk_container_add(GTK_CONTAINER(scroll), panel->notes_list);
    gtk_box_pack_start(GTK_BOX(top_box), scroll, TRUE, TRUE, 0);
    
    gtk_paned_pack1(GTK_PANED(panel->container), top_box, FALSE, TRUE);
    
    // Bottom section: Editor
    GtkWidget *bottom_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    
    GtkWidget *editor_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(editor_label), "<b>Content</b>");
    gtk_box_pack_start(GTK_BOX(bottom_box), editor_label, FALSE, FALSE, 5);
    
    // Markdown editor (GtkTextView inside a scrolled window) with basic settings
    GtkWidget *md_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(md_scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GtkWidget *md_textview = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(md_textview), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(md_textview), 8);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(md_textview), 8);
    PangoFontDescription *mono = pango_font_description_from_string("Monospace 11");
    GtkStyleContext *ctx = gtk_widget_get_style_context(md_textview);
    gtk_style_context_add_class(ctx, "markdown-textview");
    pango_font_description_free(mono);
    gtk_container_add(GTK_CONTAINER(md_scroll), md_textview);

    panel->editor = md_scroll;
    // Store the Markdown editor pointer for later access (preview/edit toggles can use this)
    g_object_set_data(G_OBJECT(panel->editor), "markdown_textview", md_textview);
    gtk_box_pack_start(GTK_BOX(bottom_box), panel->editor, TRUE, TRUE, 0);
    
    // Button box
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_margin_start(button_box, 5);
    gtk_widget_set_margin_end(button_box, 5);
    gtk_widget_set_margin_bottom(button_box, 5);
    
    panel->save_button = gtk_button_new_with_label("Save Changes");
    gtk_widget_set_sensitive(panel->save_button, FALSE);
    g_signal_connect(panel->save_button, "clicked", G_CALLBACK(on_save_clicked), panel);
    gtk_box_pack_start(GTK_BOX(button_box), panel->save_button, TRUE, TRUE, 0);
    
    panel->delete_button = gtk_button_new_with_label("Delete Note");
    gtk_widget_set_sensitive(panel->delete_button, FALSE);
    g_signal_connect(panel->delete_button, "clicked", G_CALLBACK(on_delete_clicked), panel);
    gtk_box_pack_start(GTK_BOX(button_box), panel->delete_button, TRUE, TRUE, 0);
    
    GtkWidget *new_button = gtk_button_new_with_label("+ New Note");
    gtk_box_pack_start(GTK_BOX(button_box), new_button, TRUE, TRUE, 0);
    g_signal_connect(new_button, "clicked", G_CALLBACK(on_new_note_clicked), panel);
    
    gtk_box_pack_start(GTK_BOX(bottom_box), button_box, FALSE, FALSE, 0);
    
    gtk_paned_pack2(GTK_PANED(panel->container), bottom_box, TRUE, TRUE);
    
    return panel;
}

void notespanel_load_book(NotesPanel *panel, int book_id) {
    if (!panel || book_id <= 0) return;
    
    panel->current_book_id = book_id;
    panel->current_note_id = -1;
    
    // Clear editor
    OrgModeEditor *org = (OrgModeEditor *)g_object_get_data(G_OBJECT(panel->editor), "orgmode_editor");
    if (org && org->buffer) {
        gtk_text_buffer_set_text(org->buffer, "", -1);
        gtk_text_view_set_editable(GTK_TEXT_VIEW(org->text_view), FALSE);
    }
    gtk_widget_set_sensitive(panel->save_button, FALSE);
    gtk_widget_set_sensitive(panel->delete_button, FALSE);
    
    // Load notes
    Note **notes = NULL;
    int count = 0;
    
    BnError err = db_note_get_by_book(panel->db, book_id, &notes, &count);
    if (err != BN_SUCCESS) {
        return;
    }
    
    // Create model
    GtkListStore *store = gtk_list_store_new(NOTE_COL_NUM,
                                             G_TYPE_INT,      // ID
                                             G_TYPE_STRING,   // Title
                                             G_TYPE_STRING);  // Page
    
    for (int i = 0; i < count; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        
        char page_str[32];
        if (notes[i]->page_number > 0) {
            snprintf(page_str, sizeof(page_str), "p.%d", notes[i]->page_number);
        } else {
            snprintf(page_str, sizeof(page_str), "-");
        }
        
        gtk_list_store_set(store, &iter,
                          NOTE_COL_ID, notes[i]->id,
                          NOTE_COL_TITLE, notes[i]->title,
                          NOTE_COL_PAGE, page_str,
                          -1);
        
        note_free(notes[i]);
    }
    free(notes);
    
    gtk_tree_view_set_model(GTK_TREE_VIEW(panel->notes_list), GTK_TREE_MODEL(store));
    g_object_unref(store);
    
    if (count == 0 && org && org->buffer) {
        gtk_text_buffer_set_text(org->buffer, "No notes yet.\n\nClick '+ New Note' to create one.", -1);
    }
}

void notespanel_clear(NotesPanel *panel) {
    if (!panel) return;
    
    panel->current_book_id = -1;
    panel->current_note_id = -1;
    
    // Clear list
    gtk_tree_view_set_model(GTK_TREE_VIEW(panel->notes_list), NULL);
    
    // Clear editor
    GtkWidget *md_textview = (GtkWidget *)g_object_get_data(G_OBJECT(panel->editor), "markdown_textview");
    if (md_textview) {
        GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(md_textview));
        gtk_text_buffer_set_text(buf, "Select a book to view notes", -1);
        gtk_text_view_set_editable(GTK_TEXT_VIEW(md_textview), FALSE);
    }
    
    gtk_widget_set_sensitive(panel->save_button, FALSE);
    gtk_widget_set_sensitive(panel->delete_button, FALSE);
}

void notespanel_destroy(NotesPanel *panel) {
    if (!panel) return;
    // Clear Markdown editor pointer if present
    g_object_set_data(G_OBJECT(panel->editor), "markdown_textview", NULL);
    free(panel);
}

static void on_note_selected(GtkTreeView *view, gpointer data) {
    (void)view;
    NotesPanel *panel = (NotesPanel *)data;
    
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(panel->notes_list));
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        return;
    }
    
    int note_id;
    gtk_tree_model_get(model, &iter, NOTE_COL_ID, &note_id, -1);
    
    panel->current_note_id = note_id;
    
    // Load note content (we need to get it from DB)
    Note **notes = NULL;
    int count = 0;
    
    BnError err = db_note_get_by_book(panel->db, panel->current_book_id, &notes, &count);
    if (err != BN_SUCCESS) return;
    
    // Find our note
    for (int i = 0; i < count; i++) {
        if (notes[i]->id == note_id) {
            OrgModeEditor *org = (OrgModeEditor *)g_object_get_data(G_OBJECT(panel->editor), "orgmode_editor");
            if (org && org->buffer) {
                gtk_text_buffer_set_text(org->buffer, notes[i]->content, -1);
                gtk_text_view_set_editable(GTK_TEXT_VIEW(org->text_view), TRUE);
            }
            
            gtk_widget_set_sensitive(panel->save_button, TRUE);
            gtk_widget_set_sensitive(panel->delete_button, TRUE);
            
            note_free(notes[i]);
            break;
        }
        note_free(notes[i]);
    }
    free(notes);
}

static void on_save_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    NotesPanel *panel = (NotesPanel *)data;
    
    if (panel->current_note_id <= 0) return;
    
    // Get content from editor
    OrgModeEditor *org = (OrgModeEditor *)g_object_get_data(G_OBJECT(panel->editor), "orgmode_editor");
    if (!org || !org->buffer) return;
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(org->buffer, &start, &end);
    char *content = gtk_text_buffer_get_text(org->buffer, &start, &end, FALSE);
    
    // Load current note to update
    Note **notes = NULL;
    int count = 0;
    BnError err = db_note_get_by_book(panel->db, panel->current_book_id, &notes, &count);
    
    if (err != BN_SUCCESS) {
        g_free(content);
        return;
    }
    
    // Find and update our note
    for (int i = 0; i < count; i++) {
        if (notes[i]->id == panel->current_note_id) {
            // Update content
            free(notes[i]->content);
            notes[i]->content = g_strdup(content);
            
            // Save to database
            err = db_note_update(panel->db, notes[i]);
            
            if (err == BN_SUCCESS) {
                GtkWidget *dialog = gtk_message_dialog_new(NULL,
                    GTK_DIALOG_MODAL,
                    GTK_MESSAGE_INFO,
                    GTK_BUTTONS_OK,
                    "Note saved successfully!");
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                
                // Reload notes list
                notespanel_load_book(panel, panel->current_book_id);
            } else {
                GtkWidget *dialog = gtk_message_dialog_new(NULL,
                    GTK_DIALOG_MODAL,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_OK,
                    "Error saving note");
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
            }
            
            note_free(notes[i]);
            break;
        }
        note_free(notes[i]);
    }
    free(notes);
    g_free(content);
}

static void on_delete_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    NotesPanel *panel = (NotesPanel *)data;
    
    if (panel->current_note_id <= 0) return;
    
    // Confirm deletion
    GtkWidget *dialog = gtk_message_dialog_new(NULL,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "Delete this note? This cannot be undone.");
    
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    if (response == GTK_RESPONSE_YES) {
        BnError err = db_note_delete(panel->db, panel->current_note_id);
        
        if (err == BN_SUCCESS) {
            panel->current_note_id = -1;
            
            // Clear editor
            OrgModeEditor *org = (OrgModeEditor *)g_object_get_data(G_OBJECT(panel->editor), "orgmode_editor");
            if (org && org->buffer) {
                gtk_text_buffer_set_text(org->buffer, "", -1);
                gtk_text_view_set_editable(GTK_TEXT_VIEW(org->text_view), FALSE);
            }
            gtk_widget_set_sensitive(panel->save_button, FALSE);
            gtk_widget_set_sensitive(panel->delete_button, FALSE);
            
            // Reload list
            notespanel_load_book(panel, panel->current_book_id);
        } else {
            GtkWidget *error_dialog = gtk_message_dialog_new(NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Error deleting note");
            gtk_dialog_run(GTK_DIALOG(error_dialog));
            gtk_widget_destroy(error_dialog);
        }
    }
}

static void on_new_note_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    NotesPanel *panel = (NotesPanel *)data;
    
    if (panel->current_book_id <= 0) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK,
            "Please select a book first");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    // Create dialog for new note
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "New Note",
        NULL,
        GTK_DIALOG_MODAL,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Create", GTK_RESPONSE_OK,
        NULL);
    
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 10);
    
    // Title entry
    GtkWidget *title_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *title_label = gtk_label_new("Title:");
    GtkWidget *title_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(title_entry), "Note title");
    gtk_box_pack_start(GTK_BOX(title_box), title_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(title_box), title_entry, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(content_area), title_box, FALSE, FALSE, 5);
    
    // Page entry
    GtkWidget *page_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *page_label = gtk_label_new("Page:");
    GtkWidget *page_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(page_entry), "Optional");
    gtk_box_pack_start(GTK_BOX(page_box), page_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(page_box), page_entry, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(content_area), page_box, FALSE, FALSE, 5);
    
    // Content area
    GtkWidget *content_label = gtk_label_new("Content:");
    gtk_box_pack_start(GTK_BOX(content_area), content_label, FALSE, FALSE, 5);
    
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scroll, 400, 200);
    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    gtk_container_add(GTK_CONTAINER(scroll), text_view);
    gtk_box_pack_start(GTK_BOX(content_area), scroll, TRUE, TRUE, 5);
    
    gtk_widget_show_all(content_area);
    
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (response == GTK_RESPONSE_OK) {
        const char *title = gtk_entry_get_text(GTK_ENTRY(title_entry));
        const char *page_str = gtk_entry_get_text(GTK_ENTRY(page_entry));
        int page_number = atoi(page_str);
        
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        GtkTextIter start, end;
        gtk_text_buffer_get_bounds(buffer, &start, &end);
        char *content = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
        
        // Create note
        Note *note = NULL;
        BnError err = note_create(&note, panel->current_book_id, 
                                 title && strlen(title) > 0 ? title : NULL,
                                 content, page_number);
        
        if (err == BN_SUCCESS) {
            err = db_note_insert(panel->db, note);
            
            if (err == BN_SUCCESS) {
                notespanel_load_book(panel, panel->current_book_id);
            } else {
                GtkWidget *error_dialog = gtk_message_dialog_new(NULL,
                    GTK_DIALOG_MODAL,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_OK,
                    "Error creating note");
                gtk_dialog_run(GTK_DIALOG(error_dialog));
                gtk_widget_destroy(error_dialog);
            }
            
            note_free(note);
        }
        
        g_free(content);
    }
    
    gtk_widget_destroy(dialog);
}
