#include "halo/tags/flags.hpp"
#include "halo/objects/record_access.hpp"
#include <string.h>
#include "halo/models/api.hpp"
#include "halo/units/unit.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/units/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/objects/api.hpp"
#include "halo/models/models.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/game/api.hpp"
#include "halo/units/api.hpp"

static auto &global_zero_vector2d_pointer = halo::link::ref<float *>(halo::units::vars().global_zero_vector2d_pointer);
static auto &global_zero_vector3d_pointer = halo::link::ref<real_point3d *>(halo::units::vars().global_zero_vector3d_pointer);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);

namespace halo::units {

namespace unit_update_aiming_overlay_angles_local {

static void aiming_angles_in_unit_frame(uint32_t unit_index, real_vector3d *direction, float *yaw, float *pitch)
{
    real_matrix4x3 frame;
    real_vector3d local;

    frame.scale = 1.0f;
    halo::objects::object_get_orientation(&frame.forward, unit_index, &frame.up);
    halo::math::vector3d_cross_product(frame.left, frame.forward, frame.up);
    frame.position = *global_zero_vector3d_pointer;
    halo::math::matrix4x3_inverse_transform_normal(local, *direction, frame);
    *yaw = (float)halo::libm::atan2((double)local.j, (double)local.i);
    *pitch = (float)halo::libm::atan2((double)local.k, halo::libm::sqrt((double)(local.i * local.i + local.j * local.j)));
}

static void aiming_screen_limits(const animation_aiming_screen *screen, float *out)
{
    out[0] = -((float)(int16_t)screen->right_frame_count * screen->right_yaw_per_frame);
    out[1] = (float)(int16_t)screen->left_frame_count * screen->left_yaw_per_frame;
    out[2] = -((float)(int16_t)screen->down_pitch_frame_count * screen->down_pitch_per_frame);
    out[3] = (float)(int16_t)screen->up_pitch_frame_count * screen->up_pitch_per_frame;
}

}

/**
 * Engine function unit_update_aiming_overlay_angles.
 *
 * @address 0x563b50
 */
void UnitView::update_aiming_overlay_angles(void *output)
{
    using namespace unit_update_aiming_overlay_angles_local;
    uint32_t unit_index = datum_handle;
    unit_object *unit = reinterpret_cast<unit_object *>(*(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + halo::datum_slot(unit_index) * 0xc + 8));
    Unit *unit_tag = halo::objects::tag_as<Unit>(*(datum_index *)unit);
    ModelAnimations *graph = halo::objects::tag_as<ModelAnimations>(halo::objects::tag_handle(unit_tag->base.animation_graph));
    ModelAnimationsAnimation *animations = halo::objects::block_elements<ModelAnimationsAnimation>(graph->animations);
    ModelAnimationsAnimationGraphUnitSeat *block;
    float aim_yaw;
    float aim_pitch;
    int8_t state;

    if (unit->unit.overlays[0].animation_index != -1) {
        halo::models::animation_view((ModelAnimationsAnimation *)(reinterpret_cast<uint8_t *>(animations) + unit->unit.overlays[0].animation_index * 0xb4)).replace_frame_orientations((int16_t)(uint16_t)unit->unit.overlays[0].frame, (real_orientation *)output);
    }
    if (unit->unit.overlays[1].animation_index != -1) {
        halo::models::animation_view((ModelAnimationsAnimation *)(reinterpret_cast<uint8_t *>(animations) + unit->unit.overlays[1].animation_index * 0xb4)).overlay_frame_orientations((int16_t)(uint16_t)unit->unit.overlays[1].frame, (real_orientation *)output);
    }
    if (unit->unit.overlays[2].animation_index != -1) {
        halo::models::animation_view((ModelAnimationsAnimation *)(reinterpret_cast<uint8_t *>(animations) + unit->unit.overlays[2].animation_index * 0xb4)).overlay_frame_orientations((int16_t)(uint16_t)unit->unit.overlays[2].frame, (real_orientation *)output);
    }
    unit->unit.aiming_bounds_valid = 0;
    unit->unit.looking_bounds_valid = 0;
    if (test_flag(unit_tag->unit_flags, tags::unit_tag_flag::simple_creature) || (uint8_t)unit->unit.animation_definition_index == 0xff) {
        return;
    }
    block = reinterpret_cast<ModelAnimationsAnimationGraphUnitSeat *>(&halo::objects::block_element<ModelAnimationsAnimationGraphUnitSeat>(graph->units, (int8_t)(uint8_t)unit->unit.animation_definition_index));

    if ((uint8_t)unit->unit.emotion_animation_frame != 0xff) {
        int16_t emotion = ((int32_t)block->animations.count > 0xb) ? ((int16_t *)block->animations.pointer)[0xb] : -1;

        if (unit->unit.emotion_animation_index != -1) {
            emotion = unit->unit.emotion_animation_index;
        }
        if (emotion != -1) {
            uint8_t *record = reinterpret_cast<uint8_t *>(animations) + emotion * 0xb4;
            int8_t frame = (int8_t)(uint8_t)unit->unit.emotion_animation_frame;

            if (frame >= 0 && frame < *(int16_t *)(record + 0x22)) {
                halo::models::animation_view(reinterpret_cast<ModelAnimationsAnimation *>(record)).overlay_frame_orientations(frame, reinterpret_cast<real_orientation *>(output));
            }
        }
    }
    if (unit->unit.mouth_aperture > 0.0f && (int32_t)block->animations.count > 0xa &&
        ((int16_t *)block->animations.pointer)[0xa] != -1) {
        halo::models::animation_view((ModelAnimationsAnimation *)(reinterpret_cast<uint8_t *>(animations) + ((int16_t *)block->animations.pointer)[0xa] * 0xb4)).overlay_frame_orientations_weighted(0, unit->unit.mouth_aperture, (real_orientation *)output);
    }
    if ((uint8_t)unit->unit.animation_state_flags & 2) {
        int32_t slot;

        for (slot = 2; slot < 5; slot++) {
            if (slot < (int32_t)block->animations.count && ((int16_t *)block->animations.pointer)[slot] != -1) {
                uint8_t *record = reinterpret_cast<uint8_t *>(animations) + ((int16_t *)block->animations.pointer)[slot] * 0xb4;
                int32_t last_frame = *(int16_t *)(record + 0x22) - 1;

                halo::models::animation_view(reinterpret_cast<ModelAnimationsAnimation *>(record)).overlay_interpolated_frame_orientations((float)last_frame * unit->unit.animation_controls_smoothed[(slot - 2)], reinterpret_cast<real_orientation *>(output));
            }
        }
    }

    if (test_flag(unit_tag->unit_flags, tags::unit_tag_flag::has_no_aiming)) {
        return;
    }
    state = (int8_t)(uint8_t)unit->unit.animation_state;
    if ((state >= 0x17 && state <= 0x23) || state == 0x29 || (uint8_t)unit->unit.replacement_animation_state != 0) {
        return;
    }

    aim_yaw = global_zero_vector2d_pointer[0];
    aim_pitch = global_zero_vector2d_pointer[1];
    if (unit->unit.aiming_animation_index != -1) {
        animation_aiming_screen *screen = reinterpret_cast<animation_aiming_screen *>(&halo::objects::block_element<ModelAnimationsAnimationGraphWeapon>(block->weapons, (int8_t)(uint8_t)unit->unit.animation_weapon_index).right_yaw_per_frame);

        aiming_angles_in_unit_frame(unit_index, (real_vector3d *)&unit->unit.aiming_vector, &aim_yaw, &aim_pitch);
        unit->unit.aiming_bounds_valid = 1;
        aiming_screen_limits(screen, (float *)&unit->unit.aiming_bounds);
        halo::models::animation_view((ModelAnimationsAnimation *)(reinterpret_cast<uint8_t *>(animations) + unit->unit.aiming_animation_index * 0xb4)).aiming_screen_blend(screen, aim_yaw, aim_pitch, (real_orientation *)output);
    }

    if (unit->unit.current_weapon_index == -1 && unit->unit.controlling_player == k_datum_index_none) {
        return;
    }
    if (unit->unit.looking_animation_index != -1) {
        animation_aiming_screen *screen = reinterpret_cast<animation_aiming_screen *>(&block->right_yaw_per_frame);
        float look_yaw;
        float look_pitch;

        aiming_angles_in_unit_frame(unit_index, (real_vector3d *)&unit->unit.looking_vector, &look_yaw, &look_pitch);
        unit->unit.looking_bounds_valid = 1;
        look_yaw -= aim_yaw;
        look_pitch -= aim_pitch;
        aiming_screen_limits(screen, (float *)&unit->unit.looking_bounds);
        halo::models::animation_view((ModelAnimationsAnimation *)(reinterpret_cast<uint8_t *>(animations) + unit->unit.looking_animation_index * 0xb4)).aiming_screen_blend(screen, look_yaw, look_pitch, (real_orientation *)output);
    }
}

/**
 * REWRITTEN from objdump 0x570720..0x570834 (raw object offsets). Clears unit +0x204 bits 0x2000000 and 0x80
 * and object +0x107 bit 3. When globals +0x18c is set and its +0x78 damage effect is not none, builds a
 * damage_data (damage_data_initialize sentinels) whose responsible player / object / team come from the
 * unit's +0x410 object (creator falling back to +0x410 itself) and applies it to this unit. Finally, unless
 * object +0x106 bit 2 is set, sets +0x106 bit 5.
 *
 * @address 0x570720
 */
void UnitView::update_autoaim_interaction()
{
    uint32_t unit_index = datum_handle;
    unit_object *obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
    GlobalsFallingDamage *tracked = halo::objects::block_elements<GlobalsFallingDamage>(global_globals->falling_damage);

    clear_flag(obj->unit.flags, units::unit_flag::idle_turn_seeded);
    halo::raw_at<uint8_t>(obj, 0x107) &= 0xf7;
    clear_flag(obj->unit.flags, units::unit_flag::disoriented);

    if (tracked != 0) {
        datum_index damage_effect = halo::objects::tag_handle(tracked->flaming_death_damage);
        if (damage_effect != k_datum_index_none) {
            object *source = reinterpret_cast<object *>(halo::objects::object_try_and_get(*(datum_index *)&obj->unit.flaming_responsible_object, k_datum_index_none));
            damage_data dd;

            memset(&dd, 0, sizeof(dd));
            dd.damage_effect_tag = damage_effect;
            dd.material_type = -1;
            dd.responsible_player = k_datum_index_none;
            dd.responsible_object = k_datum_index_none;
            dd.team_index = -1;
            dd.location_cluster_index = -1;
            dd.random_blend = 1.0f;
            dd.multiplier = 1.0f;
            if (source != 0) {
                datum_index creator = source->creator_object;
                dd.responsible_player = *(datum_index *)&source->owner_linkage;
                if (creator == k_datum_index_none) {
                    creator = *(datum_index *)&obj->unit.flaming_responsible_object;
                }
                dd.responsible_object = creator;
                dd.team_index = source->owner_team;
            }
            halo::objects::object_apply_damage(&dd, unit_index, -1, -1, -1, 0);
        }
    }

    if (!test_flag(obj->base.vitality_flags, objects::vitality_flag::health_frozen)) {
        set_flag(obj->base.vitality_flags, objects::vitality_flag::die_act_of_god);
    }
}

}
