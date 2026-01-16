#ifndef LUAOTFLOAD_DEBUG_H
#define LUAOTFLOAD_DEBUG_H

#include "luaotfload_core.h"
#include <stdbool.h>

/**
 * @brief Verify the runtime environment (HarfBuzz version, LuaTeX version).
 * @return true if environment is sufficient.
 */
bool luaotfload_debug_check_environment(void);

/**
 * @brief Dumps a font instance's metadata to the log.
 * Useful for debugging font loading/shaping issues.
 */
void luaotfload_debug_dump_font(const luaotfload_font_instance_t *font);

/**
 * @brief Simple assertion wrapper that logs before aborting.
 */
void luaotfload_debug_assert(bool condition, const char *message, const char *file, int line);

#define LUAOTFLOAD_ASSERT(cond, msg) \
    luaotfload_debug_assert((cond), (msg), __FILE__, __LINE__)

#endif // LUAOTFLOAD_DEBUG_H

