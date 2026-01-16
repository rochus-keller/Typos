#include "luaotfload_features.h"
#include "luaotfload_log.h"
#include <string.h>
#include <stdlib.h>

int luaotfload_features_parse(const char *tag_str, const char *value_str, hb_feature_t *feature) {
    if (!tag_str || !feature) return -1;

    // Parse Tag using HarfBuzz utility
    if (!hb_feature_from_string(tag_str, -1, feature)) {
        // Fallback: manual tag parsing if needed, but hb_feature_from_string is robust
        // It handles "kern", "+kern", "-kern", "kern=0"
        
        // If the tag_str is just the name (e.g. "smcp") and we have a separate value
        char buffer[64];
        if (value_str) {
            snprintf(buffer, 64, "%s=%s", tag_str, value_str);
        } else {
            snprintf(buffer, 64, "%s", tag_str);
        }
        
        if (!hb_feature_from_string(buffer, -1, feature)) {
            luaotfload_log_report("features", LUAOTFLOAD_LOG_COMMON, "Invalid feature: %s", buffer);
            return -1;
        }
    }
    
    // Explicit value override if provided separately
    if (value_str) {
        // "on", "yes", "true" -> 1
        if (strcmp(value_str, "on") == 0 || strcmp(value_str, "yes") == 0 || strcmp(value_str, "true") == 0) {
            feature->value = 1;
        } 
        // "off", "no", "false" -> 0
        else if (strcmp(value_str, "off") == 0 || strcmp(value_str, "no") == 0 || strcmp(value_str, "false") == 0) {
            feature->value = 0;
        } 
        else {
            feature->value = atoi(value_str);
        }
    }

    feature->start = 0;
    feature->end = (unsigned int)-1;
    return 0;
}

void luaotfload_features_add_defaults(hb_feature_t **features, unsigned int *count) {
    // Standard defaults: kern, liga, clig, rclt
    const char *defaults[] = { "+kern", "+liga", "+clig", "+rclt" };
    size_t num_defaults = 4;

    // Realloc or malloc
    hb_feature_t *new_arr = realloc(*features, sizeof(hb_feature_t) * (*count + num_defaults));
    if (!new_arr) return;

    *features = new_arr;
    
    for (size_t i = 0; i < num_defaults; i++) {
        hb_feature_from_string(defaults[i], -1, &(*features)[*count]);
        (*features)[*count].start = 0;
        (*features)[*count].end = (unsigned int)-1;
        (*count)++;
    }
}

hb_script_t luaotfload_features_parse_script(const char *script) {
    if (!script) return HB_SCRIPT_LATIN;
    return hb_script_from_string(script, -1);
}

hb_language_t luaotfload_features_parse_language(const char *lang) {
    if (!lang) return hb_language_from_string("en", -1);
    return hb_language_from_string(lang, -1);
}

