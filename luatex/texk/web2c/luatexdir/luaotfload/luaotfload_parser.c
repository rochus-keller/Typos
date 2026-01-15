/**
 * @file luaotfload_parser.c
 * @brief Implementation of the font request parser.
 */

#include "luaotfload_parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* --- Helper: Feature List Management --- */

void luaotfload_features_init(luaotfload_feature_list_t *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void feature_add(luaotfload_feature_list_t *list, const char *key, const char *val) {
    if (list->count == list->capacity) {
        size_t new_cap = list->capacity == 0 ? 4 : list->capacity * 2;
        list->items = realloc(list->items, new_cap * sizeof(luaotfload_feature_pair_t));
        list->capacity = new_cap;
    }
    
    list->items[list->count].key = strdup(key);
    list->items[list->count].value = val ? strdup(val) : NULL;
    list->count++;
}

void luaotfload_features_free(luaotfload_feature_list_t *list) {
    for (size_t i = 0; i < list->count; i++) {
        free(list->items[i].key);
        free(list->items[i].value);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
}

/* --- Helper: String Utils --- */

static char* strndup_safe(const char *s, size_t n) {
    char *p = malloc(n + 1);
    if (p) {
        memcpy(p, s, n);
        p[n] = '\0';
    }
    return p;
}

/* --- Parsing Context --- */

typedef struct {
    const char *p;
} p_ctx;

static void skip_space(p_ctx *ctx) {
    while (*ctx->p && isspace((unsigned char)*ctx->p)) ctx->p++;
}

static int peek(p_ctx *ctx) {
    return *ctx->p;
}

static int next_char(p_ctx *ctx) {
    if (*ctx->p) return *ctx->p++;
    return 0;
}

static int match(p_ctx *ctx, char c) {
    if (*ctx->p == c) {
        ctx->p++;
        return 1;
    }
    return 0;
}

/* Read until delimiter or EOS */
static char* read_until(p_ctx *ctx, const char *delims) {
    const char *start = ctx->p;
    while (*ctx->p && !strchr(delims, *ctx->p)) {
        ctx->p++;
    }
    return strndup_safe(start, ctx->p - start);
}

/* --- Recursive Descent Functions --- */

/* Feature -> Key "=" Value | "+" Key | "-" Key */
static void parse_feature(p_ctx *ctx, luaotfload_feature_list_t *out_features) {
    skip_space(ctx);
    
    char switch_char = 0;
    if (peek(ctx) == '+' || peek(ctx) == '-') {
        switch_char = next_char(ctx);
    }
    
    /* Key */
    const char *k_start = ctx->p;
    while (isalnum((unsigned char)*ctx->p) || *ctx->p == '_' || *ctx->p == '.') {
        ctx->p++;
    }
    
    if (k_start == ctx->p) return; /* Empty key */
    char *key = strndup_safe(k_start, ctx->p - k_start);
    
    skip_space(ctx);
    
    if (switch_char) {
        /* Boolean feature */
        feature_add(out_features, key, switch_char == '+' ? "true" : "false");
    } else if (match(ctx, '=')) {
        /* Key=Value */
        skip_space(ctx);
        /* Value goes until semicolon or end */
        char *val = read_until(ctx, ";");
        feature_add(out_features, key, val);
        free(val);
    } else {
        /* Implicit "true" */
        feature_add(out_features, key, "true");
    }
    
    free(key);
}

/* Features -> Feature [";" Feature]* */
static void parse_features(p_ctx *ctx, luaotfload_feature_list_t *out_features) {
    while (*ctx->p) {
        parse_feature(ctx, out_features);
        if (!match(ctx, ';')) break;
    }
}

/* Request -> Prefix ":" Name [":" Features] */
int luaotfload_parse_font_request(const char *request, 
                                  luaotfload_font_query_t *out_query,
                                  luaotfload_feature_list_t *out_features) 
{
    if (!request || !out_query) return -1;
    
    p_ctx ctx_obj = { request };
    p_ctx *ctx = &ctx_obj;
    
    /* 1. Prefix */
    char *prefix = read_until(ctx, ":");
    if (!match(ctx, ':')) {
        /* No colon found? Treat whole string as name (Anon lookup) */
        out_query->name = prefix; /* Consumes prefix */
        out_query->location = strdup("system"); /* Default assumption */
        return 0;
    }
    
    /* Map prefix to logic (stub logic) */
    if (strcmp(prefix, "file") == 0) {
        out_query->location = strdup("file");
    } else if (strcmp(prefix, "name") == 0) {
        out_query->location = strdup("system");
    } else {
        /* Unknown prefix, assume name */
        /* But we need to put the prefix back into the name if it wasn't a reserved keyword */
        /* For this simple parser, just assume prefix was intended location hint */
        out_query->location = strdup("system"); 
    }
    free(prefix);
    
    /* 2. Name */
    char *name = read_until(ctx, ":");
    out_query->name = name;
    
    /* 3. Features (Optional) */
    if (match(ctx, ':')) {
        if (out_features) {
            parse_features(ctx, out_features);
        }
    }
    
    /* Default style if not parsed from features */
    out_query->style = NULL; 
    
    return 0;
}