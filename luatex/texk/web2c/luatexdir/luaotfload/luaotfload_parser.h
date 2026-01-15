/**
 * @file luaotfload_parser.h
 * @brief Hand-written recursive descent parser for font request strings.
 */

#ifndef LUAOTFLOAD_PARSER_H
#define LUAOTFLOAD_PARSER_H

#include "luaotfload_database.h"

/**
 * @brief Represents a feature key-value pair extracted from the request.
 * e.g., "mode" -> "harf", "+kern" -> "true"
 */
typedef struct {
    char *key;
    char *value;
} luaotfload_feature_pair_t;

/**
 * @brief Container for parsed features.
 */
typedef struct {
    luaotfload_feature_pair_t *items;
    size_t count;
    size_t capacity;
} luaotfload_feature_list_t;

/**
 * @brief Parse a canonical font request string.
 *
 * Syntax:
 *   Request -> Prefix ":" Name [":" Features]
 *   Prefix  -> "name" | "file" | "path" | "anon"
 *   Features -> Feature [";" Feature]*
 *   Feature -> Key "=" Value | "+" Key | "-" Key
 *
 * Example: "name:Arial:mode=harf;script=latn"
 *
 * @param request The raw string.
 * @param out_query Pointer to an existing query struct to populate.
 * @param out_features Pointer to a feature list to populate.
 * @return 0 on success, non-zero on parse error.
 */
int luaotfload_parse_font_request(const char *request, 
                                  luaotfload_font_query_t *out_query,
                                  luaotfload_feature_list_t *out_features);

/**
 * @brief Helper to initialize a feature list.
 */
void luaotfload_features_init(luaotfload_feature_list_t *list);

/**
 * @brief Helper to free a feature list.
 */
void luaotfload_features_free(luaotfload_feature_list_t *list);

#endif /* LUAOTFLOAD_PARSER_H */