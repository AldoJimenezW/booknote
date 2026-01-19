#include "pdfviewer.h"
#include <stdlib.h>
#include <string.h>

static void render_page(PDFViewer *viewer);
static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
static void on_prev_clicked(GtkWidget *widget, gpointer data);
static void on_next_clicked(GtkWidget *widget, gpointer data);
static void update_controls(PDFViewer *viewer);

PDFViewer* pdfviewer_create(void) {
    PDFViewer *viewer = calloc(1, sizeof(PDFViewer));
    if (!viewer) return NULL;
    
    viewer->current_page_num = 0;
    viewer->total_pages = 0;
    viewer->zoom_level = 1.0;
    viewer->document = NULL;
    viewer->current_page = NULL;
    viewer->current_filepath = NULL;
    
    // Main container
    viewer->container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    // Drawing area for PDF rendering
    viewer->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(viewer->drawing_area, 600, 800);
    g_signal_connect(viewer->drawing_area, "draw", G_CALLBACK(on_draw), viewer);
    
    // Scrolled window
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), viewer->drawing_area);
    gtk_box_pack_start(GTK_BOX(viewer->container), scrolled, TRUE, TRUE, 0);
    
    // Navigation controls
    GtkWidget *nav_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_margin_start(nav_box, 5);
    gtk_widget_set_margin_end(nav_box, 5);
    gtk_widget_set_margin_top(nav_box, 5);
    gtk_widget_set_margin_bottom(nav_box, 5);
    
    // Previous button
    viewer->prev_button = gtk_button_new_with_label("◀");
    gtk_widget_set_sensitive(viewer->prev_button, FALSE);
    g_signal_connect(viewer->prev_button, "clicked", G_CALLBACK(on_prev_clicked), viewer);
    gtk_box_pack_start(GTK_BOX(nav_box), viewer->prev_button, FALSE, FALSE, 0);
    
    // Page label
    viewer->page_label = gtk_label_new("No PDF loaded");
    gtk_box_pack_start(GTK_BOX(nav_box), viewer->page_label, TRUE, FALSE, 0);
    
    // Next button
    viewer->next_button = gtk_button_new_with_label("▶");
    gtk_widget_set_sensitive(viewer->next_button, FALSE);
    g_signal_connect(viewer->next_button, "clicked", G_CALLBACK(on_next_clicked), viewer);
    gtk_box_pack_start(GTK_BOX(nav_box), viewer->next_button, FALSE, FALSE, 0);
    
    // Spacer
    gtk_box_pack_start(GTK_BOX(nav_box), gtk_separator_new(GTK_ORIENTATION_VERTICAL), 
                      FALSE, FALSE, 10);
    
    // Zoom out button
    GtkWidget *zoom_out_btn = gtk_button_new_with_label("−");
    g_signal_connect_swapped(zoom_out_btn, "clicked", G_CALLBACK(pdfviewer_zoom_out), viewer);
    gtk_box_pack_start(GTK_BOX(nav_box), zoom_out_btn, FALSE, FALSE, 0);
    
    // Zoom label
    viewer->zoom_label = gtk_label_new("100%");
    gtk_widget_set_size_request(viewer->zoom_label, 60, -1);
    gtk_box_pack_start(GTK_BOX(nav_box), viewer->zoom_label, FALSE, FALSE, 5);
    
    // Zoom in button
    GtkWidget *zoom_in_btn = gtk_button_new_with_label("+");
    g_signal_connect_swapped(zoom_in_btn, "clicked", G_CALLBACK(pdfviewer_zoom_in), viewer);
    gtk_box_pack_start(GTK_BOX(nav_box), zoom_in_btn, FALSE, FALSE, 0);
    
    // Fit width button
    GtkWidget *fit_btn = gtk_button_new_with_label("Fit");
    g_signal_connect_swapped(fit_btn, "clicked", G_CALLBACK(pdfviewer_zoom_fit_width), viewer);
    gtk_box_pack_start(GTK_BOX(nav_box), fit_btn, FALSE, FALSE, 5);
    
    gtk_box_pack_start(GTK_BOX(viewer->container), nav_box, FALSE, FALSE, 0);

    return viewer;
}

