#include "luaotfload_core.h"
#include "luaotfload_database.h"
#include <stdio.h>

int main() {
    luaotfload_font_query_t query = { .name = "Arial", .size = 12.0 };
    luaotfload_config_t conf;
    luaotfload_db_init(&conf);
    luaotfload_font_entry_t* entry = luaotfload_db_find_closest(&query);
    printf("Found font at: %s\n", entry ? entry->filename : "NULL");
    luaotfload_db_cleanup();
    return 0;
}

