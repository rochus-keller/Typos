/**
 * @file luaotfload_harf_shaper.h
 * @brief Bridge between LuaTeX node lists and HarfBuzz shaping.
 */

#ifndef LUAOTFLOAD_HARF_SHAPER_H
#define LUAOTFLOAD_HARF_SHAPER_H

#include "luaotfload_core.h"
#include "mock_luatex.h" /* Mock nodes for standalone testing */

/**
 * @brief Shape a list of nodes using the specified font.
 * 
 * This function iterates over the node list, collects consecutive characters
 * belonging to the same font, shapes them using HarfBuzz, and updates the
 * nodes with Glyph IDs and positioning adjustments (kerns/offsets).
 * 
 * @param font The font instance to use for shaping.
 * @param head The head of the node list (linked list).
 * @param script The OpenType script tag (e.g., HB_SCRIPT_LATIN).
 * @param language The OpenType language tag (e.g., hb_language_from_string("en", -1)).
 * @param features Optional array of features to apply (e.g., kern, liga).
 * @param num_features Length of the features array.
 * 
 * @return 0 on success, non-zero on failure.
 */
int luaotfload_shape_nodes(luaotfload_font_instance_t *font, 
                           node_t *head,
                           hb_script_t script,
                           hb_language_t language,
                           const hb_feature_t *features,
                           unsigned int num_features);

#endif /* LUAOTFLOAD_HARF_SHAPER_H */