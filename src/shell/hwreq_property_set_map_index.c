// hwreq_property_set_map_index  (not a Ghidra function; std::map<std::string, hwreq_property_set *>::operator[])
// address 0x57b6e0, size 189 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x57b6e0..0x57b79c: lower_bound of the key; when that is the head or the key
//   compares below it, a (copy of the key, NULL) pair is inserted with that node as the hint
//   (tree_hint_insert_unique 0x57ba50) and the local key copy released. Returns the address of the node's value.
// blam-cc: EDI key, stack -> map (callee pops 4)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

typedef struct hwreq_map_value_type {
    msvc_std_string key;
    uint32_t value;
} hwreq_map_value_type; // size 0x20, see tree_node_allocate.c

extern hwreq_map_node *tree_lower_bound(msvc_std_map *tree, const msvc_std_string *search_key); // 0x57c530, blam-cc: EAX tree, ECX key
extern int32_t string_compare(const msvc_std_string *this, uint32_t n1, uint32_t pos, const char *s, uint32_t n2); // 0x57ce10
extern msvc_std_string *string_assign_substr(msvc_std_string *this, const msvc_std_string *right, uint32_t pos,
    uint32_t count); // 0x57b830, blam-cc: ECX this, stack right, pos, count
extern hwreq_map_node *tree_hint_insert_unique(msvc_std_map *tree, hwreq_map_node **result_holder,
    hwreq_map_node *hint, const hwreq_map_value_type *value); // 0x57ba50

hwreq_property_set **hwreq_property_set_map_index(msvc_std_string *key, msvc_std_map *map)
{
    hwreq_map_node *node = tree_lower_bound(map, key);

    if (node == (hwreq_map_node *)map->head ||
        string_compare(key, key->size, 0, node->key.capacity >= 0x10 ? (const char *)node->key.buffer.heap_buffer :
            node->key.buffer.inline_buffer, node->key.size) < 0) {
        hwreq_map_value_type pair;
        hwreq_map_node *inserted;

        pair.key.capacity = 0xf;
        pair.key.size = 0;
        pair.key.buffer.inline_buffer[0] = 0;
        string_assign_substr(&pair.key, key, 0, 0xffffffff);
        pair.value = 0;
        tree_hint_insert_unique(map, &inserted, node, &pair);
        node = inserted;
        if (pair.key.capacity >= 0x10) {
            free((void *)pair.key.buffer.heap_buffer);
        }
    }
    return (hwreq_property_set **)&node->value;
}
