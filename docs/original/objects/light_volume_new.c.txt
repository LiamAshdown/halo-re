// light_volume_new  (not a Ghidra function; an object widget type callback)
// address 0x4fe6d0, size 80 bytes
// name confidence: 0.6  rewrite confidence: 0.85
// evidence: object widget type table (records of 0x28 from 0x0069c010: fourcc, flag, initialize, dispose,
//   clear_disposing_flag, reset, new, delete, update, render) slot 0x69c0a0 = new entry of 'mgs2'. Only reachable
//   through that table. First-boot track: placing a campaign level's objects (weapons carry widgets).
// objdump 0x4fe6d0: datum_new(light_volume_instances); on success the element's +4 takes the definition tag (the original
//   writes through the inline datum_get result without a NULL check). Returns the new index or -1.
// blam-cc: stack -> definition_tag (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern datum_index datum_new(data_array *array); // 0x4d0480, blam-cc: EDX -> array
extern data_array *light_volume_instances; // 0x006b8d70
static void *datum_try_get(data_array *array, datum_index index)
{
    int16_t absolute = (int16_t)index;
    int16_t salt;

    if (absolute < 0 || absolute >= array->last_index) {
        return 0;
    }
    salt = *(int16_t *)((uint8_t *)array->data + absolute * array->size);
    if (salt == 0 || ((int16_t)(index >> 16) != 0 && (int16_t)(index >> 16) != salt)) {
        return 0;
    }
    return (uint8_t *)array->data + absolute * array->size;
}

datum_index light_volume_new(datum_index definition_tag)
{
    datum_index index = datum_new(light_volume_instances);

    if (index != k_datum_index_none) {
        *(datum_index *)((uint8_t *)datum_try_get(light_volume_instances, index) + 4) = definition_tag;
    }
    return index;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
