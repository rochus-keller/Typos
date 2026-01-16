#include "luaotfload_io.h"
#include "luaotfload_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

// If linking inside LuaHBTeX, we might have access to <kpathsea/kpathsea.h>
// For this standalone/portable implementation, we'll mock the minimal Kpse interface
// or assume we bind to the symbols exported by the engine.

// Declaration of Kpathsea functions usually available in the binary
extern char *kpse_find_file(const char *name, int format, int must_exist);
#define kpse_opentype_format 44 
#define kpse_truetype_format 42

void luaotfload_io_init(void) {
    // Kpathsea should already be initialized by LuaHBTeX or our bridge
    luaotfload_log_report("io", LUAOTFLOAD_LOG_TRACE, "IO ready");
}

char *luaotfload_io_find_file(const char *filename) {
    // Try OpenType first
    char *path = kpse_find_file(filename, kpse_opentype_format, 0);
    if (!path) {
        // Try TrueType
        path = kpse_find_file(filename, kpse_truetype_format, 0);
    }
    
    if (path) {
        luaotfload_log_report("io", LUAOTFLOAD_LOG_TRACE, "Resolved '%s' -> '%s'", filename, path);
    } else {
        luaotfload_log_report("io", LUAOTFLOAD_LOG_SEARCH, "Could not resolve '%s'", filename);
    }
    return path;
}

bool luaotfload_io_file_exists(const char *path) {
    return access(path, F_OK) != -1;
}

void *luaotfload_io_read_file(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (length <= 0) {
        fclose(f);
        return NULL;
    }

    void *buffer = malloc(length + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(buffer, 1, length, f);
    fclose(f);
    
    ((char*)buffer)[length] = '\0'; // Null-terminate just in case it's text
    if (size) *size = (size_t)length;
    
    return buffer;
}

