#include "libraryview.h"
#include "../database/queries.h"
#include "../core/book.h"
#include "../external/isbn.h"
#include "../external/cover.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

static void on_edit_selected_clicked(GtkButton *button, gpointer data);
static void on_delete_selected_clicked(GtkButton *button, gpointer data);

static void libraryview_show_edit_dialog(GtkWidget *parent,
                                         Database *db,
                                         int book_id,
                                         void (*on_refreshed)(void *user_data),
                                         void *user_data) {
    if (book_id <= 0) {
        GtkWidget *warn = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                                 "No book selected to edit.");
        gtk_dialog_run(GTK_DIALOG(warn));
        gtk_widget_destroy(warn);
        return;
    }

    Book *book = NULL;
    BnError err = db_book_get_by_id(db, book_id, &book);
    if (err != BN_SUCCESS || !book) {
        GtkWidget *errdlg = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                   "Failed to load book for editing.");
        gtk_dialog_run(GTK_DIALOG(errdlg));
        gtk_widget_destroy(errdlg);
        return;
    }

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Edit Book",
        GTK_WINDOW(parent),
        GTK_DIALOG_MODAL,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Save", GTK_RESPONSE_OK,
        NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 12);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_box_pack_start(GTK_BOX(content), grid, TRUE, TRUE, 0);

    GtkWidget *isbn_label = gtk_label_new("ISBN:");
    gtk_widget_set_halign(isbn_label, GTK_ALIGN_END);
    GtkWidget *isbn_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(isbn_entry), book->isbn ? book->isbn : "");

    GtkWidget *title_label = gtk_label_new("Title:");
    gtk_widget_set_halign(title_label, GTK_ALIGN_END);
    GtkWidget *title_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(title_entry), book->title ? book->title : "");

    GtkWidget *author_label = gtk_label_new("Author:");
    gtk_widget_set_halign(author_label, GTK_ALIGN_END);
    GtkWidget *author_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(author_entry), book->author ? book->author : "");

    GtkWidget *publisher_label = gtk_label_new("Publisher:");
    gtk_widget_set_halign(publisher_label, GTK_ALIGN_END);
    GtkWidget *publisher_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(publisher_entry), book->publisher ? book->publisher : "");

    GtkWidget *year_label = gtk_label_new("Year:");
    gtk_widget_set_halign(year_label, GTK_ALIGN_END);
    GtkWidget *year_entry = gtk_entry_new();
    if (book->year > 0) {
        char year_buf[16];
        snprintf(year_buf, sizeof(year_buf), "%d", book->year);
        gtk_entry_set_text(GTK_ENTRY(year_entry), year_buf);
    }

    GtkWidget *file_label = gtk_label_new("PDF File:");
    gtk_widget_set_halign(file_label, GTK_ALIGN_END);
    GtkWidget *file_chooser = gtk_file_chooser_button_new("Select PDF", GTK_FILE_CHOOSER_ACTION_OPEN);
    GtkFileFilter *pdf_filter = gtk_file_filter_new();
    gtk_file_filter_add_mime_type(pdf_filter, "application/pdf");
    gtk_file_filter_set_name(pdf_filter, "PDF files");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_chooser), pdf_filter);
    if (book->filepath && g_file_test(book->filepath, G_FILE_TEST_EXISTS)) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(file_chooser), book->filepath);
    }

    gtk_grid_attach(GTK_GRID(grid), isbn_label,      0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), isbn_entry,      1, 0, 2, 1);

    gtk_grid_attach(GTK_GRID(grid), title_label,     0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), title_entry,     1, 1, 2, 1);

    gtk_grid_attach(GTK_GRID(grid), author_label,    0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), author_entry,    1, 2, 2, 1);

    gtk_grid_attach(GTK_GRID(grid), publisher_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), publisher_entry, 1, 3, 2, 1);

    gtk_grid_attach(GTK_GRID(grid), year_label,      0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), year_entry,      1, 4, 2, 1);

    gtk_grid_attach(GTK_GRID(grid), file_label,      0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), file_chooser,    1, 5, 2, 1);

    gtk_widget_show_all(content);

    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK) {
        const char *isbn = gtk_entry_get_text(GTK_ENTRY(isbn_entry));
        const char *title = gtk_entry_get_text(GTK_ENTRY(title_entry));
        const char *author = gtk_entry_get_text(GTK_ENTRY(author_entry));
        const char *publisher = gtk_entry_get_text(GTK_ENTRY(publisher_entry));
        const char *year_str = gtk_entry_get_text(GTK_ENTRY(year_entry));
        char *filepath = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));

        if (!title || strlen(title) == 0 || !filepath || strlen(filepath) == 0) {
            GtkWidget *err = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                    "Title and PDF file are required.");
            gtk_dialog_run(GTK_DIALOG(err));
            gtk_widget_destroy(err);
            if (filepath) g_free(filepath);
            gtk_widget_destroy(dialog);
            book_free(book);
            return;
        }

        book->title = g_strdup(title);
        book->filepath = g_strdup(filepath);
        if (isbn && strlen(isbn) > 0) book_set_isbn(book, isbn); else book_set_isbn(book, NULL);
        if (author && strlen(author) > 0) book_set_author(book, author); else book_set_author(book, NULL);
        if (publisher && strlen(publisher) > 0) book_set_publisher(book, publisher); else book_set_publisher(book, NULL);
        int year = atoi(year_str);
        book_set_year(book, year > 0 ? year : 0);
        book->updated_at = time(NULL);

        BnError uerr = db_book_update(db, book);
        if (uerr != BN_SUCCESS) {
            GtkWidget *err = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                    "Error updating book in database.");
            gtk_dialog_run(GTK_DIALOG(err));
            gtk_widget_destroy(err);
        } else if (on_refreshed) {
            on_refreshed(user_data);
        }

        if (filepath) g_free(filepath);
    }

    gtk_widget_destroy(dialog);
    book_free(book);
}

