#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtCore/QCommandLineParser>
#include <QtCore/QVector>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include <cairo.h>
#include <cairo-ft.h>
#include <cairo-pdf.h>

#include <cmath>
#include <limits>

// NOTE: this version assumes a text which has either no \n or at least no double \n\n. The latter would
// mean a hard paragraph break, but this algorithm currently only supports a single paragraph, even if the
// text can flow to more than one page.


static double pt(double v) { return v; } // Cairo PDF uses points (user units).

struct ShapedWord {
    QString text;
    QVector<hb_codepoint_t> gid;
    QVector<hb_position_t> xoff26_6;
    QVector<hb_position_t> yoff26_6;
    QVector<hb_position_t> xadv26_6;
    hb_position_t width26_6 = 0;
};

struct LineBreak {
    int startWord = 0; // inclusive
    int endWord   = 0; // exclusive
    double ratio  = 0.0;
};

static QByteArray readAllUtf8(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QTextStream(stderr) << "Cannot open input: " << path << "\n";
        return {};
    }
    return f.readAll();
}

static QVector<QString> splitParagraphs(const QString& s) {
    // Treat empty lines as paragraph breaks.
    QString normalized = s;
    normalized.replace("\r\n", "\n");
    normalized.replace("\r", "\n");
#if 1
    normalized.replace("\n\n", "\n");
    return QVector<QString>() << normalized;
#else
    // not supported yet; we need something like \parfillskip, which requires an explicit item stream
    QStringList parts = normalized.split("\n\n", QString::KeepEmptyParts);
    QVector<QString> out;
    out.reserve(parts.size());
    for (const QString& p : parts) {
        QString t = p.trimmed();
        if (!t.isEmpty())
            out.push_back(t);
    }
    return out;
#endif
}

static QStringList splitWordsSimple(const QString& para) {
    // Minimal: collapse all whitespace to single separators.
    return para.split(QRegExp("\\s+"), QString::SkipEmptyParts);
}

static ShapedWord shapeWord(hb_font_t* hbFont, const QString& wordUtf16) {
    ShapedWord w;
    w.text = wordUtf16;

    hb_buffer_t* buf = hb_buffer_create();

    QByteArray utf8 = wordUtf16.toUtf8();
    hb_buffer_add_utf8(buf, utf8.constData(), utf8.size(), 0, utf8.size());
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_guess_segment_properties(buf);

    hb_shape(hbFont, buf, nullptr, 0);

    unsigned int n = 0;
    hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buf, &n);
    hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buf, &n);

    w.gid.resize(int(n));
    w.xoff26_6.resize(int(n));
    w.yoff26_6.resize(int(n));
    w.xadv26_6.resize(int(n));

    hb_position_t penX = 0;
    for (unsigned int i = 0; i < n; ++i) {
        w.gid[int(i)]     = infos[i].codepoint;      // GID after shaping
        w.xoff26_6[int(i)] = penX + pos[i].x_offset;
        w.yoff26_6[int(i)] = pos[i].y_offset;
        w.xadv26_6[int(i)] = pos[i].x_advance;
        penX += pos[i].x_advance;
    }
    w.width26_6 = penX;

    hb_buffer_destroy(buf);
    return w;
}

struct ParaLayoutData {
    QVector<ShapedWord> words;
    hb_position_t spaceW26_6 = 0;
    hb_position_t spaceStretch26_6 = 0;
    hb_position_t spaceShrink26_6 = 0;
};

static ParaLayoutData prepareParagraph(hb_font_t* hbFont, const QString& para) {
    ParaLayoutData d;
    QStringList words = splitWordsSimple(para);
    d.words.reserve(words.size());
    for (const QString& w : words)
        d.words.push_back(shapeWord(hbFont, w));

    // Measure a single space from shaping (v0).
    ShapedWord sp = shapeWord(hbFont, " ");
    d.spaceW26_6 = sp.width26_6;
    d.spaceStretch26_6 = d.spaceW26_6 / 3; // heuristic
    d.spaceShrink26_6  = d.spaceW26_6 / 5; // heuristic
    return d;
}

static double badnessFromRatio(double r) {
    // TeX-like: badness ~ (100*|r|)^3, but keep numbers reasonable.
    double ar = std::abs(r);
    if (ar > 10.0) return 1e9;
    double x = 100.0 * ar;
    return x * x * x;
}

