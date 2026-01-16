#include "luaotfload_core.h"
#include "luaotfload_log.h"
#include "luaotfload_config.h"
#include "luaotfload_debug.h"
#include <stdlib.h>

// Global Configuration Instance
static luaotfload_config_t global_config;

// Global Font Registry (Simple array for Phase 1, hash map for production)
#define MAX_OPEN_FONTS 256
luaotfload_font_instance_t *font_registry[MAX_OPEN_FONTS] = {0};

int luaotfload_init(lua_State *L) {
    // 1. Initialize Logging first (so we can see errors)
    // Start with a safe default until config is loaded
    luaotfload_log_init(LUAOTFLOAD_LOG_COMMON);
    luaotfload_log_report("core", LUAOTFLOAD_LOG_COMMON, "Initializing luaotfload C core...");

    // 2. Check Environment
    if (!luaotfload_debug_check_environment()) {
        luaotfload_log_report("core", LUAOTFLOAD_LOG_COMMON, "CRITICAL: Environment check failed.");
        return -1;
    }

    // 3. Load Configuration
    // In a real scenario, we would parse "luaotfload.conf" here.
    // For now, we load defaults as tested.
    luaotfload_config_set_defaults(&global_config);

    luaotfload_io_init();  // Initialize kpathsea

    
    // Update log level based on config
    luaotfload_log_init(global_config.run.log_level);
    luaotfload_log_report("core", LUAOTFLOAD_LOG_SEARCH, "Configuration loaded.");

    // 4. Initialize Subsystems (Placeholders for your other modules)
    // luaotfload_database_init(&global_config); 
    // luaotfload_harf_provider_init();

    luaotfload_log_report("core", LUAOTFLOAD_LOG_COMMON, "Initialization complete.");
    return 0;
}

void luaotfload_cleanup(void) {
    luaotfload_log_report("core", LUAOTFLOAD_LOG_COMMON, "Shutting down...");
    
    // Free configuration memory
    luaotfload_config_free(&global_config);

    // Free font registry
    for (int i = 0; i < MAX_OPEN_FONTS; i++) {
        if (font_registry[i]) {
            // In a real impl, you'd call a font destructor here
            free(font_registry[i]); 
            font_registry[i] = NULL;
        }
    }
}

luaotfload_font_instance_t* luaotfload_get_font_instance(font_id id) {
    if (id < 0 || id >= MAX_OPEN_FONTS) return NULL;
    return font_registry[id];
}

