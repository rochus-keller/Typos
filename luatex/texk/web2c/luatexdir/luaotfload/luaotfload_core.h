/**
 * @file luaotfload_core.h
 * @brief Core data structures and initialization for the C implementation of luaotfload.
 *
 * This header defines the fundamental structures used to bridge LuaTeX, HarfBuzz,
 * and the internal logic of the library. It replaces the dynamic Lua tables used
 * in the original implementation (specifically the `tfmdata` table and its `hb`
 * sub-table) with strict C structs.
 */

#ifndef LUAOTFLOAD_CORE_H
#define LUAOTFLOAD_CORE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* Lua headers (required for LuaTeX integration) */
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/* HarfBuzz headers */
#include <hb.h>
#include <hb-ot.h>

/* 
 * Simulation of LuaTeX internal types.
 * In the actual integration, these might come from LuaTeX's C API headers 
 * if available, or stay as opaque handles managed via the Lua stack.
 */
typedef int32_t halfword; /* Represents a pointer to a TeX node */
typedef int     font_id;  /* Internal TeX font ID number */

/* ========================================================================= */
/*                               Configuration                               */
/* ========================================================================= */

/**
 * @brief Log levels mirroring `luaotfload-log.lua`.
 */
typedef enum {
    LUAOTFLOAD_LOG_NONE    = 0,
    LUAOTFLOAD_LOG_COMMON  = 1,
    LUAOTFLOAD_LOG_LOADING = 2,
    LUAOTFLOAD_LOG_SEARCH  = 3,
    LUAOTFLOAD_LOG_TRACE   = 4
} luaotfload_log_level_t;

/**
 * @brief Global configuration state.
 * 
 * Maps to the `config.luaotfload` table found in `luaotfload-configuration.lua`.
 */
typedef struct {
    struct {
        luaotfload_log_level_t log_level;
        bool live;                  /* true for TeX run, false for tool */
        const char *resolver;       /* "cached" or "normal" */
        const char *definer;        /* "patch", "generic", etc. */
    } run;

    struct {
        char **location_precedence; /* "system", "texmf", "local" */
        size_t location_count;
        char **formats;             /* "otf", "ttf", "ttc", etc. */
        size_t format_count;
        bool scan_local;
        bool skip_read;
        bool strip;
        bool update_live;
        bool compress;
        long max_fonts;
    } db;

    struct {
        char *names_dir;
        char *cache_dir;
        char *index_file;
        char *lookups_file;
    } paths;

    struct {
        bool statistics;
        int termwidth;
        bool keepnames;
    } misc;

} luaotfload_config_t;

/* ========================================================================= */
/*                              Font Instance                                */
/* ========================================================================= */

/**
 * @brief Represents a loaded font instance ready for shaping.
 * 
 * This struct replaces the `tfmdata` Lua table used in the legacy code.
 * Specifically, it aggregates data previously found in `tfmdata`, `tfmdata.hb`,
 * and `tfmdata.hb.shared`.
 * 
 * Life Cycle:
 * 1. Created during the `define_font` callback.
 * 2. Stored in a registry mapping LuaTeX font IDs to these instances.
 * 3. Accessed during the `pre_linebreak_filter` (shaping) callback.
 */
typedef struct luaotfload_font_instance {
    /* --- LuaTeX Identification --- */
    font_id id;                 /* The internal ID assigned by LuaTeX */
    
    /* --- HarfBuzz Objects --- */
    hb_face_t *face;            /* The face (corresponds to tfmdata.hb.shared.face) */
    hb_font_t *font;            /* The scaled font (corresponds to tfmdata.hb.shared.font) */
    
    /* --- Source Information --- */
    char *filename;             /* Path to the font file */
    unsigned int subfont_index; /* Index for TTC collections (0-based) */
    char *ps_name;              /* PostScript name (for PDF generation) */
    char *full_name;            /* Full name for identification */
    
    /* --- Metrics & Scaling --- */
    double point_size;          /* Requested size in scaled points (sp) */
    double units_per_em;        /* UPEM from the font file */
    double scale_factor;        /* Conversion factor from font units to TeX dimensions */
    double extend;              /* Horizontal expansion/contraction (fake stretch) */
    double slant;               /* Artificial slant (fake italic) */
    
    /* --- Variations (Variable Fonts) --- */
    bool is_variable;
    hb_variation_t *variations; /* Array of active axis settings */
    unsigned int variation_count;
    
    /* --- Shaping Configuration --- */
    hb_feature_t *features;     /* Active OpenType features (+kern, -liga, etc.) */
    unsigned int feature_count;
    
    hb_script_t script;         /* Target script (e.g., HB_SCRIPT_LATIN) */
    hb_language_t language;     /* Target language system */
    
    /* --- Caching & Optimization --- */
    /* 
     * In the Lua code, `tfmdata.hb.shared` stored data common to multiple 
     * sizes of the same font. In C, we may keep a separate reference-counted
     * struct for the face data to avoid reloading the file from disk.
     */
    void *user_data;            /* Pointer to arbitrary data (e.g., node list cache) */

} luaotfload_font_instance_t;

/* ========================================================================= */
/*                             Core API Functions                            */
/* ========================================================================= */

/**
 * @brief Initialize the luaotfload core library.
 * 
 * Validates the LuaTeX version, initializes the logger, establishes 
 * HarfBuzz bindings, and sets up configuration defaults.
 * 
 * @param L The main Lua state from LuaTeX.
 * @return 0 on success, error code otherwise.
 */
int luaotfload_init(lua_State *L);

/**
 * @brief Clean up and free global resources.
 */
void luaotfload_cleanup(void);

/**
 * @brief Retrieve a font instance by its LuaTeX ID.
 * 
 * @param id The LuaTeX font ID.
 * @return Pointer to the font instance, or NULL if not found/managed by luaotfload.
 */
luaotfload_font_instance_t* luaotfload_get_font_instance(font_id id);

#endif /* LUAOTFLOAD_CORE_H */