static void on_edit_selected_clicked(GtkButton *button, gpointer data) {
    (void)button;
    LibraryView *view = (LibraryView *)data;
    if (!view) return;
    libraryview_show_edit_dialog(view->container, view->db, view->selected_book_id,
                                 (void (*)(void *))libraryview_load_books, view);
}

static void on_delete_selected_clicked(GtkButton *button, gpointer data) {
    (void)button;
    LibraryView *view = (LibraryView *)data;
    if (!view) return;

    if (view->selected_book_id <= 0) {
        GtkWidget *warn = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                                 "No book selected to delete.");
        gtk_dialog_run(GTK_DIALOG(warn));
        gtk_widget_destroy(warn);
        return;
    }

    GtkWidget *confirm = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
                                                "Delete selected book?\nThis will remove it from the library.");
    gtk_dialog_add_buttons(GTK_DIALOG(confirm), "Cancel", GTK_RESPONSE_CANCEL, "Delete", GTK_RESPONSE_OK, NULL);
    int resp = gtk_dialog_run(GTK_DIALOG(confirm));
    gtk_widget_destroy(confirm);
    if (resp != GTK_RESPONSE_OK) return;

    BnError derr = db_book_delete(view->db, view->selected_book_id);
    if (derr != BN_SUCCESS) {
        GtkWidget *err = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                "Error deleting book.");
        gtk_dialog_run(GTK_DIALOG(err));
        gtk_widget_destroy(err);
        return;
    }

    view->selected_book_id = -1;
    libraryview_load_books(view);
}

static void on_book_card_clicked(GtkButton *button, gpointer data) {
    LibraryView *view = (LibraryView *)data;
    if (!view) return;

    gpointer idp = g_object_get_data(G_OBJECT(button), "book_id");
    int book_id = GPOINTER_TO_INT(idp);

    view->selected_book_id = book_id;

    if (view->on_book_selected) {
        view->on_book_selected(book_id, view->user_data);
    }
}


/* Bundle widget pointers for ISBN fetch callback */
typedef struct {
    GtkEntry *isbn_entry;
    GtkEntry *title_entry;
    GtkEntry *author_entry;
    GtkEntry *publisher_entry;
    GtkEntry *year_entry;
} IsbnFetchWidgets;

static void on_fetch_isbn_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    IsbnFetchWidgets *w = (IsbnFetchWidgets *)user_data;
    if (!w) return;

    const char *isbn = gtk_entry_get_text(w->isbn_entry);
    if (!isbn || strlen(isbn) == 0) {
        GtkWidget *warn = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                                 "Please enter an ISBN to fetch metadata.");
        gtk_dialog_run(GTK_DIALOG(warn));
        gtk_widget_destroy(warn);
        return;
    }

    IsbnMetadata *meta = NULL;
    int rc = isbn_fetch_metadata(isbn, &meta);
    if (rc != ISBN_OK || !meta) {
        GtkWidget *err = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                "Failed to fetch metadata for the provided ISBN.");
        gtk_dialog_run(GTK_DIALOG(err));
        gtk_widget_destroy(err);
        return;
    }

    gtk_entry_set_text(w->title_entry, meta->title ? meta->title : "");
    gtk_entry_set_text(w->author_entry, meta->author ? meta->author : "");
    gtk_entry_set_text(w->publisher_entry, meta->publisher ? meta->publisher : "");

    if (meta->year > 0) {
        char year_buf[16];
        snprintf(year_buf, sizeof(year_buf), "%d", meta->year);
        gtk_entry_set_text(w->year_entry, year_buf);
    }

    isbn_free_metadata(meta);
}