gboolean pdfviewer_load_file(PDFViewer *viewer, const char *filepath) {
    if (!viewer || !filepath) return FALSE;
    
    // Close previous document
    if (viewer->current_page) {
        g_object_unref(viewer->current_page);
        viewer->current_page = NULL;
    }
    if (viewer->document) {
        g_object_unref(viewer->document);
        viewer->document = NULL;
    }
    g_free(viewer->current_filepath);
    viewer->current_filepath = NULL;
    
    // Build file URI
    char *uri;
    if (g_path_is_absolute(filepath)) {
        uri = g_filename_to_uri(filepath, NULL, NULL);
    } else {
        char *absolute = g_build_filename(g_get_current_dir(), filepath, NULL);
        uri = g_filename_to_uri(absolute, NULL, NULL);
        g_free(absolute);
    }
    
    if (!uri) return FALSE;
    
    // Load document
    GError *error = NULL;
    viewer->document = poppler_document_new_from_file(uri, NULL, &error);
    g_free(uri);
    
    if (error) {
        g_error_free(error);
        return FALSE;
    }
    
    if (!viewer->document) return FALSE;
    
    // Get document info
    viewer->total_pages = poppler_document_get_n_pages(viewer->document);
    viewer->current_page_num = 0;
    viewer->current_filepath = g_strdup(filepath);
    
    // Load first page
    viewer->current_page = poppler_document_get_page(viewer->document, 0);
    if (!viewer->current_page) {
        g_object_unref(viewer->document);
        viewer->document = NULL;
        return FALSE;
    }
    
    // Update UI
    update_controls(viewer);
    // Fit to width by default
    pdfviewer_zoom_fit_width(viewer);
    render_page(viewer);
    
    return TRUE;
}

void pdfviewer_clear(PDFViewer *viewer) {
    if (!viewer) return;
    
    if (viewer->current_page) {
        g_object_unref(viewer->current_page);
        viewer->current_page = NULL;
    }
    if (viewer->document) {
        g_object_unref(viewer->document);
        viewer->document = NULL;
    }
    g_free(viewer->current_filepath);
    viewer->current_filepath = NULL;
    
    viewer->current_page_num = 0;
    viewer->total_pages = 0;
    
    gtk_label_set_text(GTK_LABEL(viewer->page_label), "No PDF loaded");
    gtk_widget_set_sensitive(viewer->prev_button, FALSE);
    gtk_widget_set_sensitive(viewer->next_button, FALSE);
    
    gtk_widget_queue_draw(viewer->drawing_area);
}

void pdfviewer_goto_page(PDFViewer *viewer, int page_num) {
    if (!viewer || !viewer->document) return;
    if (page_num < 0 || page_num >= viewer->total_pages) return;
    
    if (viewer->current_page) {
        g_object_unref(viewer->current_page);
    }
    
    viewer->current_page = poppler_document_get_page(viewer->document, page_num);
    if (!viewer->current_page) return;
    
    viewer->current_page_num = page_num;
    update_controls(viewer);
    render_page(viewer);
}

void pdfviewer_next_page(PDFViewer *viewer) {
    if (!viewer || !viewer->document) return;
    if (viewer->current_page_num < viewer->total_pages - 1) {
        pdfviewer_goto_page(viewer, viewer->current_page_num + 1);
    }
}

void pdfviewer_prev_page(PDFViewer *viewer) {
    if (!viewer || !viewer->document) return;
    if (viewer->current_page_num > 0) {
        pdfviewer_goto_page(viewer, viewer->current_page_num - 1);
    }
}

void pdfviewer_zoom_in(PDFViewer *viewer) {
    if (!viewer) return;
    viewer->zoom_level *= 1.2;
    if (viewer->zoom_level > 3.0) viewer->zoom_level = 3.0;
    
    char zoom_text[32];
    snprintf(zoom_text, sizeof(zoom_text), "%.0f%%", viewer->zoom_level * 100);
    gtk_label_set_text(GTK_LABEL(viewer->zoom_label), zoom_text);
    
    render_page(viewer);
}

void pdfviewer_zoom_out(PDFViewer *viewer) {
    if (!viewer) return;
    viewer->zoom_level /= 1.2;
    if (viewer->zoom_level < 0.3) viewer->zoom_level = 0.3;
    
    char zoom_text[32];
    snprintf(zoom_text, sizeof(zoom_text), "%.0f%%", viewer->zoom_level * 100);
    gtk_label_set_text(GTK_LABEL(viewer->zoom_label), zoom_text);
    
    render_page(viewer);
}

void pdfviewer_zoom_fit(PDFViewer *viewer) {
    if (!viewer) return;
    viewer->zoom_level = 1.0;
    gtk_label_set_text(GTK_LABEL(viewer->zoom_label), "100%");
    render_page(viewer);
}

