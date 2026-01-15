/**
 * @file mock_luatex.h
 * @brief Mock structures for LuaTeX nodes to allow standalone compilation and testing.
 */

#ifndef MOCK_LUATEX_H
#define MOCK_LUATEX_H

/* Node Types (Simplified) */
enum {
    NODE_GLYPH = 0,
    NODE_DISC  = 1,
    NODE_GLUE  = 2,
    NODE_KERN  = 3
};

/* Generic Node Structure */
typedef struct node_t {
    int id;                 /* Type ID (GLYPH, KERN, etc.) */
    int subtype;
    struct node_t *next;
    
    /* Glyph Fields */
    int char_code;          /* Unicode codepoint or Glyph ID */
    int font;               /* Font ID */
    
    /* Dimensions / Positioning */
    int x_offset;
    int y_offset;
    int width;              /* Advance width */
    int height;
    int depth;
    
    /* Kern Fields */
    int kern;
} node_t;

/* Helper to create a glyph node */
static inline node_t* new_glyph(int ch) {
    node_t *n = (node_t*)calloc(1, sizeof(node_t));
    n->id = NODE_GLYPH;
    n->char_code = ch;
    return n;
}

#endif /* MOCK_LUATEX_H */