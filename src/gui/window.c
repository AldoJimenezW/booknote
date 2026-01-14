#include "window.h"
#include "booklist.h"
#include "notesview.h"
#include "../database/queries.h"
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

static void refresh_book_list(void *data) {
    MainWindow *win = (MainWindow *)data;

    // Reload books
    GtkTreeModel *model = booklist_create_model(win->db);
    gtk_tree_view_set_model(GTK_TREE_VIEW(win->book_list), model);
    g_object_unref(model);
}

static void on_book_selected(GtkTreeView *view, gpointer data) {
    (void)view;
    MainWindow *win = (MainWindow *)data;

    int book_id = booklist_get_selected_id(GTK_TREE_VIEW(win->book_list));
    if (book_id <= 0) {
        notespanel_clear(win->notes_panel);
        gtk_widget_set_sensitive(win->delete_book_button, FALSE);
        return;
    }

    win->current_book_id = book_id;

    // Load notes for selected book
    notespanel_load_book(win->notes_panel, book_id);

    // Enable delete button
    gtk_widget_set_sensitive(win->delete_book_button, TRUE);

    printf("Book selected: ID %d\n", book_id);
}

static void on_add_book_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    MainWindow *win = (MainWindow *)data;

    booklist_show_add_dialog(win->window, win->db, refresh_book_list, win);
}

static void on_delete_book_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    MainWindow *win = (MainWindow *)data;

    int book_id = booklist_get_selected_id(GTK_TREE_VIEW(win->book_list));
    if (book_id <= 0) {
        return;
    }

    // Confirm deletion
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(win->window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "Delete this book and all its notes?\n\nThis cannot be undone.");

    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (response == GTK_RESPONSE_YES) {
        BnError err = db_book_delete(win->db, book_id);

        if (err == BN_SUCCESS) {
            // Clear notes panel
            notespanel_clear(win->notes_panel);
            win->current_book_id = -1;

            // Refresh book list
            refresh_book_list(win);

            // Disable delete button
            gtk_widget_set_sensitive(win->delete_book_button, FALSE);
        } else {
            GtkWidget *error = gtk_message_dialog_new(GTK_WINDOW(win->window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Error deleting book");
            gtk_dialog_run(GTK_DIALOG(error));
            gtk_widget_destroy(error);
        }
    }
}

static void on_menu_quit(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    gtk_main_quit();
}

static void on_menu_toggle_notes(GtkWidget *widget, gpointer data) {
    (void)widget;
    MainWindow *win = (MainWindow *)data;
    window_toggle_notes(win);
}

static void on_menu_toggle_sidebar(GtkWidget *widget, gpointer data) {
    (void)widget;
    MainWindow *win = (MainWindow *)data;
    window_toggle_sidebar(win);
}

static void on_menu_about(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;

    GtkWidget *dialog = gtk_message_dialog_new(
        NULL,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "booknote v0.2-dev\n\n"
        "Note-taking for technical books\n"
        "Built with GTK3 and Poppler\n\n"
        "https://github.com/AldoJimenezW/booknote"
    );
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static GtkWidget* create_menu_bar(MainWindow *win) {
    GtkWidget *menu_bar = gtk_menu_bar_new();

    // File menu
    GtkWidget *file_menu = gtk_menu_new();
    GtkWidget *file_item = gtk_menu_item_new_with_label("File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);

    GtkWidget *quit_item = gtk_menu_item_new_with_label("Quit");
    g_signal_connect(quit_item, "activate", G_CALLBACK(on_menu_quit), win);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_item);

    // View menu
    GtkWidget *view_menu = gtk_menu_new();
    GtkWidget *view_item = gtk_menu_item_new_with_label("View");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_item), view_menu);

    GtkWidget *toggle_sidebar_item = gtk_menu_item_new_with_label("Toggle Sidebar");
    g_signal_connect(toggle_sidebar_item, "activate", G_CALLBACK(on_menu_toggle_sidebar), win);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), toggle_sidebar_item);

    GtkWidget *toggle_notes_item = gtk_menu_item_new_with_label("Toggle Notes");
    g_signal_connect(toggle_notes_item, "activate", G_CALLBACK(on_menu_toggle_notes), win);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), toggle_notes_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), view_item);

    // Help menu
    GtkWidget *help_menu = gtk_menu_new();
    GtkWidget *help_item = gtk_menu_item_new_with_label("Help");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_item), help_menu);

    GtkWidget *about_item = gtk_menu_item_new_with_label("About");
    g_signal_connect(about_item, "activate", G_CALLBACK(on_menu_about), win);
    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), about_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), help_item);

    return menu_bar;
}

MainWindow* window_create(Database *db) {
    MainWindow *win = calloc(1, sizeof(MainWindow));
    if (!win) return NULL;

    win->db = db;
    win->notes_visible = TRUE;
    win->current_book_id = -1;
    win->sidebar_visible = TRUE;

    // Create main window
    win->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win->window), WINDOW_TITLE);
    gtk_window_set_default_size(GTK_WINDOW(win->window), DEFAULT_WIDTH, DEFAULT_HEIGHT);
    gtk_window_set_position(GTK_WINDOW(win->window), GTK_WIN_POS_CENTER);

    // Connect signals
    g_signal_connect(win->window, "destroy", G_CALLBACK(on_destroy), win);
    g_signal_connect(win->window, "key-press-event", G_CALLBACK(on_key_press), win);

    // Create main container
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(win->window), main_vbox);

    // Add menu bar
    GtkWidget *menu_bar = create_menu_bar(win);
    gtk_box_pack_start(GTK_BOX(main_vbox), menu_bar, FALSE, FALSE, 0);

    win->main_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(main_vbox), win->main_paned, TRUE, TRUE, 0);

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

    // Tree view for books
    win->book_list = gtk_tree_view_new();

    // Setup view
    booklist_setup_view(GTK_TREE_VIEW(win->book_list));

    // Load books from database
    GtkTreeModel *model = booklist_create_model(db);
    gtk_tree_view_set_model(GTK_TREE_VIEW(win->book_list), model);
    g_object_unref(model);

    // Connect selection signal
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(win->book_list));
    g_signal_connect(selection, "changed", G_CALLBACK(on_book_selected), win);

    gtk_container_add(GTK_CONTAINER(sidebar_scroll), win->book_list);

    gtk_box_pack_start(GTK_BOX(win->book_sidebar), sidebar_scroll, TRUE, TRUE, 0);

    // Add button
    GtkWidget *add_button = gtk_button_new_with_label("+ Add Book");
    gtk_box_pack_start(GTK_BOX(win->book_sidebar), add_button, FALSE, FALSE, 5);
    g_signal_connect(add_button, "clicked", G_CALLBACK(on_add_book_clicked), win);

    // Delete book button
    win->delete_book_button = gtk_button_new_with_label("Delete Book");
    gtk_widget_set_sensitive(win->delete_book_button, FALSE);  // Disabled by default
    g_signal_connect(win->delete_book_button, "clicked", G_CALLBACK(on_delete_book_clicked), win);
    gtk_box_pack_start(GTK_BOX(win->book_sidebar), win->delete_book_button, FALSE, FALSE, 5);

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
    win->notes_panel = notespanel_create(db);
    gtk_paned_pack2(GTK_PANED(win->content_paned), win->notes_panel->container, FALSE, TRUE);

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
