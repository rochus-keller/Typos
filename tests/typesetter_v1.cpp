#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtCore/QCommandLineParser>
#include <QtCore/QVector>
#include <QtCore/QHash>
#include <QtCore/QByteArray>
#include <QtDebug>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include <cairo.h>
#include <cairo-ft.h>
#include <cairo-pdf.h>
#include <QFileInfo>

#include <cmath>
#include <limits>
#include <algorithm>

static QByteArray readAll(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return f.readAll();
}

static QVector<QString> splitParagraphs(const QString& s) {
#if 0
    QString t = s;
    t.replace("\r\n", "\n");
    t.replace("\r", "\n");
    // Split on blank lines (one or more empty/whitespace-only lines)
    QStringList parts = t.split(QRegExp("\\n\\s*\\n"), QString::SkipEmptyParts);
    QVector<QString> out;
    out.reserve(parts.size());
    for (const QString& p : parts) {
        QString pp = p.trimmed();
        if (!pp.isEmpty())
            out.push_back(pp);
    }
    return out;
#else
    // Treat empty lines as paragraph breaks.
    QString normalized = s;
    normalized.replace("\r\n", "\n");
    normalized.replace("\r", "\n");
    // not supported yet; we need something like \parfillskip, which requires an explicit item stream
    QStringList parts = normalized.split("\n\n", QString::KeepEmptyParts);
    QVector<QString> out;
    out.reserve(parts.size());
    for (const QString& p : parts) {
        QString t = p.simplified();
        Q_ASSERT(!t.contains('\n'));
        Q_ASSERT(!t.contains('\r'));
        if (!t.isEmpty())
            out.push_back(t);
    }
    return out;
#endif
}

enum Style { Regular, Bold, Italic };

struct Run {
    Style style;
    QString text; // no markup delimiters inside
};

static void flush(Style st, QString& buf,  QVector<Run>& runs) {
    if (!buf.isEmpty())
        runs.push_back({st, buf});
    buf.clear();
}


// v1 markup: **bold** and __italic__, no overlap, no nesting.
static QVector<Run> parseRunsNoNesting(const QString& para) {
    QVector<Run> runs;
    runs.push_back({Style::Regular, ""});

    Style cur = Style::Regular;
    QString buf;

    for (int i = 0; i < para.size(); ) {
        if (i + 1 < para.size() && para[i] == '*' && para[i+1] == '*') {
            flush(cur, buf, runs);
            // toggle bold (only allowed from/to Regular)
            cur = (cur == Style::Bold) ? Style::Regular : Style::Bold;
            i += 2;
            continue;
        }
        if (i + 1 < para.size() && para[i] == '_' && para[i+1] == '_') {
            flush(cur, buf, runs);
            // toggle italic (only allowed from/to Regular)
            cur = (cur == Style::Italic) ? Style::Regular : Style::Italic;
            i += 2;
            continue;
        }
        buf.append(para[i]);
        ++i;
    }
    flush(cur, buf, runs);

    // Merge adjacent runs of same style
    QVector<Run> merged;
    for (const Run& r : runs) {
        if (r.text.isEmpty())
            continue;
        if (!merged.isEmpty() && merged.back().style == r.style)
            merged.back().text += r.text;
        else
            merged.push_back(r);
    }
    return merged;
}

struct ShapedGlyph {
    hb_codepoint_t gid;   // glyph index in font
    hb_position_t xoff26_6;
    hb_position_t yoff26_6;
    hb_position_t xadv26_6;
};

struct ShapedSpan {
    Style style;
    QVector<ShapedGlyph> glyphs;
    hb_position_t width26_6 = 0;
};

struct FontCtx {
    QString path;
    FT_Face face = nullptr;
    hb_font_t* hbFont = nullptr;
    cairo_scaled_font_t* cairoScaled = nullptr;
    hb_position_t spaceW26_6 = 0;
    hb_position_t spaceStretch26_6 = 0;
    hb_position_t spaceShrink26_6 = 0;
};

typedef QHash<Style, FontCtx*> Fonts;

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

