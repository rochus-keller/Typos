#include "luaotfload_config.h"
#include "luaotfload_log.h"
#include <string.h>
#include <stdlib.h>

// Helper to duplicate strings safely
static char *safe_strdup(const char *s) {
    return s ? strdup(s) : NULL;
}

void luaotfload_config_set_defaults(luaotfload_config_t *cfg) {
    if (!cfg) return;

    // Zero out first to ensure clean state
    memset(cfg, 0, sizeof(luaotfload_config_t));

    // -- Run defaults --
    cfg->run.log_level = LUAOTFLOAD_LOG_COMMON;
    cfg->run.live = true;
    cfg->run.resolver = "cached";
    cfg->run.definer = "generic";

    // -- Database defaults --
    cfg->db.scan_local = true;
    cfg->db.skip_read = false;
    cfg->db.strip = true;
    cfg->db.update_live = true;
    cfg->db.compress = true;
    cfg->db.max_fonts = -1; // Unlimited

    // Locations (dynamic allocation)
    const char *default_locs[] = { "system", "texmf" }; // Removed "local" for default safety
    cfg->db.location_count = 2;
    cfg->db.location_precedence = malloc(sizeof(char*) * cfg->db.location_count);
    for(size_t i=0; i < cfg->db.location_count; i++) {
        cfg->db.location_precedence[i] = safe_strdup(default_locs[i]);
    }

    // Formats
    const char *default_fmts[] = { "otf", "ttf", "ttc" };
    cfg->db.format_count = 3;
    cfg->db.formats = malloc(sizeof(char*) * cfg->db.format_count);
    for(size_t i=0; i < cfg->db.format_count; i++) {
        cfg->db.formats[i] = safe_strdup(default_fmts[i]);
    }

    // -- Paths defaults --
    cfg->paths.names_dir = safe_strdup("names");
    cfg->paths.cache_dir = safe_strdup("luaotfload"); // usually inside generic/
    cfg->paths.index_file = safe_strdup("luaotfload-names.lua"); // or .luc
    cfg->paths.lookups_file = safe_strdup("luaotfload-lookup-cache.lua");

    // -- Misc defaults --
    cfg->misc.statistics = false;
    cfg->misc.termwidth = 80;
    cfg->misc.keepnames = false;
}

int luaotfload_config_parse_file(luaotfload_config_t *config, const char *filename) {
    // TODO: Implement Kpathsea lookup and file parsing.
    // For Phase 1, we rely on defaults.
    luaotfload_log_report("config", LUAOTFLOAD_LOG_SEARCH, 
                         "Configuration file parsing not yet implemented for: %s", filename);
    return 0; // Pretend success for now
}

void luaotfload_config_free(luaotfload_config_t *cfg) {
    if (!cfg) return;

    if (cfg->db.location_precedence) {
        for (size_t i = 0; i < cfg->db.location_count; i++) free(cfg->db.location_precedence[i]);
        free(cfg->db.location_precedence);
    }
    if (cfg->db.formats) {
        for (size_t i = 0; i < cfg->db.format_count; i++) free(cfg->db.formats[i]);
        free(cfg->db.formats);
    }
    
    free(cfg->paths.names_dir);
    free(cfg->paths.cache_dir);
    free(cfg->paths.index_file);
    free(cfg->paths.lookups_file);
    
    // Reset pointers to prevent use-after-free
    memset(cfg, 0, sizeof(luaotfload_config_t));
}

