// glow_delete  (not a Ghidra function; an object widget type callback)
// address 0x4fcd40, size 109 bytes
// name confidence: 0.6  rewrite confidence: 0.85
// evidence: object widget type table (records of 0x28 from 0x0069c010: fourcc, flag, initialize, dispose,
//   clear_disposing_flag, reset, new, delete, update, render) slot 0x69c07c = delete entry of 'glw!'. Only reachable
//   through that table. First-boot track: placing a campaign level's objects (weapons carry widgets).
// objdump 0x4fcd40..0x4fcdad: every particle on the glow's list (first at +0x250, next at +0x5c, particle
//   index at +4) is deleted from glow_particle_data, then the glow from glow_data. The glow comes from an inline
//   datum_get with no NULL check, as in the original.
// blam-cc: stack -> glow_index (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void datum_delete(data_array *array, datum_index index); // 0x4d0510, blam-cc: EAX -> array, EDX -> index
extern data_array *glow_data;          // 0x008603a0
extern data_array *glow_particle_data; // 0x008603a4
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

void glow_delete(datum_index glow_index)
{
    glow *self = (glow *)datum_try_get(glow_data, glow_index);
    uint8_t *particle = (uint8_t *)self->first_particle;

    while (particle != 0) {
        uint8_t *next = *(uint8_t **)(particle + 0x5c);

        datum_delete(glow_particle_data, *(datum_index *)(particle + 4));
        particle = next;
    }
    datum_delete(glow_data, glow_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