static bool initFont(FT_Library ft, FontCtx* f, const QString& fontPath, double sizePt) {
    f->path = fontPath;
    if (FT_New_Face(ft, fontPath.toUtf8().constData(), 0, &f->face) != 0)
        return false;

    // 72 dpi so 1pt == 1/72 inch in PDF user units.
    FT_Set_Char_Size(f->face, 0, (FT_F26Dot6)std::llround(sizePt * 64.0), 72, 72);

    f->hbFont = hb_ft_font_create_referenced(f->face);
    // Force using FT functions; if this is not here, only the first paragraph is correctly typeset.
    // This took me a whole day to trace down:
    hb_ft_font_set_funcs(f->hbFont);
    f->cairoScaled = makeCairoScaledFont(f->face, sizePt);

    // Measure a space in this font; use generous stretch/shrink in v1 to avoid dead ends.
    hb_buffer_t* buf = hb_buffer_create();
    QByteArray sp = QByteArray(" ");
    hb_buffer_add_utf8(buf, sp.constData(), sp.size(), 0, sp.size());
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_guess_segment_properties(buf);
    hb_shape(f->hbFont, buf, nullptr, 0);

    unsigned int n = 0;
    hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buf, &n);

    hb_position_t w = 0;
    for (unsigned int i = 0; i < n; ++i)
        w += pos[i].x_advance;

    hb_buffer_destroy(buf);

    f->spaceW26_6 = w;
    f->spaceStretch26_6 = w * 3; // v1: generous to keep feasibility
    f->spaceShrink26_6  = w * 1;

    return true;
}

static void destroyFont(FontCtx* f) {
    if (f->cairoScaled)
        cairo_scaled_font_destroy(f->cairoScaled);
    if (f->hbFont)
        hb_font_destroy(f->hbFont);
    if (f->face)
        FT_Done_Face(f->face);
    delete f;
}

static const FontCtx* fontForStyle(const Fonts& fonts, Style s) {
    if (fonts.contains(s))
        return fonts[s];
    return fonts[Style::Regular];
}

static ShapedSpan shapeTextSpan(const Fonts& fonts, Style style, const QString& text) {
    const FontCtx* f = fontForStyle(fonts, style);
    ShapedSpan out;
    out.style = style;

    hb_buffer_t* buf = hb_buffer_create();
    QByteArray utf8 = text.toUtf8();

    hb_buffer_add_utf8(buf, utf8.constData(), utf8.size(), 0, utf8.size());
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_guess_segment_properties(buf);

    hb_shape(f->hbFont, buf, nullptr, 0);

    unsigned int n = 0;
    hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buf, &n);
    hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buf, &n);

    out.glyphs.resize(int(n));
    hb_position_t penX = 0;
    for (unsigned int i = 0; i < n; ++i) {
        ShapedGlyph g;
        g.gid = infos[i].codepoint;                 // glyph index
        g.xoff26_6 = penX + pos[i].x_offset;
        g.yoff26_6 = pos[i].y_offset;
        g.xadv26_6 = pos[i].x_advance;
        out.glyphs[int(i)] = g;
        penX += pos[i].x_advance;
    }
    out.width26_6 = penX;

    hb_buffer_destroy(buf);
    return out;
}

// ---- Item stream ----

enum ItemType { Box, Glue, Penalty };

struct Item {
    ItemType type;

    // Dimensions in hb_position_t 26.6
    hb_position_t width = 0;
    hb_position_t stretch = 0;
    hb_position_t shrink = 0;

    // Penalty fields
    int penalty = 0;        // +INF forbids break; -INF forces break
    bool flagged = false;

    // Box payload
    int spanIndex = -1;     // index into shapedSpans
};

static const int PENALTY_INF  = 1000000000;
static const int PENALTY_NINF = -1000000000;

// Legal break if:
// - Item is a Penalty with penalty < INF
// - Item is a Glue and previous item is a Box (space after word)
static bool isLegalBreak(const QVector<Item>& items, int idx) {
    if (idx < 0 || idx >= items.size())
        return false;
    const Item& it = items[idx];
    if (it.type == ItemType::Penalty)
        return it.penalty < PENALTY_INF;
    if (it.type == ItemType::Glue) {
        if (idx == 0)
            return false;
        return items[idx - 1].type == ItemType::Box;
    }
    return false;
}