void libraryview_show_add_dialog(GtkWidget *parent,
                                 Database *db,
                                 void (*on_refreshed)(void *user_data),
                                 void *user_data) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Add Book",
        GTK_WINDOW(parent),
        GTK_DIALOG_MODAL,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Add", GTK_RESPONSE_OK,
        NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 12);

    // Form fields
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_box_pack_start(GTK_BOX(content), grid, TRUE, TRUE, 0);

    GtkWidget *isbn_label = gtk_label_new("ISBN:");
    gtk_widget_set_halign(isbn_label, GTK_ALIGN_END);
    GtkWidget *isbn_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(isbn_entry), "Optional (for metadata and cover)");

    GtkWidget *title_label = gtk_label_new("Title:");
    gtk_widget_set_halign(title_label, GTK_ALIGN_END);
    GtkWidget *title_entry = gtk_entry_new();

    GtkWidget *author_label = gtk_label_new("Author:");
    gtk_widget_set_halign(author_label, GTK_ALIGN_END);
    GtkWidget *author_entry = gtk_entry_new();

    GtkWidget *publisher_label = gtk_label_new("Publisher:");
    gtk_widget_set_halign(publisher_label, GTK_ALIGN_END);
    GtkWidget *publisher_entry = gtk_entry_new();

    GtkWidget *year_label = gtk_label_new("Year:");
    gtk_widget_set_halign(year_label, GTK_ALIGN_END);
    GtkWidget *year_entry = gtk_entry_new();

    GtkWidget *file_label = gtk_label_new("PDF File:");
    gtk_widget_set_halign(file_label, GTK_ALIGN_END);
    GtkWidget *file_chooser = gtk_file_chooser_button_new("Select PDF", GTK_FILE_CHOOSER_ACTION_OPEN);
    GtkFileFilter *pdf_filter = gtk_file_filter_new();
    gtk_file_filter_add_mime_type(pdf_filter, "application/pdf");
    gtk_file_filter_set_name(pdf_filter, "PDF files");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_chooser), pdf_filter);

    GtkWidget *fetch_button = gtk_button_new_with_label("Fetch from ISBN");
    gtk_widget_set_halign(fetch_button, GTK_ALIGN_START);

    // Layout
    gtk_grid_attach(GTK_GRID(grid), isbn_label,      0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), isbn_entry,      1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), fetch_button,    2, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), title_label,     0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), title_entry,     1, 1, 2, 1);

    gtk_grid_attach(GTK_GRID(grid), author_label,    0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), author_entry,    1, 2, 2, 1);

    gtk_grid_attach(GTK_GRID(grid), publisher_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), publisher_entry, 1, 3, 2, 1);

    gtk_grid_attach(GTK_GRID(grid), year_label,      0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), year_entry,      1, 4, 2, 1);

    gtk_grid_attach(GTK_GRID(grid), file_label,      0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), file_chooser,    1, 5, 2, 1);

    // Fetch metadata handler
    IsbnFetchWidgets *widgets = g_malloc0(sizeof(IsbnFetchWidgets));
    widgets->isbn_entry      = GTK_ENTRY(isbn_entry);
    widgets->title_entry     = GTK_ENTRY(title_entry);
    widgets->author_entry    = GTK_ENTRY(author_entry);
    widgets->publisher_entry = GTK_ENTRY(publisher_entry);
    widgets->year_entry      = GTK_ENTRY(year_entry);
    g_signal_connect(fetch_button, "clicked", G_CALLBACK(on_fetch_isbn_clicked), widgets);

    gtk_widget_show_all(content);

    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK) {
        const char *isbn = gtk_entry_get_text(GTK_ENTRY(isbn_entry));
        const char *title = gtk_entry_get_text(GTK_ENTRY(title_entry));
        const char *author = gtk_entry_get_text(GTK_ENTRY(author_entry));
        const char *publisher = gtk_entry_get_text(GTK_ENTRY(publisher_entry));
        const char *year_str = gtk_entry_get_text(GTK_ENTRY(year_entry));
        char *filepath = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));

        if (!title || strlen(title) == 0 || !filepath || strlen(filepath) == 0) {
            GtkWidget *err = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                    "Title and PDF file are required.");
            gtk_dialog_run(GTK_DIALOG(err));
            gtk_widget_destroy(err);
            gtk_widget_destroy(dialog);
            return;
        }

        int year = atoi(year_str);

        Book *book = NULL;
        if (book_create(&book, title, filepath) != BN_SUCCESS || !book) {
            GtkWidget *err = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                    "Failed to create book object.");
            gtk_dialog_run(GTK_DIALOG(err));
            gtk_widget_destroy(err);
            gtk_widget_destroy(dialog);
            return;
        }

        // Set optional metadata
        if (isbn && strlen(isbn) > 0) book_set_isbn(book, isbn);
        if (author && strlen(author) > 0) book_set_author(book, author);
        if (publisher && strlen(publisher) > 0) book_set_publisher(book, publisher);
        if (year > 0) book_set_year(book, year);

        // Attempt cover download if ISBN present
        char *cover_path = NULL;
        if (isbn && strlen(isbn) > 0) {
            char *cover_url = isbn_cover_url_medium(isbn);
            if (cover_url) {
                char *cached = NULL;
                int c_rc = isbn_download_cover(cover_url, &cached);
                if (c_rc == ISBN_OK && cached) {
                    cover_path = cached; // take ownership
                }
                free(cover_url);
            }
        }
        // Fallback: try to extract a cover from the PDF if ISBN cover was not found
        if (!cover_path && filepath && strlen(filepath) > 0) {
            // Ensure cache dir exists and build a target path
            if (cover_ensure_cache_dir() == COVER_OK) {
                char *dir = cover_cache_dir();
                if (dir) {
                    char target[512];
                    snprintf(target, sizeof(target), "%s/%ld.jpg", dir, (long)time(NULL));
                    free(dir);

                    char *extracted = NULL;
                    int e_rc = cover_extract_from_pdf(filepath, target, &extracted, 320);
                    if (e_rc == COVER_OK && extracted) {
                        cover_path = extracted; // take ownership
                    }
                }
            }
        }
        book->cover_path = cover_path ? g_strdup(cover_path) : NULL;

        // Timestamps
        book->added_at = time(NULL);
        book->updated_at = book->added_at;

        // Insert into DB
        BnError derr = db_book_insert(db, book);
        if (derr != BN_SUCCESS) {
            GtkWidget *err = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                    "Error inserting book into database.");
            gtk_dialog_run(GTK_DIALOG(err));
            gtk_widget_destroy(err);
            if (cover_path) free(cover_path);
            book_free(book);
            gtk_widget_destroy(dialog);
            return;
        }

        // Free temp cover_path owned string
        if (cover_path) free(cover_path);

        // Refresh library view
        if (on_refreshed) {
            on_refreshed(user_data);
        }
        if (filepath) g_free(filepath);
    }

    gtk_widget_destroy(dialog);
}

