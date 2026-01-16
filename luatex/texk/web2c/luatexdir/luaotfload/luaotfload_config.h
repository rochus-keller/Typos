#ifndef LUAOTFLOAD_CONFIG_H
#define LUAOTFLOAD_CONFIG_H

#include "luaotfload_core.h"

/**
 * @brief Populates the configuration struct with hardcoded defaults.
 * This mirrors the defaults table in luaotfload-configuration.lua.
 */
void luaotfload_config_set_defaults(luaotfload_config_t *config);

/**
 * @brief Parses a configuration file and updates the config struct.
 * 
 * @param config Pointer to the configuration struct to update.
 * @param filename Path to the configuration file (usually luaotfload.conf).
 * @return 0 on success, non-zero error code on failure.
 */
int luaotfload_config_parse_file(luaotfload_config_t *config, const char *filename);

/**
 * @brief Cleanup allocated strings within the config struct.
 */
void luaotfload_config_free(luaotfload_config_t *config);

#endif // LUAOTFLOAD_CONFIG_H