void pdfviewer_zoom_fit_width(PDFViewer *viewer) {
    if (!viewer || !viewer->current_page) return;
    
    // Get page dimensions
    double page_width, page_height;
    poppler_page_get_size(viewer->current_page, &page_width, &page_height);
    
    // Get viewport width
    GtkAllocation alloc;
    gtk_widget_get_allocation(viewer->drawing_area, &alloc);
    int viewport_width = alloc.width;
    
    if (viewport_width > 100) { // Sanity check
        viewer->zoom_level = (viewport_width - 40.0) / page_width; // 40px margins
    } else {
        viewer->zoom_level = 1.0;
    }
    
    char zoom_text[32];
    snprintf(zoom_text, sizeof(zoom_text), "%.0f%%", viewer->zoom_level * 100);
    gtk_label_set_text(GTK_LABEL(viewer->zoom_label), zoom_text);
    
    render_page(viewer);
}

void pdfviewer_destroy(PDFViewer *viewer) {
    if (!viewer) return;
    
    if (viewer->current_page) {
        g_object_unref(viewer->current_page);
    }
    if (viewer->document) {
        g_object_unref(viewer->document);
    }
    g_free(viewer->current_filepath);
    free(viewer);
}

static void render_page(PDFViewer *viewer) {
    if (!viewer || !viewer->current_page) return;
    
    // Get page dimensions
    double width, height;
    poppler_page_get_size(viewer->current_page, &width, &height);
    
    // Apply zoom
    width *= viewer->zoom_level;
    height *= viewer->zoom_level;
    
    // Update drawing area size
    gtk_widget_set_size_request(viewer->drawing_area, (int)width, (int)height);
    gtk_widget_queue_draw(viewer->drawing_area);
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    PDFViewer *viewer = (PDFViewer *)data;
    
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    
    if (!viewer->current_page) {
        // Draw placeholder
        cairo_set_source_rgb(cr, 0.95, 0.95, 0.95);
        cairo_paint(cr);
        
        cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 16);
        
        cairo_text_extents_t extents;
        const char *text = "Select a book to view PDF";
        cairo_text_extents(cr, text, &extents);
        
        cairo_move_to(cr, (alloc.width - extents.width) / 2, (alloc.height - extents.height) / 2);
        cairo_show_text(cr, text);
        
        return TRUE;
    }
    
    // Get page dimensions
    double page_width, page_height;
    poppler_page_get_size(viewer->current_page, &page_width, &page_height);
    
    // Calculate scaled dimensions
    double scaled_width = page_width * viewer->zoom_level;
    double scaled_height = page_height * viewer->zoom_level;
    
    // Center the PDF
    double x_offset = (alloc.width - scaled_width) / 2.0;
    double y_offset = (alloc.height - scaled_height) / 2.0;
    
    // Ensure non-negative offsets
    if (x_offset < 0) x_offset = 0;
    if (y_offset < 0) y_offset = 0;
    
    // Background
    cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
    cairo_paint(cr);
    
    // Translate to center position
    cairo_translate(cr, x_offset, y_offset);
    
    // White page background
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, 0, 0, scaled_width, scaled_height);
    cairo_fill(cr);
    
    // Page shadow
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.3);
    cairo_rectangle(cr, 5, 5, scaled_width, scaled_height);
    cairo_fill(cr);
    
    // White background again on top of shadow
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, 0, 0, scaled_width, scaled_height);
    cairo_fill(cr);
    
    // Scale for zoom and render
    cairo_scale(cr, viewer->zoom_level, viewer->zoom_level);
    poppler_page_render(viewer->current_page, cr);
    
    return TRUE;
}

static void on_prev_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    pdfviewer_prev_page((PDFViewer *)data);
}

static void on_next_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    pdfviewer_next_page((PDFViewer *)data);
}

static void update_controls(PDFViewer *viewer) {
    if (!viewer) return;
    
    // Update page label
    char label[64];
    snprintf(label, sizeof(label), "Page %d / %d", 
             viewer->current_page_num + 1, viewer->total_pages);
    gtk_label_set_text(GTK_LABEL(viewer->page_label), label);
    
    // Update button states
    gtk_widget_set_sensitive(viewer->prev_button, viewer->current_page_num > 0);
    gtk_widget_set_sensitive(viewer->next_button, 
                            viewer->current_page_num < viewer->total_pages - 1);
}