// Measure line from startItem (inclusive) up to breakAt (inclusive),
// but if breakAt is Glue, exclude that trailing glue from width/stretch/shrink.
struct LineMeasure {
    hb_position_t natural = 0;
    hb_position_t stretch = 0;
    hb_position_t shrink = 0;
    int breakPenalty = 0;
    bool endsWithFlaggedPenalty = false;
};

static LineMeasure measureLine(const QVector<Item>& items, int startItem, int breakAt) {
    LineMeasure m;
    if (startItem > breakAt)
        return m;

    int end = breakAt;
    if (items[end].type == ItemType::Glue) {
        end -= 1; // drop trailing space at line end
    }

    for (int k = startItem; k <= end; ++k) {
        const Item& it = items[k];
        if (it.type == ItemType::Box) {
            m.natural += it.width;
        } else if (it.type == ItemType::Glue) {
            m.natural += it.width;
            m.stretch += it.stretch;
            m.shrink  += it.shrink;
        } else if (it.type == ItemType::Penalty) {
            m.natural += it.width;
            // penalty width typically 0; keep it generic
        }
    }

    if (items[breakAt].type == ItemType::Penalty) {
        m.breakPenalty = items[breakAt].penalty;
        m.endsWithFlaggedPenalty = items[breakAt].flagged;
    } else {
        m.breakPenalty = 0;
        m.endsWithFlaggedPenalty = false;
    }

    return m;
}

static bool lineFeasible(hb_position_t lineW26_6,
                         const LineMeasure& m,
                         double& outRatio)
{
    hb_position_t diff = lineW26_6 - m.natural;
    if (diff == 0) {
        outRatio = 0.0;
        return true;
    }

    if (diff > 0) {
        if (m.stretch == 0)
            return false;
        outRatio = double(diff) / double(m.stretch);
        return outRatio <= 10.0;
    } else {
        if (m.shrink == 0)
            return false;
        outRatio = double(diff) / double(m.shrink); // negative
        return outRatio >= -1.0;
    }
}

static double badnessFromRatio(double r) {
    double ar = std::abs(r);
    if (ar > 10.0)
        return 1e9;
    double x = 100.0 * ar;
    return x * x * x;
}

struct LineBreak {
    int startItem = 0;  // inclusive
    int breakAt = 0;    // inclusive index of glue/penalty where we break
    double ratio = 0.0;
};

static int itemAfterBreak(int breakAt) {
    // Next line starts after the break item; if break is Glue, skip it.
    int s = breakAt + 1;
    return s;
}

