#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "luaotfload_parser.h"

void print_query(const luaotfload_font_query_t *q) {
    printf("Query:\n");
    printf("  Name:     %s\n", q->name ? q->name : "(null)");
    printf("  Location: %s\n", q->location ? q->location : "(null)");
    printf("  Style:    %s\n", q->style ? q->style : "(null)");
}

void print_features(const luaotfload_feature_list_t *f) {
    printf("Features (%zu):\n", f->count);
    for (size_t i = 0; i < f->count; i++) {
        printf("  [%zu] %s = %s\n", i, f->items[i].key, f->items[i].value ? f->items[i].value : "(null)");
    }
}

int main(void) {
    const char *input = "name:Arial:mode=harf;+liga";
    printf("Parsing: '%s'\n", input);

    luaotfload_font_query_t query;
    memset(&query, 0, sizeof(query));
    
    luaotfload_feature_list_t features;
    luaotfload_features_init(&features);

    int ret = luaotfload_parse_font_request(input, &query, &features);
    
    if (ret == 0) {
        print_query(&query);
        print_features(&features);
    } else {
        printf("Parse failed with code %d\n", ret);
    }

    /* Verify specific expectations */
    if (strcmp(query.name, "Arial") != 0) printf("FAIL: Name mismatch\n");
    if (features.count != 2) printf("FAIL: Feature count mismatch\n");
    else {
        if (strcmp(features.items[0].key, "mode") == 0 && strcmp(features.items[0].value, "harf") == 0)
            printf("PASS: mode=harf\n");
        else printf("FAIL: mode check\n");

        if (strcmp(features.items[1].key, "liga") == 0 && strcmp(features.items[1].value, "true") == 0)
            printf("PASS: +liga\n");
        else printf("FAIL: liga check\n");
    }

    /* Cleanup */
    free((void*)query.name);
    free((void*)query.location);
    free((void*)query.style);
    luaotfload_features_free(&features);

    return 0;
}