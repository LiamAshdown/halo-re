#include <string.h>
#include "halo/units/unit.hpp"

extern "C" {
extern data_array *object_data;
extern tag_instance *tag_instances;
extern float *global_zero_vector2d_pointer;
extern real_point3d *global_zero_vector3d_pointer;
extern void animation_replace_frame_orientations(void *animation, int16_t frame, void *out_orientations);
extern void animation_overlay_frame_orientations(void *animation, int16_t frame, void *out_orientations);
extern void animation_overlay_frame_orientations_weighted(void *animation, int16_t frame, float weight, void *out_orientations);
extern void animation_overlay_interpolated_frame_orientations(void *animation, float frame, void *out_orientations);
extern void animation_aiming_screen_blend(void *animation, void *screen, real yaw, real pitch, void *orientation_out);
extern void object_get_orientation(real_vector3d *out_forward, uint32_t object_index, real_vector3d *out_up);
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b);
extern void matrix4x3_inverse_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m);
extern double atan2(double y, double x);
extern double sqrt(double x);
extern Globals *global_globals;
extern object * object_try_and_get(datum_index object_index, uint32_t type_mask);
extern void object_apply_damage(damage_data *dd, uint32_t object_index, int16_t node_index, int16_t region_index, int16_t material_index, uint32_t plane);
}

