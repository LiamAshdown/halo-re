// object_lights_detach_from_structure_bsp  (not a Ghidra function; structure bsp deactivate slot 2)
// address 0x4f2cb0, size 159 bytes
// name confidence: 0.5  rewrite confidence: 0.9
// evidence: structure_bsp_deactivate_procedures[2] (0x0069e918) holds 0x4f2cb0; its activate
//   counterpart is object_lights_refresh_transforms 0x4f2d50 (activate slot 1).
// objdump 0x4f2cb0..0x4f2d4e: every light (light_data 0x00860b14, 0x7c each; the next datum is found
//   inline, the same scan as datum_next) with flag byte +0x02 bit 2 set: when bit 1 is also set its
//   cluster references are dropped (cluster_reference_remove_all, EBX = light_cluster_first 0x00860b20,
//   stack handle, &light +0x10) and bit 2 is cleared, then bit 2 is set again.
// blam-cc: no arguments

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "structures.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *light_data;                     // 0x00860b14
extern cluster_reference_group light_cluster_first; // 0x00860b20

extern datum_index datum_next(int16_t after_index, data_array *array); // memory module, 0x4d0630
extern void cluster_reference_remove_all(uint32_t handle, datum_index *link, cluster_reference_group *cluster_list); // 0x552020

void object_lights_detach_from_structure_bsp(void)
{
    datum_index handle;

    for (handle = datum_next(-1, light_data); handle != k_datum_index_none;
         handle = datum_next((int16_t)handle, light_data)) {
        uint8_t *light = (uint8_t *)light_data->data + (handle & 0xffff) * 0x7c;

        if (light[2] & 4) {
            if (light[2] & 2) {
                cluster_reference_remove_all(handle, (datum_index *)(light + 0x10), &light_cluster_first);
                light[2] &= 0xfb;
            }
            light[2] |= 4;
        }
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
