#include "window.h"
#include <stdlib.h>

#define WINDOW_TITLE "booknote"
#define DEFAULT_WIDTH 1200
#define DEFAULT_HEIGHT 800
#define SIDEBAR_WIDTH 200
#define NOTES_WIDTH 400

static void on_destroy(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    gtk_main_quit();
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    (void)widget;
    MainWindow *win = (MainWindow *)data;
    
    // Check for Control key
    gboolean ctrl = (event->state & GDK_CONTROL_MASK);
    
    if (ctrl) {
        switch (event->keyval) {
            case GDK_KEY_q:
                gtk_main_quit();
                return TRUE;
            case GDK_KEY_b:
                window_toggle_notes(win);
                return TRUE;
            case GDK_KEY_l:
                window_toggle_sidebar(win);
                return TRUE;
        }
    }
    
    return FALSE;
}

MainWindow* window_create(Database *db) {
    MainWindow *win = calloc(1, sizeof(MainWindow));
    if (!win) return NULL;
    
    win->db = db;
    win->notes_visible = TRUE;
    win->sidebar_visible = TRUE;
    
    // Create main window
    win->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win->window), WINDOW_TITLE);
    gtk_window_set_default_size(GTK_WINDOW(win->window), DEFAULT_WIDTH, DEFAULT_HEIGHT);
    gtk_window_set_position(GTK_WINDOW(win->window), GTK_WIN_POS_CENTER);
    
    // Connect signals
    g_signal_connect(win->window, "destroy", G_CALLBACK(on_destroy), win);
    g_signal_connect(win->window, "key-press-event", G_CALLBACK(on_key_press), win);
    
    // Main horizontal paned (sidebar | content)
    win->main_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_container_add(GTK_CONTAINER(win->window), win->main_paned);
    
    // Left panel - Book sidebar
    win->book_sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_size_request(win->book_sidebar, SIDEBAR_WIDTH, -1);
    
    GtkWidget *sidebar_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(sidebar_label), "<b>Books</b>");
    gtk_box_pack_start(GTK_BOX(win->book_sidebar), sidebar_label, FALSE, FALSE, 10);
    
    // Scrolled window for book list
    GtkWidget *sidebar_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sidebar_scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    // Tree view for books (placeholder)
    win->book_list = gtk_tree_view_new();
    gtk_container_add(GTK_CONTAINER(sidebar_scroll), win->book_list);
    gtk_box_pack_start(GTK_BOX(win->book_sidebar), sidebar_scroll, TRUE, TRUE, 0);
    
    // Add button
    GtkWidget *add_button = gtk_button_new_with_label("+ Add Book");
    gtk_box_pack_start(GTK_BOX(win->book_sidebar), add_button, FALSE, FALSE, 5);
    
    gtk_paned_pack1(GTK_PANED(win->main_paned), win->book_sidebar, FALSE, TRUE);
    
    // Content paned (PDF | Notes)
    win->content_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_pack2(GTK_PANED(win->main_paned), win->content_paned, TRUE, TRUE);
    
    // Center panel - PDF viewer
    win->pdf_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    GtkWidget *pdf_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(pdf_label), "<b>PDF Viewer</b>");
    gtk_box_pack_start(GTK_BOX(win->pdf_container), pdf_label, FALSE, FALSE, 10);
    
    win->pdf_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(win->pdf_scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    GtkWidget *pdf_placeholder = gtk_label_new(
        "PDF viewer will render here\n\n"
        "Select a book from the sidebar"
    );
    gtk_container_add(GTK_CONTAINER(win->pdf_scrolled), pdf_placeholder);
    gtk_box_pack_start(GTK_BOX(win->pdf_container), win->pdf_scrolled, TRUE, TRUE, 0);
    
    // Navigation controls
    GtkWidget *nav_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(nav_box), gtk_button_new_with_label("<"), FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(nav_box), gtk_label_new("Page 1 / 1"), TRUE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(nav_box), gtk_button_new_with_label(">"), FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(nav_box), gtk_button_new_with_label("100%"), FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(win->pdf_container), nav_box, FALSE, FALSE, 5);
    
    gtk_paned_pack1(GTK_PANED(win->content_paned), win->pdf_container, TRUE, TRUE);
    
    // Right panel - Notes
    win->notes_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_size_request(win->notes_container, NOTES_WIDTH, -1);
    
    GtkWidget *notes_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(notes_label), "<b>Notes</b>");
    gtk_box_pack_start(GTK_BOX(win->notes_container), notes_label, FALSE, FALSE, 10);
    
    // Notes view
    GtkWidget *notes_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(notes_scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    win->notes_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(win->notes_view), GTK_WRAP_WORD);
    gtk_container_add(GTK_CONTAINER(notes_scroll), win->notes_view);
    gtk_box_pack_start(GTK_BOX(win->notes_container), notes_scroll, TRUE, TRUE, 0);
    
    // Add note button
    GtkWidget *add_note_button = gtk_button_new_with_label("+ Add Note");
    gtk_box_pack_start(GTK_BOX(win->notes_container), add_note_button, FALSE, FALSE, 5);
    
    gtk_paned_pack2(GTK_PANED(win->content_paned), win->notes_container, FALSE, TRUE);
    
    // Set initial paned positions
    gtk_paned_set_position(GTK_PANED(win->main_paned), SIDEBAR_WIDTH);
    gtk_paned_set_position(GTK_PANED(win->content_paned), 
                          DEFAULT_WIDTH - SIDEBAR_WIDTH - NOTES_WIDTH);
    
    return win;
}

void window_destroy(MainWindow *win) {
    if (!win) return;
    free(win);
}

void window_toggle_notes(MainWindow *win) {
    if (!win) return;
    
    win->notes_visible = !win->notes_visible;
    
    if (win->notes_visible) {
        gtk_widget_show(win->notes_container);
    } else {
        gtk_widget_hide(win->notes_container);
    }
}

void window_toggle_sidebar(MainWindow *win) {
    if (!win) return;
    
    win->sidebar_visible = !win->sidebar_visible;
    
    if (win->sidebar_visible) {
        gtk_widget_show(win->book_sidebar);
    } else {
        gtk_widget_hide(win->book_sidebar);
    }
}
