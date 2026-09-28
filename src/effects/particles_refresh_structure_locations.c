// particles_refresh_structure_locations  (not a Ghidra function; structure bsp activate slot 4)
// address 0x455d20, size 315 bytes
// name confidence: 0.5  rewrite confidence: 0.85
// evidence: structure_bsp_activate_procedures[4] (0x0069e8ec) holds 0x455d20; same leaf/cluster refresh as
//   effects_refresh_structure_locations 0x450e80 (activate slot 3), over particle_data.
// objdump 0x455d20..0x455e5a: every particle (particle_data 0x0087abd0, 0x70 each; next datum found inline)
//   gets a position: its own (+0x30) when free (+0x08 == -1); with flag 0x40 the first person weapon marker
//   (first_person_weapon_interfaces 0x006b2d98 + weapon (+0x0f) * 0x1ea0 + marker (+0x0c) * 0x34 + 0x10b4);
//   else the object's node matrix (object +0x1f2 offset, marker * 0x34, position +0x28), deleting the
//   particle when object_try_and_get(-1) fails. bsp3d_node_find_leaf gives the leaf (+0x28) and the leaf's
//   cluster (+0x2c); a particle outside the level (-1 cluster) is deleted (datum_delete, EAX array, EDX).
// blam-cc: no arguments

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern data_array *particle_data;                       // 0x0087abd0
extern uint8_t *first_person_weapon_interfaces;            // 0x006b2d98, stride 0x1ea0
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90
extern ScenarioStructureBSP *global_structure_bsp;

extern datum_index datum_next(int16_t after_index, data_array *array); // memory module, 0x4d0630
extern void datum_delete(data_array *array, datum_index index); // 0x4d0510, blam-cc: EAX -> array, EDX -> index
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0, EAX node, ECX bsp, EDX point

void particles_refresh_structure_locations(void)
{
    datum_index handle;

    for (handle = datum_next(-1, particle_data); handle != k_datum_index_none;
         handle = datum_next((int16_t)handle, particle_data)) {
        particle *entry = (particle *)((uint8_t *)particle_data->data + (handle & 0xffff) * 0x70);
        real_point3d *point;
        uint32_t leaf;
        int16_t cluster;

        if (entry->object_index == k_datum_index_none) {
            point = &entry->position;
        } else if (entry->flags & 0x40) {
            point = (real_point3d *)(first_person_weapon_interfaces + entry->first_person_weapon_index * 0x1ea0 +
                entry->marker_index * 0x34 + 0x10b4);
        } else {
            uint8_t *owner = (uint8_t *)object_try_and_get(entry->object_index, 0xffffffff);

            if (owner == 0) {
                datum_delete(particle_data, handle);
                continue;
            }
            point = (real_point3d *)(owner + ((struct object *)owner)->nodes.offset + entry->marker_index * 0x34 + 0x28);
        }
        leaf = bsp3d_node_find_leaf(0, global_collision_bsp, point);
        entry->location.leaf_index = (int32_t)leaf;
        if (leaf == 0xffffffff) {
            cluster = -1;
        } else {
            cluster = *(int16_t *)((uint8_t *)global_structure_bsp->leaves.pointer + (leaf & 0x7fffffff) * 0x10 + 8);
        }
        entry->location.cluster_index = cluster;
        if (cluster == -1) {
            datum_delete(particle_data, handle);
        }
    }
}
