// sounds_refresh_structure_locations  (not a Ghidra function; structure bsp activate slot 11)
// address 0x549a00, size 235 bytes
// name confidence: 0.4  rewrite confidence: 0.85
// evidence: structure_bsp_activate_procedures[11] (0x0069e908) holds 0x549a00; same leaf/cluster refresh as
//   effects_refresh_structure_locations 0x450e80 (activate slot 3), over sound_data.
// objdump 0x549a00..0x549aea: with sound initialized (0x00725200), enabled (0x00725201) and not disabled
//   (0x007252b6), every sound (sound_data 0x007252c0, 0xb0 each; next datum found inline) whose +0x14 word
//   is 1 has its leaf (+0x44) and cluster (+0x48, the structure bsp +0xe4 leaf's +0x08, -1 outside)
//   re-found from its position (+0x20) with bsp3d_node_find_leaf (EAX 0, ECX global_collision_bsp, EDX).
// blam-cc: no arguments

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "sound.h"

extern uint8_t sound_initialized;                       // 0x00725200
extern uint8_t sound_enabled;                           // 0x00725201
extern uint8_t sound_disabled;                          // 0x007252b6
extern data_array *sound_data;                          // 0x007252c0
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c

extern datum_index datum_next(int16_t after_index, data_array *array); // memory module, 0x4d0630
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0, EAX node, ECX bsp, EDX point

void sounds_refresh_structure_locations(void)
{
    datum_index handle;

    if (!sound_initialized || !sound_enabled || sound_disabled) {
        return;
    }
    for (handle = datum_next(-1, sound_data); handle != k_datum_index_none;
         handle = datum_next((int16_t)handle, sound_data)) {
        sound *entry = (sound *)sound_data->data + (handle & 0xffff);
        uint32_t leaf;

        if (entry->location.type != 1) {
            continue;
        }
        leaf = bsp3d_node_find_leaf(0, global_collision_bsp, (real_point3d *)&entry->location.position);
        entry->location.leaf_index = (int32_t)leaf;
        if (leaf == 0xffffffff) {
            entry->location.cluster_index = -1;
        } else {
            entry->location.cluster_index = (int16_t)
                ((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[leaf & 0x7fffffff].cluster;
        }
    }
}
