#include <gtk/gtk.h>
#include <stdio.h>
#include "window.h"
#include "../utils/error.h"

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);

    // Load CSS stylesheet
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "src/gui/style.css", NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    
    // Open database
    Database *db = NULL;
    BnError err = db_open(&db, NULL);
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
    
    printf("booknote GUI started\n");
    printf("Database: %s\n", db->path);
    printf("\nKeyboard shortcuts:\n");
    printf("  Ctrl+Q - Quit\n");
    printf("  Ctrl+B - Toggle notes panel\n");
    printf("  Ctrl+L - Show library view\n");
    
    // Create main window
    MainWindow *win = window_create(db);
    if (!win) {
        fprintf(stderr, "Failed to create window\n");
        db_close(db);
        return 1;
    }
    
    // Show window
    gtk_widget_show_all(win->window);
    
    // Run main loop
    gtk_main();
    
    // Cleanup
    window_destroy(win);
    db_close(db);
    
    return 0;
}
