#include <platform/cairo/graphic_cairo.h>
#include <latex.h>
#include <cairo.h>
#include <cairo-pdf.h>
#include <cairo-ft.h>
#include <ft2build.h>
#include <freetype/freetype.h>
#include <codecvt>
#include <locale>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

static std::wstring utf8_to_wstring(const std::string& utf8_str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(utf8_str);
}

static const std::string f1 = "z^2 = x^2 + y^2";

int main(int argc, const char** argv)
{
    const char *filename = "output.pdf";
    const int _padding = 20;
    const float _text_size = 10.0f; // points
    const int wrap_width = 0; // no wrapping

    std::string code;

    if( argc > 1 )
    {
        std::ifstream file(argv[1]);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << argv[1] << std::endl;
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        code = buffer.str();
    }else
        code = f1;

    tex::LaTeX::init();

    cairo_surface_t *surface = cairo_pdf_surface_create(filename, 595, 842);
    cairo_t *cr = cairo_create(surface);

    tex::TeXRender* _render = tex::LaTeX::parse(
          utf8_to_wstring(code),
          wrap_width,
          _text_size,
          _text_size / 3.f,
          0xff424242);

    tex::Graphics2D_cairo g2(cr);
    _render->draw(g2, _padding, _padding);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    tex::LaTeX::release();
    printf("Successfully created %s\n", filename);
    return 0;
}
