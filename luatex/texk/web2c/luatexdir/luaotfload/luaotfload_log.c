#include "luaotfload_log.h"
#include <stdio.h>
#include <stdlib.h>

static struct {
    luaotfload_log_level_t level;
    luaotfload_log_callback_t callback;
    void *callback_data;
} log_state = {
    .level = LUAOTFLOAD_LOG_COMMON,
    .callback = NULL,
    .callback_data = NULL
};

void luaotfload_log_init(luaotfload_log_level_t level) {
    log_state.level = level;
}

void luaotfload_log_set_callback(luaotfload_log_callback_t callback, void *user_data) {
    log_state.callback = callback;
    log_state.callback_data = user_data;
}

static void log_dispatch(const char *buffer) {
    if (log_state.callback) {
        log_state.callback(buffer, log_state.callback_data);
    } else {
        fprintf(stderr, "luaotfload: %s\n", buffer);
    }
}

void luaotfload_log(luaotfload_log_level_t level, const char *fmt, ...) {
    if (level > log_state.level) return;

    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    log_dispatch(buffer);
}

void luaotfload_log_report(const char *component, luaotfload_log_level_t level, const char *fmt, ...) {
    if (level > log_state.level) return;

    char message[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    char buffer[1200];
    snprintf(buffer, sizeof(buffer), "[%s] %s", component, message);
    
    log_dispatch(buffer);
}

