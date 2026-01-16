#include <lua.h>
#include <lauxlib.h>
#include "luaotfload_core.h"
#include "luaotfload_log.h"

// --- Wrapper Functions ---

/**
 * @brief Lua wrapper for luaotfload_init
 * Usage: require("luaotfload.core").init()
 */
static int l_init(lua_State *L) {
    // Get argv[0] from arg table if available
    lua_getglobal(L, "arg");
    if (lua_istable(L, -1)) {
        lua_rawgeti(L, -1, 0);  // arg[0] is the program name
        const char *progname = lua_tostring(L, -1);
        if (progname) {
            kpse_set_program_name(progname, "luahbtex");
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    int result = luaotfload_init(L);
    lua_pushboolean(L, result == 0);
    return 1;
}

/**
 * @brief Lua wrapper for luaotfload_cleanup
 * Usage: require("luaotfload.core").cleanup()
 */
static int l_cleanup(lua_State *L) {
    luaotfload_cleanup();
    return 0;
}

/**
 * @brief Helper to redirect C logs to LuaTeX's print function
 */
static void luatex_log_callback(const char *msg, void *user_data) {
    // We cannot safely call Lua API from arbitrary C threads, 
    // but assuming single-threaded LuaTeX for now:
    // (In production, this needs careful thread handling or just use stderr)
    fprintf(stdout, "%s\n", msg); 
}

static int l_set_log_callback(lua_State *L) {
    luaotfload_log_set_callback(luatex_log_callback, NULL);
    return 0;
}

// --- Library Registration ---
extern int luaotfload_shape_process(lua_State *L);
extern int luaotfload_define_font_wrapper(lua_State *L);

static const struct luaL_Reg luaotfload_lib[] = {
    {"init", l_init},
    {"cleanup", l_cleanup},
    {"set_log_sink", l_set_log_callback},
    {"define_font", luaotfload_define_font_wrapper},
    {"shape_process", luaotfload_shape_process},
    {NULL, NULL} /* Sentinel */
};

/**
 * @brief The Main Entry Point
 * This function name is CRITICAL. It must match "luaopen_" + library name.
 * If you compile as "luaotfload_core.so", this function must be "luaopen_luaotfload_core".
 */
int luaopen_luaotfload_core(lua_State *L) {
    luaL_newlib(L, luaotfload_lib);
    
    // Add version info
    lua_pushstring(L, "3.30-C-Refactor");
    lua_setfield(L, -2, "version");

    return 1;
}

