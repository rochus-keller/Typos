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
#include <assert.h>

/* A generic dummy function for empty hooks */
static int dummy_func(lua_State *L) {
    fprintf(stderr,"*** luaotfload.main called\n");
    fflush(stderr);
    return 0;
}

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


    // RETURN VALUE: Push the 'luaotfload' table
    lua_getglobal(L, "luaotfload");
    return 1;
}

static int is_otf_request(const char *name) {
    /* bracketed filename OR explicit .otf/.ttf */
    if( *name == ':' )
        return 1;
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
        fflush(stderr);
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
        fflush(stderr);
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
        fflush(stderr);
        lua_pop(L, 1);
        // stack: -
        lua_pushnil(L);
        // stack: nil
        return 1;
    }
    fprintf(stderr,"*** call_font_reader(%s %s) WARNING old TeX fonts are obsolete and only marginally supported\n", funcname, name);
    fflush(stderr);
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

static int error_return(lua_State *L, const char* message, const char* name)
{
    fprintf(stderr,message, name);
    fflush(stderr);
    lua_pushnil(L);
    return 1;
}

static int find_file(lua_State* L, char* filename, int fnsize, const char* name)
{
    // stack: -

    /* call kpse.find_file ---------- */
    lua_getglobal(L, "kpse");
    // stack: kpse
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return error_return(L, "*** define_font(%s) failure kpse\n", name);
    }

    lua_getfield(L, -1, "find_file");
    // stack: kpse, func
    lua_pushstring(L, filename);
    lua_pushstring(L, "opentype fonts");
    lua_pushboolean(L, 1);
    // stack: kpse, func, args

    if (lua_pcall(L, 3, 1, 0) != LUA_OK) {
        lua_pop(L, 2); /* error + kpse */
        return error_return(L, "*** define_font(%s) failure call kpse.find_file\n", name);
    }

    // stack: kpse, result
    if (!lua_isstring(L, -1)) {
        lua_pop(L, 2); /* result + kpse */
        return error_return(L, "*** define_font(%s) failure result kpse.find_file\n", name);
    }

    // stack: kpse, result
    strncpy(filename, lua_tostring(L, -1), fnsize - 1);
    filename[fnsize - 1] = '\0';
    lua_pop(L, 2); /* result + kpse */

    // stack: -
    return 0;
}

static int parse_filename(lua_State* L, char* filename, int fnsize, const char* name)
{
    // TODO: so far this only considers the actual name in [] without the additiona attributes following it
    static char last_filename[127] = {0};

    if( name && *name == ':' )
    {
        if( last_filename[0] == 0 )
            return error_return(L, "*** define_font(%s) font without name\n", name);
        else
            strcpy(filename,last_filename);
    }else
    {
        const char *start = strchr(name, '[');
        const char *end   = strchr(name, ']');
        if (start && end && end > start) {
            size_t len = (size_t)(end - start - 1);
            if (len >= fnsize)
                len = fnsize - 1;
            memcpy(filename, start + 1, len);
            filename[len] = '\0';
        } else {
            strncpy(filename, name, fnsize - 1);
            filename[fnsize - 1] = '\0';
        }
        strcpy(last_filename, filename);
    }
    return 0;
}

static int push_hb_face_font(lua_State* L, char* filename, const char* name)
{
    const int top = lua_gettop(L);
    lua_getglobal(L, "luaharfbuzz");
    // stack: hb
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        // stack: -
        assert(top == lua_gettop(L));
        return error_return(L, "*** define_font(%s) failure luaharfbuzz\n", name);
    }
    const int hb_idx = lua_gettop(L);

    /* face = luaharfbuzz.Face.new(filename) */
    lua_getfield(L, hb_idx, "Face");
    // stack: hb, tbl
    if (!lua_istable(L, -1)) {
        lua_pop(L, 2);
        // stack: -
        assert(top == lua_gettop(L));
        return error_return(L, "*** define_font(%s) failure Face\n", name);
    }
    lua_getfield(L, -1, "new");
    // stack: hb, tbl, func
    lua_pushstring(L, filename);
    // stack: hb, tbl, func, arg
    lua_call(L, 1, 1);           /* returns face object */
    // stack: hb, tbl, face
    if (lua_isnil(L, -1)) {
        lua_pop(L, 3);
        // stack: -
        assert(top == lua_gettop(L));
        return error_return(L, "*** define_font(%s) failure Face.new\n", name);
    }
    lua_remove(L, -2);           /* remove Face table */
    // stack: hb, face
    const int face_idx = lua_gettop(L);
    // stack: hb, face

    /* font = luaharfbuzz.Font.new(face) */
    lua_getfield(L, hb_idx, "Font");
    // stack: hb, face, fonttab
    if (!lua_istable(L, -1)) {
        lua_pop(L, 3);
        // stack: -
        assert(top == lua_gettop(L));
        return error_return(L, "*** define_font(%s) failure Font\n", name);
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
        assert(top == lua_gettop(L));
        return error_return(L, "*** define_font(%s) failure Font.new\n", name);
    }
    // stack: hb, face, font
    assert(top+3 == lua_gettop(L));
    return 0;
}