namespace halo::units {

namespace unit_update_aiming_overlay_angles_local {

static void aiming_angles_in_unit_frame(uint32_t unit_index, real_vector3d *direction, float *yaw, float *pitch)
{
    real_matrix4x3 frame;
    real_vector3d local;

    frame.scale = 1.0f;
    object_get_orientation(&frame.forward, unit_index, &frame.up);
    vector3d_cross_product(&frame.left, &frame.forward, &frame.up);
    frame.position = *global_zero_vector3d_pointer;
    matrix4x3_inverse_transform_normal(&local, direction, &frame);
    *yaw = (float)atan2((double)local.j, (double)local.i);
    *pitch = (float)atan2((double)local.k, sqrt((double)(local.i * local.i + local.j * local.j)));
}

static void aiming_screen_limits(const uint8_t *screen, float *out)
{
    out[0] = -((float)*(int16_t *)(screen + 0x08) * *(float *)(screen + 0x00));
    out[1] = (float)*(int16_t *)(screen + 0x0a) * *(float *)(screen + 0x04);
    out[2] = -((float)*(int16_t *)(screen + 0x14) * *(float *)(screen + 0x0c));
    out[3] = (float)*(int16_t *)(screen + 0x16) * *(float *)(screen + 0x10);
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
    uint8_t *unit = *(uint8_t **)((uint8_t *)object_data->data + (unit_index & 0xffff) * 0xc + 8);
    uint8_t *unit_tag = (uint8_t *)tag_instances[*(datum_index *)unit & 0xffff].data;
    uint8_t *graph = (uint8_t *)tag_instances[*(datum_index *)&((struct Unit *)unit_tag)->base.animation_graph.tag_id & 0xffff].data;
    uint8_t *animations = *(uint8_t **)&((ModelAnimations *)graph)->animations.pointer;
    uint8_t *block;
    float aim_yaw;
    float aim_pitch;
    int8_t state;

    if (((struct unit_object *)unit)->unit.overlays[0].animation_index != -1) {
        animation_replace_frame_orientations(animations + ((struct unit_object *)unit)->unit.overlays[0].animation_index * 0xb4,
            (int16_t)(uint16_t)((struct unit_object *)unit)->unit.overlays[0].frame, output);
    }
    if (((struct unit_object *)unit)->unit.overlays[1].animation_index != -1) {
        animation_overlay_frame_orientations(animations + ((struct unit_object *)unit)->unit.overlays[1].animation_index * 0xb4,
            (int16_t)(uint16_t)((struct unit_object *)unit)->unit.overlays[1].frame, output);
    }
    if (((struct unit_object *)unit)->unit.overlays[2].animation_index != -1) {
        animation_overlay_frame_orientations(animations + ((struct unit_object *)unit)->unit.overlays[2].animation_index * 0xb4,
            (int16_t)(uint16_t)((struct unit_object *)unit)->unit.overlays[2].frame, output);
    }
    unit[0x2b6] = 0;
    unit[0x2b7] = 0;
    if ((*(uint32_t *)&((struct Unit *)unit_tag)->unit_flags & 0x800) || (uint8_t)((struct unit_object *)unit)->unit.animation_definition_index == 0xff) {
        return;
    }
    block = *(uint8_t **)&((ModelAnimations *)graph)->units.pointer + (int8_t)(uint8_t)((struct unit_object *)unit)->unit.animation_definition_index * 0x64;

    if ((uint8_t)((struct unit_object *)unit)->unit.emotion_animation_frame != 0xff) {
        int16_t emotion = (*(int32_t *)(block + 0x40) > 0xb) ? (*(int16_t **)(block + 0x44))[0xb] : -1;

        if (((unit_object *)unit)->unit.emotion_animation_index != -1) {
            emotion = ((unit_object *)unit)->unit.emotion_animation_index;
        }
        if (emotion != -1) {
            uint8_t *record = animations + emotion * 0xb4;
            int8_t frame = (int8_t)(uint8_t)((struct unit_object *)unit)->unit.emotion_animation_frame;

            if (frame >= 0 && frame < *(int16_t *)(record + 0x22)) {
                animation_overlay_frame_orientations(record, frame, output);
            }
        }
    }
    if (((unit_object *)unit)->unit.mouth_aperture > 0.0f && *(int32_t *)(block + 0x40) > 0xa &&
        (*(int16_t **)(block + 0x44))[0xa] != -1) {
        animation_overlay_frame_orientations_weighted(animations + (*(int16_t **)(block + 0x44))[0xa] * 0xb4, 0,
            ((unit_object *)unit)->unit.mouth_aperture, output);
    }
    if ((uint8_t)((struct unit_object *)unit)->unit.animation_state_flags & 2) {
        int32_t slot;

        for (slot = 2; slot < 5; slot++) {
            if (slot < *(int32_t *)(block + 0x40) && (*(int16_t **)(block + 0x44))[slot] != -1) {
                uint8_t *record = animations + (*(int16_t **)(block + 0x44))[slot] * 0xb4;
                int32_t last_frame = *(int16_t *)(record + 0x22) - 1;

                animation_overlay_interpolated_frame_orientations(record,
                    (float)last_frame * *(float *)(unit + 0x364 + (slot - 2) * 4), output);
            }
        }
    }

    if (*(uint32_t *)&((struct Unit *)unit_tag)->unit_flags & 0x400) {
        return;
    }
    state = (int8_t)(uint8_t)((struct unit_object *)unit)->unit.animation_state;
    if ((state >= 0x17 && state <= 0x23) || state == 0x29 || (uint8_t)((struct unit_object *)unit)->unit.replacement_animation_state != 0) {
        return;
    }

    aim_yaw = global_zero_vector2d_pointer[0];
    aim_pitch = global_zero_vector2d_pointer[1];
    if (((unit_object *)unit)->unit.aiming_animation_index != -1) {
        uint8_t *screen = *(uint8_t **)(block + 0x5c) + (int8_t)(uint8_t)((struct unit_object *)unit)->unit.animation_weapon_index * 0xbc + 0x60;

        aiming_angles_in_unit_frame(unit_index, (real_vector3d *)&((struct unit_object *)unit)->unit.aiming_vector, &aim_yaw, &aim_pitch);
        unit[0x2b6] = 1;
        aiming_screen_limits(screen, (float *)&((struct unit_object *)unit)->unit.aiming_bounds);
        animation_aiming_screen_blend(animations + ((unit_object *)unit)->unit.aiming_animation_index * 0xb4, screen, aim_yaw, aim_pitch, output);
    }

    if (((unit_object *)unit)->unit.current_weapon_index == -1 && ((unit_object *)unit)->unit.controlling_player == k_datum_index_none) {
        return;
    }
    if (((struct unit_object *)unit)->unit.looking_animation_index != -1) {
        uint8_t *screen = block + 0x20;
        float look_yaw;
        float look_pitch;

        aiming_angles_in_unit_frame(unit_index, (real_vector3d *)&((struct unit_object *)unit)->unit.looking_vector, &look_yaw, &look_pitch);
        unit[0x2b7] = 1;
        look_yaw -= aim_yaw;
        look_pitch -= aim_pitch;
        aiming_screen_limits(screen, (float *)&((struct unit_object *)unit)->unit.looking_bounds);
        animation_aiming_screen_blend(animations + ((struct unit_object *)unit)->unit.looking_animation_index * 0xb4, screen, look_yaw, look_pitch,
            output);
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
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    uint8_t *tracked = (uint8_t *)global_globals->falling_damage.pointer;

    ((unit_object *)obj)->unit.flags &= ~0x02000000u;
    obj[0x107] &= 0xf7;
    ((unit_object *)obj)->unit.flags &= ~0x00000080u;

    if (tracked != 0) {
        datum_index damage_effect = *(datum_index *)(tracked + 0x78);
        if (damage_effect != k_datum_index_none) {
            uint8_t *source = (uint8_t *)object_try_and_get(*(datum_index *)&((struct unit_object *)obj)->unit.flaming_responsible_object, 0xffffffff);
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
                datum_index creator = ((struct object *)source)->creator_object;
                dd.responsible_player = *(datum_index *)&((struct object *)source)->owner_linkage;
                if (creator == k_datum_index_none) {
                    creator = *(datum_index *)&((struct unit_object *)obj)->unit.flaming_responsible_object;
                }
                dd.responsible_object = creator;
                dd.team_index = ((struct object *)source)->owner_team;
            }
            object_apply_damage(&dd, unit_index, -1, -1, -1, 0);
        }
    }

    if ((((unit_object *)obj)->base.vitality_flags & 4) == 0) {
        ((unit_object *)obj)->base.vitality_flags |= 0x20;
    }
}

}
