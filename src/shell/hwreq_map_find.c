// hwreq_map_find  (not a Ghidra function; std::map<std::string, T*>::find for the hwreq parser's maps)
// address 0x57b7a0, size 89 bytes
// name confidence: 0.8   rewrite confidence: 0.9
// WRITTEN 2026-09-28 from objdump 0x57b7a0..0x57b7f8: lower_bound of the key (tree_lower_bound 0x57c530); unless that
//   is the head or the key compares below the node's key (string_compare 0x57ce10 < 0), the node, else the head. The
//   binary writes the iterator through EBX and returns that address; the callers only read the node, so it is
//   returned directly (as their prototypes already expect).
// blam-cc: EDI map, ESI key, EBX iterator slot

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "fn_shell.h"


hwreq_map_node *hwreq_map_find(msvc_std_map *map, msvc_std_string *key)
{
    hwreq_map_node *node = tree_lower_bound(map, key);

    if (node != (hwreq_map_node *)map->head) {
        const char *node_key = node->key.capacity >= 0x10 ? (const char *)node->key.buffer.heap_buffer : node->key.buffer.inline_buffer;

        if (string_compare(key, key->size, 0, node_key, node->key.size) >= 0) {
            return node;
        }
    }
    return (hwreq_map_node *)map->head;
}
