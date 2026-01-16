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

/**
 * @brief Lua wrapper for the define_font callback.
 * Expected Lua table fields: name, size, features (optional)
 */

int luaotfload_define_font_wrapper(lua_State *L) {
    // 1. Validate input
    if (!lua_istable(L, 1)) {
        return 0;
    }

    // 2. Extract name and size
    lua_getfield(L, 1, "name");
    const char *name = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "size");
    double size_sp = lua_tonumber(L, -1);
    lua_pop(L, 1);

    double size_pt = size_sp / 65536.0;

    // 3. Resolve and load
    char *path = luaotfload_io_find_file(name);
    if (!path) {
        luaotfload_log_report("bridge", LUAOTFLOAD_LOG_COMMON,
                             "Could not find: %s", name);
        return 0;
    }

    luaotfload_font_instance_t *inst = luaotfload_harf_load_face(path, 0, size_pt);
    free(path);

    if (!inst) return 0;

    // 4. Find free slot
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

    luaotfload_log_report("bridge", LUAOTFLOAD_LOG_LOADING,
                         "Loaded: %s -> ID %d", name, fid);

    // 5. BUILD THE FONT TABLE (This is what was missing!)
    lua_newtable(L); // Create the font table

    // Required fields for LuaTeX
    lua_pushstring(L, name);
    lua_setfield(L, -2, "name");

    lua_pushstring(L, inst->filename);
    lua_setfield(L, -2, "filename");

    lua_pushnumber(L, size_sp);
    lua_setfield(L, -2, "size");

    lua_pushnumber(L, inst->units_per_em);
    lua_setfield(L, -2, "units_per_em");

    lua_pushstring(L, "real"); // Font type
    lua_setfield(L, -2, "type");

    lua_pushstring(L, "harf"); // Mode identifier
    lua_setfield(L, -2, "mode");

    lua_pushinteger(L, fid); // Store our ID for retrieval
    lua_setfield(L, -2, "harf_id");

    // Create empty characters table (required by LuaTeX)
    lua_newtable(L);
    lua_setfield(L, -2, "characters");

    // Shaper flag (tells LuaTeX to call pre_linebreak_filter)
    lua_pushboolean(L, 1);
    lua_setfield(L, -2, "shapers");

    return 1; // Return the font table
}


