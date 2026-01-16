#include <lua.h>
#include <lauxlib.h>
#include "luaotfload_core.h"
#include "luaotfload_log.h"

// Define compatibility types if your code uses 'node_t' vs 'halfword'
// Assuming node_t* maps to LuaTeX's halfword (int) or void* 
// ADJUST THIS TYPEDEF TO MATCH YOUR HEADER:
typedef void node_t; 

// Import your existing function
extern void luaotfload_shape_nodes(luaotfload_font_instance_t *font, 
                                   node_t *head,
                                   hb_script_t script,
                                   hb_language_t language,
                                   const hb_feature_t *features,
                                   unsigned int num_features);

// LuaTeX Internal API (needed for list traversal)
#include "ptexlib.h"
#define is_glyph(p) (type(p) == glyph_node)

/**
 * @brief The Driver Function
 * This is registered as the 'pre_linebreak_filter' callback.
 */
int luaotfload_shape_process(lua_State *L) {
    // 1. Get the head of the node list from Lua
    halfword head = (halfword)lua_touserdata(L, 1);
    if (!head) return 0;

    // 2. Iterate through the list to find runs of the same font
    halfword p = head;
    
    while (p) {
        if (is_glyph(p)) {
            font_id fid = font(p);
            
            // 3. Lookup the font in our Registry
            luaotfload_font_instance_t *inst = luaotfload_get_font_instance(fid);
            
            if (inst) {
                // Found a run managed by us!
                halfword run_head = p;
                halfword run_tail = p;
                
                // Find the end of this run
                // (Keep going as long as font matches)
                while (vlink(run_tail) && is_glyph(vlink(run_tail)) && font(vlink(run_tail)) == fid) {
                    run_tail = vlink(run_tail);
                }
                
                // Isolate the run (temporarily break the list)
                halfword next_node = vlink(run_tail);
                vlink(run_tail) = 0; // Null-terminate for the shaper
                
                // Note: We cast halfword (int) to node_t* if required
                luaotfload_shape_nodes(inst, 
                                     (node_t*)run_head, 
                                     inst->script, 
                                     inst->language, 
                                     inst->features, 
                                     inst->feature_count);
                
                // Re-link the list
                vlink(run_tail) = next_node;
                
                // Advance
                p = next_node;
                continue;
            }
        }
        p = vlink(p);
    }

    // Return the list head to Lua
    lua_pushlightuserdata(L, (void*)head);
    return 1;
}