LibraryView* libraryview_create(Database *db) {
    LibraryView *view = calloc(1, sizeof(LibraryView));
    if (!view) return NULL;
    
    view->db = db;
    view->on_book_selected = NULL;
    view->user_data = NULL;
    view->selected_book_id = -1;
    
    // Main container with padding
    view->container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    // Header bar
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_name(header, "library-header");
    gtk_widget_set_margin_start(header, 20);
    gtk_widget_set_margin_end(header, 20);
    gtk_widget_set_margin_top(header, 20);
    gtk_widget_set_margin_bottom(header, 20);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_widget_set_name(title, "library-title");
    gtk_label_set_markup(GTK_LABEL(title), "<span size='x-large' weight='bold'>My Library</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(header), title, TRUE, TRUE, 0);
    
    // Header actions: Add / Edit / Delete
    view->add_button = gtk_button_new_with_label("+ Add Book");
    gtk_widget_set_name(view->add_button, "add-book-button");
    gtk_widget_set_size_request(view->add_button, 120, 40);

    view->edit_button = gtk_button_new_with_label("Edit Selected");
    gtk_widget_set_size_request(view->edit_button, 120, 40);

    view->delete_button = gtk_button_new_with_label("Delete Selected");
    gtk_widget_set_size_request(view->delete_button, 120, 40);

    // Pack actions (right side)
    gtk_box_pack_end(GTK_BOX(header), view->delete_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(header), view->edit_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(header), view->add_button, FALSE, FALSE, 0);

    g_signal_connect(view->edit_button, "clicked", G_CALLBACK(on_edit_selected_clicked), view);
    g_signal_connect(view->delete_button, "clicked", G_CALLBACK(on_delete_selected_clicked), view);

    gtk_box_pack_start(GTK_BOX(view->container), header, FALSE, FALSE, 0);
    
    // Scrolled window for books
    view->scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(view->scrolled),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    
    // FlowBox for book grid
    view->grid = gtk_flow_box_new();
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(view->grid), GTK_SELECTION_NONE);
    gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(view->grid), TRUE);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(view->grid), 20);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(view->grid), 20);
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(view->grid), 2);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(view->grid), 6);
    gtk_widget_set_margin_start(view->grid, 20);
    gtk_widget_set_margin_end(view->grid, 20);
    gtk_widget_set_margin_bottom(view->grid, 20);
    
    gtk_container_add(GTK_CONTAINER(view->scrolled), view->grid);
    gtk_box_pack_start(GTK_BOX(view->container), view->scrolled, TRUE, TRUE, 0);
    
    return view;
}

