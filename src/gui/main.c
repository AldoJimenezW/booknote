#include <gtk/gtk.h>
#include <stdio.h>
#include "../database/db.h"

#define APP_TITLE "booknote GUI v0.2-dev"
#define DEFAULT_WIDTH 1200
#define DEFAULT_HEIGHT 800

typedef struct {
    Database *db;
    GtkWidget *window;
} AppState;

static void on_window_destroy(GtkWidget *widget, gpointer data) {
    (void)widget;
    AppState *app = (AppState *)data;
    
    if (app->db) {
        db_close(app->db);
    }
    
    gtk_main_quit();
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    (void)widget;
    (void)data;
    
    // Ctrl+Q to quit
    if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_q) {
        gtk_main_quit();
        return TRUE;
    }
    
    return FALSE;
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    
    // Initialize app state
    AppState app = {0};
    
    // Open database
    BnError err = db_open(&app.db, NULL);
    if (err != BN_SUCCESS) {
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Failed to open database"
        );
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return 1;
    }
    
    // Create main window
    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), APP_TITLE);
    gtk_window_set_default_size(GTK_WINDOW(app.window), DEFAULT_WIDTH, DEFAULT_HEIGHT);
    gtk_window_set_position(GTK_WINDOW(app.window), GTK_WIN_POS_CENTER);
    
    // Connect signals
    g_signal_connect(app.window, "destroy", G_CALLBACK(on_window_destroy), &app);
    g_signal_connect(app.window, "key-press-event", G_CALLBACK(on_key_press), &app);
    
    // Create simple layout for now
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app.window), vbox);
    
    // Header label
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), 
        "<span size='large' weight='bold'>booknote</span>\n"
        "<span size='small'>GUI Development Preview</span>");
    gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 20);
    
    // Info label
    GtkWidget *info = gtk_label_new(
        "Database connected successfully!\n\n"
        "This is the foundation for the GUI version.\n"
        "Press Ctrl+Q to quit."
    );
    gtk_box_pack_start(GTK_BOX(vbox), info, TRUE, TRUE, 20);
    
    // Show all widgets
    gtk_widget_show_all(app.window);
    
    printf("booknote GUI started\n");
    printf("Database: %s\n", app.db->path);
    printf("Press Ctrl+Q to quit\n");
    
    // Run GTK main loop
    gtk_main();
    
    return 0;
}