static QVector<LineBreak> breakKnuthPlassItems(const QVector<Item>& items, hb_position_t lineW26_6)
{
    const int N = items.size();

    QVector<int> breaks;
    breaks.reserve(N);
    for (int i = 0; i < N; ++i)
        if (isLegalBreak(items, i))
            breaks.push_back(i);

    // DP over breakpoints:
    // state is "breakpoint index in breaks array" + also allow "start" at -1
    const int B = breaks.size();
    QVector<double> best(B, std::numeric_limits<double>::infinity());
    QVector<int> prev(B, -1);
    QVector<double> bestRatio(B, 0.0);
    QVector<int> startItemAt(B, 0); // start item for line ending at breaks[b]

    // Initialize: lines starting at item 0.
    for (int bi = 0; bi < B; ++bi) {
        int bAt = breaks[bi];
        LineMeasure m = measureLine(items, 0, bAt);
        double ratio = 0.0;
        if (!lineFeasible(lineW26_6, m, ratio))
            continue;

        double bad = badnessFromRatio(ratio);
        // Include penalty in demerits (simplified)
        double pen = (items[bAt].type == ItemType::Penalty) ? double(items[bAt].penalty) : 0.0;
        if (pen >= PENALTY_INF)
            continue;
        if (pen <= PENALTY_NINF)
            pen = -10000.0; // forced break

        double demerits = (10.0 + bad + pen) * (10.0 + bad + pen);
        best[bi] = demerits;
        prev[bi] = -1;
        bestRatio[bi] = ratio;
        startItemAt[bi] = 0;
    }

    // Transitions
    for (int bj = 0; bj < B; ++bj) {
        if (!std::isfinite(best[bj]))
            continue;
        int start = itemAfterBreak(breaks[bj]);

        for (int bi = bj + 1; bi < B; ++bi) {
            int bAt = breaks[bi];

            // Do not allow breaking before start
            if (bAt < start)
                continue;

            LineMeasure m = measureLine(items, start, bAt);
            double ratio = 0.0;
            if (!lineFeasible(lineW26_6, m, ratio))
                continue;

            double bad = badnessFromRatio(ratio);
            double pen = (items[bAt].type == ItemType::Penalty) ? double(items[bAt].penalty) : 0.0;
            if (pen >= PENALTY_INF)
                continue;
            if (pen <= PENALTY_NINF)
                pen = -10000.0;

            double demerits = (10.0 + bad + pen) * (10.0 + bad + pen);
            double cost = best[bj] + demerits;

            if (cost < best[bi]) {
                best[bi] = cost;
                prev[bi] = bj;
                bestRatio[bi] = ratio;
                startItemAt[bi] = start;
            }
        }
    }

    // Find the best forced end-of-paragraph break:
    // Prefer a forced penalty at the end; simplest: take the last breakpoint.
    int endBi = -1;
    for (int bi = B - 1; bi >= 0; --bi) {
        if (!std::isfinite(best[bi]))
            continue;
        // require that this break is forced (our paragraph end)
        const Item& it = items[breaks[bi]];
        if (it.type == ItemType::Penalty && it.penalty <= PENALTY_NINF/2) {
            endBi = bi;
            break;
        }
    }
    if (endBi < 0) {
        // fallback: take best reachable last breakpoint
        double bestCost = std::numeric_limits<double>::infinity();
        for (int bi = 0; bi < B; ++bi) {
            if (best[bi] < bestCost) {
                bestCost = best[bi];
                endBi = bi;
            }
        }
    }
    if (endBi < 0)
        return {};

    // Reconstruct
    QVector<LineBreak> lines;
    int bi = endBi;
    while (bi >= 0) {
        LineBreak lb;
        lb.startItem = startItemAt[bi];
        lb.breakAt   = breaks[bi];
        lb.ratio     = bestRatio[bi];
        lines.push_back(lb);
        bi = prev[bi];
    }
    std::reverse(lines.begin(), lines.end());
    return lines;
}

static void emitWord(Style st, const QString& w, const Fonts& fonts,
                     QVector<ShapedSpan>& shapedSpansOut, QVector<Item>& itemsOut) {
    ShapedSpan span = shapeTextSpan(fonts, st, w);
    int idx = shapedSpansOut.size();
    shapedSpansOut.push_back(span);

    Item it;
    it.type = ItemType::Box;
    it.width = shapedSpansOut[idx].width26_6;
    it.spanIndex = idx;
    itemsOut.push_back(it);
}

static void emitSpace(Style st, const Fonts& fonts, QVector<ShapedSpan>& shapedSpansOut, QVector<Item>& itemsOut) {
    const FontCtx* f = fontForStyle(fonts, st);
    Item it;
    it.type = ItemType::Glue;
    it.width = f->spaceW26_6;
    it.stretch = f->spaceStretch26_6;
    it.shrink  = f->spaceShrink26_6;
    itemsOut.push_back(it);
}

