/**
 * @file test_shape.c
 * @brief Integration test for the shaper bridge.
 */

#include <stdio.h>
#include <stdlib.h>
#include "luaotfload_harf_shaper.h"
#include "luaotfload_harf_provider.h"
#include "luaotfload_database.h"
#include "luaotfload_parser.h"

void print_nodes(node_t *head) {
    node_t *curr = head;
    while (curr) {
        printf("[ID: %d | Glyph: %u | Width: %d | XOff: %d] -> ", 
               curr->id, curr->char_code, curr->width, curr->x_offset);
        curr = curr->next;
    }
    printf("NULL\n");
}

int main(void) {
    /* 1. Setup Font (using our previous infrastructure) */
    luaotfload_db_init(NULL);
    luaotfload_font_query_t query = { .name = "Arial" };
    luaotfload_font_entry_t *entry = luaotfload_db_find_closest(&query);
    
    if (!entry) {
        fprintf(stderr, "DB Lookup failed (is Arial installed?)\n");
        return 1;
    }

    /* Load with 12pt size */
    luaotfload_font_instance_t *font = luaotfload_harf_load_face(entry->fullpath, 0, 12.0);
    if (!font) {
        fprintf(stderr, "Font load failed\n");
        return 1;
    }

    /* 2. Create Node List: "AV" */
    /* 'A' = 65, 'V' = 86 */
    node_t *n1 = new_glyph(65);
    node_t *n2 = new_glyph(86);
    n1->next = n2;

    printf("--- Before Shaping ---\n");
    print_nodes(n1);

    /* 3. Shape */
    /* Enable kerning explicitly */
    hb_feature_t features[1];
    hb_feature_from_string("+kern", -1, &features[0]);

    printf("Shaping with kern...\n");
    int res = luaotfload_shape_nodes(font, n1, 
                                     HB_SCRIPT_LATIN, 
                                     hb_language_from_string("en", -1),
                                     features, 1);

    if (res != 0) {
        fprintf(stderr, "Shaping failed\n");
        return 1;
    }

    printf("--- After Shaping ---\n");
    print_nodes(n1);

    /* 
     * Verification:
     * Ideally, the width of the first glyph ('A') should be affected if 
     * HarfBuzz applied the kern to the advance width, OR 
     * x_advance should be different.
     * Since we map x_advance -> node->width, we expect n1->width to be 
     * less than the unkerned width (if we checked that).
     * At minimum, Glyph IDs (char_code) should be mapped (non-zero).
     */
    if (n1->char_code == 0 || n2->char_code == 0) {
        printf("FAIL: Glyphs not mapped (IDs are 0)\n");
    } else {
        printf("PASS: Glyphs mapped successfully\n");
    }

    /* Cleanup */
    luaotfload_harf_free_instance(font);
    luaotfload_db_free_entry(entry);
    luaotfload_db_cleanup();
    free(n1);
    free(n2);

    return 0;
}