void libraryview_load_books(LibraryView *view) {
    if (!view) return;
    
    // Clear existing books
    GList *children = gtk_container_get_children(GTK_CONTAINER(view->grid));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
    
    // Load books from database
    Book **books = NULL;
    int count = 0;
    
    BnError err = db_book_get_all(view->db, &books, &count);
    if (err != BN_SUCCESS || count == 0) {
        // Show empty state
        GtkWidget *empty_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_widget_set_valign(empty_box, GTK_ALIGN_CENTER);
        gtk_widget_set_halign(empty_box, GTK_ALIGN_CENTER);
        
        GtkWidget *empty_label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(empty_label), 
            "<span size='large'>No books yet</span>\n"
            "<span size='small'>Click '+ Add Book' to get started</span>");
        gtk_label_set_justify(GTK_LABEL(empty_label), GTK_JUSTIFY_CENTER);
        
        gtk_box_pack_start(GTK_BOX(empty_box), empty_label, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(view->grid), empty_box);
        gtk_widget_show_all(view->grid);
        return;
    }
    
    // Create book cards
    for (int i = 0; i < count; i++) {
        // Card container
        GtkWidget *card = gtk_button_new();
        gtk_widget_set_size_request(card, 200, 300);
        g_object_set_data(G_OBJECT(card), "book_id", GINT_TO_POINTER(books[i]->id));
        g_signal_connect(card, "clicked", G_CALLBACK(on_book_card_clicked), view);
        
        GtkWidget *card_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_widget_set_margin_start(card_box, 15);
        gtk_widget_set_margin_end(card_box, 15);
        gtk_widget_set_margin_top(card_box, 15);
        gtk_widget_set_margin_bottom(card_box, 15);
        
        // Cover image (try real cover_path, fallback to placeholder)
        GtkWidget *cover = gtk_image_new();
        gtk_widget_set_size_request(cover, 170, 220);

        gboolean cover_set = FALSE;
        if (books[i]->cover_path && g_file_test(books[i]->cover_path, G_FILE_TEST_EXISTS)) {
            GError *img_err = NULL;
            GdkPixbuf *pix = gdk_pixbuf_new_from_file_at_scale(books[i]->cover_path, 170, 220, TRUE, &img_err);
            if (pix) {
                gtk_image_set_from_pixbuf(GTK_IMAGE(cover), pix);
                g_object_unref(pix);
                cover_set = TRUE;
            }
            if (img_err) {
                g_error_free(img_err);
            }
        }

        if (!cover_set) {
            // Placeholder colored box
            cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, 170, 220);
            cairo_t *cr = cairo_create(surface);

            // Random-ish color based on book ID
            double hue = ((books[i]->id * 137) % 360) / 360.0;
            double r, g, b;
            // Simple HSV to RGB (S=0.3, V=0.6 for muted colors)
            double c = 0.6 * 0.3;
            double x = c * (1 - fabs(fmod(hue * 6, 2) - 1));
            double m = 0.6 - c;
            if (hue < 1.0/6) { r = c; g = x; b = 0; }
            else if (hue < 2.0/6) { r = x; g = c; b = 0; }
            else if (hue < 3.0/6) { r = 0; g = c; b = x; }
            else if (hue < 4.0/6) { r = 0; g = x; b = c; }
            else if (hue < 5.0/6) { r = x; g = 0; b = c; }
            else { r = c; g = 0; b = x; }

            cairo_set_source_rgb(cr, r + m, g + m, b + m);
            cairo_paint(cr);

            // Draw book icon/text
            cairo_set_source_rgb(cr, 1, 1, 1);
            cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
            cairo_set_font_size(cr, 14);
            cairo_move_to(cr, 10, 30);
            cairo_show_text(cr, "BOOK");

            cairo_destroy(cr);

            unsigned char *src_data = cairo_image_surface_get_data(surface);
            int src_stride = cairo_image_surface_get_stride(surface);
            if (src_data && src_stride > 0) {
                int out_w = 170;
                int out_h = 220;
                int dst_stride = out_w * 3; /* RGB */
                unsigned char *dst_data = g_malloc(out_h * dst_stride);
                if (dst_data) {
                    for (int y = 0; y < out_h; y++) {
                        const unsigned char *src_row = src_data + y * src_stride;
                        unsigned char *dst_row = dst_data + y * dst_stride;
                        for (int x = 0; x < out_w; x++) {
                            unsigned char b = src_row[x * 4 + 0];
                            unsigned char g = src_row[x * 4 + 1];
                            unsigned char r = src_row[x * 4 + 2];
                            dst_row[x * 3 + 0] = r;
                            dst_row[x * 3 + 1] = g;
                            dst_row[x * 3 + 2] = b;
                        }
                    }
                    void destroy_notify(guchar *pixels, gpointer data) {
                        (void)data;
                        g_free(pixels);
                    }
                    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data(
                        dst_data,
                        GDK_COLORSPACE_RGB,
                        FALSE,
                        8,
                        out_w,
                        out_h,
                        dst_stride,
                        destroy_notify,
                        NULL
                    );
                    if (pixbuf) {
                        gtk_image_set_from_pixbuf(GTK_IMAGE(cover), pixbuf);
                        g_object_unref(pixbuf);
                    } else {
                        g_free(dst_data);
                    }
                }
            }
            cairo_surface_destroy(surface);
        }

        gtk_box_pack_start(GTK_BOX(card_box), cover, FALSE, FALSE, 0);
        
        // Title (truncated)
        char title_text[60];
        snprintf(title_text, sizeof(title_text), "%.55s%s", 
                books[i]->title, strlen(books[i]->title) > 55 ? "..." : "");
        GtkWidget *title_label = gtk_label_new(title_text);
        gtk_label_set_line_wrap(GTK_LABEL(title_label), TRUE);
        gtk_label_set_max_width_chars(GTK_LABEL(title_label), 20);
        gtk_label_set_justify(GTK_LABEL(title_label), GTK_JUSTIFY_CENTER);
        PangoAttrList *attrs = pango_attr_list_new();
        pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
        gtk_label_set_attributes(GTK_LABEL(title_label), attrs);
        pango_attr_list_unref(attrs);
        gtk_box_pack_start(GTK_BOX(card_box), title_label, FALSE, FALSE, 0);
        
        // Author
        if (books[i]->author) {
            char author_text[40];
            snprintf(author_text, sizeof(author_text), "%.35s%s",
                    books[i]->author, strlen(books[i]->author) > 35 ? "..." : "");
            GtkWidget *author_label = gtk_label_new(author_text);
            gtk_label_set_line_wrap(GTK_LABEL(author_label), TRUE);
            gtk_label_set_max_width_chars(GTK_LABEL(author_label), 20);
            gtk_widget_set_opacity(author_label, 0.7);
            gtk_box_pack_start(GTK_BOX(card_box), author_label, FALSE, FALSE, 0);
        }
        
        gtk_container_add(GTK_CONTAINER(card), card_box);
        gtk_container_add(GTK_CONTAINER(view->grid), card);
        
        book_free(books[i]);
    }
    free(books);
    
    gtk_widget_show_all(view->grid);
}

void libraryview_set_callback(LibraryView *view,
                              void (*callback)(int book_id, gpointer data),
                              gpointer user_data) {
    if (!view) return;
    view->on_book_selected = callback;
    view->user_data = user_data;
}

void libraryview_destroy(LibraryView *view) {
    if (!view) return;
    free(view);
}


