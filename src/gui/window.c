#include "window.h"
#include "libraryview.h"
#include "notesview.h"
#include "../database/queries.h"
#include <stdlib.h>

#define WINDOW_TITLE "booknote"
#define DEFAULT_WIDTH 1200
#define DEFAULT_HEIGHT 800
#define NOTES_WIDTH 400

static void on_destroy(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    gtk_main_quit();
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    (void)widget;
    MainWindow *win = (MainWindow *)data;

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
                window_show_library(win);
                return TRUE;
        }
    }

    return FALSE;
}

static void on_library_book_selected(int book_id, gpointer user_data) {
    MainWindow *win = (MainWindow *)user_data;
    if (book_id <= 0) {
        notespanel_clear(win->notes_panel);
        return;
    }
    window_show_reading(win, book_id);
}

static void on_add_book_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    MainWindow *win = (MainWindow *)data;

    libraryview_show_add_dialog(win->window, win->db, (void (*)(void *))libraryview_load_books, win->library_view);
}

static void on_back_clicked(GtkWidget *button, gpointer data) {
    (void)button;
    MainWindow *win = (MainWindow *)data;
    window_show_library(win);
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

static void on_menu_about(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;

    GtkWidget *dialog = gtk_message_dialog_new(
        NULL,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "booknote v0.4.0-dev\n\n"
        "Personal library with notes\n"
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

static GtkWidget* create_reading_view(MainWindow *win) {
    // Header with back button
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(header, 16);
    gtk_widget_set_margin_end(header, 16);
    gtk_widget_set_margin_top(header, 12);
    gtk_widget_set_margin_bottom(header, 12);
    gtk_widget_set_name(header, "reading-header");

    win->back_button = gtk_button_new_with_label("Back to Library");
    gtk_widget_set_name(win->back_button, "back-button");
    g_signal_connect(win->back_button, "clicked", G_CALLBACK(on_back_clicked), win);
    gtk_box_pack_start(GTK_BOX(header), win->back_button, FALSE, FALSE, 0);

    GtkWidget *spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(header), spacer, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), header, FALSE, FALSE, 0);

    // Reading content: PDF viewer | Notes panel
    GtkWidget *content_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    win->content_paned = content_paned;

    win->pdf_viewer = pdfviewer_create();
    gtk_paned_pack1(GTK_PANED(content_paned), win->pdf_viewer->container, TRUE, TRUE);

    win->notes_panel = notespanel_create(win->db);
    win->notes_container = win->notes_panel->container;
    gtk_paned_pack2(GTK_PANED(content_paned), win->notes_panel->container, FALSE, TRUE);

    gtk_box_pack_start(GTK_BOX(vbox), content_paned, TRUE, TRUE, 0);

    // Initial paned position
    gtk_paned_set_position(GTK_PANED(content_paned), DEFAULT_WIDTH - NOTES_WIDTH);

    return vbox;
}

MainWindow* window_create(Database *db) {
    MainWindow *win = calloc(1, sizeof(MainWindow));
    if (!win) return NULL;

    win->db = db;
    win->notes_visible = TRUE;
    win->current_book_id = -1;

    // Main window
    win->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win->window), WINDOW_TITLE);
    gtk_window_set_default_size(GTK_WINDOW(win->window), DEFAULT_WIDTH, DEFAULT_HEIGHT);
    gtk_window_set_position(GTK_WINDOW(win->window), GTK_WIN_POS_CENTER);

    g_signal_connect(win->window, "destroy", G_CALLBACK(on_destroy), win);
    g_signal_connect(win->window, "key-press-event", G_CALLBACK(on_key_press), win);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(win->window), main_vbox);

    GtkWidget *menu_bar = create_menu_bar(win);
    gtk_box_pack_start(GTK_BOX(main_vbox), menu_bar, FALSE, FALSE, 0);

    // GtkStack for navigation
    win->stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(win->stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(win->stack), 250);
    gtk_box_pack_start(GTK_BOX(main_vbox), win->stack, TRUE, TRUE, 0);

    // Library view
    win->library_view = libraryview_create(db);
    libraryview_set_callback(win->library_view, on_library_book_selected, win);
    gtk_widget_set_name(win->library_view->add_button, "add-book-button");
    g_signal_connect(win->library_view->add_button, "clicked", G_CALLBACK(on_add_book_clicked), win);

    win->library_container = win->library_view->container;
    gtk_stack_add_named(GTK_STACK(win->stack), win->library_container, "library");

    // Reading view
    win->reading_container = create_reading_view(win);
    gtk_stack_add_named(GTK_STACK(win->stack), win->reading_container, "reading");

    // Show Library by default
    libraryview_load_books(win->library_view);
    gtk_stack_set_visible_child_name(GTK_STACK(win->stack), "library");

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

void window_show_library(MainWindow *win) {
    if (!win) return;
    gtk_stack_set_visible_child_name(GTK_STACK(win->stack), "library");
    libraryview_load_books(win->library_view);
}

void window_show_reading(MainWindow *win, int book_id) {
    if (!win) return;

    win->current_book_id = book_id;

    Book *book = NULL;
    BnError err = db_book_get_by_id(win->db, book_id, &book);
    if (err == BN_SUCCESS && book) {
        // Load PDF
        if (!pdfviewer_load_file(win->pdf_viewer, book->filepath)) {
            GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(win->window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Failed to load PDF: %s", book->filepath);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        }
        book_free(book);
    }

    // Load notes
    notespanel_load_book(win->notes_panel, book_id);

    gtk_stack_set_visible_child_name(GTK_STACK(win->stack), "reading");
}

