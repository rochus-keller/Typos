#ifndef GRAPHIC_CAIRO_H_INCLUDED
#define GRAPHIC_CAIRO_H_INCLUDED

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

#include "config.h"
#include "graphic/graphic.h"

#include <cairo.h>
#include <cairo-ft.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace tex {

class Font_cairo : public Font {
public:
    explicit Font_cairo(std::string family = "", int style = PLAIN, float size = 1.f);
    Font_cairo(const std::string& file, float size);

    std::string getFamily() const;
    int getStyle() const;

    // C API accessors
    cairo_font_face_t* getCairoFontFace() const;
    hb_font_t* getHbFont() const;

    float getSize() const override;
    sptr<Font> deriveFont(int style) const override;

    bool operator==(const Font& f) const override;
    bool operator!=(const Font& f) const override;

    ~Font_cairo() override = default;

private:
    struct Face {
        std::string file;
        std::string family;  
        FT_Face ft_face = nullptr;
        cairo_font_face_t* cairo_face = nullptr;
        hb_font_t* hb_font = nullptr;

        ~Face();
    };

    static FT_Library& ftLibrary(); // init-on-first-use
    static std::shared_ptr<Face> loadFaceForFileAndSize(const std::string& file, float size);

    int _style;
    float _size;          // points/pixels as used by MicroTeX
    std::string _family;  // TODO
    std::string _file;    // if created from a file
    std::shared_ptr<Face> _face; // non-null if created from file
};

class TextLayout_cairo : public TextLayout {
public:
    TextLayout_cairo(const std::wstring& src, const sptr<Font>& font);

    void getBounds(Rect& r) override;
    void draw(Graphics2D& g2, float x, float y) override;

private:
    std::vector<cairo_glyph_t> _glyphs;
    float _ascent = 0.f;
    float _descent = 0.f;
    float _width = 0.f;
    float _height = 0.f;

    // baseline-relative origin assumed at (0,0)
};

class Graphics2D_cairo : public Graphics2D {
public:
    explicit Graphics2D_cairo(cairo_t* cr);

    cairo_t* getCairoContext() const;

    void setColor(color c) override;
    color getColor() const override;

    void setStroke(const Stroke& s) override;
    const Stroke& getStroke() const override;
    void setStrokeWidth(float w) override;

    const Font* getFont() const override;
    void setFont(const Font* font) override;

    void translate(float dx, float dy) override;
    void scale(float sx, float sy) override;
    void rotate(float angle) override;
    void rotate(float angle, float px, float py) override;
    void reset() override;

    float sx() const override;
    float sy() const override;

    void drawChar(wchar_t c, float x, float y) override;
    void drawText(const std::wstring& t, float x, float y) override;

    void drawLine(float x1, float y1, float x2, float y2) override;

    void drawRect(float x, float y, float w, float h) override;
    void fillRect(float x, float y, float w, float h) override;

    void drawRoundRect(float x, float y, float w, float h, float rx, float ry) override;
    void fillRoundRect(float x, float y, float w, float h, float rx, float ry) override;

private:
    static Font_cairo _default_font;

    cairo_t* _cr;
    color _color = BLACK;
    Stroke _stroke;
    const Font_cairo* _font = nullptr;
    float _sx = 1.f, _sy = 1.f;

    void roundRect(float x, float y, float w, float h, float rx, float ry);

    static std::string wstringToUtf8(const std::wstring& w);
    static void shapeUtf8ToCairoGlyphs(const Font_cairo& font,
                                      const std::string& utf8,
                                      std::vector<cairo_glyph_t>& out_glyphs,
                                      double& out_advance_x);

    void applyFontToCairo(const Font_cairo& font);
    friend class TextLayout_cairo;
};

} // namespace tex

#endif
