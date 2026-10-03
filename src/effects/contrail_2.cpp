#include "halo/effects/effects.hpp"
#include "halo/memory/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/link.hpp"
#include "halo/effects/vars.hpp"

extern "C" {
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point);
void contrail_refresh_lightmap();
}
static auto &contrail_data = halo::link::ref<data_array *>(halo::effects::vars().contrail_data);
static auto &contrail_point_data = halo::link::ref<data_array *>(halo::effects::vars().contrail_point_data);

namespace halo::effects {

/**
 * For every point of every live contrail that already has a valid cluster, re-probes its
 * position against the structure BSP and refreshes its cached leaf/cluster location.
 *
 * @address 0x44cda0
 */
void contrail_ref::refresh_lightmap()
{
    datum_index contrail_index = halo::memory::datum_next(-1, contrail_data);

    while (contrail_index != k_datum_index_none) {
        contrail *self = &((contrail *)contrail_data->data)[(uint16_t)contrail_index];
        int list;

        for (list = 0; list < 4; list++) {
            datum_index point_index = self->first_point[list];

            while (point_index != k_datum_index_none) {
                contrail_point *point = &((contrail_point *)contrail_point_data->data)[(uint16_t)point_index];

                if (point->location.cluster_index != -1) {
                    int32_t leaf = halo::physics::bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)halo::physics::globals().collision_bsp, &point->position);

                    point->location.leaf_index = leaf;
                    if (leaf == -1) {
                        point->location.cluster_index = -1;
                    } else {
                        point->location.cluster_index = *(int16_t *)((uint8_t *)halo::scenario::globals().structure_bsp->leaves.pointer +
                            (uint32_t)(leaf & 0x7fffffff) * 0x10 + 8);
                    }
                }

                point_index = point->next_point;
            }
        }

        contrail_index = halo::memory::datum_next((int16_t)contrail_index, contrail_data);
    }
}

}

namespace halo::effects {

void contrail_refresh_lightmap()
{
    halo::effects::contrail_ref::refresh_lightmap();
}

}
