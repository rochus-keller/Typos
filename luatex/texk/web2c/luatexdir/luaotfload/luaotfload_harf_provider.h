/**
 * @file luaotfload_harf_provider.h
 * @brief HarfBuzz font creation and instantiation.
 */

#ifndef LUAOTFLOAD_HARF_PROVIDER_H
#define LUAOTFLOAD_HARF_PROVIDER_H

#include "luaotfload_core.h"

/**
 * @brief Load a font file and create a configured HarfBuzz font instance.
 *
 * This function performs the following steps:
 * 1. Loads the file into an `hb_blob_t`.
 * 2. Creates an `hb_face_t` from the blob.
 * 3. Creates an `hb_font_t` from the face.
 * 4. Configures font scaling (26.6 fixed point format).
 * 5. Wraps everything in a `luaotfload_font_instance_t`.
 *
 * @param path Absolute path to the font file.
 * @param index Face index (0 for single files, N for TTC collections).
 * @param point_size Requested size in points (e.g., 10.0 or 12.0).
 * 
 * @return A new font instance, or NULL on failure.
 */
luaotfload_font_instance_t* luaotfload_harf_load_face(const char *path, int index, double point_size);

/**
 * @brief Destroy a font instance and release HarfBuzz resources.
 */
void luaotfload_harf_free_instance(luaotfload_font_instance_t *instance);

#endif /* LUAOTFLOAD_HARF_PROVIDER_H */