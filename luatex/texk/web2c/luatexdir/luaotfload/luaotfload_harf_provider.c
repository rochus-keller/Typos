/**
 * @file luaotfload_harf_provider.c
 * @brief Implementation of HarfBuzz loading logic.
 */

#include "luaotfload_harf_provider.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

luaotfload_font_instance_t* luaotfload_harf_load_face(const char *path, int index, double point_size) {
    if (!path) return NULL;

    luaotfload_font_instance_t *instance = calloc(1, sizeof(luaotfload_font_instance_t));
    if (!instance) {
        fprintf(stderr, "luaotfload: Out of memory allocating font instance\n");
        return NULL;
    }

    /* 1. Load Blob */
    hb_blob_t *blob = hb_blob_create_from_file(path);
    if (!blob || hb_blob_get_length(blob) == 0) {
        fprintf(stderr, "luaotfload: Failed to load font file '%s' (empty or unreadable)\n", path);
        if (blob) hb_blob_destroy(blob);
        free(instance);
        return NULL;
    }

    /* 2. Create Face */
    hb_face_t *face = hb_face_create(blob, index);
    hb_blob_destroy(blob); /* Face keeps a reference */

    if (hb_face_get_empty() == face) {
        fprintf(stderr, "luaotfload: Failed to create HarfBuzz face from '%s'\n", path);
        hb_face_destroy(face);
        free(instance);
        return NULL;
    }

    /* 3. Create Font */
    hb_font_t *font = hb_font_create(face);
    if (hb_font_get_empty() == font) {
        fprintf(stderr, "luaotfload: Failed to create HarfBuzz font object\n");
        hb_face_destroy(face);
        hb_font_destroy(font);
        free(instance);
        return NULL;
    }

    /* 4. Set Scale (26.6 format: size * 64) */
    /* 
     * Note: LuaTeX internally uses Scaled Points (sp), where 1 pt = 65536 sp.
     * HarfBuzz usually expects 26.6 fixed point for rasterizers (FreeType).
     * For pure shaping, we often set scale to UPEM, but the prompt requirement
     * specifies size * 64. We follow the requirement.
     */
    int scale = (int)(point_size * 64.0);
    hb_font_set_scale(font, scale, scale);

    /* 5. Populate Instance Struct */
    instance->face = face;
    instance->font = font;
    instance->filename = strdup(path); /* Need include <string.h> */
    instance->subfont_index = index;
    instance->point_size = point_size;
    
    /* Get UPEM from face for completeness */
    instance->units_per_em = hb_face_get_upem(face);

    return instance;
}

void luaotfload_harf_free_instance(luaotfload_font_instance_t *instance) {
    if (!instance) return;

    if (instance->font) hb_font_destroy(instance->font);
    if (instance->face) hb_face_destroy(instance->face);
    
    if (instance->filename) free(instance->filename);
    if (instance->ps_name) free(instance->ps_name);
    if (instance->full_name) free(instance->full_name);
    if (instance->variations) free(instance->variations);
    if (instance->features) free(instance->features);

    free(instance);
}