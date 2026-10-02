// structure_runtime_decals_evict  (not a Ghidra function; structure bsp deactivate slot 8)
// address 0x553070, size 93 bytes
// name confidence: 0.4  rewrite confidence: 0.9
// evidence: structure_bsp_deactivate_procedures[8] (0x0069e930) holds 0x553070; the cluster walk matches
//   structure_decals_update_switch_transitions 0x5530d0 (clusters +0x134/+0x138, 0x68 each, first decal
//   +0x0c, decal count +0x0e).
// objdump 0x553070..0x5530cc: when the structure bsp (0x00746f9c) has runtime decals (+0x258 nonzero),
//   every cluster with decals is handed to decal_evict_object_decals 0x44e310 (stack index).
// blam-cc: no arguments

#include "tags.h"
#include "memory.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern ScenarioStructureBSP *global_structure_bsp;

extern void decal_evict_object_decals(int16_t cluster_index); // 0x44e310

void structure_runtime_decals_evict(void)
{
    int16_t cluster_count;
    int16_t cluster_index;

    if (global_structure_bsp->runtime_decals.count == 0) {
        return;
    }
    cluster_count = *(int16_t *)&global_structure_bsp->clusters.count;
    for (cluster_index = 0; cluster_index < cluster_count; cluster_index++) {
        uint8_t *cluster = (uint8_t *)global_structure_bsp->clusters.pointer + cluster_index * 0x68;

        if (*(uint16_t *)(cluster + 0xc) != 0xffff && *(int16_t *)(cluster + 0xe) != 0) {
            decal_evict_object_decals(cluster_index);
        }
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