// Build item stream from runs:
// - Split runs into tokens: sequences of non-space => Box, space sequences => Glue.
// - Append paragraph-end fill glue + forced penalty.
static void buildItemsForParagraph(const Fonts& fonts,
                                   const QVector<Run>& runs,
                                   QVector<ShapedSpan>& shapedSpansOut,
                                   QVector<Item>& itemsOut)
{
    shapedSpansOut.clear();
    itemsOut.clear();

    for (const Run& r : runs) {
        QString t = r.text;
        // Keep whitespace as separators, but emit *one* space glue per whitespace run.
        int i = 0;
        while (i < t.size()) {
            if (t[i].isSpace()) {
                while (i < t.size() && t[i].isSpace())
                    ++i;
                emitSpace(r.style, fonts, shapedSpansOut, itemsOut);
            } else {
                int j = i;
                while (j < t.size() && !t[j].isSpace())
                    ++j;
                emitWord(r.style, t.mid(i, j - i), fonts, shapedSpansOut, itemsOut);
                i = j;
            }
        }
    }

    // Remove trailing spaces (glue) before paragraph end
    while (!itemsOut.isEmpty() && itemsOut.back().type == ItemType::Glue)
        itemsOut.removeLast();

    // Paragraph end: "fill glue" then forced break penalty.
    // Fill glue: width 0, huge stretch, so last line doesn't justify spaces.
    Item fill;
    fill.type = ItemType::Glue;
    fill.width = 0;
    fill.stretch = hb_position_t(1) << 30; // huge
    fill.shrink = 0;
    itemsOut.push_back(fill);

    Item endp;
    endp.type = ItemType::Penalty;
    endp.width = 0;
    endp.penalty = PENALTY_NINF; // must break
    endp.flagged = false;
    itemsOut.push_back(endp);
}

