#include "luaotfload_debug.h"
#include "luaotfload_log.h"
#include <hb.h>
#include <stdlib.h>

bool luaotfload_debug_check_environment(void) {
    // Check HarfBuzz version (example requirement: 2.6.3+)
    unsigned int major, minor, micro;
    hb_version(&major, &minor, &micro);
    
    luaotfload_log_report("debug", LUAOTFLOAD_LOG_COMMON, 
                         "HarfBuzz version: %u.%u.%u", major, minor, micro);

    if (major < 2) {
        luaotfload_log_report("debug", LUAOTFLOAD_LOG_COMMON, 
                             "CRITICAL: HarfBuzz version too old.");
        return false;
    }
    return true;
}

void luaotfload_debug_dump_font(const luaotfload_font_instance_t *f) {
    if (!f) {
        luaotfload_log_report("debug", LUAOTFLOAD_LOG_TRACE, "Font Instance: NULL");
        return;
    }

    luaotfload_log_report("debug", LUAOTFLOAD_LOG_TRACE, "--- Font Instance [%d] ---", f->id);
    luaotfload_log_report("debug", LUAOTFLOAD_LOG_TRACE, "  Filename: %s", f->filename ? f->filename : "<none>");
    luaotfload_log_report("debug", LUAOTFLOAD_LOG_TRACE, "  Full Name: %s", f->full_name ? f->full_name : "<none>");
    luaotfload_log_report("debug", LUAOTFLOAD_LOG_TRACE, "  Subfont Index: %u", f->subfont_index);
    luaotfload_log_report("debug", LUAOTFLOAD_LOG_TRACE, "  Point Size: %.2f", f->point_size);
    luaotfload_log_report("debug", LUAOTFLOAD_LOG_TRACE, "  UPEM: %.2f", f->units_per_em);
    luaotfload_log_report("debug", LUAOTFLOAD_LOG_TRACE, "  Variable: %s", f->is_variable ? "yes" : "no");
    luaotfload_log_report("debug", LUAOTFLOAD_LOG_TRACE, "  Features: %u active", f->feature_count);
    luaotfload_log_report("debug", LUAOTFLOAD_LOG_TRACE, "------------------------");
}

void luaotfload_debug_assert(bool condition, const char *message, const char *file, int line) {
    if (!condition) {
        luaotfload_log_report("ASSERT", LUAOTFLOAD_LOG_COMMON, 
                             "Assertion failed in %s:%d: %s", file, line, message);
        abort();
    }
}

