// scenery_new  (not a Ghidra function; the scenery type's creation callback)
// address 0x4fa7e0, size 134 bytes
// name confidence: 0.7  rewrite confidence: 0.9
// evidence: object_type_definition scenery (0x0069ba68) field +0x28 (query_create; object_type_definitions_query_0x28
//   calls it for every new object, cdecl with the object handle, and a zero result fails the creation). Only reachable
//   through that table; first-boot track: reached while placing the UI map's scenery.
//   objdump 0x4fa7e0..0x4fa865: when the scenery tag's animation graph (+0x44) exists and has animations (graph
//   +0x74 > 0), a random first animation is chosen (animation_choose_random_permutation(graph, 0, stream 1)); a
//   chosen one goes to object +0xd0 with the graph at +0xcc and flag 0x80. Flag 0x40000 is always set; the result
//   is 1.
// blam-cc: stack -> object_index (cdecl); returns AL
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation,
    int32_t stream); // 0x4d6280, blam-cc: EAX -> animation_graph_tag, DX -> first_animation, stack -> stream

uint8_t scenery_new(datum_index object_index)
{
    uint8_t *object = *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
    uint8_t *definition = (uint8_t *)tag_instances[*(datum_index *)object & 0xffff].data;
    datum_index graph = *(datum_index *)&((struct Object *)definition)->animation_graph.tag_id;

    if (graph != k_datum_index_none && *(int32_t *)((uint8_t *)tag_instances[graph & 0xffff].data + 0x74) > 0) {
        int16_t animation = animation_choose_random_permutation(graph, 0, 1);
        if (animation != -1) {
            *(int16_t *)(object + 0xd0) = animation;
            *(datum_index *)(object + 0xcc) = *(datum_index *)&((struct Object *)definition)->animation_graph.tag_id;
            *(uint32_t *)(object + 0x10) |= 0x80;
        }
    }
    *(uint32_t *)(object + 0x10) |= 0x40000;
    return 1;
}
