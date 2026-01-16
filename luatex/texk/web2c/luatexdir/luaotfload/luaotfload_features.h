#ifndef LUAOTFLOAD_FEATURES_H
#define LUAOTFLOAD_FEATURES_H

#include "luaotfload_core.h"
#include <hb.h>

/**
 * @brief Parse a feature string (e.g., "smcp=1", "+kern") into a HarfBuzz feature.
 * 
 * @param tag_str The feature tag string.
 * @param value_str The value string (can be NULL for boolean toggles).
 * @param feature Output pointer to the HarfBuzz feature struct.
 * @return 0 on success, non-zero on error.
 */
int luaotfload_features_parse(const char *tag_str, const char *value_str, hb_feature_t *feature);

/**
 * @brief Apply a global list of default features to a feature array.
 * (e.g., +liga, +kern which are on by default).
 */
void luaotfload_features_add_defaults(hb_feature_t **features, unsigned int *count);

/**
 * @brief Normalize a script tag string (e.g. "latn") to a HarfBuzz script.
 */
hb_script_t luaotfload_features_parse_script(const char *script);

/**
 * @brief Normalize a language tag string (e.g. "DEU") to a HarfBuzz language.
 */
hb_language_t luaotfload_features_parse_language(const char *lang);

#endif // LUAOTFLOAD_FEATURES_H

