/**
 * @file luaotfload_database.h
 * @brief Font database interface for discovery and lookup.
 *
 * This module handles the persistent indexing of system fonts and provides
 * search capabilities (fuzzy matching, optical sizing resolution) to map
 * user requests (names) to physical file paths.
 */

#ifndef LUAOTFLOAD_DATABASE_H
#define LUAOTFLOAD_DATABASE_H

#include "luaotfload_core.h"

/**
 * @brief Represents a user query for a font.
 * Maps roughly to the specification table passed to `lookup_font_name`.
 */
typedef struct {
    const char *name;       /* Primary lookup key (e.g., "Arial", "Minion Pro") */
    const char *style;      /* Style modifier (e.g., "b", "i", "bi", "r") */
    const char *location;   /* Preferred location ("system", "local", "texmf") */
    
    double size;            /* Requested size in sp (for optical size selection) */
    
    bool exact_match;       /* If true, disable fuzzy matching logic */
    bool resolve_conflicts; /* Whether to resolve file name conflicts */
} luaotfload_font_query_t;

/**
 * @brief Represents a physical font file resolved from the database.
 */
typedef struct {
    char *fullpath;         /* Absolute path to the font file */
    char *filename;         /* Base filename (e.g., "arial.ttf") */
    char *format;           /* "otf", "ttf", "ttc", "afm" */
    
    unsigned int index;     /* Subfont index (0-based) for collections (TTC) */
    
    /* Metadata cached in the database used for selection */
    char *ps_name;          /* PostScript Name */
    char *family_name;      /* Preferred Family Name */
    char *style_name;       /* Subfamily/Style Name */
    
    double design_size;     /* Optical design size (0.0 if undefined) */
    double weight;          /* Weight class (usWeightClass) */
    double width;           /* Width class (usWidthClass) */
    bool italic;            /* Italic flag */
} luaotfload_font_entry_t;

/* ========================================================================= */
/*                              Database API                                 */
/* ========================================================================= */

/**
 * @brief Initialize the database subsystem.
 * 
 * In a full implementation, this loads the persistent `luaotfload-names.lua` 
 * (or binary equivalent) into memory or verifies its freshness.
 * 
 * @param config Global configuration (defines paths, formats, etc).
 * @return 0 on success, non-zero error code otherwise.
 */
int luaotfload_db_init(luaotfload_config_t *config);

/**
 * @brief Release database resources.
 */
void luaotfload_db_cleanup(void);

/**
 * @brief Find the closest matching font for a given query.
 * 
 * This implements the core resolution logic found in `luaotfload-database.lua`,
 * including:
 * 1. Exact name matching.
 * 2. Fuzzy/Levenshtein matching (if exact match fails).
 * 3. Optical size selection (choosing best fit from a family).
 * 
 * @param query The search specifications.
 * @return A newly allocated font entry (caller must free), or NULL if not found.
 */
luaotfload_font_entry_t* luaotfload_db_find_closest(const luaotfload_font_query_t *query);

/**
 * @brief Free a font entry object returned by the database.
 * 
 * @param entry The entry to free. Safe to call with NULL.
 */
void luaotfload_db_free_entry(luaotfload_font_entry_t *entry);

/**
 * @brief Reload the database (force rescan).
 * 
 * @return 0 on success.
 */
int luaotfload_db_reload(void);

#endif /* LUAOTFLOAD_DATABASE_H */