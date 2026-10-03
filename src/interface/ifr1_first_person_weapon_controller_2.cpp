#include "halo/interface/ifr1_first_person_weapon_controller.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/models/api.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/render/api.hpp"
#include "halo/main/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"

extern "C" {
extern first_person_weapon_interface *first_person_weapon_interfaces;
extern Globals *global_globals;
extern void *global_zero_vector3d_pointer;
extern float effect_random_fraction(void);
extern double fmod(double x, double y);
extern double sqrt(double x);
extern int32_t __ftol(double x);
extern real_point3d render_camera_global;
extern real_vector3d camera_forward_x;
extern real_vector3d camera_up;
extern double fpatan(double y, double x);
extern int16_t current_local_player_index;
extern float zoom_static_tint_r;
extern float zoom_static_tint_g;
extern float zoom_static_tint_b;
}

#define FP_FLOAT(fp, offset) (*(float *)((uint8_t *)(fp) + (offset)))

static object *object_get(datum_index object_index)
{
    return *(object **)((char *)halo::objects::globals().object_data->data + 8 + (object_index & halo::k_slot_mask) * 0xc);
}

static ModelAnimationsAnimationGraphFirstPersonWeaponAnimations *first_person_weapon_list(
    ModelAnimations *animations)
{
    if (animations->first_person_weapons.count == 0) {
        return (ModelAnimationsAnimationGraphFirstPersonWeaponAnimations *)0;
    }
    return (ModelAnimationsAnimationGraphFirstPersonWeaponAnimations *)
        animations->first_person_weapons.pointer;
}

static void seed_aim(first_person_weapon_interface *fp)
{
    FP_FLOAT(fp, 0x60) = (float)fpatan(camera_forward_x.j, camera_forward_x.i);
    FP_FLOAT(fp, 0x64) = (float)fpatan(camera_forward_x.k,
                                       sqrt(camera_forward_x.j * camera_forward_x.j +
                                            camera_forward_x.i * camera_forward_x.i));
    *(real_point3d *)((uint8_t *)fp + 0x70) = render_camera_global;
}

static void overlay_channel(ModelAnimationsAnimation *animation, float value, int32_t positive,
                            int32_t negative, void *animation_control)
{
    if (value > 0.0f) {
        halo::models::animation_overlay_frame_orientations_weighted(animation, positive, value, reinterpret_cast<real_orientation *>(animation_control));
    } else if (value < 0.0f) {
        halo::models::animation_overlay_frame_orientations_weighted(animation, negative, -value, reinterpret_cast<real_orientation *>(animation_control));
    }
}

