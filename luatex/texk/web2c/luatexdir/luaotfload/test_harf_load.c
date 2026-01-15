/**
 * @file test_harf_load.c
 * @brief Integration test: Parser -> Database -> HarfBuzz Provider
 */

#include <stdio.h>
#include <stdlib.h>
#include "luaotfload_parser.h"
#include "luaotfload_database.h"
#include "luaotfload_harf_provider.h"

int main(int argc, char **argv) {
    /* 1. Parse Request */
    /* Use 'Arial' by default, or user arg if provided */
    const char *request = (argc > 1) ? argv[1] : "name:Arial:mode=harf";
    printf("Request: '%s'\n", request);

    luaotfload_font_query_t query = {0};
    luaotfload_feature_list_t features = {0};
    
    if (luaotfload_parse_font_request(request, &query, &features) != 0) {
        fprintf(stderr, "Parse failed\n");
        return 1;
    }

    printf("Parsed Query: Name='%s'\n", query.name);

    /* 2. Database Lookup */
    /* Initialize stub DB */
    luaotfload_db_init(NULL);
    
    luaotfload_font_entry_t *entry = luaotfload_db_find_closest(&query);
    if (!entry) {
        fprintf(stderr, "Database lookup failed for '%s'\n", query.name);
        return 1;
    }

    printf("Found Font: %s (%s)\n", entry->family_name, entry->fullpath);

    /* 3. HarfBuzz Load */
    double size = 12.0; /* 12pt */
    luaotfload_font_instance_t *inst = luaotfload_harf_load_face(entry->fullpath, entry->index, size);
    
    if (!inst) {
        fprintf(stderr, "HarfBuzz loading failed\n");
        return 1;
    }

    /* 4. Verify */
    unsigned int glyph_count = hb_face_get_glyph_count(inst->face);
    printf("SUCCESS: Loaded face with %u glyphs\n", glyph_count);
    printf("UPEM: %f\n", inst->units_per_em);

    /* Cleanup */
    luaotfload_harf_free_instance(inst);
    luaotfload_db_free_entry(entry);
    luaotfload_db_cleanup();
    /* Free parser results */
    free((void*)query.name);
    free((void*)query.location);
    if(query.style) free((void*)query.style);
    luaotfload_features_free(&features);

    return 0;
}