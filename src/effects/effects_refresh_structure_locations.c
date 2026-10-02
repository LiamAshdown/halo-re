// effects_refresh_structure_locations  (not a Ghidra function; structure bsp activate slot 3)
// address 0x450e80, size 284 bytes
// name confidence: 0.5  rewrite confidence: 0.85
// evidence: structure_bsp_activate_procedures[3] (0x0069e8e8) holds 0x450e80; the leaf/cluster store
//   matches decal_rehash_object_decals 0x44e000 (activate slot 7).
// objdump 0x450e80..0x450f9b: every effect (effect_data 0x0087abdc, 0xfc each; next datum found inline)
//   not attached to an object (+0x3c == -1) takes its first plain location marker (the inlined first step
//   of effect_marker_next 0x453180, mode 0, from +0x5c); without one it is deleted (effect_delete
//   0x450be0), else its location (+0x10 leaf, +0x14 cluster) is re-found from the marker position (+0x30)
//   with bsp3d_node_find_leaf (EAX 0, ECX global_collision_bsp 0x00746f90, EDX point) and the leaf's
//   cluster (structure bsp +0xe4, 0x10 each, +0x08), -1 outside.
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
extern data_array *effect_data;                         // 0x0087abdc
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90
extern ScenarioStructureBSP *global_structure_bsp;

extern datum_index datum_next(int16_t after_index, data_array *array); // memory module, 0x4d0630
extern effect_location_marker *effect_marker_next(effect *self, datum_index *marker, int32_t mode); // 0x453180
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0, EAX node, ECX bsp, EDX point
extern void effect_delete(datum_index effect_handle); // 0x450be0

void effects_refresh_structure_locations(void)
{
    datum_index handle;

    for (handle = datum_next(-1, effect_data); handle != k_datum_index_none;
         handle = datum_next((int16_t)handle, effect_data)) {
        effect *entry = (effect *)((uint8_t *)effect_data->data + (handle & 0xffff) * 0xfc);
        datum_index marker;
        effect_location_marker *location;
        uint32_t leaf;

        if (entry->object_index != k_datum_index_none) {
            continue;
        }
        marker = entry->location_markers[0];
        location = effect_marker_next(entry, &marker, 0);
        if (location == 0) {
            effect_delete(handle);
            continue;
        }
        leaf = bsp3d_node_find_leaf(0, global_collision_bsp, (real_point3d *)((uint8_t *)location + 0x30));
        entry->location.leaf_index = (int32_t)leaf;
        if (leaf == 0xffffffff) {
            entry->location.cluster_index = -1;
        } else {
            entry->location.cluster_index =
                *(int16_t *)((uint8_t *)global_structure_bsp->leaves.pointer + (leaf & 0x7fffffff) * 0x10 + 8);
        }
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
