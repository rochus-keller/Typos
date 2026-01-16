#ifndef LUAOTFLOAD_LOG_H
#define LUAOTFLOAD_LOG_H

#include "luaotfload_core.h"
#include <stdarg.h>

/**
 * @brief Function pointer type for custom log output (e.g., routing to TeX console).
 */
typedef void (*luaotfload_log_callback_t)(const char *message, void *user_data);

/**
 * @brief Initialize the logging subsystem.
 * @param level The initial filtering level.
 */
void luaotfload_log_init(luaotfload_log_level_t level);

/**
 * @brief Register a custom callback for log messages.
 * If not set, logs default to stderr.
 */
void luaotfload_log_set_callback(luaotfload_log_callback_t callback, void *user_data);

/**
 * @brief Log a formatted message if the priority allows it.
 * @param level The severity of the message.
 * @param fmt Printf-style format string.
 */
void luaotfload_log(luaotfload_log_level_t level, const char *fmt, ...);

/**
 * @brief Log a message with a specific component tag (common in luaotfload).
 * Example: luaotfload_log_report("db", LUAOTFLOAD_LOG_INFO, "Scanning...");
 */
void luaotfload_log_report(const char *component, luaotfload_log_level_t level, const char *fmt, ...);

#endif // LUAOTFLOAD_LOG_H

