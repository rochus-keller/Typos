/**
 * @file luaotfload_database.c
 * @brief Implementation of the font database lookup.
 *
 * NOTE: This is currently a STUB implementation for architectural verification.
 * The full directory scanning and fuzzy matching logic is omitted.
 */

#include "luaotfload_database.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* Global database state (stub) */
static struct {
    bool initialized;
    luaotfload_config_t *config;
} db_state = { false, NULL };

/* Helper for C99 string duplication */
static char* strdup_safe(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* new_s = malloc(len);
    if (new_s) memcpy(new_s, s, len);
    return new_s;
}

int luaotfload_db_init(luaotfload_config_t *config) {
    if (db_state.initialized) return 0;
    
    db_state.config = config;
    db_state.initialized = true;
    
    /* In full impl: Load luaotfload-names.lua / .luc here */
    
    return 0;
}

void luaotfload_db_cleanup(void) {
    db_state.config = NULL;
    db_state.initialized = false;
}

/* Stub helper for case-insensitive comparison */
static int strcasecmp_stub(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (tolower((unsigned char)*s1) != tolower((unsigned char)*s2))
            return 1;
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

luaotfload_font_entry_t* luaotfload_db_find_closest(const luaotfload_font_query_t *query) {
    if (!db_state.initialized) return NULL;
    if (!query || !query->name) return NULL;

    /* 
     * STUB: Hardcoded lookup to verify linking pipeline.
     * We simulate finding "Arial" regardless of case.
     */
    if (strcasecmp_stub(query->name, "arial") == 0) {
        luaotfload_font_entry_t *entry = calloc(1, sizeof(luaotfload_font_entry_t));
        if (!entry) return NULL;

        /* Simulate a system path finding */
        #ifdef _WIN32
        entry->fullpath = strdup_safe("C:\\Windows\\Fonts\\arial.ttf");
        #else
        entry->fullpath = strdup_safe("/usr/share/fonts/truetype/msttcorefonts/arial.ttf");
        #endif

        entry->filename     = strdup_safe("arial.ttf");
        entry->format       = strdup_safe("ttf");
        entry->index        = 0;
        entry->ps_name      = strdup_safe("ArialMT");
        entry->family_name  = strdup_safe("Arial");
        entry->style_name   = strdup_safe("Regular");
        entry->design_size  = 0.0; /* Not optical */
        entry->weight       = 400.0;
        entry->width        = 5.0; /* Medium */
        entry->italic       = false;

        return entry;
    }

    /* 
     * STUB: Hardcoded lookup for a TeX Gyre font to test otf loading
     */
    if (strcasecmp_stub(query->name, "texgyretermes") == 0) {
        luaotfload_font_entry_t *entry = calloc(1, sizeof(luaotfload_font_entry_t));
        if (!entry) return NULL;

        /* Path assumption for testing */
        entry->fullpath     = strdup_safe("texgyretermes-regular.otf");
        entry->filename     = strdup_safe("texgyretermes-regular.otf");
        entry->format       = strdup_safe("otf");
        entry->index        = 0;
        entry->ps_name      = strdup_safe("TeXGyreTermes-Regular");
        entry->family_name  = strdup_safe("TeX Gyre Termes");
        entry->style_name   = strdup_safe("Regular");
        entry->design_size  = 0.0; 
        entry->weight       = 400.0;
        entry->italic       = false;

        return entry;
    }

    return NULL; /* Not found */
}

void luaotfload_db_free_entry(luaotfload_font_entry_t *entry) {
    if (!entry) return;
    
    free(entry->fullpath);
    free(entry->filename);
    free(entry->format);
    free(entry->ps_name);
    free(entry->family_name);
    free(entry->style_name);
    
    free(entry);
}

int luaotfload_db_reload(void) {
    /* STUB: In full impl, this would rescan directories and rebuild the index */
    if (!db_state.initialized) return -1;
    return 0;
}