namespace halo::interface {

/**
 * Original engine function first_person_weapon_update; the author notes are in
 * docs/original/interface/first_person_weapon_update.txt.
 *
 * @address 0x493150
 */
void FirstPersonWeaponController::update()
{
    halo::interface::debug_play_diagnostics();
    first_person_weapon_interface *fp = &first_person_weapon_interfaces[local_player_index];

    if (fp->weapon_index != (datum_index)-1 && halo::objects::object_try_and_get(fp->weapon_index, 4) == 0) {
        fp->weapon_index = (datum_index)-1;
    }

    if (fp->unit_index != (datum_index)-1 && fp->weapon_index != (datum_index)-1) {
        object *weapon_obj = object_get(fp->weapon_index);
        object *unit_obj = object_get(fp->unit_index);
        Weapon *weapon_tag = (Weapon *)halo::cache::globals().tag_instances[weapon_obj->definition_tag & halo::k_slot_mask].data;
        datum_index animation_graph = *(datum_index *)&weapon_tag->first_person_animations.tag_id;
        ModelAnimations *animations = (ModelAnimations *)halo::cache::globals().tag_instances[animation_graph & halo::k_slot_mask].data;
        ModelAnimationsAnimationGraphFirstPersonWeaponAnimations *list;
        uint32_t *weapon_flags = (uint32_t *)((uint8_t *)weapon_obj + 0x22c);
        datum_index frame_sound;
        uint8_t is_moving;
        float target_yaw;
        float target_pitch;

        if (fp->state == 3 || fp->state == 1) {
            if ((*weapon_flags & 2) != 0) {
                halo::interface::first_person_weapon_set_state(local_player_index, 1, 0x16);
            }
            if ((*weapon_flags & 1) == 0) {
                halo::interface::first_person_weapon_set_state(local_player_index, 1, 0);
            }
        }

        if (halo::models::animation_state_advance(*(datum_index *)&weapon_tag->first_person_animations.tag_id, reinterpret_cast<animation_state *>(&fp->current_animation),
                         reinterpret_cast<int32_t *>(&frame_sound), static_cast<animation_random_stream>(0)) == 2) {
            halo::interface::first_person_weapon_update_state(local_player_index);
        }

        if (frame_sound != (datum_index)-1 && halo::camera::camera_get_type_for_player(local_player_index) == 0) {
            fp->frame_sound_index = halo::sound::sound_start_at_object_marker(fp->weapon_index, (Point3D *)global_zero_vector3d_pointer,
                                            (Vector3D *)halo::math::globals().global_forward3d_pointer, frame_sound, -1, 1.0f,
                                            local_player_index != -1);
            fp->frame_sound_state = fp->state;
        }

        {
            real_vector3d *throttle = (real_vector3d *)((uint8_t *)unit_obj + 0x278);
            is_moving = 1;
            if (!(sqrt(throttle->k * throttle->k + throttle->j * throttle->j +
                       throttle->i * throttle->i) > 0.1f)) {
                is_moving = 0;
            }
        }
        if (object_get(fp->unit_index)->type == 0 && halo::units::biped_is_idle_eligible(fp->unit_index) != 0) {
            is_moving = 0;
        }

        if (fp->moving_animation != -1) {
            halo::models::animation_state_advance(*(datum_index *)&weapon_tag->first_person_animations.tag_id, reinterpret_cast<animation_state *>(&fp->moving_animation),
                         reinterpret_cast<int32_t *>((datum_index *)0), static_cast<animation_random_stream>(0));
            if (!is_moving) {
                if (fp->state == 0) {
                    halo::interface::first_person_weapon_snapshot_pose(local_player_index, 6);
                }
                fp->moving_animation = -1;
            }
        } else if (is_moving) {
            list = first_person_weapon_list(animations);
            *(int16_t *)fp->unknown_1c = 0;
            if ((int32_t)list->animations.count > 3) {
                fp->moving_animation = ((int16_t *)list->animations.pointer)[3];
            } else {
                fp->moving_animation = -1;
            }
        }

        if (fp->overcharged_animation == -1) {
            if (fp->state == 4) {
                list = first_person_weapon_list(animations);
                FP_FLOAT(fp, 0x24) = 0.0f;
                if ((int32_t)list->animations.count > 0xf) {
                    fp->overcharged_animation = ((int16_t *)list->animations.pointer)[0xf];
                } else {
                    fp->overcharged_animation = -1;
                }
            }
        } else if (fp->state == 4) {
            ModelAnimationsAnimation *animation =
                &((ModelAnimationsAnimation *)animations->animations.pointer)[fp->overcharged_animation];
            float charged_fraction = *(float *)((uint8_t *)weapon_obj + 0x244);
            FP_FLOAT(fp, 0x24) = (float)fmod((charged_fraction + 1.0f) + (charged_fraction + 1.0f) +
                                             FP_FLOAT(fp, 0x24),
                                             (double)(int16_t)animation->frame_count);
        } else {
            fp->overcharged_animation = -1;
        }

        if (fp->unknown_30[0x20] != 0) {
            halo::math::real_seek_toward_clamped(0, FP_FLOAT(fp, 0x38), FP_FLOAT(fp, 0x30),
                                     *(float *)((uint8_t *)unit_obj + 0x278), 0.08f, 0.5f, -1.0f, 1.0f);
            halo::math::real_seek_toward_clamped(0, FP_FLOAT(fp, 0x3c), FP_FLOAT(fp, 0x34),
                                     *(float *)((uint8_t *)unit_obj + 0x27c), 0.08f, 0.5f, -1.0f, 1.0f);
            target_yaw = halo::game::angle_delta_wrapped(FP_FLOAT(fp, 0x68), FP_FLOAT(fp, 0x60)) * 30.0f;
            target_pitch = halo::game::angle_delta_wrapped(FP_FLOAT(fp, 0x6c), FP_FLOAT(fp, 0x64)) * -30.0f;
            if (target_yaw < -1.0f) {
                target_yaw = -1.0f;
            } else if (target_yaw > 1.0f) {
                target_yaw = 1.0f;
            }
            if (target_pitch < -1.0f) {
                target_pitch = -1.0f;
            } else if (target_pitch > 1.0f) {
                target_pitch = 1.0f;
            }
            halo::math::real_seek_toward_clamped(0, FP_FLOAT(fp, 0x48), FP_FLOAT(fp, 0x40), target_yaw,
                                     0.03f, 0.2f, -1.0f, 1.0f);
            halo::math::real_seek_toward_clamped(0, FP_FLOAT(fp, 0x4c), FP_FLOAT(fp, 0x44), target_pitch,
                                     0.03f, 0.2f, -1.0f, 1.0f);
        }
        halo::math::real_seek_toward_clamped(0, fp->charge, fp->recoil, 0.0f, 0.01f, 0.2f, 0.0f, 1.0f);
        if (fp->recoil == 1.0f) {
            fp->charge = 0.0f;
        }

        if (fp->blend_end > 0) {
            fp->blend_start++;
            if (fp->blend_start >= fp->blend_end) {
                fp->blend_end = 0;
            }
        }

        {
            local_player_control *control =
                &halo::game::globals().player_control->local_players[local_player_index];

            if (control->nameplate_weight == 0.0f &&
                (local_player_index == -1 || control->desired_zoom_level == -1) &&
                fp->recoil == 0.0f &&
                FP_FLOAT(fp, 0x30) == 0.0f && FP_FLOAT(fp, 0x34) == 0.0f &&
                FP_FLOAT(fp, 0x40) == 0.0f && FP_FLOAT(fp, 0x44) == 0.0f) {
                if (fp->state == 0) {
                    GlobalsPlayerInformation *player_information =
                        (GlobalsPlayerInformation *)global_globals->player_information.pointer;
                    if (fp->idle_delay_ticks == 0) {
                        fp->idle_delay_ticks = (int16_t)__ftol(
                            halo::math::random_range_real(player_information->first_person_idle_time[0],
                                              player_information->first_person_idle_time[1]) * 30.0f);
                    }
                    fp->idle_ticks++;
                    if (fp->idle_ticks > fp->idle_delay_ticks) {
                        fp->idle_delay_ticks = 0;
                        if (!(halo::effects::effect_random_fraction() <
                              player_information->first_person_skip_fraction)) {
                            halo::interface::first_person_weapon_set_state(local_player_index, 1, 5);
                        }
                    }
                } else {
                    fp->idle_ticks = 0;
                }
            } else {
                fp->idle_ticks = 0;
                if (fp->state == 5) {
                    halo::interface::first_person_weapon_set_state(local_player_index, 1, 0);
                }
            }
        }
    }

    fp->shutdown_countdown--;
    if (fp->shutdown_countdown <= 0) {
        halo::interface::first_person_weapon_interface_tick_reset(local_player_index);
    }
}

/**
 * Original engine function first_person_weapon_update_animation_controls; the author notes are in
 * docs/original/interface/first_person_weapon_update_animation_controls.txt.
 *
 * @address 0x493740
 */
void FirstPersonWeaponController::update_animation_controls()
{
    first_person_weapon_interface *fp = &first_person_weapon_interfaces[local_player_index];
    uint8_t *fp_raw = (uint8_t *)fp;

    if (fp->unknown_30[0x20] == 0) {
        seed_aim(fp);
    }
    FP_FLOAT(fp, 0x68) = FP_FLOAT(fp, 0x60);
    FP_FLOAT(fp, 0x6c) = FP_FLOAT(fp, 0x64);
    *(real_point3d *)(fp_raw + 0x7c) = *(real_point3d *)(fp_raw + 0x70);
    seed_aim(fp);
    *(real_vector3d *)(fp_raw + 0x54) = camera_forward_x;
    fp->unknown_30[0x20] = 1;

    if (fp->weapon_index != (datum_index)-1 && halo::objects::object_try_and_get(fp->weapon_index, 4) == 0) {
        fp->weapon_index = (datum_index)-1;
    }
    if (fp->weapon_index == (datum_index)-1) {
        return;
    }

    {
        object *weapon_obj = *(object **)((char *)halo::objects::globals().object_data->data + 8 +
                                          (fp->weapon_index & halo::k_slot_mask) * 0xc);
        Weapon *weapon_tag = (Weapon *)halo::cache::globals().tag_instances[weapon_obj->definition_tag & halo::k_slot_mask].data;
        void *model = halo::cache::globals().tag_instances[weapon_tag->first_person_model.tag_id.index].data;
        ModelAnimations *animations =
            (ModelAnimations *)halo::cache::globals().tag_instances[weapon_tag->first_person_animations.tag_id.index].data;
        ModelAnimationsAnimation *animation_block;
        ModelAnimationsAnimationGraphFirstPersonWeaponAnimations *list;
        int16_t *list_entries;
        void *animation_control = fp_raw + 0x8c;
        int16_t index;

        if (animations->first_person_weapons.count != 0 &&
            (list = (ModelAnimationsAnimationGraphFirstPersonWeaponAnimations *)
                 animations->first_person_weapons.pointer) != 0) {
            animation_block = (ModelAnimationsAnimation *)animations->animations.pointer;
            list_entries = (int16_t *)list->animations.pointer;

            if (fp->current_animation != -1) {
                halo::models::animation_get_frame_orientations(&animation_block[fp->current_animation], (GBXModel *)0,
                             (int16_t)*(uint16_t *)fp->current_animation_frame, reinterpret_cast<real_orientation *>(animation_control));
            } else {
                halo::models::model_nodes_get_default_transforms(reinterpret_cast<GBXModel *>(model), reinterpret_cast<real_orientation *>(animation_control));
            }

            if ((int32_t)list->animations.count > 0x11 && (index = list_entries[0x11]) != -1) {
                ModelAnimationsAnimation *ammunition = &animation_block[index];
                int16_t *magazine = (int16_t *)((uint8_t *)weapon_obj + 0x2b0);

                if (weapon_tag->weapon_type == 2 &&
                    (first_person_weapon_interfaces[0].state == 0xd ||
                     first_person_weapon_interfaces[0].state == 0xe)) {
                    int16_t elapsed = (int16_t)(magazine[2] - magazine[1]);
                    int32_t frame = (uint16_t)magazine[4];

                    if (elapsed >= 0x2c) {
                        WeaponMagazine *magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer;
                        double fraction = (double)(elapsed - 0x2c) * (double)0.2f;
                        int16_t target;
                        if (fraction > 1.0) {
                            fraction = 1.0;
                        }
                        target = magazine[3];
                        if (target > (int16_t)magazine_tag->rounds_loaded_maximum) {
                            target = magazine_tag->rounds_loaded_maximum;
                        }
                        frame += __ftol((double)(target - (int16_t)frame) * fraction);
                    }
                    halo::models::animation_overlay_frame_orientations(ammunition, frame, reinterpret_cast<real_orientation *>(animation_control));
                } else if (magazine[4] < (int16_t)ammunition->frame_count) {
                    halo::models::animation_overlay_frame_orientations(ammunition, (uint16_t)magazine[4], reinterpret_cast<real_orientation *>(animation_control));
                }
            }

            if (fp->moving_animation != -1) {
                halo::models::animation_overlay_frame_orientations(&animation_block[fp->moving_animation],
                             (uint16_t)*(int16_t *)fp->unknown_1c, reinterpret_cast<real_orientation *>(animation_control));
            }
            if (fp->overcharged_animation != -1) {
                halo::models::animation_overlay_interpolated_frame_orientations_weighted(&animation_block[fp->overcharged_animation], FP_FLOAT(fp, 0x24),
                             *(float *)((uint8_t *)weapon_obj + 0x244) + 0.5f, reinterpret_cast<real_orientation *>(animation_control));
            }

            if ((int32_t)list->animations.count > 4 && (index = list_entries[4]) != -1 &&
                (int16_t)animation_block[index].frame_count >= 9) {
                ModelAnimationsAnimation *overlays = &animation_block[index];
                overlay_channel(overlays, FP_FLOAT(fp, 0x30), 0, 1, animation_control);
                overlay_channel(overlays, FP_FLOAT(fp, 0x34), 3, 2, animation_control);
                overlay_channel(overlays, FP_FLOAT(fp, 0x40), 4, 5, animation_control);
                overlay_channel(overlays, FP_FLOAT(fp, 0x44), 7, 6, animation_control);
                if (fp->recoil > 0.0f) {
                    halo::models::animation_overlay_frame_orientations_weighted(overlays, 8, fp->recoil, reinterpret_cast<real_orientation *>(animation_control));
                }
            }

            if (fp->blend_end > 0) {
                halo::models::model_nodes_blend_transforms(reinterpret_cast<real_orientation *>(animation_control), (int16_t)animations->nodes.count,
                                             reinterpret_cast<real_orientation *>(fp_raw + 0x88c), (uint16_t)fp->blend_start,
                                             (uint16_t)fp->blend_end);
            }
        }

        halo::models::animation_graph_nodes_build_matrices(
            *(datum_index *)&weapon_tag->first_person_animations.tag_id, &render_camera_global,
            reinterpret_cast<real_matrix4x3 *>(fp_raw + 0x108c), reinterpret_cast<real_orientation *>(fp_raw + 0x8c), &camera_forward_x, &camera_up);
    }
}

/**
 * Original engine function first_person_weapon_update_zoom_static_tint; the author notes are in
 * docs/original/interface/first_person_weapon_update_zoom_static_tint.txt.
 * blam-cc: AL -> enabled
 *
 * @address 0x494af0
 */
void FirstPersonWeaponController::update_zoom_static_tint(uint8_t enabled)
{
    float intensity;
    int32_t hud_interface;
    WeaponHUDInterfaceScreenEffect *effect;
    float scale;
    float source_value;
    float product;
    int16_t source;

    zoom_static_tint_r = 0.0f;
    zoom_static_tint_g = 0.0f;
    zoom_static_tint_b = 0.0f;

    if (enabled == 0 || current_local_player_index == -1) {
        return;
    }
    hud_interface = halo::interface::local_player_get_weapon_hud_interface(&intensity);
    if (hud_interface == -1) {
        return;
    }
    if ((int32_t)((WeaponHUDInterface *)halo::cache::globals().tag_instances[hud_interface & halo::k_slot_mask].data)->screen_effect.count <= 0) {
        return;
    }
    effect = (WeaponHUDInterfaceScreenEffect *)
        ((WeaponHUDInterface *)halo::cache::globals().tag_instances[hud_interface & halo::k_slot_mask].data)->screen_effect.pointer;

    if ((int16_t)halo::game::local_player_get_zoom_level(current_local_player_index) == -1 &&
        (effect->mask_flags & 1) != 0) {
        return;
    }
    if (halo::main::render_local_view_count() > 1) {
        return;
    }
    if (*(datum_index *)&effect->mask_fullscreen.tag_id == (datum_index)-1) {
        return;
    }
    if ((effect->desaturation_flags & 2) == 0) {
        return;
    }

    if (intensity < 0.0f) {
        scale = 0.0f;
    } else if (intensity > 1.0f) {
        scale = 1.0f;
    } else {
        scale = intensity;
    }
    scale = scale * effect->desaturation_intensity;

    source = effect->desaturation_script_source;
    if (halo::render::cinematic_screen_effect_get_script_value(source) < 0.0f) {
        source_value = 0.0f;
    } else if (halo::render::cinematic_screen_effect_get_script_value(source) > 1.0f) {
        source_value = 1.0f;
    } else {
        source_value = halo::render::cinematic_screen_effect_get_script_value(source);
    }

    product = source_value * scale;
    if (product > 0.0f) {
        zoom_static_tint_r = (product < 0.75f) ? product : 0.75f;
        zoom_static_tint_g = (product < 0.75f) ? product : 0.75f;
        zoom_static_tint_b = (product < 0.75f) ? product : 0.75f;
    }
}

}
