/*
* Copyright 2025 Rochus Keller <mailto:me@rochus-keller.ch>
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

#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/*
 * Override OpTeX's file checker to always say "YES".
 * This forces OpTeX to issue the \font command, which triggers define_font.
 */
static int always_true(lua_State *L) {
    printf("***** optex.checkfont %s\n", luaL_checkstring(L, 1));
    lua_pushstring(L, luaL_checkstring(L, 1)); // Return the filename as-is
    return 1;
}

/* A generic dummy function for empty hooks */
static int dummy_func(lua_State *L) {
    return 0;
}

/*
 * The 'luaotfload-main' stub
 * Called when OpTeX runs require('luaotfload-main')
 */

static int define_font(lua_State *L);

static int luaotfload_stub(lua_State *L) {
    // 1. Create or update 'optex' global table
    lua_getglobal(L, "optex");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_setglobal(L, "optex");
        lua_getglobal(L, "optex");
    }
    // optex.hook_into_luaotfload = function() end
    lua_pushcfunction(L, dummy_func);
    lua_setfield(L, -2, "hook_into_luaotfload");
    lua_pop(L, 1);

    // 2. Create 'luaotfload' global table
    lua_newtable(L);
    // luaotfload.main = function() end
    lua_pushcfunction(L, dummy_func);
    lua_setfield(L, -2, "main");
    lua_setglobal(L, "luaotfload");

#if 0
    // 3. Register our NATIVE C font loader to the TeX callback, already registered below
#endif

#if 0
    // optex.checkfont = function(name) return name end
    lua_getglobal(L, "optex");
    lua_pushcfunction(L, always_true);
    lua_setfield(L, -2, "checkfont");
    lua_pop(L, 1);
#endif

    // RETURN VALUE: Push the 'luaotfload' table
    lua_getglobal(L, "luaotfload");
    return 1;
}

static int is_otf_request(const char *name) {
    /* bracketed filename OR explicit .otf/.ttf */
    return (strchr(name,'[') && strchr(name,']')) ||
           (strstr(name, ".otf") != NULL) ||
           (strstr(name, ".ttf") != NULL) ||
           (strstr(name, ".otc") != NULL) ||
           (strstr(name, ".ttc") != NULL);
}

static int call_font_reader(lua_State *L, const char *funcname, const char *name, lua_Number size) {
    /* font[funcname](tfmname, size) -> fonttable|nil */
    // stack: -
    lua_getglobal(L, "font");
    // stack: font
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        // stack: -
        fprintf(stderr,"*** call_font_reader(%s %s) get global font failed\n", funcname, name);
        return 0;
    }

    lua_getfield(L, -1, funcname);
    // stack: font, func
    lua_remove(L, -2);
    // stack: func

    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        // stack: -
        fprintf(stderr,"*** call_font_reader(%s %s) font[funcname] is not a function\n", funcname, name);
        return 0;
    }

    lua_pushstring(L, name);
    lua_pushnumber(L, size);
    // stack: func, name, size

    if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
        // stack: msg
        const char *err = lua_tostring(L, -1);
        fprintf(stderr, "*** font.%s(%s,%g) ERROR: %s\n",
                funcname, name, (double)size, err ? err : "(non-string)");
        lua_pop(L, 1);
        // stack: -
        lua_pushnil(L);
        // stack: nil
        return 1;
    }
    // stack: result (fonttable or nil)
    return 1;
}

static lua_Number normalize_size(lua_Number size)
{
    if (size < 0) {
        /* matches luatex/luaotfload practice: -1000 => 10pt => 655360sp */
        size = floor((65536.0 * (-size)) / 100.0 + 0.5);
    }
    return size;
}

