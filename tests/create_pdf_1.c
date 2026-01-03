#include <stdio.h>
#include <cairo.h>
#include <cairo-pdf.h>
#include <cairo-ft.h>
#include <ft2build.h>
#include <freetype/freetype.h>

int main() {
    const char *filename = "output.pdf";
    const char *font_path = "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"; // Change as needed

    // 1. Create a PDF surface (A4 size: 595 x 842 points)
    cairo_surface_t *surface = cairo_pdf_surface_create(filename, 595, 842);
    cairo_t *cr = cairo_create(surface);

    // 2. Draw some boxes
    // Red box
    cairo_set_source_rgb(cr, 0.8, 0.2, 0.2);
    cairo_rectangle(cr, 50, 50, 100, 100);
    cairo_fill(cr);

    // Blue outlined box
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.8);
    cairo_set_line_width(cr, 5.0);
    cairo_rectangle(cr, 200, 50, 100, 100);
    cairo_stroke(cr);

    // 3. Load font using FreeType (Required since Fontconfig is disabled)
    FT_Library ft_library;
    FT_Face ft_face;

    if (FT_Init_FreeType(&ft_library)) {
        fprintf(stderr, "Error: Could not initialize FreeType\n");
        return 1;
    }

    if (FT_New_Face(ft_library, font_path, 0, &ft_face)) {
        fprintf(stderr, "Error: Could not load font at %s\n", font_path);
        return 1;
    }

    // 4. Bind FreeType face to Cairo
    cairo_font_face_t *font_face = cairo_ft_font_face_create_for_ft_face(ft_face, 0);
    cairo_set_font_face(cr, font_face);
    cairo_set_font_size(cr, 24.0);

    // 5. Draw text lines
    cairo_set_source_rgb(cr, 0, 0, 0); // Black text
    cairo_move_to(cr, 50, 200);
    cairo_show_text(cr, "Cairo PDF Minimal Build");

    cairo_set_font_size(cr, 14.0);
    cairo_move_to(cr, 50, 230);
    cairo_show_text(cr, "This text was rendered using FreeType directly, bypassing Fontconfig.");

    // 6. Cleanup
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    cairo_font_face_destroy(font_face);
    FT_Done_Face(ft_face);
    FT_Done_FreeType(ft_library);

    printf("Successfully created %s\n", filename);
    return 0;
}
