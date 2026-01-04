/*
* Copyright 2026 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the Typos project.
*
* The following is the license that applies to this copy of the
* file. For a license to use the file under conditions
* other than those described here, please email to me@rochus-keller.ch.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

// NOTE: migrated from the original graphic_cairo.h, but with HarfBuzz instead of Pango and no wrappers

#include "graphic_cairo.h"
#include <cassert>
#include <cmath>
#include <codecvt>
#include <locale>
#include <sstream>

namespace tex {

// ---------------- Font_cairo::Face ----------------

Font_cairo::Face::~Face() {
    if (hb_font) {
        hb_font_destroy(hb_font);
        hb_font = nullptr;
    }
    if (cairo_face) {
        cairo_font_face_destroy(cairo_face);
        cairo_face = nullptr;
    }
    if (ft_face) {
        FT_Done_Face(ft_face);
        ft_face = nullptr;
    }
}

// ---------------- Font_cairo ----------------

Font_cairo::Font_cairo(std::string family, int style, float size)
    : _style(style), _size(size), _family(std::move(family)) {}

Font_cairo::Font_cairo(const std::string& file, float size)
    : _style(PLAIN), _size(size), _file(file) {
    _face = loadFaceForFileAndSize(file, size);
    _family = _face ? _face->family : "";
}

std::string Font_cairo::getFamily() const { return _family; }
int Font_cairo::getStyle() const { return _style; }
float Font_cairo::getSize() const { return _size; }

cairo_font_face_t* Font_cairo::getCairoFontFace() const {
    return _face ? _face->cairo_face : nullptr;
}

hb_font_t* Font_cairo::getHbFont() const {
    return _face ? _face->hb_font : nullptr;
}

sptr<Font> Font_cairo::deriveFont(int style) const {
    // Keep same file if file-based; TODO: family→file mapping.
    if (!_file.empty())
        return sptr<Font>(new Font_cairo(_file, _size)); // style ignored for file fonts
    return sptr<Font>(new Font_cairo(_family, style, _size));
}

bool Font_cairo::operator==(const Font& f) const {
    const Font_cairo& o = static_cast<const Font_cairo&>(f);
    return _size == o._size && _style == o._style && _family == o._family && _file == o._file;
}

bool Font_cairo::operator!=(const Font& f) const { return !(*this == f); }

FT_Library& Font_cairo::ftLibrary() {
    static FT_Library lib = nullptr;
    static bool inited = false;
    if (!inited) {
        if (FT_Init_FreeType(&lib) != 0) {
            lib = nullptr;
        }
        inited = true;
    }
    return lib;
}

static std::string sizeKey(float size) {
    // 26.6 fixed point key is stable across float formatting noise
    int s = (int)std::lround(size * 64.0);
    std::ostringstream os;
    os << s;
    return os.str();
}

std::shared_ptr<Font_cairo::Face>
Font_cairo::loadFaceForFileAndSize(const std::string& file, float size) {
    struct Cache {
        std::map<std::string, std::weak_ptr<Face>> faces;
    };
    static Cache cache;

    const std::string key = file + "|" + sizeKey(size);

    auto it = cache.faces.find(key);
    if (it != cache.faces.end()) {
        if (auto sp = it->second.lock())
            return sp;
    }

    FT_Library& lib = ftLibrary();
    if (!lib) return nullptr;

    std::shared_ptr<Face> face(new Face());
    face->file = file;

    if (FT_New_Face(lib, file.c_str(), 0, &face->ft_face) != 0) {
        return nullptr;
    }

    // Set size for HarfBuzz/FT metrics. 72 dpi is typical for "points".
    // Adjust DPI if your coordinate system is different.
    FT_Set_Char_Size(face->ft_face, 0, (FT_F26Dot6)std::lround(size * 64.0), 72, 72);

    // Cairo font face from FT_Face
    face->cairo_face = cairo_ft_font_face_create_for_ft_face(face->ft_face, 0);

    // HarfBuzz font from FT_Face
    face->hb_font = hb_ft_font_create_referenced(face->ft_face);

    // Ensure HB uses same scale (26.6)
    hb_font_set_scale(face->hb_font,
                      (int)std::lround(size * 64.0),
                      (int)std::lround(size * 64.0));

    // Optional: store family name from FT (not required)
    if (face->ft_face->family_name)
        face->family = face->ft_face->family_name;

    cache.faces[key] = face;
    return face;
}

// bridge methods required by tex::Font
Font* Font::create(const std::string& file, float size) {
    return new Font_cairo(file, size);
}
sptr<Font> Font::_create(const std::string& name, int style, float size) {
    return sptr<Font>(new Font_cairo(name, style, size));
}

// ---------------- TextLayout_cairo (HarfBuzz) ----------------

TextLayout_cairo::TextLayout_cairo(const std::wstring& src, const sptr<Font> &font) {
    auto f = std::static_pointer_cast<Font_cairo>(font);
    if (!f || !f->getHbFont() || !f->getCairoFontFace()) {
        // Degenerate: no shaping possible.
        _width = 0; _height = 0; _ascent = 0; _descent = 0;
        return;
    }

    // Shape into cairo_glyph_t
    std::vector<cairo_glyph_t> glyphs;
    double advance_x = 0;

    // Use helper from Graphics2D_cairo to avoid duplication
    // (Could be factored out into a shared free function.)
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    std::string utf8 = conv.to_bytes(src);

    Graphics2D_cairo::shapeUtf8ToCairoGlyphs(*f, utf8, glyphs, advance_x);
    _glyphs.swap(glyphs);
    _width = (float)advance_x;

    // Metrics: use Cairo’s font extents for baseline metrics
    cairo_surface_t* tmp_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* tmp_cr = cairo_create(tmp_surf);

    cairo_set_font_face(tmp_cr, f->getCairoFontFace());
    cairo_set_font_size(tmp_cr, f->getSize());

    cairo_font_extents_t fe;
    cairo_font_extents(tmp_cr, &fe);
    _ascent  = (float)fe.ascent;
    _descent = (float)fe.descent;
    _height  = (float)fe.height;

    cairo_destroy(tmp_cr);
    cairo_surface_destroy(tmp_surf);
}

void TextLayout_cairo::getBounds(Rect& r) {
    r.x = 0;
    r.y = -_ascent;
    r.w = _width;
    r.h = _ascent + _descent; // matches baseline notion used in your old code
}

void TextLayout_cairo::draw(Graphics2D& g2, float x, float y) {
    auto& g = static_cast<Graphics2D_cairo&>(g2);
    cairo_t* cr = g.getCairoContext();

    cairo_save(cr);
    cairo_translate(cr, x, y);

    if (!_glyphs.empty())
        cairo_show_glyphs(cr, _glyphs.data(), (int)_glyphs.size());

    cairo_restore(cr);
}

sptr<TextLayout> TextLayout::create(const std::wstring& src, const sptr<Font>& font) {
    return sptr<TextLayout>(new TextLayout_cairo(src, font));
}

// ---------------- Graphics2D_cairo ----------------

Font_cairo Graphics2D_cairo::_default_font("", PLAIN, 20.f);

Graphics2D_cairo::Graphics2D_cairo(cairo_t* cr) : _cr(cr) {
    _stroke = Stroke();
    setColor(BLACK);
    setStroke(_stroke);
    setFont(&_default_font);
}

cairo_t* Graphics2D_cairo::getCairoContext() const { return _cr; }

void Graphics2D_cairo::setColor(color c) {
    _color = c;
    const double a = color_a(c) / 255.0;
    const double r = color_r(c) / 255.0;
    const double g = color_g(c) / 255.0;
    const double b = color_b(c) / 255.0;
    cairo_set_source_rgba(_cr, r, g, b, a);
}

color Graphics2D_cairo::getColor() const { return _color; }

void Graphics2D_cairo::setStroke(const Stroke& s) {
    _stroke = s;
    cairo_set_line_width(_cr, s.lineWidth);

    cairo_line_cap_t cap = CAIRO_LINE_CAP_ROUND;
    switch (s.cap) {
        case CAP_BUTT:   cap = CAIRO_LINE_CAP_BUTT; break;
        case CAP_ROUND:  cap = CAIRO_LINE_CAP_ROUND; break;
        case CAP_SQUARE: cap = CAIRO_LINE_CAP_SQUARE; break;
    }
    cairo_set_line_cap(_cr, cap);

    cairo_line_join_t join = CAIRO_LINE_JOIN_ROUND;
    switch (s.join) {
        case JOIN_BEVEL: join = CAIRO_LINE_JOIN_BEVEL; break;
        case JOIN_ROUND: join = CAIRO_LINE_JOIN_ROUND; break;
        case JOIN_MITER: join = CAIRO_LINE_JOIN_MITER; break;
    }
    cairo_set_line_join(_cr, join);
    cairo_set_miter_limit(_cr, s.miterLimit);
}

const Stroke& Graphics2D_cairo::getStroke() const { return _stroke; }

void Graphics2D_cairo::setStrokeWidth(float w) {
    _stroke.lineWidth = w;
    cairo_set_line_width(_cr, w);
}

const Font* Graphics2D_cairo::getFont() const { return _font; }
void Graphics2D_cairo::setFont(const Font* font) { _font = static_cast<const Font_cairo*>(font); }

void Graphics2D_cairo::translate(float dx, float dy) { cairo_translate(_cr, dx, dy); }
void Graphics2D_cairo::scale(float sx, float sy) { _sx *= sx; _sy *= sy; cairo_scale(_cr, sx, sy); }
void Graphics2D_cairo::rotate(float angle) { cairo_rotate(_cr, angle); }

void Graphics2D_cairo::rotate(float angle, float px, float py) {
    cairo_translate(_cr, px, py);
    cairo_rotate(_cr, angle);
    cairo_translate(_cr, -px, -py);
}

void Graphics2D_cairo::reset() {
    cairo_identity_matrix(_cr);
    _sx = _sy = 1.f;
}

float Graphics2D_cairo::sx() const { return _sx; }
float Graphics2D_cairo::sy() const { return _sy; }

void Graphics2D_cairo::drawChar(wchar_t c, float x, float y) {
    std::wstring s;
    s.push_back(c);
    drawText(s, x, y);
}

std::string Graphics2D_cairo::wstringToUtf8(const std::wstring& w) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(w);
}

void Graphics2D_cairo::applyFontToCairo(const Font_cairo& font) {
    if (auto face = font.getCairoFontFace())
        cairo_set_font_face(_cr, face);
    cairo_set_font_size(_cr, font.getSize());
}

// Core shaping helper: HarfBuzz -> cairo_glyph_t[]
void Graphics2D_cairo::shapeUtf8ToCairoGlyphs(const Font_cairo& font,
                                              const std::string& utf8,
                                              std::vector<cairo_glyph_t>& out_glyphs,
                                              double& out_advance_x) {
    out_glyphs.clear();
    out_advance_x = 0.0;

    hb_font_t* hb_font = font.getHbFont();
    if (!hb_font) return;

    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8(buf, utf8.c_str(), (int)utf8.size(), 0, (int)utf8.size());
    hb_buffer_guess_segment_properties(buf);

    hb_shape(hb_font, buf, nullptr, 0);

    unsigned int count = 0;
    hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buf, &count);
    hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buf, &count);

    out_glyphs.resize(count);

    double x = 0.0;
    double y = 0.0;

    for (unsigned int i = 0; i < count; i++) {
        // HarfBuzz positions are 26.6; convert to double user units.
        const double x_offset  = pos[i].x_offset  / 64.0;
        const double y_offset  = pos[i].y_offset  / 64.0;
        const double x_advance = pos[i].x_advance / 64.0;
        const double y_advance = pos[i].y_advance / 64.0;

        cairo_glyph_t& g = out_glyphs[i];
        g.index = info[i].codepoint;
        g.x = x + x_offset;
        g.y = y - y_offset; // Cairo Y axis grows downward; HB offsets are typically upward-positive

        x += x_advance;
        y += y_advance;
    }

    out_advance_x = x;
    hb_buffer_destroy(buf);
}

void Graphics2D_cairo::drawText(const std::wstring& t, float x, float y) {
    if (!_font) return;

    // If we have HB+FT face, render glyphs; otherwise fall back to cairo_show_text.
    if (_font->getHbFont() && _font->getCairoFontFace()) {
        applyFontToCairo(*_font);

        std::vector<cairo_glyph_t> glyphs;
        double advance_x = 0.0;
        const std::string utf8 = wstringToUtf8(t);
        shapeUtf8ToCairoGlyphs(*_font, utf8, glyphs, advance_x);

        cairo_save(_cr);
        cairo_translate(_cr, x, y);
        if (!glyphs.empty())
            cairo_show_glyphs(_cr, glyphs.data(), (int)glyphs.size());
        cairo_restore(_cr);
        return;
    }

    // Fallback TODO
    cairo_move_to(_cr, x, y);
    const std::string utf8 = wstringToUtf8(t);
    cairo_show_text(_cr, utf8.c_str());
}

void Graphics2D_cairo::drawLine(float x1, float y1, float x2, float y2) {
    cairo_move_to(_cr, x1, y1);
    cairo_line_to(_cr, x2, y2);
    cairo_stroke(_cr);
}

void Graphics2D_cairo::drawRect(float x, float y, float w, float h) {
    cairo_rectangle(_cr, x, y, w, h);
    cairo_stroke(_cr);
}

void Graphics2D_cairo::fillRect(float x, float y, float w, float h) {
    cairo_rectangle(_cr, x, y, w, h);
    cairo_fill(_cr);
}

void Graphics2D_cairo::roundRect(float x, float y, float w, float h, float rx, float ry) {
    // Basic rounded rect (elliptical arcs)
    const double x0 = x, y0 = y, x1 = x + w, y1 = y + h;
    const double rrx = std::min<double>(rx, w * 0.5);
    const double rry = std::min<double>(ry, h * 0.5);

    cairo_new_sub_path(_cr);
    cairo_arc(_cr, x1 - rrx, y0 + rry, std::min(rrx, rry), -M_PI_2, 0);
    cairo_arc(_cr, x1 - rrx, y1 - rry, std::min(rrx, rry), 0, M_PI_2);
    cairo_arc(_cr, x0 + rrx, y1 - rry, std::min(rrx, rry), M_PI_2, M_PI);
    cairo_arc(_cr, x0 + rrx, y0 + rry, std::min(rrx, rry), M_PI, 3 * M_PI_2);
    cairo_close_path(_cr);
}

void Graphics2D_cairo::drawRoundRect(float x, float y, float w, float h, float rx, float ry) {
    roundRect(x, y, w, h, rx, ry);
    cairo_stroke(_cr);
}

void Graphics2D_cairo::fillRoundRect(float x, float y, float w, float h, float rx, float ry) {
    roundRect(x, y, w, h, rx, ry);
    cairo_fill(_cr);
}

} // namespace tex
