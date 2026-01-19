#ifndef BOOKNOTE_PDFVIEWER_H
#define BOOKNOTE_PDFVIEWER_H

#include <gtk/gtk.h>
#include <poppler.h>

/**
 * PDF Viewer widget structure
 */
typedef struct {
    GtkWidget *container;       // Main container
    GtkWidget *drawing_area;    // Cairo drawing area
    GtkWidget *page_label;      // "Page X / Y"
    GtkWidget *prev_button;     // Previous page
    GtkWidget *next_button;     // Next page
    GtkWidget *zoom_label;      // "100%"
    
    PopplerDocument *document;  // Current PDF document
    PopplerPage *current_page;  // Current page
    
    int current_page_num;       // Current page number (0-indexed)
    int total_pages;            // Total pages in document
    double zoom_level;          // Zoom level (1.0 = 100%)
    
    char *current_filepath;     // Path to current PDF
} PDFViewer;

/**
 * Create PDF viewer
 */
PDFViewer* pdfviewer_create(void);

/**
 * Load PDF file
 */
gboolean pdfviewer_load_file(PDFViewer *viewer, const char *filepath);

/**
 * Clear viewer (no PDF loaded)
 */
void pdfviewer_clear(PDFViewer *viewer);

/**
 * Navigate pages
 */
void pdfviewer_goto_page(PDFViewer *viewer, int page_num);
void pdfviewer_next_page(PDFViewer *viewer);
void pdfviewer_prev_page(PDFViewer *viewer);

/**
 * Zoom controls
 */
void pdfviewer_zoom_in(PDFViewer *viewer);
void pdfviewer_zoom_out(PDFViewer *viewer);
void pdfviewer_zoom_fit(PDFViewer *viewer);
void pdfviewer_zoom_fit_width(PDFViewer *viewer);

/**
 * Destroy viewer
 */
void pdfviewer_destroy(PDFViewer *viewer);

#endif // BOOKNOTE_PDFVIEWER_H
