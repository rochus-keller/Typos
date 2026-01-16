#include <lua.h>
#include <lauxlib.h>
#include "luaotfload_core.h"
#include "luaotfload_harf_provider.h"
#include "luaotfload_log.h"
#include "luaotfload_io.h"
#include <string.h>

// External font registry (from luaotfload_core.c)
// You may need to expose this via a getter function instead
extern luaotfload_font_instance_t *font_registry[256];

static int build_font_table(lua_State *L, luaotfload_font_instance_t *inst, double size);

/**
 * @brief Lua wrapper for the define_font callback.
 * Expected Lua table fields: name, size, features (optional)
 */

int luaotfload_define_font_wrapper(lua_State *L) {
    if (!lua_istable(L, 1)) return 0;

    lua_getfield(L, 1, "name");
    const char *name = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "size");
    double size_sp = lua_tonumber(L, -1);
    lua_pop(L, 1);

    char *path = (name && name[0] == '/') ? strdup(name) : luaotfload_io_find_file(name);
    if (!path) {
        luaotfload_log_report("bridge", LUAOTFLOAD_LOG_COMMON, "Could not find: %s", name);
        return 0;
    }

    double size_pt = size_sp / 65536.0;
    luaotfload_font_instance_t *inst = luaotfload_harf_load_face(path, 0, size_pt);
    free(path);

    if (!inst) return 0;

    // Register font
    font_id fid = -1;
    for (int i = 1; i < 256; i++) {
        if (!font_registry[i]) {
            font_registry[i] = inst;
            inst->id = i;
            fid = i;
            break;
        }
    }

    if (fid == -1) {
        luaotfload_harf_free_instance(inst);
        return 0;
    }

    // BUILD THE COMPLETE FONT TABLE
    build_font_table(L, inst, size_sp);

    // Add our internal ID
    lua_pushinteger(L, fid);
    lua_setfield(L, -2, "harf_id");

    luaotfload_log_report("bridge", LUAOTFLOAD_LOG_LOADING, "Font loaded: ID %d", fid);

    return 1;
}



/**
 * Build the complete LuaTeX font table with character metrics.
 * This is the C port of scalefont() from luaotfload-harf-define.lua
 */
static int build_font_table(lua_State *L, luaotfload_font_instance_t *inst, double size) {
    hb_font_t *hbfont = inst->font;
    hb_face_t *hbface = inst->face;
    unsigned int upem = inst->units_per_em;

    // Scale factor: size / upem
    double scale = size / upem;

    // Set HarfBuzz scale to UPEM (we scale in TeX, not HarfBuzz)
    hb_font_set_scale(hbfont, upem, upem);

    // Create the main font table
    lua_newtable(L);

    // --- Basic Info ---
    lua_pushstring(L, inst->filename);
    lua_setfield(L, -2, "filename");

    lua_pushnumber(L, size);
    lua_setfield(L, -2, "size");

    lua_pushnumber(L, upem);
    lua_setfield(L, -2, "units_per_em");

    lua_pushstring(L, "opentype"); // or "truetype" based on font
    lua_setfield(L, -2, "format");

    lua_pushstring(L, "harf");
    lua_setfield(L, -2, "mode");

    // --- Build Characters Table ---
    lua_newtable(L); // characters = {}

    unsigned int glyph_count = hb_face_get_glyph_count(hbface);
    unsigned int gid_offset = 0x120000; // Standard offset for direct glyph access

    // Get font extents for fallback
    hb_font_extents_t font_extents;
    hb_font_get_h_extents(hbfont, &font_extents);
    hb_position_t ascender = font_extents.ascender ? font_extents.ascender : upem * 0.8;
    hb_position_t descender = font_extents.descender ? font_extents.descender : upem * 0.2;

    // Iterate all glyphs and extract metrics
    for (unsigned int gid = 0; gid < glyph_count; gid++) {
        lua_newtable(L); // character table for this glyph

        // Width
        hb_position_t width = hb_font_get_glyph_h_advance(hbfont, gid);
        lua_pushnumber(L, width * scale);
        lua_setfield(L, -2, "width");

        // Height and Depth
        hb_glyph_extents_t extents;
        hb_position_t height, depth;
        if (hb_font_get_glyph_extents(hbfont, gid, &extents)) {
            height = extents.y_bearing;
            depth = extents.y_bearing + extents.height;
        } else {
            height = ascender;
            depth = descender;
        }

        lua_pushnumber(L, height * scale);
        lua_setfield(L, -2, "height");

        lua_pushnumber(L, -depth * scale);
        lua_setfield(L, -2, "depth");

        // Index
        lua_pushinteger(L, gid);
        lua_setfield(L, -2, "index");

        // Store at gid_offset + gid
        lua_rawseti(L, -2, gid_offset + gid);
    }

    // Map Unicode codepoints to glyphs
    hb_set_t *unicodes = hb_set_create();
    hb_face_collect_unicodes(hbface, unicodes);

    hb_codepoint_t unicode = HB_SET_VALUE_INVALID;
    while (hb_set_next(unicodes, &unicode)) {
        hb_codepoint_t gid;
        if (hb_font_get_nominal_glyph(hbfont, unicode, &gid)) {
            // Copy the glyph table
            lua_rawgeti(L, -1, gid_offset + gid); // Get glyph table
            lua_pushinteger(L, unicode);
            lua_setfield(L, -2, "tounicode");
            lua_rawseti(L, -2, unicode); // characters[unicode] = glyph_table
        }
    }
    hb_set_destroy(unicodes);

    lua_setfield(L, -2, "characters");

    // --- Font Parameters ---
    lua_newtable(L); // parameters = {}

    // Get space advance
    hb_codepoint_t space_gid;
    hb_position_t space_width;
    if (hb_font_get_nominal_glyph(hbfont, 0x0020, &space_gid)) {
        space_width = hb_font_get_glyph_h_advance(hbfont, space_gid);
    } else {
        space_width = upem / 2;
    }

    lua_pushnumber(L, space_width * scale);
    lua_setfield(L, -2, "space");

    lua_pushnumber(L, space_width * scale / 2);
    lua_setfield(L, -2, "space_stretch");

    lua_pushnumber(L, space_width * scale / 3);
    lua_setfield(L, -2, "space_shrink");

    lua_pushnumber(L, size);
    lua_setfield(L, -2, "quad");

    // x-height (try 'x' glyph)
    hb_codepoint_t x_gid;
    hb_position_t xheight = ascender / 2;
    if (hb_font_get_nominal_glyph(hbfont, 'x', &x_gid)) {
        hb_glyph_extents_t ext;
        if (hb_font_get_glyph_extents(hbfont, x_gid, &ext)) {
            xheight = ext.y_bearing;
        }
    }
    lua_pushnumber(L, xheight * scale);
    lua_setfield(L, -2, "x_height");

    lua_setfield(L, -2, "parameters");

    return 1; // Return the font table
}
