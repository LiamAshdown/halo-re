#include "halo/units/unit.hpp"
#include "halo/core/collision_flags.hpp"
#include "projectiles.h"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/physics/vars.hpp"
#include "halo/units/vars.hpp"

static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &material_table_warning_issued = halo::link::ref<uint8_t>(halo::physics::vars().material_table_warning_issued);
static auto &material_table_bad_index = halo::link::ref<int32_t>(halo::physics::vars().material_table_bad_index);
static auto &material_table_fallback = halo::link::ref<uint8_t [0x374]>(halo::physics::vars().material_table_fallback);
static auto &global_zero_vector3d_pointer = halo::link::ref<const real_point3d *>(halo::units::vars().global_zero_vector3d_pointer);

namespace halo::units {

/**
 * Triggers a material/impact visual effect associated with a given material index (looked up in the global
 * material-effects table, or a per-material fallback record when the index is out of range), and, if a tag id
 * is also given, a second effect from that tag's own +0x120 field -- used after melee hits. FIXED (objdump
 * 0x56f210..0x56f2cc): EDX carries the object the sounds play on (0x56f21d mov esi,edx); both sounds are
 * sound_start_at_object_marker(ESI object, ECX *0x006966f8, EAX *0x00696718, tag, -1, 1.0, 0).
 *
 * @address 0x56f210
 */
void halo::units::unit_trigger_material_hit_effect(int16_t material_index, datum_index unit_tag_id, datum_index object_index)
{
    uint8_t *material_record;

    if (material_index < 0 || material_index >= *(int32_t *)&global_globals->materials.count) {
        if (material_table_warning_issued == 0) {
            material_table_bad_index = -1;
            material_table_warning_issued = 1;
        }
        material_record = material_table_fallback;
    } else {
        material_record = (uint8_t *)global_globals->materials.pointer + material_index * 0x374;
    }

    if (*(datum_index *)(material_record + 0x370) != k_datum_index_none) {
        halo::sound::sound_start_at_object_marker(object_index, (Point3D *)global_zero_vector3d_pointer,
            (Vector3D *)halo::math::globals().global_forward3d_pointer, *(datum_index *)(material_record + 0x370), -1, 1.0f, 0);
    }

    if (unit_tag_id != k_datum_index_none) {
        uint8_t *tag_data = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(unit_tag_id)].data;
        datum_index effect = *(datum_index *)(tag_data + 0x120);
        if (effect != k_datum_index_none) {
            halo::sound::sound_start_at_object_marker(object_index, (Point3D *)global_zero_vector3d_pointer,
                (Vector3D *)halo::math::globals().global_forward3d_pointer, effect, -1, 1.0f, 0);
        }
    }
}

/**
 * REWRITTEN from objdump. Needs the animation graph (tag +0x44) with a node array (graph +0x24 count, +0x28
 * pointer). The object basis (matrix4x3_from_forward_up(up +0x80, forward +0x74) with position +0x5c) places
 * each suspension entry (array +0x68 count, +0x6c pointer, stride 0x14: +0 contact index, +2 node, +4/+8 the
 * extension range).
 *
 * @address 0x575170
 */
uint32_t UnitView::update_marker_traction_effects()
{
    uint32_t object_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)obj)].data;
    uint8_t *graph;
    uint8_t *node_array;
    uint8_t *physics;
    real_matrix4x3 basis;
    real max_rise = 0.0f;
    int16_t i;

    if (*(int32_t *)&((Unit *)tag)->base.animation_graph.tag_id == -1) {
        return 0;
    }
    graph = (uint8_t *)halo::cache::globals().tag_instances[*(uint32_t *)&((Unit *)tag)->base.animation_graph.tag_id & 0xffff].data;
    if (*(int32_t *)&((ModelAnimations *)graph)->vehicles.count == 0) {
        return 0;
    }
    node_array = *(uint8_t **)&((ModelAnimations *)graph)->vehicles.pointer;
    if (node_array == 0) {
        return 0;
    }
    physics = (uint8_t *)halo::cache::globals().tag_instances[*(uint32_t *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;
    halo::math::matrix4x3_from_forward_up(*((real_vector3d *)&((struct object *)obj)->up), *((real_vector3d *)&((struct object *)obj)->forward), basis);
    basis.position = *(real_point3d *)&((unit_object *)obj)->base.position.x;

    for (i = 0; (int32_t)i < (int32_t)((struct ModelAnimationsAnimationGraphVehicleAnimations *)node_array)->suspension_animations.count; i++) {
        uint8_t *entry = (uint8_t *)((struct ModelAnimationsAnimationGraphVehicleAnimations *)node_array)->suspension_animations.pointer + (int32_t)i * 0x14;
        int16_t contact_index = *(int16_t *)entry;
        uint8_t *mass_point;
        uint8_t old_byte;
        real old, range, offset, v, rise;
        real_point3d point;
        real_vector3d normal;
        real_point3d origin;
        real_vector3d delta;
        collision_result result;

        if (contact_index < 0 || (int32_t)contact_index >= *(int32_t *)(physics + 0x74) ||
            *(int16_t *)(entry + 2) == -1) {
            continue;
        }
        mass_point = *(uint8_t **)(physics + 0x78) + (int32_t)contact_index * 0x80;
        old_byte = obj[0x4f4 + i];
        old = old_byte == 0xff ? 1.0f : (real)old_byte * 0.003921569f;
        halo::math::matrix4x3_transform_point(point, *(real_point3d *)(mass_point + 0x38), basis);
        halo::math::matrix4x3_transform_normal(normal, *(real_vector3d *)(mass_point + 0x50), basis);
        range = *(real *)(entry + 4) - *(real *)(entry + 8);
        offset = *(real *)(entry + 8) - *(real *)(physics + 0x14) - range;
        origin.x = normal.i * offset + point.x;
        origin.y = normal.j * offset + point.y;
        origin.z = normal.k * offset + point.z;
        range = range + range;
        delta.i = normal.i * range;
        delta.j = normal.j * range;
        delta.k = normal.k * range;
        halo::physics::collision_test_movement_segment(halo::to_bits(halo::collision_test_flag::structure_bsp | halo::collision_test_flag::nearby_objects | halo::collision_test_flag::object_scenery | halo::collision_test_flag::object_machine), &origin, &delta, object_index, &result);
        v = (1.0f - result.t) + (1.0f - result.t);
        if (!(v >= 0.0f)) {
            v = 0.0f;
        } else if (!(v <= 1.0f)) {
            v = 1.0f;
        }
        rise = v - old;
        if (rise > max_rise) {
            max_rise = rise;
        }
        obj[0x4f4 + i] = halo::math::lerp_find_threshold_byte(0.0f, 1.0f, (v + old) * 0.5f);
    }

    if (*(int32_t *)(tag + 0x3bc) != -1 && max_rise > 0.3f) {
        real scale = (max_rise - 0.3f) * 1.6666667f;
        if (!(scale >= 0.0f)) {
            scale = 0.0f;
        } else if (!(scale <= 1.0f)) {
            scale = 1.0f;
        }
        halo::sound::sound_start_at_object_marker(object_index, (Point3D *)global_zero_vector3d_pointer,
            (Vector3D *)halo::math::globals().global_forward3d_pointer, *(datum_index *)(tag + 0x3bc), -1, scale, 0);
        return 1;
    }
    return 0;
}

}
