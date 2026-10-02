// decals_detach_from_structure_bsp  (not a Ghidra function; structure bsp deactivate slot 9)
// address 0x44e140, size 222 bytes
// name confidence: 0.5  rewrite confidence: 0.9
// evidence: structure_bsp_deactivate_procedures[9] (0x0069e934) holds 0x44e140; the inverse of
//   decal_rehash_object_decals 0x44e000 (activate slot 7), which moves object decals back into clusters.
// objdump 0x44e140..0x44e21d: with decal_data valid (+0x24), for every cluster (0x200, outer) and layer
//   (5, inner; row stride 0x800) the chain in decal_grid.cluster_first is walked: each decal's cluster
//   (+0x04) becomes -1, and at the tail (+0x34 == -1) the tail is linked in front of first_object_decal
//   (whose +0x30 gets the tail), the head becomes first_object_decal and the cell is cleared.
// blam-cc: no arguments

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *decal_data;       // 0x0087abe4
extern decal_grid *decal_grid_block; // 0x006b0ad8

void decals_detach_from_structure_bsp(void)
{
    int32_t cluster;
    int32_t layer;

    if (!decal_data->valid) {
        return;
    }
    for (cluster = 0; cluster < 0x200; cluster++) {
        for (layer = 0; layer < 5; layer++) {
            datum_index *cell = &decal_grid_block->cluster_first[layer][cluster];
            datum_index head = *cell;
            datum_index handle = head;

            while (handle != k_datum_index_none) {
                decal *entry = (decal *)((uint8_t *)decal_data->data + (handle & 0xffff) * 0x38);
                datum_index next = entry->next_decal;

                entry->cluster_index = -1;
                if (next == k_datum_index_none) {
                    datum_index first = decal_grid_block->first_object_decal;

                    entry->next_decal = first;
                    if (first != k_datum_index_none) {
                        ((decal *)((uint8_t *)decal_data->data + (first & 0xffff) * 0x38))->previous_decal = handle;
                    }
                    decal_grid_block->first_object_decal = head;
                    *cell = k_datum_index_none;
                }
                handle = next;
            }
        }
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
