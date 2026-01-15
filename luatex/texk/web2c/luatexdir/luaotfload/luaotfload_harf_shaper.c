/**
 * @file luaotfload_harf_shaper.c
 * @brief Implementation of the shaping logic.
 */

#include "luaotfload_harf_shaper.h"
#include <stdio.h>
#include <stdlib.h>

/* Helper to convert 26.6 fixed point to integer (rounding) */
static inline int fixed_to_int(hb_position_t v) {
    return (int)round(v / 64.0);
}

int luaotfload_shape_nodes(luaotfload_font_instance_t *font, 
                           node_t *head,
                           hb_script_t script,
                           hb_language_t language,
                           const hb_feature_t *features,
                           unsigned int num_features) 
{
    if (!font || !font->font || !head) return -1;

    hb_buffer_t *buffer = hb_buffer_create();
    if (!buffer) return -1;

    /* 
     * 1. Collect characters from the node list.
     * In a real implementation, we handle font switching and direction changes.
     * Here we assume a homogeneous list for the test case.
     */
    node_t *curr = head;
    unsigned int count = 0;
    while (curr) {
        if (curr->id == NODE_GLYPH) {
            hb_buffer_add(buffer, curr->char_code, count);
            count++;
        }
        curr = curr->next;
    }

    if (count == 0) {
        hb_buffer_destroy(buffer);
        return 0;
    }

    /* 2. Configure Buffer */
    hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);
    hb_buffer_set_script(buffer, script);
    hb_buffer_set_language(buffer, language);
    hb_buffer_set_content_type(buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);
    hb_buffer_guess_segment_properties(buffer);

    /* 3. Shape */
    hb_shape(font->font, buffer, features, num_features);

    /* 4. Map Results back to Nodes */
    unsigned int glyph_count;
    hb_glyph_info_t *infos = hb_buffer_get_glyph_infos(buffer, &glyph_count);
    hb_glyph_position_t *positions = hb_buffer_get_glyph_positions(buffer, &glyph_count);

    /* 
     * Mapping back is complex if glyph count changes (ligatures).
     * For this simplified test version, we assume 1:1 or N:1 reduction logic stub.
     * (Full logic requires careful cluster tracking implemented in the Lua version).
     */
    
    curr = head;
    unsigned int cluster_idx = 0;

    for (unsigned int i = 0; i < glyph_count; i++) {
        /* Find the node corresponding to this glyph */
        /* In this simplified loop we just walk forward for testing 1:1 cases mostly */
        if (!curr) break;

        /* Update Glyph ID */
        curr->char_code = infos[i].codepoint;

        /* Apply Positioning */
        /* x_advance is usually applied to width */
        /* x_offset is applied to visual offset */
        
        /* 
         * Note: LuaTeX nodes store width. HarfBuzz gives advance.
         * If advance differs from natural width, we might need a kern.
         * Here we just update width directly for simplicity.
         */
        curr->width = fixed_to_int(positions[i].x_advance);
        curr->x_offset = fixed_to_int(positions[i].x_offset);
        curr->y_offset = fixed_to_int(positions[i].y_offset);

        curr = curr->next;
    }

    /* 
     * If shaping reduced glyph count (ligature), we should hide/free remaining nodes.
     * Stub behavior: Just stop updating.
     */

    hb_buffer_destroy(buffer);
    return 0;
}