static lua_Number get_upem(lua_State *L, int face_idx, const char* name)
{
    /* upem = face:get_upem() */
    lua_getfield(L, face_idx, "get_upem");
    // stack: hb, face, font, func
    lua_pushvalue(L, face_idx);
    // stack: hb, face, font, func, face
    lua_call(L, 1, 1);
    // stack: hb, face, font, upem
    const lua_Number upem = lua_tonumber(L, -1);
    lua_pop(L, 1);
    if( upem <= 0 )
    {
        lua_pop(L, 3);
        // stack: -
        error_return(L, "*** define_font(%s) failure get_upem\n", name);
    }
    return upem;
}

static int is_open_type(const char* filename)
{
    FILE* f = fopen(filename,"r");
    if( f == NULL )
        return 0;
    char buf[4] = {0};
    fread(buf, 4, 1, f);
    const int res = strncmp(buf, "OTTO", 4);
    fclose(f);
    return res == 0;
}

static int create_result_table(lua_State *L, const char* filename, int face_idx, lua_Number size, lua_Number upem)
{
    // stack: -
    const int top = lua_gettop(L);

    lua_newtable(L);
    // stack: tbl
    const int res_idx = lua_gettop(L);

    lua_getfield(L, face_idx, "get_name");
    lua_pushvalue(L, face_idx);
    lua_pushinteger(L, 4); // Full Font Name
    lua_call(L, 2, 1);
    lua_setfield(L, res_idx, "fullname");

    lua_pushstring(L, filename);
    lua_setfield(L, res_idx, "filename");

    lua_getfield(L, face_idx, "get_name");
    lua_pushvalue(L, face_idx);
    lua_pushinteger(L, 6); // PostScript Name
    lua_call(L, 2, 1);
    lua_pushvalue(L, -1); // duplicate
    lua_setfield(L, res_idx, "psname");
    lua_setfield(L, res_idx, "name");

    lua_pushnumber(L, size);
    lua_setfield(L, res_idx, "size");

    lua_pushnumber(L, 655360); // TODO: assumption, usually 10pt = 655360 sp
    lua_setfield(L, res_idx, "designsize");

    if( is_open_type(filename) )
        lua_pushstring(L, "opentype");
    else
        lua_pushstring(L, "truetype");
    lua_setfield(L, res_idx, "format");

    lua_pushinteger(L, 2); // unicode
    lua_setfield(L, res_idx, "encodingbytes");

    lua_pushstring(L, "harf");
    lua_setfield(L, res_idx, "mode");

    lua_pushstring(L, "real");
    lua_setfield(L, res_idx, "type");

    lua_pushinteger(L, (lua_Integer)upem);
    lua_setfield(L, res_idx, "unitsperem");

    lua_pushstring(L, "subset");
    lua_setfield(L, res_idx, "embedding");

    assert( lua_gettop(L) == top + 1);
    return 0;
}

static int check_if_math(lua_State *L, int face_idx)
{
    // stack: -
    int has_math = 0;
    /* Preferred: Face:ot_math_has_data() because hb_ot_math_has_data takes a face. */
    lua_getfield(L, face_idx, "ot_math_has_data");
    // stack: func
    if (lua_isfunction(L, -1)) {
        lua_pushvalue(L, face_idx);
        // stack: func, face
        if (lua_pcall(L, 1, 1, 0) == LUA_OK) {
            has_math = lua_toboolean(L, -1);
            // stack: bool
            lua_pop(L, 1);
        } else {
            // stack: msg
            lua_pop(L, 1); /* error */
        }
    }else
        lua_pop(L, 1);
    // stack: -
    return has_math;
}