static int define_font(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    lua_Number size  = normalize_size(luaL_checknumber(L, 2));

    if (size < 0)
        size = (-655.36) * size;  /* same normalization used in LuaTeX wiki examples */

    fprintf(stderr,"*** define_font(%s)\n", name);

    /* LuaTeX passes (name, size, id); keep at most 3 args on stack. */
    lua_settop(L, 3);

    if (!is_otf_request(name)) {
        /* Prefer VF if present, else TFM */
        /* Try VF: if it returns nil, try TFM */
        if (call_font_reader(L, "read_vf", name, size) && !lua_isnil(L, -1))
            return 1;
        lua_pop(L, 1); /* pop nil */
        if (call_font_reader(L, "read_tfm", name, size) && !lua_isnil(L, -1))
            return 1;
        lua_pop(L, 1);
        return 0;
    }

    /* ---------- 1) parse filename ---------- */
    char filename[1024];
    filename[0] = '\0';

    const char *start = strchr(name, '[');
    const char *end   = strchr(name, ']');
    if (start && end && end > start) {
        size_t len = (size_t)(end - start - 1);
        if (len >= sizeof(filename)) len = sizeof(filename) - 1;
        memcpy(filename, start + 1, len);
        filename[len] = '\0';
    } else {
        strncpy(filename, name, sizeof(filename) - 1);
        filename[sizeof(filename) - 1] = '\0';
    }

    /* ---------- 2) kpse.find_file ---------- */
    lua_getglobal(L, "kpse");
    // stack: kpse
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        fprintf(stderr,"*** define_font(%s) failure kpse\n", name);
        return 0; }

    lua_getfield(L, -1, "find_file");
    // stack: kpse, func
    lua_pushstring(L, filename);
    lua_pushstring(L, "opentype fonts");
    lua_pushboolean(L, 1);
    // stack: kpse, func, args

    if (lua_pcall(L, 3, 1, 0) != LUA_OK) {
        lua_pop(L, 2); /* error + kpse */
        fprintf(stderr,"*** define_font(%s) failure call kpse.find_file\n", name);
        return 0;
    }

    // stack: kpse, result
    if (!lua_isstring(L, -1)) {
        lua_pop(L, 2); /* result + kpse */
        fprintf(stderr,"*** define_font(%s) failure result kpse.find_file\n", name);
        return 0;
    }

    // stack: kpse, result
    strncpy(filename, lua_tostring(L, -1), sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = '\0';
    lua_pop(L, 2); /* result + kpse */

    // stack: -

    /* ---------- 3) luaharfbuzz Face/Font ---------- */
    lua_getglobal(L, "luaharfbuzz");
    // stack: hb
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        // stack: -
        fprintf(stderr,"*** define_font(%s) failure luaharfbuzz\n", name);
        return 0; }
    const int hb_idx = lua_gettop(L);

    /* face = luaharfbuzz.Face.new(filename) */
    lua_getfield(L, hb_idx, "Face");
    // stack: hb, Face
    if (!lua_istable(L, -1)) {
        lua_pop(L, 2);
        // stack: -
        fprintf(stderr,"*** define_font(%s) failure Face\n", name);
        return 0;
    }
    lua_getfield(L, -1, "new");
    // stack: hb, Face, func
    lua_pushstring(L, filename);
    // stack: hb, Face, func, arg
    lua_call(L, 1, 1);           /* returns face object */
    // stack: hb, Face, result
    lua_remove(L, -2);           /* remove Face table */
    // stack: hb, result
    const int face_idx = lua_gettop(L);
    if (lua_isnil(L, face_idx)) {
        lua_pop(L, 2);
        // stack: -
        fprintf(stderr,"*** define_font(%s) failure Face.new\n", name);
        return 0;
    }
    // stack: hb, face

    /* font = luaharfbuzz.Font.new(face) */
    lua_getfield(L, hb_idx, "Font");
    // stack: hb, face, fonttab
    if (!lua_istable(L, -1)) {
        lua_pop(L, 3);
        // stack: -
        fprintf(stderr,"*** define_font(%s) failure Font\n", name);
        return 0;
    }
    lua_getfield(L, -1, "new");
    // stack: hb, face, fonttab, func
    lua_pushvalue(L, face_idx);
    // stack: hb, face, font, func, face
    lua_call(L, 1, 1);           /* returns font object */
    // stack: hb, face, fonttab, font
    lua_remove(L, -2);           /* remove Font table */
    // stack: hb, face, font
    const int font_idx = lua_gettop(L);
    if (lua_isnil(L, font_idx)) {
        lua_pop(L, 3);
        // stack: -
        fprintf(stderr,"*** define_font(%s) failure Font.new\n", name);
        return 0;
    }

    /* upem = face:get_upem() */
    lua_getfield(L, face_idx, "get_upem");
    // stack: hb, face, font, func
    lua_pushvalue(L, face_idx);
    // stack: hb, face, font, func, face
    lua_call(L, 1, 1);
    // stack: hb, face, font, upem
    const lua_Number upem = lua_tonumber(L, -1);
    lua_pop(L, 1);
    // stack: hb, face, font
    if (upem <= 0) {
        lua_pop(L, 3);
        // stack: -
        fprintf(stderr,"*** define_font(%s) failure get_upem\n", name);
        return 0;
    }

    lua_Number scale = size / upem;

    /* ---------- 4) result table ---------- */
    lua_newtable(L);
    // stack: hb, face, font, tbl
    const int res_idx = lua_gettop(L);
    lua_pushstring(L, name);
    lua_setfield(L, res_idx, "name");
    // stack: hb, face, font, tbl
    lua_pushnumber(L, size);
    lua_setfield(L, res_idx, "size");
    // stack: hb, face, font, tbl
    lua_pushstring(L, "harf");
    lua_setfield(L, res_idx, "mode");
    // stack: hb, face, font, tbl

    /* ---------- 5) optional mathparameters (robust probing) ---------- */
    int has_math = 0;

    /* Preferred: Face:ot_math_has_data() because hb_ot_math_has_data takes a face. */
    lua_getfield(L, face_idx, "ot_math_has_data");
    // stack: hb, face, font, tbl, func
    if (lua_isfunction(L, -1)) {
        lua_pushvalue(L, face_idx);
        // stack: hb, face, font, tbl, func, face
        if (lua_pcall(L, 1, 1, 0) == LUA_OK) {
            has_math = lua_toboolean(L, -1);
            // stack: hb, face, font, tbl, bool
            lua_pop(L, 1);
        } else {
            // stack: hb, face, font, tbl, msg
            lua_pop(L, 1); /* error */
        }
        // stack: hb, face, font, tbl
    } else {
        lua_pop(L, 1); /* non-function */
        // stack: hb, face, font, tbl
        /* Fallback: luaharfbuzz.ot_math_has_data(face) if your binding uses that style. */
        lua_getfield(L, hb_idx, "ot_math_has_data");
        // stack: hb, face, font, tbl, func
        if (lua_isfunction(L, -1)) {
            lua_pushvalue(L, face_idx);
            // stack: hb, face, font, tbl, func, face
            if (lua_pcall(L, 1, 1, 0) == LUA_OK) {
                has_math = lua_toboolean(L, -1);
                // stack: hb, face, font, tbl, bool
                lua_pop(L, 1);
            } else {
                // stack: hb, face, font, tbl, msg
                lua_pop(L, 1);
            }
        } else {
            lua_pop(L, 1);
        }
        // stack: hb, face, font, tbl
    }

    if (has_math) {
        /* Only do this if your luaharfbuzz build actually provides these helpers. */
        lua_getfield(L, hb_idx, "ot_math_constant_get_table");
        // stack: hb, face, font, tbl, func
        if (lua_isfunction(L, -1) && lua_pcall(L, 0, 1, 0) == LUA_OK && lua_istable(L, -1)) {
            const int map_idx = lua_gettop(L);

            lua_newtable(L);
            const int mp_idx = lua_gettop(L);

            lua_pushnil(L);
            while (lua_next(L, map_idx) != 0) {
                const char *param_name = lua_tostring(L, -2);
                int id = (int)lua_tointeger(L, -1);

                lua_getfield(L, font_idx, "ot_math_get_constant");
                if (lua_isfunction(L, -1)) {
                    lua_pushvalue(L, font_idx);
                    lua_pushinteger(L, id);
                    if (lua_pcall(L, 2, 1, 0) == LUA_OK) {
                        lua_Number val = lua_tonumber(L, -1);
                        lua_pop(L, 1);
                        lua_pushnumber(L, floor(val * scale));
                        lua_setfield(L, mp_idx, param_name);
                    } else {
                        lua_pop(L, 1); /* error */
                    }
                } else {
                    lua_pop(L, 1);
                }

                lua_pop(L, 1); /* pop value(id), keep key for lua_next */
            }

            lua_pop(L, 1); /* pop constants map */
            lua_setfield(L, res_idx, "mathparameters"); /* pops mp table */
        } else {
            lua_pop(L, 1); /* pop whatever came back */
        }
    }

    /* ---------- 6) characters (minimal example) ---------- */
    lua_newtable(L);
    const int chars_idx = lua_gettop(L);
    // stack: hb, face, font, restbl, charstbl

    lua_getfield(L, font_idx, "get_nominal_glyph");
    const int f_gid = lua_gettop(L);
    lua_getfield(L, font_idx, "get_glyph_h_advance");
    const int f_adv = lua_gettop(L);

    for (int cp = 32; cp < 0x10000; cp++) {
        lua_pushvalue(L, f_gid);
        lua_pushvalue(L, font_idx);
        lua_pushinteger(L, cp);
        lua_call(L, 2, 1);
        int gid = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);
        if (gid <= 0) continue;

        lua_newtable(L);
        const int cd_idx = lua_gettop(L);

        lua_pushvalue(L, f_adv);
        lua_pushvalue(L, font_idx);
        lua_pushinteger(L, gid);
        lua_call(L, 2, 1);
        lua_Number adv = lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_pushnumber(L, floor(adv * scale));
        lua_setfield(L, cd_idx, "width");

        lua_rawseti(L, chars_idx, cp);
    }

    lua_pop(L, 2); /* pop cached funcs */
    lua_setfield(L, res_idx, "characters"); /* pops chars */

    /* leave only result table as return value */
    /* stack currently: ... hb face font result */
    lua_replace(L, 1);    /* put result at stack slot 1 */
    lua_settop(L, 1);     /* discard hb/face/font */

    //fprintf(stderr,"*** define_font(%s) ok return %s\n", name, lua_type(L, -1));


    return 1;
}




void luaopen_fontloader(lua_State *L) {

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "loaded"); /* Stack: package, loaded */
    lua_newtable(L);
    /* Add any dummy fields OpTeX might check, usually none needed if stub is minimal */
    lua_pushstring(L, "0.0.0");
    lua_setfield(L, -2, "version");
    lua_setfield(L, -2, "luaotfload-main");
    lua_pop(L, 2); /* Pop loaded, package */

    lua_getglobal(L, "callback");
    lua_getfield(L, -1, "register");
    lua_pushstring(L, "define_font");
    lua_pushcfunction(L, define_font);
    lua_call(L, 2, 1);
    lua_pop(L, 1);
}