static bool lineFeasible(hb_position_t lineW26_6,
                         hb_position_t naturalW26_6,
                         hb_position_t stretch26_6,
                         hb_position_t shrink26_6,
                         double& outRatio)
{
    hb_position_t diff = lineW26_6 - naturalW26_6;
    if (diff == 0) { outRatio = 0.0; return true; }

    if (diff > 0) {
        if (stretch26_6 == 0) return false;
        outRatio = double(diff) / double(stretch26_6);
        return outRatio <= 10.0;
    } else {
        if (shrink26_6 == 0) return false;
        outRatio = double(diff) / double(shrink26_6); // negative
        return outRatio >= -1.0;
    }
}

static QVector<LineBreak> breakKnuthPlassWords(const ParaLayoutData& p, double lineWidthPt)
{
    const int n = p.words.size();
    const hb_position_t lineW26_6 = hb_position_t(std::llround(lineWidthPt * 64.0));

    QVector<double> best(n + 1, std::numeric_limits<double>::infinity());
    QVector<int> prev(n + 1, -1);
    QVector<double> bestRatio(n + 1, 0.0);

    best[0] = 0.0;

    for (int j = 1; j <= n; ++j) {
        hb_position_t wordsW = 0;
        for (int i = j - 1; i >= 0; --i) {
            wordsW += p.words[i].width26_6;
            int spaces = (j - i - 1);
            hb_position_t natural = wordsW + hb_position_t(spaces) * p.spaceW26_6;
            hb_position_t stretch = hb_position_t(spaces) * p.spaceStretch26_6;
            hb_position_t shrink  = hb_position_t(spaces) * p.spaceShrink26_6;

            double ratio = 0.0;
            if (!lineFeasible(lineW26_6, natural, stretch, shrink, ratio))
                continue;

            double b = badnessFromRatio(ratio);
            double cost = best[i] + b * b; // simplistic demerits

            if (cost < best[j]) {
                best[j] = cost;
                prev[j] = i;
                bestRatio[j] = ratio;
            }
        }
    }

    QVector<LineBreak> lines;
    if (prev[n] < 0) {
        // Fallback: single line
        LineBreak lb;
        lb.startWord = 0;
        lb.endWord = n;
        lb.ratio = 0.0;
        lines.push_back(lb);
        return lines;
    }

    // Reconstruct breaks
    int j = n;
    while (j > 0) {
        int i = prev[j];
        LineBreak lb;
        lb.startWord = i;
        lb.endWord = j;
        lb.ratio = bestRatio[j];
        lines.push_back(lb);
        j = i;
    }
    std::reverse(lines.begin(), lines.end());
    return lines;
}

static cairo_scaled_font_t* makeCairoScaledFont(FT_Face face, double sizePt) {
    cairo_font_face_t* cff = cairo_ft_font_face_create_for_ft_face(face, 0);

    cairo_matrix_t fontMatrix;
    cairo_matrix_init_scale(&fontMatrix, sizePt, sizePt);

    cairo_matrix_t ctm;
    cairo_matrix_init_identity(&ctm);

    cairo_font_options_t* opts = cairo_font_options_create();
    cairo_scaled_font_t* sf = cairo_scaled_font_create(cff, &fontMatrix, &ctm, opts);

    cairo_font_options_destroy(opts);
    cairo_font_face_destroy(cff);
    return sf;
}

