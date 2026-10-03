#include <string.h>
#include "halo/units/unit.hpp"
#include "halo/core/collision_flags.hpp"
#include "projectiles.h"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/devices/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/game/api.hpp"

static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &ai_marker_name_a = halo::link::ref<char []>(halo::units::vars().ai_marker_name_a);

namespace halo::units {

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot((h))].data)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot((t))].data)
/**
 * Engine function unit_melee_attack_scan.
 *
 * @address 0x56f550
 */
void UnitView::melee_attack_scan()
{
    uint32_t unit_index = datum_handle;
    uint8_t *obj = OBJECT_DATA(unit_index);
    uint8_t *unit_tag = TAG_DATA(*(datum_index *)obj);
    real_vector3d *aim = (real_vector3d *)&((struct unit_object *)obj)->unit.aiming_vector;
    object_marker marker;
    real_point3d origin;
    real_vector3d perp;
    real_vector3d side;
    uint32_t best_object = k_datum_index_none;
    int32_t material = -1;
    int16_t best_type = 0;
    float best_fraction = 0.0f;
    uint32_t breakable_index = k_datum_index_none;
    int32_t breakable_surface = 0;
    datum_index secondary_effect = k_datum_index_none;
    datum_index damage_effect;
    int32_t row;
    int32_t col;

    halo::objects::object_get_node_local_transform(unit_index, ai_marker_name_a, &marker, 1);
    origin = marker.node_transform.position;
    halo::math::vector3d_build_perpendicular(perp, *aim);
    halo::math::vector3d_normalize_with_length(perp);
    side.i = aim->j * perp.k - aim->k * perp.j;
    side.j = aim->k * perp.i - aim->i * perp.k;
    side.k = aim->i * perp.j - aim->j * perp.i;

    for (row = -2; row <= 2; row++) {
        float rowf = (float)row;

        for (col = -2; col <= 2; col++) {
            float colf = (float)col;
            real_vector3d delta;
            collision_result hit;

            delta.i = (rowf * perp.i + colf * side.i) * 0.1f + aim->i * 0.8f;
            delta.j = (rowf * perp.j + colf * side.j) * 0.1f + aim->j * 0.8f;
            delta.k = (rowf * perp.k + colf * side.k) * 0.1f + aim->k * 0.8f;
            if (!halo::physics::collision_test_movement_segment(halo::to_bits(halo::collision_test_flag::front_face | halo::collision_test_flag::ignore_invisible | halo::collision_test_flag::structure_bsp | halo::collision_test_flag::water_surface | halo::collision_test_flag::nearby_objects | halo::collision_test_flag::unstick), &origin, &delta, unit_index, &hit)) {
                continue;
            }
            if (*(int16_t *)&hit == 2) {
                if (best_object == k_datum_index_none) {
                    material = *(int32_t *)&hit.material_type;
                    if (hit.surface_flags & 8) {
                        breakable_index = (breakable_index & 0xffff0000u) | hit.breakable_surface_index;
                        breakable_surface = hit.surface_index;
                    }
                }
            } else if (*(int16_t *)&hit == 3) {
                uint32_t candidate = hit.object_index;
                uint8_t *cand = OBJECT_DATA(candidate);
                int16_t type;

                if (((struct object *)cand)->type != 2 && ((struct object *)cand)->parent_object != k_datum_index_none) {
                    candidate = ((struct object *)cand)->parent_object;
                    cand = OBJECT_DATA(candidate);
                }
                type = ((struct object *)cand)->type;
                if (best_object != k_datum_index_none) {
                    if (type != 0) {
                        continue;
                    }
                    if (best_type == 0 && !(best_fraction > hit.t)) {
                        continue;
                    }
                }
                best_object = candidate;
                best_type = type;
                material = *(int32_t *)&hit.material_type;
                best_fraction = hit.t;
            }
        }
    }

    damage_effect = k_datum_index_none;
    {
        int16_t weapon_slot = ((unit_object *)obj)->unit.current_weapon_index;

        if (weapon_slot != -1) {
            datum_index weapon_index = *(datum_index *)(obj + 0x2f8 + weapon_slot * 4);

            if (weapon_index != k_datum_index_none) {
                uint8_t *weapon_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(weapon_index));

                damage_effect = *(datum_index *)(weapon_tag + 0x3a0);
                secondary_effect = *(datum_index *)(weapon_tag + 0x3b0);
            }
        }
    }
    if (damage_effect == k_datum_index_none) {
        damage_effect = *(datum_index *)&((struct Unit *)unit_tag)->melee_damage.tag_id;
    }

    if (best_object != k_datum_index_none) {
        uint8_t *best = OBJECT_DATA(best_object);

        if (((struct object *)best)->type == 1 && ((struct object *)best)->network_role != 1) {
            float scale = ((struct Unit *)TAG_DATA(*(datum_index *)best))->base.acceleration_scale * 0.035f;

            side.i = scale * aim->i;
            side.j = scale * aim->j;
            side.k = scale * aim->k;
            UnitView(best_object).apply_impulse_to_seat(&side);
        }
    }

    if (damage_effect != k_datum_index_none) {
        damage_data dd;

        memset(&dd, 0, sizeof(dd));
        dd.flags |= 1;
        dd.damage_effect_tag = damage_effect;
        dd.responsible_player = ((unit_object *)obj)->unit.controlling_player;
        dd.responsible_object = unit_index;
        dd.team_index = ((unit_object *)obj)->base.owner_team;
        dd.location_leaf_index = ((unit_object *)obj)->base.location_leaf_index;
        *(int32_t *)&dd.location_cluster_index = *(int32_t *)&((unit_object *)obj)->base.location_cluster_index;
        dd.epicentre = origin;
        dd.origin = *(real_point3d *)&((unit_object *)obj)->base.bounding_center.x;
        dd.direction = *aim;
        dd.random_blend = 1.0f;
        dd.multiplier = 1.0f;
        dd.material_type = (int16_t)material;

        if (best_object == k_datum_index_none) {
            if ((int16_t)breakable_index != -1) {
                halo::physics::breakable_surface_apply_damage(&dd, (int32_t)breakable_index, breakable_surface);
            }
        } else {
            float speed_scale = *(float *)((uint8_t *)global_globals->player_information.pointer + 0x34);

            if (((struct object *)OBJECT_DATA(best_object))->type == 7) {
                halo::devices::device_machine_melee_attacked(best_object);
            }
            if (speed_scale > 0.0f) {
                float f = (((unit_object *)obj)->base.velocity.k * ((unit_object *)obj)->base.forward.k +
                           ((unit_object *)obj)->base.velocity.j * ((unit_object *)obj)->base.forward.j +
                           ((unit_object *)obj)->base.forward.i * ((unit_object *)obj)->base.velocity.i) * 30.0f / speed_scale;

                if (f < 0.0f) {
                    f = 0.0f;
                } else if (f > 1.0f) {
                    f = 1.0f;
                }
                dd.random_blend = f;
            }
            if (((unit_object *)obj)->base.type == 0 && *(int8_t *)(obj + 0x501) > 0x0f) {
                dd.random_blend = 1.5f;
            }
            if (((struct object *)OBJECT_DATA(best_object))->type == 0) {
                halo::objects::object_apply_damage(&dd, best_object, -1, -1, -1, 0);
            }
        }
    }

    if ((int16_t)material != -1) {
        ::halo::units::unit_trigger_material_hit_effect((int16_t)material, damage_effect, unit_index);
        if (secondary_effect != k_datum_index_none) {
            damage_data dd;

            memset(&dd, 0, sizeof(dd));
            dd.flags |= 8;
            dd.damage_effect_tag = secondary_effect;
            dd.responsible_player = k_datum_index_none;
            dd.responsible_object = k_datum_index_none;
            dd.team_index = -1;
            dd.location_cluster_index = -1;
            dd.epicentre = *(real_point3d *)&((unit_object *)obj)->base.bounding_center.x;
            dd.origin = *(real_point3d *)&((unit_object *)obj)->base.bounding_center.x;
            dd.direction.i = -aim->i;
            dd.direction.j = -aim->j;
            dd.direction.k = -aim->k;
            dd.random_blend = 1.0f;
            dd.multiplier = 1.0f;
            dd.material_type = -1;
            halo::objects::object_apply_damage(&dd, unit_index, -1, -1, -1, 0);
        }
    }
    ((struct unit_object *)obj)->unit.melee_state = 0;
}
#undef OBJECT_DATA
#undef TAG_DATA

}