static int fill_math(lua_State *L, int hb_idx, int font_idx, int res_idx, lua_Number scale)
{
    /* Only do this if your luaharfbuzz build actually provides these helpers. */
    lua_getfield(L, hb_idx, "ot_math_constants");
    if (lua_istable(L, -1)) {
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
    return 0;
}

static int fill_chars_table(lua_State *L, int font_idx, int res_idx, lua_Number scale, lua_Number* space_width)
{
    const int top = lua_gettop(L);
    // stack: -
    lua_newtable(L);
    const int chars_idx = lua_gettop(L);
    // stack: charstbl

    lua_getfield(L, font_idx, "get_nominal_glyph");
    const int get_nominal_glyph = lua_gettop(L); //   (Font, Codepoint) -> GlyphID
    // stack: charstbl, func
    lua_getfield(L, font_idx, "get_glyph_h_advance");
    const int get_glyph_h_advance = lua_gettop(L);  // (Font, GlyphID) -> advance for for horizontal text
    // stack: charstbl, func, func
    lua_getfield(L, font_idx, "get_glyph_extents");
    const int get_glyph_extents = lua_gettop(L);
    // stack: charstbl, func, func, func

    for (int cp = 32; cp < 0x10000; cp++) {
        lua_pushvalue(L, get_nominal_glyph);
        lua_pushvalue(L, font_idx);
        lua_pushinteger(L, cp);
        lua_call(L, 2, 1);
        const int gid = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);
        if (gid <= 0)
            continue;

        lua_newtable(L);
        const int cd_idx = lua_gettop(L);

        lua_pushvalue(L, get_glyph_h_advance);
        lua_pushvalue(L, font_idx);
        lua_pushinteger(L, gid);
        lua_call(L, 2, 1);
        const lua_Number advance = lua_tonumber(L, -1);
        if( cp == 32 && space_width )
            *space_width = advance;
        lua_pop(L, 1);

        lua_pushnumber(L, floor(advance * scale));
        lua_setfield(L, cd_idx, "width");

        lua_pushvalue(L, get_glyph_extents);
        lua_pushvalue(L, font_idx);
        lua_pushinteger(L, gid);
        lua_call(L, 2, 1);
        if( !lua_isnil(L, -1) )
        {
            lua_getfield(L, -1, "y_bearing");
            const lua_Number y_bearing = lua_tonumber(L, -1);
            lua_pop(L, 1);
            lua_getfield(L, -1, "height");
            const lua_Number height = lua_tonumber(L, -1);
            lua_pop(L, 1);
            lua_pushnumber(L, floor(y_bearing * scale));
            lua_setfield(L, cd_idx, "height");
            lua_Number depth = -(y_bearing + height);
            if( depth < 0.0 )
                depth = 0.0;
            lua_pushnumber(L, floor(depth * scale));
            lua_setfield(L, cd_idx, "depth");
        }
        lua_pop(L, 1);

        lua_pushinteger(L, gid);
        lua_setfield(L, cd_idx, "index");

        lua_pushinteger(L, cp);
        lua_setfield(L, cd_idx, "tounicode");

        lua_rawseti(L, chars_idx, cp);
        // stack: charstbl, func, func, func
    }
    // stack: charstbl, func, func, func

    lua_pop(L, 3); /* pop cached funcs */
    // stack: charstbl
    lua_setfield(L, res_idx, "characters"); /* pops chars */
    // stack: -
    assert( top == lua_gettop(L));
    return 0;
}

static void set_char_params(lua_State *L, int res_idx, lua_Number space_width, lua_Number size, lua_Number scale)
{
    const int top = lua_gettop(L);
    lua_newtable(L);
    const int param_idx = lua_gettop(L);

    lua_pushnumber(L, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, param_idx, "slant");
    lua_rawseti(L, param_idx, 1);

    lua_pushinteger(L, floor(space_width * scale));
    lua_pushvalue(L, -1);
    lua_setfield(L, param_idx, "space");
    lua_rawseti(L, param_idx, 2);

    lua_pushinteger(L, floor(space_width * scale * 0.5));
    lua_pushvalue(L, -1);
    lua_setfield(L, param_idx, "spacestretch");
    lua_rawseti(L, param_idx, 3);

    lua_pushinteger(L, floor(space_width * scale * 0.33) );
    lua_pushvalue(L, -1);
    lua_setfield(L, param_idx, "spaceshrink");
    lua_rawseti(L, param_idx, 4);

    lua_pushinteger(L, floor(space_width * scale * 0.45));
    lua_pushvalue(L, -1);
    lua_setfield(L, param_idx, "xheight");
    lua_rawseti(L, param_idx, 5);

    lua_pushinteger(L, floor(size));
    lua_pushvalue(L, -1);
    lua_setfield(L, param_idx, "quad");
    lua_rawseti(L, param_idx, 6);

    lua_pushinteger(L, floor(space_width * scale * 0.33));
    lua_pushvalue(L, -1);
    lua_setfield(L, param_idx, "extraspace");
    lua_rawseti(L, param_idx, 7);

    lua_setfield(L, res_idx, "parameters");

    assert( top == lua_gettop(L));
}