static void drawParagraph(cairo_t* cr,
                          cairo_scaled_font_t* sf,
                          const ParaLayoutData& p,
                          const QVector<LineBreak>& lines,
                          double& cursorY,
                          double pageW, double pageH,
                          double marginL, double marginT, double marginB,
                          double lineHeightPt)
{
    cairo_set_scaled_font(cr, sf);

    for (const LineBreak& lb : lines) {
        if (cursorY + lineHeightPt > (pageH - marginB)) {
            cairo_show_page(cr);
            cursorY = marginT;
            cairo_set_scaled_font(cr, sf);
        }

        const int start = lb.startWord;
        const int end   = lb.endWord;
        const int spaces = (end - start - 1);

        double spacePt = double(p.spaceW26_6) / 64.0;
        if (spaces > 0) {
            if (lb.ratio >= 0.0)
                spacePt += lb.ratio * (double(p.spaceStretch26_6) / 64.0);
            else
                spacePt += lb.ratio * (double(p.spaceShrink26_6) / 64.0); // ratio is negative
        }

        double x = marginL;
        double baselineY = cursorY;

        for (int wi = start; wi < end; ++wi) {
            const ShapedWord& w = p.words[wi];

            QVector<cairo_glyph_t> glyphs;
            glyphs.resize(w.gid.size());

            for (int gi = 0; gi < w.gid.size(); ++gi) {
                cairo_glyph_t g;
                g.index = (unsigned long)w.gid[gi];
                g.x = x + double(w.xoff26_6[gi]) / 64.0;
                g.y = baselineY - double(w.yoff26_6[gi]) / 64.0;
                glyphs[gi] = g;
            }

            cairo_show_glyphs(cr, glyphs.constData(), glyphs.size());

            x += double(w.width26_6) / 64.0;
            if (wi + 1 < end)
                x += spacePt;
        }

        cursorY += lineHeightPt;
    }

    // Paragraph spacing (v0)
    cursorY += lineHeightPt * 0.5;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();

    QCommandLineOption inOpt({"i","in"}, "Input UTF-8 text file.", "file");
    QCommandLineOption outOpt({"o","out"}, "Output PDF file.", "file");
    QCommandLineOption fontOpt({"f","font"}, "TTF/OTF font file path.", "file");
    QCommandLineOption sizeOpt({"s","size"}, "Font size in points.", "pt", "11");

    parser.addOption(inOpt);
    parser.addOption(outOpt);
    parser.addOption(fontOpt);
    parser.addOption(sizeOpt);

    parser.process(app);

    const QString inPath   = parser.value(inOpt);
    const QString outPath  = parser.isSet(outOpt) ? parser.value(outOpt) : "output.pdf";
    const QString fontPath = parser.isSet(fontOpt) ? parser.value(fontOpt) : "fonts/lmroman10-regular.otf";
    const double fontSizePt = parser.value(sizeOpt).toDouble();

    if (inPath.isEmpty() || outPath.isEmpty() || fontPath.isEmpty()) {
        QTextStream(stderr) << "Usage: --in input.txt --out out.pdf --font font.ttf [--size 11]\n";
        return 2;
    }

    // Read input
    QByteArray bytes = readAllUtf8(inPath);
    if (bytes.isEmpty()) return 2;
    QString text = QString::fromUtf8(bytes);

    // FreeType + HarfBuzz
    FT_Library ft = nullptr;
    if (FT_Init_FreeType(&ft) != 0) {
        QTextStream(stderr) << "FT_Init_FreeType failed.\n";
        return 2;
    }

    FT_Face face = nullptr;
    if (FT_New_Face(ft, fontPath.toUtf8().constData(), 0, &face) != 0) {
        QTextStream(stderr) << "FT_New_Face failed: " << fontPath << "\n";
        FT_Done_FreeType(ft);
        return 2;
    }

    // Match 1pt == 1/72 inch (PDF points); use 72 dpi to align FT size to points.
    FT_Set_Char_Size(face, 0, (FT_F26Dot6)std::llround(fontSizePt * 64.0), 72, 72);

    hb_font_t* hbFont = hb_ft_font_create_referenced(face);

    cairo_scaled_font_t* sf = makeCairoScaledFont(face, fontSizePt);

    // PDF setup (A4 in points)
    const double pageW = pt(595.0);
    const double pageH = pt(842.0);
    const double marginL = pt(60.0);
    const double marginR = pt(60.0);
    const double marginT = pt(70.0);
    const double marginB = pt(70.0);

    const double lineWidth = pageW - marginL - marginR;
    const double lineHeightPt = fontSizePt * 1.25;

    cairo_surface_t* surface = cairo_pdf_surface_create(outPath.toUtf8().constData(), pageW, pageH);
    cairo_t* cr = cairo_create(surface);

    // black text
    cairo_set_source_rgb(cr, 0, 0, 0);

    double cursorY = marginT;

    QVector<QString> paras = splitParagraphs(text);
    for (const QString& para : paras) {
        ParaLayoutData pdata = prepareParagraph(hbFont, para);
        QVector<LineBreak> lines = breakKnuthPlassWords(pdata, lineWidth);
        drawParagraph(cr, sf, pdata, lines, cursorY, pageW, pageH, marginL, marginT, marginB, lineHeightPt);
    }

    cairo_show_page(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    cairo_scaled_font_destroy(sf);

    hb_font_destroy(hbFont);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return 0;
}