static void drawLines(cairo_t* cr,
                      const Fonts& fonts,
                      const QVector<ShapedSpan>& spans,
                      const QVector<Item>& items,
                      const QVector<LineBreak>& lines,
                      double& cursorY,
                      double pageW, double pageH,
                      double marginL, double marginT, double marginB,
                      double lineHeightPt)
{
    for (const LineBreak& lb : lines) {
        if (cursorY + lineHeightPt > (pageH - marginB)) {
            cairo_show_page(cr);
            cursorY = marginT;
        }

        double x = marginL;
        double baselineY = cursorY;

        int end = lb.breakAt;
        if (items[end].type == ItemType::Glue)
            end -= 1; // drop trailing glue

        for (int k = lb.startItem; k <= end; ++k) {
            const Item& it = items[k];

            if (it.type == ItemType::Box) {
                const ShapedSpan& sp = spans[it.spanIndex];
                const FontCtx* f = fontForStyle(fonts, sp.style);

                cairo_set_scaled_font(cr, f->cairoScaled);

                QVector<cairo_glyph_t> glyphs;
                glyphs.resize(sp.glyphs.size());
                for (int gi = 0; gi < sp.glyphs.size(); ++gi) {
                    cairo_glyph_t g;
                    g.index = (unsigned long)sp.glyphs[gi].gid;
                    g.x = x + double(sp.glyphs[gi].xoff26_6) / 64.0;
                    g.y = baselineY - double(sp.glyphs[gi].yoff26_6) / 64.0;
                    glyphs[gi] = g;
                }
                cairo_show_glyphs(cr, glyphs.constData(), glyphs.size());

                x += double(it.width) / 64.0;
            } else if (it.type == ItemType::Glue) {
                double w = double(it.width) / 64.0;
                if (lb.ratio > 0.0)
                    w += lb.ratio * (double(it.stretch) / 64.0);
                else if (lb.ratio < 0.0)
                    w += lb.ratio * (double(it.shrink) / 64.0); // ratio negative
                x += w;
            } else {
                // Penalty inside line: width typically 0, ignore in v1 rendering
                x += double(it.width) / 64.0;
            }
        }

        cursorY += lineHeightPt;
    }

    cursorY += lineHeightPt * 0.5; // paragraph spacing
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();

    QCommandLineOption inOpt({"i","in"}, "Input UTF-8 text file.", "file");
    QCommandLineOption outOpt({"o","out"}, "Output PDF file.", "file");
    QCommandLineOption regOpt({"r","font-regular"}, "Regular font file path.", "file");
    QCommandLineOption boldOpt({"b","font-bold"}, "Bold font file path (optional).", "file");
    QCommandLineOption italOpt({"t","font-italic"}, "Italic font file path (optional).", "file");
    QCommandLineOption sizeOpt({"s","size"}, "Font size in points.", "pt", "11");

    parser.addOption(inOpt);
    parser.addOption(outOpt);
    parser.addOption(regOpt);
    parser.addOption(boldOpt);
    parser.addOption(italOpt);
    parser.addOption(sizeOpt);

    parser.process(app);

    const QString inPath = parser.value(inOpt);
    const QString outPath  = parser.isSet(outOpt) ? parser.value(outOpt) : "output.pdf";
    const QString regPath = parser.isSet(regOpt) ? parser.value(regOpt) : "fonts/lmroman10-regular.otf";
    const QString boldPath = parser.isSet(boldOpt) ? parser.value(boldOpt) : "fonts/lmroman10-bold.otf";
    const QString italPath = parser.isSet(italOpt) ? parser.value(italOpt) : "fonts/lmroman10-italic.otf";
    const double fontSizePt = parser.value(sizeOpt).toDouble();

    if (inPath.isEmpty() || outPath.isEmpty() || regPath.isEmpty()) {
        QTextStream(stderr) << "Usage: --in input.txt --out out.pdf --font-regular Regular.ttf "
                               "[--font-bold Bold.ttf] [--font-italic Italic.ttf] [--size 11]\n";
        return 2;
    }

    if( !QFileInfo(inPath).exists() )
    {
        QTextStream(stderr) << "cannot open file for reading:" << inPath << endl;
        return 2;
    }

    QByteArray bytes = readAll(inPath);
    if (bytes.isEmpty())
    {
        QTextStream(stderr) << "no text found in file:" << inPath << endl;
        return 2;
    }
    QString text = QString::fromUtf8(bytes);

    QTextStream(stdout) << "processing..." << endl;
    FT_Library ft = nullptr;
    if (FT_Init_FreeType(&ft) != 0) {
        QTextStream(stderr) << "FT_Init_FreeType failed.\n";
        return 2;
    }

    Fonts fonts;
    FontCtx* font = new FontCtx();
    if (!initFont(ft, font, regPath, fontSizePt)) {
        delete font;
        QTextStream(stderr) << "Failed to load regular font.\n";
        FT_Done_FreeType(ft);
        return 2;
    }else
        fonts.insert(Style::Regular, font);

    if (!boldPath.isEmpty()) {
        font = new FontCtx();
        if (!initFont(ft, font, boldPath, fontSizePt)) {
            QTextStream(stderr) << "Failed to load bold font; falling back to regular.\n";
            destroyFont(font);
        }else
            fonts.insert(Style::Bold, font);
    }
    if (!italPath.isEmpty()) {
        font = new FontCtx();
        if (!initFont(ft, font, italPath, fontSizePt)) {
            QTextStream(stderr) << "Failed to load italic font; falling back to regular.\n";
            destroyFont(font);
        }else
            fonts.insert(Style::Italic, font);
    }

    // PDF (A4)
    const double pageW = 595.0;
    const double pageH = 842.0;
    const double marginL = 60.0, marginR = 60.0, marginT = 70.0, marginB = 70.0;
    const double lineWidthPt = pageW - marginL - marginR;
    const hb_position_t lineW26_6 = hb_position_t(std::llround(lineWidthPt * 64.0));
    const double lineHeightPt = fontSizePt * 1.25;

    cairo_surface_t* surface = cairo_pdf_surface_create(outPath.toUtf8().constData(), pageW, pageH);
    cairo_t* cr = cairo_create(surface);
    cairo_set_source_rgb(cr, 0, 0, 0);

    double cursorY = marginT;

    QVector<QString> paras = splitParagraphs(text);
    for (int i = 0; i < paras.size(); i++ ) {
        const QString& para = paras[i];
        QVector<Run> runs = parseRunsNoNesting(para);

        QVector<ShapedSpan> spans;
        QVector<Item> items;
        buildItemsForParagraph(fonts, runs, spans, items);

        QVector<LineBreak> lines = breakKnuthPlassItems(items, lineW26_6);
        if (lines.isEmpty()) {
            // Hard fallback: treat as one line by forcing a break at end penalty
            LineBreak lb;
            lb.startItem = 0;
            lb.breakAt = items.size() - 1;
            lb.ratio = 0.0;
            lines.push_back(lb);
        }

        drawLines(cr, fonts, spans, items, lines, cursorY, pageW, pageH, marginL, marginT, marginB, lineHeightPt);
    }

    cairo_show_page(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    // Cleanup fonts
    for (Style s : fonts.keys())
        destroyFont(fonts[s]);
    FT_Done_FreeType(ft);

    QTextStream(stdout) << "output written to " << outPath << endl;

    return 0;
}