static int define_font(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    lua_Number size  = normalize_size(luaL_checknumber(L, 2));

    if (size < 0)
        size = (-655.36) * size;  /* same normalization used in LuaTeX wiki examples */

    fprintf(stderr,"*** define_font(%s)\n", name);
    fflush(stderr);

    /* LuaTeX passes (name, size, id); keep at most 3 args on stack. */
    lua_settop(L, 3);

    if (strcmp(name, "nullfont") == 0)
        return error_return(L, "*** define_font(%s) cannot load nullfont\n", name);

    if (!is_otf_request(name)) {
        /* Prefer VF if present, else TFM */
        /* Try VF: if it returns nil, try TFM */
        if (call_font_reader(L, "read_vf", name, size) && !lua_isnil(L, -1))
            return 1;
        lua_pop(L, 1); /* pop nil */
        if (call_font_reader(L, "read_tfm", name, size) && !lua_isnil(L, -1))
            return 1;
        // stack: nil
        return 1;
    }

    char filename[1024];
    filename[0] = '\0';

    if( parse_filename(L, filename, sizeof(filename), name) )
        return 1;

    // stack: -

    if( find_file(L, filename, sizeof(filename), name) )
        return 1;

    // stack: -

    if( push_hb_face_font(L, filename, name) )
        return 1;

    // stack: hb, face, font
    const int font_idx = lua_gettop(L);
    const int face_idx = font_idx - 1;
    const int hb_idx = face_idx - 1;

    const lua_Number upem = get_upem(L, face_idx, name);
    // stack: hb, face, font
    if (upem <= 0)
        return 1;

    const lua_Number scale = size / upem;

    if( create_result_table(L, filename, face_idx, size, upem) )
        return 1;
    const int res_idx = lua_gettop(L);

    const int has_math = check_if_math(L, face_idx);

    if (has_math)
        if( fill_math(L, hb_idx, font_idx, res_idx, scale) )
            return 1;

    lua_Number space_width;
    if( fill_chars_table(L, font_idx, res_idx, scale, &space_width) )
        return 1;

    if( space_width == 0.0 )
        space_width = upem * 0.25; // Robust fallback

    set_char_params(L, res_idx, space_width, size, scale);

    // stack: hb, face, font, restbl

    /* leave only result table as return value */
    lua_replace(L, -4);
    // stack: restbl, face, font
    lua_pop(L, 2);     /* discard hb/face/font */
    // stack: restbl

#ifdef TYPOS_DEBUG
    const int top = lua_gettop(L);
    assert(lua_istable(L,-1));
    // fprintf(stderr,"*** define_font(%s) ok return\n", name);
#endif

    return 1;
}

void luaopen_fontloader(lua_State *L) {

#if 1
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");
    lua_pushcfunction(L, luaotfload_stub);
    lua_setfield(L, -2, "luaotfload-main");
    lua_pop(L, 2); /* preload, package */
#else
    // stack: -
    lua_getglobal(L, "package");
    // stack: package
    lua_getfield(L, -1, "loaded"); /* Stack: package, loaded */
    // stack: package, loaded
    luaotfload_stub(L);
    // stack: package, loaded, tbl
    /* Add any dummy fields OpTeX might check, usually none needed if stub is minimal */
    lua_pushstring(L, "0.0.0");
    // stack: package, loaded, tbl, str
    lua_setfield(L, -2, "version");
    // stack: package, loaded, tbl,
    lua_setfield(L, -2, "luaotfload-main");
    // stack: package, loaded
    lua_pop(L, 2);
    // stack: -
#endif

    // stack: -
    lua_getglobal(L, "callback");
    // stack: callback
    lua_getfield(L, -1, "register");
    // stack: callback, func
    lua_pushstring(L, "define_font");
    // stack: callback, func, str
    lua_pushcfunction(L, define_font);
    // stack: callback, func, str, func
    lua_call(L, 2, 0);
    // stack: callback
    lua_pop(L, 1);
    // stack: -
}

